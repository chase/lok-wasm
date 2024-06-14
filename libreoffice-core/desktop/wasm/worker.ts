import type {
  DocumentMethodHandler,
  ForwardedFromWorker,
  ForwardedMethodMap,
  ForwardedResolver,
  ForwardedResolverMap,
  ForwardingId,
  ForwardingMethodHandler,
  ForwardingMethodHandlers,
  FromWorker,
  GetClipbaordItem,
  GlobalMethod,
  InitializeForRenderingOptions,
  KeysMessage,
  Message,
  RectangleTwips,
  SetClipbaordItem,
  TileDim,
  TileRendererData,
  ToTileRenderer,
  ToWorker,
  ViewId,
  WorkerCallback,
} from './shared';
import type {
  Comment,
  Document,
  DocumentRef,
  HeaderFooterRect,
  ITextRanges,
  ParagraphStyle,
  RectArray,
  SanitizeOptions,
} from './soffice';
import LOK from './soffice';
// NOTE: Disabled until unoembind startup cost is under 1s
// import { init_unoembind_uno } from './bindings_uno';

// because the worker can arrive before the message handler which is initialized later after the LOK module is created,
// queue the messages for later and run them against the message handler after the rest of initialization
const messageQueue: MessageEvent<any>[] = [];
globalThis.onmessage = (message) => {
  messageQueue.push(message);
};
type EmbindingDisposable = { delete(): void };
const docMap: Record<DocumentRef, Document> = {};
const findResultMap: Record<DocumentRef, ITextRanges & EmbindingDisposable> =
  {};
const byRef = (ref: DocumentRef) => docMap[ref];
const tileRenderer: Record<DocumentRef, Record<ViewId, Worker>> = {};

const lok = await LOK({
  withFcCache: true,
  callbackHandlers: {
    callback: function (ref: DocumentRef, type: number, payload: string): void {
      const message: WorkerCallback = {
        f: 'c',
        d: ref,
        t: type,
        p: payload,
      };
      postMessage(message);
    },
  },
});

function rectArrayToRectangleTwips(r: RectArray): RectangleTwips {
  return {
    x: r[0],
    y: r[1],
    width: r[2],
    height: r[3],
  };
}

const globalHandler: GlobalMethod = {
  load: function (name: string, blob: Blob): DocumentRef | null {
    const doc = new lok.Document(`file://${lok.mountBlob(name, blob)}`);
    const ref = doc.ref();
    lok.unmountBlob();

    if (!doc.valid()) {
      doc.delete();
      return null;
    }

    docMap[ref] = doc;
    return ref;
  },

  preload: function (): void {
    lok.preload();
  },

  setIsMacOSForConfig: function (): void {
    lok.setIsMacOSForConfig();
  },
};

const handler: DocumentMethodHandler<Document> = {
  newView: function (doc: Document): DocumentRef {
    return doc.newView();
  },

  close: function (doc: Document): void {
    doc.delete();
  },

  save: function (doc: Document, format: string): ArrayBuffer {
    const tmpFile = `/${Date.now()}.${format}`;
    // Optional arguments as emscripten can be undefined,
    // but the number of parameters must match the binding signature
    // so for optional parameters, we have to pass undefined
    // https://github.com/emscripten-core/emscripten/pull/21076/files
    doc.saveAs(`file://${tmpFile}`, format, undefined);
    // only buffer is transferable
    return lok.readUnlink(tmpFile).buffer;
  },

  parts: function (doc: Document): number {
    return doc.getParts();
  },

  partRectanglesTwips: function (doc: Document): RectangleTwips[] {
    return doc.pageRects().map(rectArrayToRectangleTwips);
  },

  documentSize: function (
    doc: Document
  ): [widthTwips: number, heightTwips: number] {
    return doc.getDocumentSize();
  },

  initializeForRendering: function (
    doc: Document,
    args: InitializeForRenderingOptions = {}
  ): number {
    doc.initializeForRendering(
      `{".uno:ShowBorderShadow": {
          "type": "boolean",
          "value": false
        },
        ".uno:HideWhitespace": {
          "type": "boolean",
          "value": false
        },
        ".uno:SpellOnline": {
          "type": "boolean",
          "value": ${args.autoSpellcheck ?? true}
        },
        ".uno:Author": {
          "type": "string",
          "value": "${args.author ?? 'Macro User'}"
        }}`
    );
    return doc.getViewId();
  },

  postKeyEvent: function (
    doc: Document,
    viewId: ViewId,
    type: number,
    charCode: number,
    keyCode: number
  ): void {
    return doc.postKeyEvent(viewId, type, charCode, keyCode);
  },

  postTextInput: function (
    doc: Document,
    viewId: ViewId,
    windowId: number,
    text: string
  ): void {
    return doc.postTextInputEvent(viewId, windowId, text);
  },

  postMouseEvent: function (
    doc: Document,
    viewId: ViewId,
    type: number,
    x: number,
    y: number,
    count: number,
    buttons: number,
    modifiers: number
  ): void {
    return doc.postMouseEvent(viewId, type, x, y, count, buttons, modifiers);
  },

  setTextSelection: function (
    doc: Document,
    viewId: ViewId,
    type: number,
    x: number,
    y: Number
  ): void {
    return doc.setTextSelection(viewId, type, x, y);
  },

  setClipboard: function (
    doc: Document,
    viewId: ViewId,
    items: SetClipbaordItem[]
  ): boolean {
    return doc.setClipboard(viewId, items);
  },

  getClipboard: function (
    doc: Document,
    viewId: ViewId,
    mimeTypes: string[]
  ): GetClipbaordItem[] {
    return doc.getClipboard(viewId, mimeTypes);
  },

  paste: function (
    doc: Document,
    viewId: ViewId,
    mimeType: string,
    data: string | ArrayBuffer
  ): void {
    return doc.paste(viewId, mimeType, data);
  },

  setGraphicSelection: function (
    doc: Document,
    viewId: ViewId,
    type: number,
    x: number,
    y: number
  ): void {
    return doc.setGraphicSelection(viewId, type, x, y);
  },

  resetSelection: function (doc: Document, viewId: ViewId): void {
    return doc.resetSelection(viewId);
  },

  getCommandValues: function (
    doc: Document,
    viewId: ViewId,
    command: string
  ): any {
    const result = doc.getCommandValues(viewId, command);
    return result.startsWith('{') ? JSON.parse(result) : result;
  },

  subscribe: function (
    doc: Document,
    viewId: ViewId,
    callbacktype: number
  ): void {
    doc.subscribe(viewId, callbacktype);
  },

  unsubscribe: function (
    doc: Document,
    viewId: ViewId,
    callbacktype: number
  ): void {
    doc.unsubscribe(viewId, callbacktype);
  },

  startRendering: function (
    doc: Document,
    viewId: ViewId,
    canvases: OffscreenCanvas[],
    tileSize: TileDim,
    scale: number,
    dpi: number,
    yPos: number = 0
  ): TileRendererData {
    const ref = doc.ref();
    const result = doc.startTileRenderer(viewId, tileSize);
    const worker = new Worker(
      new URL('./tile_renderer_worker.js', import.meta.url),
      { type: 'module' }
    );
    if (!tileRenderer[ref]) {
      tileRenderer[ref] = {};
    }
    tileRenderer[ref][result.viewId] = worker;

    worker.addEventListener('message', ({ data }) => {
      if (data.idle) {
        postMessage({ f: 'idle_', d: ref });
      }
      if (data.paint) {
        postMessage({ f: 'paint_', d: ref });
      }
    });

    worker.postMessage(
      {
        t: 'i',
        c: canvases,
        d: result,
        s: scale,
        y: yPos,
        dpi,
      } as ToTileRenderer,
      { transfer: [...canvases] }
    );

    return {
      docRef: ref,
      viewId: result.viewId,
      scale,
    };
  },
  resetRendering: function (
    _doc: Document,
    _viewId: ViewId,
    _canvases: OffscreenCanvas[]
  ): void {
    throw new Error('Function not implemented.');
  },
  stopRendering: function (doc: Document, viewId: ViewId): void {
    const ref = doc.ref();
    const worker = tileRenderer[ref]?.[viewId];
    worker?.terminate();
    doc.stopTileRenderer(viewId);
  },
  setScrollTop: function (
    doc: Document,
    viewId: ViewId,
    yPx: number
  ): Promise<number> {
    const ref = doc.ref();
    const worker = tileRenderer[ref]?.[viewId];
    if (!worker) return;
    const scrollPromise = new Promise<number>((resolve) => {
      const handleMessage = ({ data }: MessageEvent) => {
        if (data.s != null) {
          resolve(data.s);
        }
        worker.removeEventListener('message', handleMessage);
      };
      worker.addEventListener('message', handleMessage);
    });

    worker.postMessage({
      t: 's',
      y: yPx,
    } as ToTileRenderer);

    return scrollPromise;
  },

  setVisibleHeight: function (
    doc: Document,
    viewId: ViewId,
    heightPx: number
  ): void {
    tileRenderer[doc.ref()]?.[viewId]?.postMessage({
      t: 'r',
      h: heightPx,
    } as ToTileRenderer);
  },

  dispatchCommand: function (
    doc: Document,
    viewId: ViewId,
    command: string,
    args?: any,
    notifyWhenFinished?: boolean
  ): void {
    doc.dispatchCommand(
      viewId,
      command,
      args && typeof args === 'string' ? args : JSON.stringify(args),
      notifyWhenFinished
    );
  },

  removeText: function (
    doc: Document,
    viewId: ViewId,
    charsBefore: number,
    charsAfter: number
  ): void {
    doc.removeText(
      viewId,
      0 /* window id is pretty much always 0 (the default window) */,
      charsBefore,
      charsAfter
    );
  },

  setClientVisibleArea: function (
    doc: Document,
    viewId: ViewId,
    x: number,
    y: number,
    width: number,
    height: number
  ): void {
    doc.setClientVisibleArea(viewId, x, y, width, height);
  },

  comments: function (doc: Document, viewId: ViewId): Comment[] {
    doc.setCurrentView(viewId);
    return doc.comments();
  },

  addComment: function (doc: Document, viewId: ViewId, text: string): void {
    doc.setCurrentView(viewId);
    doc.addComment(text);
  },

  replyComment: function (
    doc: Document,
    viewId: ViewId,
    parentId: number,
    text: string
  ): void {
    doc.setCurrentView(viewId);
    doc.replyComment(parentId, text);
  },

  deleteCommentThreads: function (
    doc: Document,
    viewId: ViewId,
    parentIds: number[]
  ): void {
    doc.setCurrentView(viewId);
    doc.deleteCommentThreads(parentIds);
  },

  deleteComment: function (
    doc: Document,
    viewId: ViewId,
    commentId: number
  ): void {
    doc.setCurrentView(viewId);
    doc.deleteComment(commentId);
  },

  resolveCommentThread: function (
    doc: Document,
    viewId: ViewId,
    parentId: number
  ): void {
    doc.setCurrentView(viewId);
    doc.resolveCommentThread(parentId);
  },

  resolveComment: function (
    doc: Document,
    viewId: ViewId,
    commentId: number
  ): void {
    doc.setCurrentView(viewId);
    doc.resolveComment(commentId);
  },

  sanitize: function (
    doc: Document,
    viewId: ViewId,
    options: SanitizeOptions
  ): void {
    doc.setCurrentView(viewId);
    doc.sanitize(options);
  },

  headerFooterRect: function (doc: Document, viewId: ViewId): HeaderFooterRect {
    doc.setCurrentView(viewId);
    return doc.headerFooterRect();
  },

  getPropertyValue: function (
    doc: Document,
    viewId: ViewId,
    property: string
  ): any {
    doc.setCurrentView(viewId);
    return doc.getPropertyValue(property);
  },

  setPropertyValue: function (
    doc: Document,
    viewId: ViewId,
    property: string,
    value: any
  ): void {
    doc.setCurrentView(viewId);
    doc.setPropertyValue(property, value);
  },

  saveCurrentSelection: function (doc: Document, viewId: ViewId): void {
    doc.setCurrentView(viewId);
    doc.saveCurrentSelection();
  },

  restoreCurrentSelection: function (doc: Document, viewId: ViewId): void {
    doc.setCurrentView(viewId);
    doc.restoreCurrentSelection();
  },

  getParagraphStyle: function <T extends string[]>(
    doc: Document,
    viewId: ViewId,
    name: string,
    properties: T
  ): ParagraphStyle<T> {
    doc.setCurrentView(viewId);
    return doc.getParagraphStyle(name, properties) as ParagraphStyle<T>;
  },

  getSelectionText: function (
    doc: Document,
    viewId: ViewId
  ): string | undefined {
    doc.setCurrentView(viewId);
    return doc.getSelectionText();
  },

  paragraphStyles: function (
    doc: Document,
    viewId: ViewId,
    properties: string[]
  ) {
    doc.setCurrentView(viewId);
    return doc.paragraphStyles(properties);
  },

  setZoom: async function (
    doc: Document,
    viewId: ViewId,
    scale: number,
    dpi: number
  ): Promise<void> {
    tileRenderer[doc.ref()]?.[viewId]?.postMessage({
      t: 'z',
      s: scale,
      d: dpi,
    } as ToTileRenderer);
  },

  getOutline: function (doc: Document, viewId: ViewId) {
    doc.setCurrentView(viewId);
    return doc.getOutline();
  },

  gotoOutline: function (doc: Document, viewId: ViewId, index: number) {
    doc.setCurrentView(viewId);
    return doc.gotoOutline(index);
  },

  setAuthor: function (doc: Document, viewId: ViewId, author: string) {
    doc.setCurrentView(viewId);
    return doc.setAuthor(author);
  },
};

const forwarding: ForwardingMethodHandlers<Document> = {
  findAll: function (
    doc: Document,
    docRef: DocumentRef,
    viewId: ViewId,
    text: string,
    options?: Partial<{
      caseSensitive: boolean;
      wholeWords: boolean;
      mode: 'wildcard' | 'regex' | 'similar';
    }>
  ): ForwardingId {
    findResultMap[docRef]?.delete();
    findResultMap[docRef] = doc.findAll(text, options) as ITextRanges &
      EmbindingDisposable;
    return [docRef, viewId, '_find'];
  },
};

const resolveForward: ForwardedResolverMap = {
  _find: function ([docRef]: ForwardingId): ITextRanges {
    return findResultMap[docRef];
  },
};

// this can be autogenerated using the TS LSP, 'Add Missing Properties'
const forwardedMethods: ForwardedMethodMap = {
  _find: {
    length: true,
    rect: true,
    rects: true,
    isCursorAt: true,
    indexAtCursor: true,
    moveCursorTo: true,
    description: true,
    descriptions: true,
    replace: true,
    replaceAll: true,
  },
};

type TransferableResult<K extends keyof Message = keyof Message> =
  FromWorker<K> & {
    r: Transferable;
  };

const tranferables: Partial<Record<keyof Message, boolean>> = {
  save: true,
};

function isTransferable<K extends keyof Message = keyof Message>(
  fromWorker: FromWorker<K>
): fromWorker is TransferableResult<K> {
  return tranferables[fromWorker.f] === true;
}

function handleForwardedMethod<K extends ForwardedResolver = ForwardedResolver>(
  data: ToWorker<K>
): void {
  const { f, a, i } = data;
  const [method, fwd, ...rest] = a as any; // TODO: lazy, not super important, but type here should be "safe"
  if (!forwardedMethods[f][method]) return;

  const m: ForwardedFromWorker<K> = {
    f,
    m: method,
    i,
    r: resolveForward[f](fwd)[method](...rest),
  };

  postMessage(m);
}

function sendResult<
  K extends keyof Message = keyof Message,
  T extends FromWorker<K> = FromWorker<K>,
>(message: T): void {
  if (isTransferable(message)) {
    postMessage(message, { transfer: [message.r] });
  } else {
    postMessage(message);
  }
}

function handleForwardingMethod<
  K extends keyof ForwardingMethodHandlers<Document>,
>(handler: ForwardingMethodHandler<Document>, data: ToWorker<K>) {
  const [ref, viewId, ...rest] = data.a;
  const doc = byRef(ref as DocumentRef);
  if (!doc) {
    console.error('doc ref missing');
    return;
  }
  postMessage({
    fwd: true,
    f: data.f,
    i: data.i,
    r: handler(doc, ref, viewId, ...rest),
  });
}

async function handleDocumentMethod<
  K extends keyof DocumentMethodHandler<Document>,
>(handler: (doc: Document, ...args: any[]) => any, data: ToWorker<K>) {
  const [ref, ...rest] = data.a;
  const doc = byRef(ref as DocumentRef);
  if (!doc) {
    console.error('doc ref missing');
    return;
  }
  const r = handler(doc, ...rest);
  sendResult<K>({
    f: data.f,
    i: data.i,
    r: r instanceof Promise ? await r : r,
  });
}

function handleGlobalMethod<K extends keyof GlobalMethod>(data: ToWorker<K>) {
  sendResult<K>({
    f: data.f,
    i: data.i,
    r: globalHandler[data.f](...data.a),
  });
}

globalThis.onmessage = async <K extends keyof Message = keyof Message>({
  data,
}: MessageEvent<ToWorker<K>>) => {
  const docHandler = handler[data.f as keyof DocumentMethodHandler<Document>];
  if (docHandler != null) {
    return handleDocumentMethod(
      docHandler,
      data as ToWorker<keyof DocumentMethodHandler<Document>>
    );
  }
  // forwarded methods are methods on classes returned by the Document (such as ITextRanges)
  if (forwardedMethods[data.f as ForwardedResolver]) {
    return handleForwardedMethod(data as ToWorker<ForwardedResolver>);
  }

  const forwardingHandler =
    forwarding[data.f as keyof ForwardingMethodHandlers<Document>];
  if (forwardingHandler != null) {
    return handleForwardingMethod(
      forwardingHandler,
      data as ToWorker<keyof ForwardingMethodHandlers<Document>>
    );
  }

  return handleGlobalMethod(data as ToWorker<keyof GlobalMethod>);
};

postMessage({
  f: '_keys',
  keys: [...Object.keys(handler), ...Object.keys(forwarding)],
  forwarded: Object.fromEntries(
    Object.entries(forwardedMethods).map(([name, method]) => [
      name,
      Object.keys(method),
    ])
  ),
} as KeysMessage);
for (const message of messageQueue) {
  globalThis.onmessage(message);
}
