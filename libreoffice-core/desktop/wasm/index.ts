import { CallbackType } from './lok_enums';
import {
  CallbackHandler,
  DocumentClient,
  DocumentClientBase,
  DocumentMethods,
  DocumentWithViewMethods,
  ForwardedFromWorker,
  ForwardedResolver,
  ForwardingFromWorker,
  ForwardingId,
  ForwardingMethod,
  FromWorker,
  Id,
  KeysMessage,
  Message,
  Ref,
  ToWorker,
  WorkerCallback,
} from './shared';
import { DocumentRef } from './soffice';

/** rendered tile size in pixels */
export const TILE_DIM_PX = 256;
/** 15 = 1440 twips-per-inch / 96 dpi */
const LOK_INTERNAL_TWIPS_TO_PX = 15;

/** pulled from tile_renderer_worker
TODO: should probably be split into a separate file */
function clipToNearest8PxZoom(w: number, s: number): number {
  const scaledWidth: number = Math.ceil(w * s);
  const mod: number = scaledWidth % 8;
  if (mod === 0) return s;

  return Math.abs((scaledWidth - mod) / w - s) <
    Math.abs((scaledWidth + 8 - mod) / w - s)
    ? (scaledWidth - mod) / w
    : (scaledWidth + 8 - mod) / w;
}

/** Calculates the conversions for twips provided a `zoom` level and `dpi` */
export function conversionTable(zoom: number, dpi: number) {
  const scale = clipToNearest8PxZoom(TILE_DIM_PX, 1 / (zoom * dpi));
  const TILE_DIM_TWIPS = Math.floor(
    TILE_DIM_PX * LOK_INTERNAL_TWIPS_TO_PX * scale
  );
  return {
    zoom,
    dpi,
    scale,
    twipsToPx: TILE_DIM_PX / TILE_DIM_TWIPS,
    pxToTwips: TILE_DIM_TWIPS / TILE_DIM_PX,
  };
}

function getScaledTwips(zoom: number) {
  return clipToNearest8PxZoom(TILE_DIM_PX, 1 / zoom) * LOK_INTERNAL_TWIPS_TO_PX;
}

/** CSS pixels are DPI indepdendent */
export function twipsToCssPx(twips: number, zoom: number) {
  return twips / getScaledTwips(zoom);
}

/** CSS pixels are DPI indepdendent */
export function cssPxToTwips(px: number, zoom: number) {
  return px * getScaledTwips(zoom);
}

const worker: Ref<Worker> = {};

const counter: Ref<Id> = { current: 0 };

class Future<T> {
  _settled = false;
  promise: Promise<T>;
  resolve: (value: T | PromiseLike<T>) => void;

  constructor() {
    this.promise = new Promise<T>((resolve) => {
      this.resolve = resolve;
    });
  }
}

const UNUSED_ID: Id = -1;

const callIdToFuture: Record<Id, Future<any>> = {};
type CallbackHandlers = {
  -readonly [K in keyof typeof CallbackType]?: Set<CallbackHandler>;
};
const subscribedEvents: { [K: number]: CallbackHandlers } = {};
const subscribedPaint: Map<number, Set<() => any>> = new Map();
const subscribedIdle: Map<number, Set<() => any>> = new Map();
const subscribedUncaughtError: Set<() => any> = new Set();

type IdleMessage = { f: 'idle_'; d: DocumentRef };
type PaintMesasge = { f: 'paint_'; d: DocumentRef };

type AllMessages<K extends keyof Message = keyof Message> =
  | FromWorker<K>
  | ForwardingFromWorker<K & keyof ForwardingMethod>
  | ForwardedFromWorker<K & keyof ForwardingMethod>
  | WorkerCallback
  | KeysMessage
  | PaintMesasge
  | IdleMessage;

function messageIsPaint<K extends keyof Message = keyof Message>(
  data: AllMessages<K>
): data is PaintMesasge {
  return data.f === 'paint_';
}

function messageIsIdle<K extends keyof Message = keyof Message>(
  data: AllMessages<K>
): data is IdleMessage {
  return data.f === 'idle_';
}

function messageIsCallback<K extends keyof Message = keyof Message>(
  data: AllMessages<K>
): data is WorkerCallback {
  return data.f === 'c';
}

function messageIsForwarding<K extends keyof Message = keyof Message>(
  data: AllMessages<K>
): data is ForwardingFromWorker {
  return (data as ForwardingFromWorker).fwd === true;
}

function messageIsKeys<K extends keyof Message = keyof Message>(
  data: AllMessages<K>
): data is KeysMessage {
  return data.f === '_keys';
}

function messageIsUncaughtError(data) {
  return data.uncaughtError === true;
}

const clientBase: DocumentClientBase = {
  ref: -1,
  viewId: -1,
  on(type: CallbackType, handler: CallbackHandler) {
    const ref = this.ref;
    if (subscribedEvents[ref] == null)
      subscribedEvents[ref] = { [type]: new Set() };
    if (subscribedEvents[ref][type] == null)
      subscribedEvents[ref][type] = new Set();
    const subscribedEvent = subscribedEvents[ref][type];
    if (subscribedEvent.size === 0) {
      loadWorkerOnce().postMessage({
        f: 'subscribe',
        i: UNUSED_ID,
        a: [ref, this.viewId ?? -1, type],
      } as ToWorker);
    }
    subscribedEvent.add(handler);
  },
  off(type: CallbackType, handler: CallbackHandler) {
    const ref = this.ref;
    const subscribedEvent = subscribedEvents[ref]?.[type];
    if (subscribedEvent != null) return;

    subscribedEvent.delete(handler);
    if (subscribedEvent.size === 0) {
      loadWorkerOnce().postMessage({
        f: 'unsubscribe',
        i: UNUSED_ID,
        a: [ref, this.viewId ?? -1, type],
      } as ToWorker);
    }
  },
  async newView(): Promise<DocumentClient> {
    const [i, future] = registerFuture<DocumentRef | null>();
    const message: ToWorker = {
      f: 'newView',
      i,
      a: [],
    };
    loadWorkerOnce().postMessage(message);

    return documentClient(this.ref, await future.promise);
  },
  afterIdle: function (callback: () => any): () => void {
    let handlers = subscribedIdle.get(this.ref);
    if (handlers == null) {
      handlers = new Set();
      subscribedIdle.set(this.ref, handlers);
    }
    handlers.add(callback);
    return () => {
      handlers.delete(callback);
    };
  },
  afterPaint: function (callback: () => any): () => void {
    let handlers = subscribedPaint.get(this.ref);
    if (handlers == null) {
      handlers = new Set();
      subscribedPaint.set(this.ref, handlers);
    }
    handlers.add(callback);
    return () => {
      handlers.delete(callback);
    };
  },
  afterUncaughtError: function (callback) {
    subscribedUncaughtError.add(callback);
    return () => {
        subscribedUncaughtError.delete(callback);
    };
  },
};

type ForwardedBase = {
  fwd: ForwardingId;
};

type ForwardedClient = ForwardedBase & {
  [K in string]: (...args: any[]) => any;
};

const forwardedClassBases: Record<ForwardedResolver, ForwardedClient> =
  {} as any; // any because the class will be completed upon initialization

function registerForwardedMethod(resolver: string, method: string) {
  let base = forwardedClassBases[resolver];
  if (base == null) {
    base = {} as any;
    forwardedClassBases[resolver] = base;
  }
  base[method] = function (...args: any[]) {
    const [i, future] = registerFuture();
    loadWorkerOnce().postMessage({
      f: resolver,
      i,
      a: [method, this.fwd, ...args],
    } as ToWorker);
    return future.promise;
  };
}

function registerClientMethod(prop: string) {
  clientBase[prop] = function (...args: any[]) {
    const [i, future] = registerFuture();
    let transfers: { transfer: Transferable[] } | undefined;
    if (prop === 'startRendering' || prop === 'resetRendering') {
      transfers = {
        transfer: [
          ...(
            args as
              | Parameters<DocumentWithViewMethods['startRendering']>
              | Parameters<DocumentWithViewMethods['resetRendering']>
          )[0],
        ],
      };
    }
    loadWorkerOnce().postMessage(
      {
        f: prop,
        i,
        a: WITHOUT_VIEW_ID[prop]
          ? [this.ref, ...args]
          : [this.ref, this.viewId, ...args],
      } as ToWorker,
      transfers
    );
    // automatically assign the view ID after intializing for rendering so that rendering calls have a valid target
    if (prop === 'initializeForRendering') {
      future.promise.then(
        (viewId: FromWorker<'initializeForRendering'>['r']) => {
          this.viewId = viewId;
        }
      );
    }
    return future.promise;
  };
}

async function handleMessage<K extends keyof Message = keyof Message>({
  data,
}: MessageEvent<AllMessages<K>>) {
  if (messageIsCallback(data)) {
    subscribedEvents[data.d]?.[data.t]?.forEach((handler) => handler(data.p));
  } else if (messageIsKeys(data)) {
    for (const prop of data.keys) {
      registerClientMethod(prop);
    }
    for (const [resolver, methods] of Object.entries(data.forwarded)) {
      for (const method of methods) {
        registerForwardedMethod(resolver, method);
      }
    }
  } else if (messageIsForwarding(data)) {
    callIdToFuture[data.i].resolve(forwardedClient(data.r));
    callIdToFuture[data.i] = undefined;
  } else if (messageIsIdle(data)) {
    const callbacks = subscribedIdle.get(data.d);
    if (callbacks) {
      for (const cb of callbacks) {
        cb();
      }
    }
  } else if (messageIsPaint(data)) {
    const callbacks = subscribedPaint.get(data.d);
    if (callbacks) {
      for (const cb of callbacks) {
        cb();
      }
    }
  } else if (messageIsUncaughtError(data)) {
    console.log('handling uncaught error');
    for (const cb of subscribedUncaughtError) {
      cb();
    }
  } else if (data.i !== UNUSED_ID) {
    callIdToFuture[data.i].resolve(data.r);
    callIdToFuture[data.i] = undefined;
  }
}

function loadWorkerOnce() {
  if (worker.current) {
    return worker.current;
  }

  worker.current = new Worker(new URL('./worker.js', import.meta.url), {
    type: 'module',
  });
  worker.current.onmessage = handleMessage;

  return worker.current;
}

function registerFuture<T>(): [Id, Future<T>] {
  // technically, this could hit Number.SAFE_INTEGER_MAX and overflow around but that means 2**53-1 calls
  counter.current = Math.max(0, (counter.current + 1) | 0);
  const i = ++counter.current;
  callIdToFuture[i] = new Future<T>();
  return [i, callIdToFuture[i]];
}

const WITHOUT_VIEW_ID: Record<keyof DocumentMethods, true> = {
  close: true,
  save: true,
  saveAs: true,
  newView: true,
  parts: true,
  partRectanglesTwips: true,
  documentSize: true,
  initializeForRendering: true,
};

function documentClient<T extends DocumentClient>(
  ref: DocumentRef | null,
  viewId: number | undefined = -1
): T | null {
  if (!ref) return null;
  const clientObject: DocumentClientBase = Object.create(clientBase, {
    viewId: {
      value: viewId,
      writable: true,
    },
    ref: {
      value: ref,
      writable: true,
    },
    idleHandlers: {
      value: new Set(),
      writable: false,
    },
    paintHandlers: {
      value: new Set(),
      writable: false,
    },
  });
  return clientObject as T;
}

function forwardedClient(fwd: ForwardingId): ForwardedClient {
  const clientBase = forwardedClassBases[fwd[2]];
  return Object.create(clientBase, {
    fwd: {
      value: fwd,
      writable: true,
    },
  });
}

export async function loadDocument<T extends DocumentClient = DocumentClient>(
  name: string,
  blob: Blob
): Promise<T | null> {
  const [i, future] = registerFuture<DocumentRef | null>();
  const message: ToWorker = {
    f: 'load',
    i,
    a: [name, blob],
  };
  loadWorkerOnce().postMessage(message);
  return future.promise.then(documentClient<T>);
}
export async function loadDocumentFromExpandedParts<
  T extends DocumentClient = DocumentClient,
>(
  name: string,
  parts: Array<{path: string, content: ArrayBuffer}>,
  readOnly = false
): Promise<DocumentClient | null> {
  const [i, future] = registerFuture<DocumentRef | null>();
  const message: ToWorker = {
    f: 'loadFromExpandedParts',
    i,
    a: [name, parts, readOnly],
  };
  loadWorkerOnce().postMessage(message);
  return future.promise.then(documentClient<T>);
}

export async function loadDocumentFromArrayBuffer<
  T extends DocumentClient = DocumentClient,
>(
  name: string,
  arrayBuffer: BlobPart,
  type: string = 'application/octet-stream'
): Promise<DocumentClient | null> {
  // TODO: this might be slower than transfering the ArrayBuffer to the worker and setting it there?
  return loadDocument<T>(name, new Blob([arrayBuffer], { type }));
}

export async function preload() {
  const message: ToWorker = {
    f: 'preload',
    i: UNUSED_ID,
    a: [],
  };
  loadWorkerOnce().postMessage(message);
}

export async function setIsMacOSForConfig() {
  const message: ToWorker = {
    f: 'setIsMacOSForConfig',
    i: UNUSED_ID,
    a: [],
  };
  loadWorkerOnce().postMessage(message);
}

declare global {
  interface ImportMeta {
    readonly hot?: {
      readonly data: Partial<{
        worker: Worker;
        counter: Id;
      }>;

      accept(): void;
      dispose(cb: (data: any) => void): void;
    };
  }
}

if (import.meta.hot) {
  if (import.meta.hot.data.worker) {
    worker.current = import.meta.hot.data.worker;
  }
  if (import.meta.hot.data.counter) {
    counter.current = import.meta.hot.data.counter;
  }
  import.meta.hot.accept();
  import.meta.hot.dispose(() => {
    import.meta.hot.data.worker = worker.current;
    import.meta.hot.data.counter = counter.current;
  });
}

export type * from './shared';
export type * from './soffice';
export * from './lok_enums';

export enum LayoutStatus {
  INVISIBLE,
  VISIBLE,
  INSERTED,
  DELETED,
  NONE,
  HIDDEN,
}

export enum SwUndoId {
  EXTERNAL = 512,
  EMPTY = 0,
  STD_BEGIN = 1,
  START = STD_BEGIN,
  END,

  REPEAT_START,
  DELETE = REPEAT_START,
  INSERT,
  OVERWRITE,
  SPLITNODE,
  INSATTR,
  SETFMTCOLL,
  RESETATTR,
  INSFMTATTR,
  INSDOKUMENT,
  COPY,
  INSTABLE,
  TABLETOTEXT,
  TEXTTOTABLE,
  SORT_TXT,
  INSLAYFMT,
  TABLEHEADLINE,
  INSSECTION,
  OUTLINE_LR,
  OUTLINE_UD,
  INSNUM,
  NUMUP,
  MOVENUM,
  INSDRAWFMT,
  NUMORNONUM,
  INC_LEFTMARGIN,
  DEC_LEFTMARGIN,
  INSERTLABEL,
  SETNUMRULESTART,
  CHGFTN,
  REDLINE,
  ACCEPT_REDLINE,
  REJECT_REDLINE,
  SPLIT_TABLE,
  DONTEXPAND,
  AUTOCORRECT,
  MERGE_TABLE,
  TRANSLITERATE,
  PASTE_CLIPBOARD,
  TYPING,
  REPEAT_END = 46,

  MOVE = REPEAT_END,
  INSGLOSSARY,
  DELBOOKMARK,
  INSBOOKMARK,
  SORT_TBL,
  DELLAYFMT,
  AUTOFORMAT,
  REPLACE,
  DELSECTION,
  CHGSECTION,
  SETDEFTATTR = 57,
  DELNUM,
  DRAWUNDO,
  DRAWGROUP,
  DRAWUNGROUP,
  DRAWDELETE,
  REREAD,
  DELGRF,
  TABLE_ATTR = 66,
  TABLE_AUTOFMT,
  TABLE_INSCOL,
  TABLE_INSROW,
  TABLE_DELBOX,
  TABLE_SPLIT,
  TABLE_MERGE,
  TBLNUMFMT,
  INSTOX,
  CLEARTOXRANGE,
  TBLCPYTBL,
  CPYTBL,
  INS_FROM_SHADOWCRSR,
  CHAINE,
  UNCHAIN,
  FTNINFO,
  COMPAREDOC = 83,
  SETFLYFRMFMT,
  SETRUBYATTR,
  TOXCHANGE = 87,
  CREATE_PAGEDESC,
  CHANGE_PAGEDESC,
  DELETE_PAGEDESC,
  HEADER_FOOTER,
  FIELD,
  TXTFMTCOL_CREATE,
  TXTFMTCOL_DELETE,
  TXTFMTCOL_RENAME,
  CHARFMT_CREATE,
  CHARFMT_DELETE,
  CHARFMT_RENAME,
  FRMFMT_CREATE,
  FRMFMT_DELETE,
  FRMFMT_RENAME,
  NUMRULE_CREATE,
  NUMRULE_DELETE,
  NUMRULE_RENAME,
  BOOKMARK_RENAME,
  INDEX_ENTRY_INSERT,
  INDEX_ENTRY_DELETE,
  COL_DELETE,
  ROW_DELETE,
  RENAME_PAGEDESC,
  NUMDOWN,

  FLYFRMFMT_TITLE,
  FLYFRMFMT_DESCRIPTION,
  TBLSTYLE_CREATE,
  TBLSTYLE_DELETE,
  TBLSTYLE_UPDATE,
  PARA_SIGN_ADD,

  UI_REPLACE,
  UI_INSERT_PAGE_BREAK,
  UI_INSERT_COLUMN_BREAK,
  UI_INSERT_ENVELOPE = 122,
  UI_DRAG_AND_COPY,
  UI_DRAG_AND_MOVE,
  UI_INSERT_CHART,
  UI_INSERT_FOOTNOTE,
  UI_INSERT_URLBTN,
  UI_INSERT_URLTXT,
  UI_DELETE_INVISIBLECNTNT,
  UI_REPLACE_STYLE,
  UI_DELETE_PAGE_BREAK,
  UI_TEXT_CORRECTION,
  UI_TABLE_DELETE,
  CONFLICT,

  INSERT_FORM_FIELD,
  OUTLINE_EDIT,
  INSERT_PAGE_NUMBER,
  UPDATE_FORM_FIELD,
  UPDATE_FORM_FIELDS,
  DELETE_FORM_FIELDS,
  UPDATE_BOOKMARK,
  UPDATE_BOOKMARKS,
  DELETE_BOOKMARKS,
  UPDATE_FIELD,
  UPDATE_FIELDS,
  DELETE_FIELDS,
  UPDATE_SECTIONS,
  CHANGE_THEME = 148,
  DELETE_SECTIONS = 149,
  FLYFRMFMT_DECORATIVE = 150,
}
