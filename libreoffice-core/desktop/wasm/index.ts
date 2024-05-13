import { CallbackType } from './lok_enums';
import {
  CallbackHandler,
  DocumentClient,
  DocumentClientBase,
  DocumentMethods,
  DocumentRef,
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

type AllMessages<K extends keyof Message = keyof Message> =
  | FromWorker<K>
  | ForwardingFromWorker<K & keyof ForwardingMethod>
  | ForwardedFromWorker<K & keyof ForwardingMethod>
  | WorkerCallback
  | KeysMessage;

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

export async function loadDocumentFromArrayBuffer<
  T extends DocumentClient = DocumentClient,
>(
  name: string,
  arrayBuffer: ArrayBufferLike,
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
