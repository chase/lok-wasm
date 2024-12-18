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
  GlobalMethod,
  InitializeForRenderingOptions,
  KeysMessage,
  Message,
  RectangleTwips,
  TileDim,
  TileRendererData,
  ToWorker,
  ViewId,
  WorkerCallback,
} from "./shared";
import type {
  GetClipbaordItem,
  SetClipbaordItem,
  Comment,
  Document,
  DocumentRef,
  ExpandedPart,
  HeaderFooterRect,
  ITextRanges,
  ParagraphStyle,
  RectArray,
  SanitizeOptions,
} from "./soffice";
import { renderer, clipToNearest8PxZoom } from "./tile_renderer";
import LOK from "./soffice";
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
const tileRenderer: Record<DocumentRef, Record<ViewId, ReturnType<typeof renderer>>> = {};
let currentDocumentUnderRender: [DocumentRef, ViewId] | undefined;

interface VisibleArea {
  topTwips: number | undefined;
  heightTwips: number | undefined;
  zoom: number | undefined;
  widthTwips: number | undefined;
}
// used to update the LOK visible area more efficiently
const visibleAreas: Record<DocumentRef, VisibleArea> = {};
// FIXME: this is kind of messy, since this is now duplicated with index.ts as well {
const TILE_DIM_PX = 256;
/** 15 = 1440 twips-per-inch / 96 dpi */
const LOK_INTERNAL_TWIPS_TO_PX = 15;
function getScaledTwips(zoom: number) {
  return clipToNearest8PxZoom(TILE_DIM_PX, 1 / zoom) * LOK_INTERNAL_TWIPS_TO_PX;
}
/** CSS pixels are DPI indepdendent */
function cssPxToTwips(px: number, zoom: number) {
  return Math.round(px * getScaledTwips(zoom));
}
// FIXME: }

console.time('load');
const lok = await LOK({
  withFcCache: true,
  callbackHandlers: {
    callback: function (ref: DocumentRef, type: number, payload: string): void {
      const message: WorkerCallback = {
        f: "c",
        d: ref,
        t: type,
        p: payload,
      };
      postMessage(message);
    },
  }
});

function postIdle(ref: DocumentRef) {
  postMessage({ f: "idle_", d: ref });
}
function postPaint(ref: DocumentRef) {
  postMessage({ f: "paint_", d: ref });
}

function rectArrayToRectangleTwips(r: RectArray): RectangleTwips {
  return {
    x: r[0],
    y: r[1],
    width: r[2],
    height: r[3],
  };
}

let running = false;
let isAlreadyIdle = false;

function run() {
  if (running) return;
  running = true;
  runLoop();
}

function runLoop() {
  lok.yield();
  if (currentDocumentUnderRender) {
    const [ref, viewId] = currentDocumentUnderRender;
    if (tileRenderer[ref]?.[viewId]?.paintAndRender()) {
      isAlreadyIdle = false;
      postPaint(ref);
    } else if (!isAlreadyIdle) {
      postIdle(ref);
      isAlreadyIdle = true;
    }
  }
  requestAnimationFrame(runLoop);
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
    run();
    return ref;
  },
  loadFromExpandedParts: function (
    name: string,
    data: Array<ExpandedPart>,
    readOnly: boolean,
  ) {
    const { Document, ExpandedDocument } = lok;
    const expandedDoc = new ExpandedDocument();
    for (const part of data) {
      expandedDoc.addPart(part.path, part.content);
    }

    const doc = new Document(expandedDoc, name, readOnly);
    const ref = doc.ref();

    if (!doc.valid()) {
      doc.delete();
      return null;
    }

    docMap[ref] = doc;
    run();
    return ref;
  },

  preload: function (): void {
    lok.preload();
    run();
    console.timeEnd('load');
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
  save: function (doc: Document): Array<{ path: string; sha: string }> {
    return doc.save();
  },
  saveAs: function (doc: Document, format: string): ArrayBuffer {
    const tmpFile = `/${Date.now()}.${format}`;
    // Optional arguments as emscripten can be undefined,
    // but the number of parameters must match the binding signature
    // so for optional parameters, we have to pass undefined
    // https://github.com/emscripten-core/emscripten/pull/21076/files
    doc.saveAs(`file://${tmpFile}`, format, undefined);
    // only buffer is transferable
    return lok.readUnlink(tmpFile).buffer as ArrayBuffer;
  },
  parts: function (doc: Document): number {
    return doc.getParts();
  },
  partRectanglesTwips: function (doc: Document): RectangleTwips[] {
    return doc.pageRects().map(rectArrayToRectangleTwips);
  },

  documentSize: function (
    doc: Document,
  ): [widthTwips: number, heightTwips: number] {
    return doc.getDocumentSize();
  },

  initializeForRendering: function (
    doc: Document,
    args: InitializeForRenderingOptions = {},
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
          "value": "${args.author ?? "Macro User"}"
        }}`,
    );
    return doc.getViewId();
  },

  postKeyEvent: function (
    doc: Document,
    viewId: ViewId,
    type: number,
    charCode: number,
    keyCode: number,
  ): void {
    return doc.postKeyEvent(viewId, type, charCode, keyCode);
  },

  postTextInput: function (
    doc: Document,
    viewId: ViewId,
    windowId: number,
    text: string,
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
    modifiers: number,
  ): void {
    return doc.postMouseEvent(viewId, type, x, y, count, buttons, modifiers);
  },

  setTextSelection: function (
    doc: Document,
    viewId: ViewId,
    type: number,
    x: number,
    y: Number,
  ): void {
    return doc.setTextSelection(viewId, type, x, y);
  },

  setClipboard: function (
    doc: Document,
    viewId: ViewId,
    items: SetClipbaordItem[],
  ): boolean {
    return doc.setClipboard(viewId, items);
  },

  getClipboard: function (
    doc: Document,
    viewId: ViewId,
    mimeTypes: string[],
  ): GetClipbaordItem[] {
    return doc.getClipboard(viewId, mimeTypes);
  },

  paste: function (
    doc: Document,
    viewId: ViewId,
    mimeType: string,
    data: string | ArrayBuffer,
  ): void {
    return doc.paste(viewId, mimeType, data);
  },

  setGraphicSelection: function (
    doc: Document,
    viewId: ViewId,
    type: number,
    x: number,
    y: number,
  ): void {
    return doc.setGraphicSelection(viewId, type, x, y);
  },

  resetSelection: function (doc: Document, viewId: ViewId): void {
    return doc.resetSelection(viewId);
  },

  getCommandValues: function (
    doc: Document,
    viewId: ViewId,
    command: string,
  ): any {
    const result = doc.getCommandValues(viewId, command);
    return result.startsWith("{") ? JSON.parse(result) : result;
  },

  subscribe: function (
    doc: Document,
    viewId: ViewId,
    callbacktype: number,
  ): void {
    doc.subscribe(viewId, callbacktype);
  },

  unsubscribe: function (
    doc: Document,
    viewId: ViewId,
    callbacktype: number,
  ): void {
    doc.unsubscribe(viewId, callbacktype);
  },

  startRendering: function (
    doc: Document,
    viewId: ViewId,
    canvases: [OffscreenCanvas],
    tileSize: TileDim,
    zoom: number,
    dpi: number,
    widthPx: number,
    heightPx: number,
    yPosPx: number,
  ): TileRendererData {
    const ref = doc.ref();
    const topTwips = cssPxToTwips(yPosPx, zoom);
    const heightTwips = cssPxToTwips(heightPx, zoom);
    const widthTwips = cssPxToTwips(widthPx, zoom);
    visibleAreas[ref] = {
      zoom,
      topTwips,
      widthTwips,
      heightTwips
    };
    doc.setClientVisibleArea(viewId, 0, topTwips, widthTwips, heightTwips);
    const result = doc.startTileRenderer(viewId, tileSize);
    if (!tileRenderer[ref]) {
      tileRenderer[ref] = {};
    }

    const r = renderer(doc);
    tileRenderer[ref][result.viewId] = r;

    currentDocumentUnderRender = [ref, result.viewId];

    r.initialize({
      c: canvases,
      d: result,
      s: zoom,
      y: yPosPx,
      dpi,
    });

    return {
      docRef: ref,
      viewId: result.viewId,
      scale: zoom,
    };
  },

  resetRendering: function (
    _doc: Document,
    _viewId: ViewId,
    _canvases: OffscreenCanvas[],
  ): void {
    throw new Error("Function not implemented.");
  },

  stopRendering: function (doc: Document, viewId: ViewId): void {
    doc.stopTileRenderer(viewId);
  },

  setScrollTop: function (
    doc: Document,
    viewId: ViewId,
    yPx: number,
  ): void {
    const ref = doc.ref();
    const r = tileRenderer[ref]?.[viewId];
    if (!r) return;
    const visibleArea = visibleAreas[ref];
    if (visibleArea && visibleArea.zoom) {
      const oldY = visibleArea.topTwips;
      visibleArea.topTwips = cssPxToTwips(yPx, visibleArea.zoom);
      if (oldY !== visibleArea.topTwips && visibleArea.widthTwips && visibleArea.heightTwips) {
        doc.setClientVisibleArea(viewId, 0, visibleArea.topTwips, visibleArea.widthTwips, visibleArea.heightTwips);
      }
    }

    r.scroll(yPx);
  },

  setVisibleHeight: function (
    doc: Document,
    viewId: ViewId,
    heightPx: number,
  ): void {
    const ref = doc.ref();
    const visibleArea = visibleAreas[ref];
    if (visibleArea && visibleArea.zoom) {
      const oldHeight = visibleArea.heightTwips;
      visibleArea.heightTwips = cssPxToTwips(heightPx, visibleArea.zoom);
      if (oldHeight !== visibleArea.heightTwips && visibleArea.widthTwips && visibleArea.topTwips) {
        doc.setClientVisibleArea(viewId, 0, visibleArea.topTwips, visibleArea.widthTwips, visibleArea.heightTwips);
      }
    }
    tileRenderer[ref]?.[viewId]?.resizeHeight(heightPx);
  },

  setDocumentWidth: function (
    doc: Document,
    viewId: ViewId,
    widthTwips: number,
  ): void {
    const ref = doc.ref();
    const visibleArea = visibleAreas[ref];
    if (visibleArea && visibleArea.widthTwips !== widthTwips) {
      visibleArea.widthTwips = widthTwips;
      if (visibleArea.heightTwips && visibleArea.topTwips) {
        doc.setClientVisibleArea(viewId, 0, visibleArea.topTwips, visibleArea.widthTwips, visibleArea.heightTwips);
      }
    }
    tileRenderer[ref]?.[viewId]?.resizeWidth(widthTwips);
  },

  dispatchCommand: function (
    doc: Document,
    viewId: ViewId,
    command: string,
    args?: any,
    notifyWhenFinished?: boolean,
  ): void {
    doc.dispatchCommand(
      viewId,
      command,
      args && typeof args === "string" ? args : JSON.stringify(args),
      notifyWhenFinished,
    );
  },

  removeText: function (
    doc: Document,
    viewId: ViewId,
    charsBefore: number,
    charsAfter: number,
  ): void {
    doc.removeText(
      viewId,
      0 /* window id is pretty much always 0 (the default window) */,
      charsBefore,
      charsAfter,
    );
  },

  setClientVisibleArea: function (
    doc: Document,
    viewId: ViewId,
    x: number,
    y: number,
    width: number,
    height: number,
  ): void {
    doc.setClientVisibleArea(viewId, x, y, width, height);
  },

  comments: function (
    doc: Document,
    viewId: ViewId,
    ids: number[] = [],
  ): Comment[] {
    doc.setCurrentView(viewId);
    return doc.comments(ids);
  },

  addComment: function (doc: Document, viewId: ViewId, text: string): void {
    doc.setCurrentView(viewId);
    doc.addComment(text);
  },

  replyComment: function (
    doc: Document,
    viewId: ViewId,
    parentId: number,
    text: string,
  ): void {
    doc.setCurrentView(viewId);
    doc.replyComment(parentId, text);
  },

  deleteCommentThreads: function (
    doc: Document,
    viewId: ViewId,
    parentIds: number[],
  ): void {
    doc.setCurrentView(viewId);
    doc.deleteCommentThreads(parentIds);
  },

  deleteComment: function (
    doc: Document,
    viewId: ViewId,
    commentId: number,
  ): void {
    doc.setCurrentView(viewId);
    doc.deleteComment(commentId);
  },

  resolveCommentThread: function (
    doc: Document,
    viewId: ViewId,
    parentId: number,
  ): void {
    doc.setCurrentView(viewId);
    doc.resolveCommentThread(parentId);
  },

  resolveComment: function (
    doc: Document,
    viewId: ViewId,
    commentId: number,
  ): void {
    doc.setCurrentView(viewId);
    doc.resolveComment(commentId);
  },

  sanitize: function (
    doc: Document,
    viewId: ViewId,
    options: SanitizeOptions,
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
    property: string,
  ): any {
    doc.setCurrentView(viewId);
    return doc.getPropertyValue(property);
  },

  setPropertyValue: function (
    doc: Document,
    viewId: ViewId,
    property: string,
    value: any,
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
    properties: T,
  ): ParagraphStyle<T> {
    doc.setCurrentView(viewId);
    return doc.getParagraphStyle(name, properties) as ParagraphStyle<T>;
  },

  getSelectionText: function (
    doc: Document,
    viewId: ViewId,
  ): string | undefined {
    doc.setCurrentView(viewId);
    return doc.getSelectionText();
  },

  paragraphStyles: function (
    doc: Document,
    viewId: ViewId,
    properties: string[],
  ) {
    doc.setCurrentView(viewId);
    return doc.paragraphStyles(properties);
  },

  setZoom: async function (
    doc: Document,
    viewId: ViewId,
    scale: number,
    dpi: number,
  ): Promise<void> {
    tileRenderer[doc.ref()]?.[viewId]?.zoom({
      s: scale,
      d: dpi,
    })
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

  getExpandedPart: function (doc: Document, _viewId: ViewId, path: string) {
    return doc.getExpandedPart(path);
  },
  listExpandedParts: function (doc: Document, _viewId: ViewId) {
    return doc.listExpandedParts();
  },
  updateComment: function (
    doc: Document,
    viewId: ViewId,
    id: number,
    text: string,
  ) {
    doc.setCurrentView(viewId);
    doc.updateComment(id, text);
  },

  addExternalUndo: function (doc: Document, viewId: ViewId) {
    doc.setCurrentView(viewId);
    return doc.addExternalUndo();
  },

  getNextUndoId: function (doc: Document, viewId: ViewId) {
    doc.setCurrentView(viewId);
    return doc.getNextUndoId();
  },

  getNextRedoId: function (doc: Document, viewId: ViewId) {
    doc.setCurrentView(viewId);
    return doc.getNextRedoId();
  },

  getUndoCount: function (doc: Document, viewId: ViewId) {
    doc.setCurrentView(viewId);
    return doc.getUndoCount();
  },

  getRedoCount: function (doc: Document, viewId: ViewId) {
    doc.setCurrentView(viewId);
    return doc.getRedoCount();
  },

  undo: function (doc: Document, viewId: ViewId, count: number) {
    doc.setCurrentView(viewId);
    doc.undo(count);
  },

  redo: function (doc: Document, viewId: ViewId, count: number) {
    doc.setCurrentView(viewId);
    doc.redo(count);
  },

  getRedlineTextRange: function (doc: Document, viewId: ViewId, id: number) {
    doc.setCurrentView(viewId);
    return doc.getRedlineTextRange(id);
  },

  getCursor: function (doc: Document, viewId: ViewId) {
    return doc.getCursor(viewId);
  },

  getCustomStringProperty: function (
    doc: Document,
    _viewId: ViewId,
    property: string,
  ) {
    return doc.getCustomStringProperty(property);
  },

  setCustomStringProperty: function (
    doc: Document,
    _viewId: ViewId,
    property: string,
    value: string,
  ) {
    doc.setCustomStringProperty(property, value);
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
      mode: "wildcard" | "regex" | "similar";
    }>,
  ): ForwardingId {
    findResultMap[docRef]?.delete();
    findResultMap[docRef] = doc.findAll(text, options) as ITextRanges &
      EmbindingDisposable;
    return [docRef, viewId, "_find"];
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
  saveAs: true,
};

function isTransferable<K extends keyof Message = keyof Message>(
  fromWorker: FromWorker<K>,
): fromWorker is TransferableResult<K> {
  return tranferables[fromWorker.f] === true;
}

function handleForwardedMethod<K extends ForwardedResolver = ForwardedResolver>(
  data: ToWorker<K>,
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
    console.error("doc ref missing");
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
    console.error("doc ref missing");
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
      data as ToWorker<keyof DocumentMethodHandler<Document>>,
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
      data as ToWorker<keyof ForwardingMethodHandlers<Document>>,
    );
  }

  return handleGlobalMethod(data as ToWorker<keyof GlobalMethod>);
};

postMessage({
  f: "_keys",
  keys: [...Object.keys(handler), ...Object.keys(forwarding)],
  forwarded: Object.fromEntries(
    Object.entries(forwardedMethods).map(([name, method]) => [
      name,
      Object.keys(method),
    ]),
  ),
} as KeysMessage);
for (const message of messageQueue) {
  globalThis.onmessage(message);
}
