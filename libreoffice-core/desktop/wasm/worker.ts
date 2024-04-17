import type {
  Message,
  FromWorker,
  DocumentRef,
  ToWorker,
  AsyncMessage,
  RectangleTwips,
  SetClipbaordItem,
  GetClipbaordItem,
  WorkerCallback,
  InitializeForRenderingOptions,
  ViewId,
  TileRendererData,
  TileDim,
  ToTileRenderer,
} from './shared';
import LOK from './soffice';
import type { Document } from './soffice';

const docMap: Record<DocumentRef, Document> = {};
const byRef = (ref: DocumentRef) => docMap[ref];
const tileRenderer: Record<DocumentRef, Record<ViewId, Worker>> = {};
self.byRef = byRef;

const lokPromise = LOK({
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

const handler: AsyncMessage = {
  load: async function (name: string, blob: Blob): Promise<DocumentRef | null> {
    const { mountBlob, unmountBlob, Document } = await lokPromise;
    const doc = new Document(`file://${mountBlob(name, blob)}`);
    const ref = doc.ref();
    unmountBlob();

    if (!doc.valid()) {
      doc.delete();
      return null;
    }

    docMap[ref] = doc;
    return ref;
  },

  close: async function (ref: DocumentRef): Promise<void> {
    const doc = docMap[ref];
    doc?.delete();
  },

  save: async function (
    ref: DocumentRef,
    format: string
  ): Promise<ArrayBuffer> {
    const { readUnlink } = await lokPromise;
    const tmpFile = `/${Date.now()}.${format}`;
    // Optional arguments as emscripten can be undefined,
    // but the number of parameters must match the binding signature
    // so for optional parameters, we have to pass undefined
    // https://github.com/emscripten-core/emscripten/pull/21076/files
    byRef(ref)?.saveAs(`file://${tmpFile}`, format, undefined);
    // only buffer is transferable
    return readUnlink(tmpFile).buffer;
  },

  parts: async function (ref: DocumentRef): Promise<number> {
    await lokPromise;
    return self.byRef(ref)?.getParts();
  },

  partRectanglesTwips: async function (
    ref: DocumentRef
  ): Promise<RectangleTwips[]> {
    await lokPromise;
    const doc = byRef(ref);
    if (!doc) return [];
    return doc
      .getPartRectangles()
      .split(/;\s*/)
      .map((rect) => {
        const [x, y, width, height] = rect.split(/,\s*/).map(Number);
        return {
          x,
          y,
          width,
          height,
        };
      });
  },

  documentSize: async function (
    ref: DocumentRef
  ): Promise<[widthTwips: number, heightTwips: number]> {
    await lokPromise;
    return byRef(ref)?.getDocumentSize();
  },

  importScript: async function (url: string) {
    importScripts(url);
  },

  initializeForRendering: async function (
    ref: DocumentRef,
    args: InitializeForRenderingOptions = {}
  ): Promise<number> {
    await lokPromise;
    const doc = byRef(ref);
    if (!doc) return -1;
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

  postKeyEvent: async function (
    ref: DocumentRef,
    viewId: ViewId,
    type: number,
    charCode: number,
    keyCode: number
  ): Promise<void> {
    await lokPromise;
    return byRef(ref)?.postKeyEvent(viewId, type, charCode, keyCode);
  },

  postTextInput: async function (
    ref: DocumentRef,
    viewId: ViewId,
    windowId: number,
    text: string
  ): Promise<void> {
    await lokPromise;
    return byRef(ref)?.postTextInputEvent(viewId, windowId, text);
  },

  postMouseEvent: async function (
    ref: DocumentRef,
    viewId: ViewId,
    type: number,
    x: number,
    y: number,
    count: number,
    buttons: number,
    modifiers: number
  ): Promise<void> {
    await lokPromise;
    return byRef(ref)?.postMouseEvent(
      viewId,
      type,
      x,
      y,
      count,
      buttons,
      modifiers
    );
  },

  setTextSelection: async function (
    ref: DocumentRef,
    viewId: ViewId,
    type: number,
    x: number,
    y: Number
  ): Promise<void> {
    await lokPromise;
    return byRef(ref)?.setTextSelection(viewId, type, x, y);
  },

  setClipboard: async function (
    ref: DocumentRef,
    viewId: ViewId,
    items: SetClipbaordItem[]
  ): Promise<boolean> {
    await lokPromise;
    return byRef(ref)?.setClipboard(viewId, items);
  },

  getClipboard: async function (
    ref: DocumentRef,
    viewId: ViewId,
    mimeTypes: string[]
  ): Promise<GetClipbaordItem[]> {
    await lokPromise;
    return byRef(ref)?.getClipboard(viewId, mimeTypes);
  },

  paste: async function (
    ref: DocumentRef,
    viewId: ViewId,
    mimeType: string,
    data: string | ArrayBuffer
  ): Promise<void> {
    await lokPromise;
    return byRef(ref)?.paste(viewId, mimeType, data);
  },

  setGraphicSelection: async function (
    ref: DocumentRef,
    viewId: ViewId,
    type: number,
    x: number,
    y: number
  ): Promise<void> {
    await lokPromise;
    return byRef(ref)?.setGraphicSelection(viewId, type, x, y);
  },

  resetSelection: async function (
    ref: DocumentRef,
    viewId: ViewId
  ): Promise<void> {
    await lokPromise;
    return byRef(ref)?.resetSelection(viewId);
  },

  getCommandValues: async function (
    ref: DocumentRef,
    viewId: ViewId,
    command: string
  ): Promise<any> {
    await lokPromise;
    const result = byRef(ref)?.getCommandValues(viewId, command);
    return result.startsWith('{') ? JSON.parse(result) : result;
  },

  preload: async function (): Promise<void> {
    (await lokPromise).preload();
  },

  subscribe: async function (
    ref: DocumentRef,
    viewId: ViewId,
    callbacktype: number
  ): Promise<void> {
    await lokPromise;
    byRef(ref)?.subscribe(viewId, callbacktype);
  },

  unsubscribe: async function (
    ref: DocumentRef,
    viewId: ViewId,
    callbacktype: number
  ): Promise<void> {
    await lokPromise;
    byRef(ref)?.unsubscribe(viewId, callbacktype);
  },

  startRendering: async function (
    ref: DocumentRef,
    viewId: ViewId,
    canvas: OffscreenCanvas,
    tileSize: TileDim,
    scale: number,
    yPos: number = 0
  ): Promise<TileRendererData> {
    await lokPromise;
    const doc = byRef(ref);
    if (!doc) {
      throw new Error("Doc doesn't exist while trying to render");
    }
    const result = doc.startTileRenderer(viewId, tileSize);
    const worker = new Worker(
      new URL('./tile_renderer_worker.js', import.meta.url),
      { type: 'module' }
    );
    if (!tileRenderer[ref]) {
      tileRenderer[ref] = {};
    }
    tileRenderer[ref][result.viewId] = worker;

    worker.postMessage(
      {
        t: 'i',
        c: canvas,
        d: result,
        s: scale,
        y: yPos,
      } as ToTileRenderer,
      { transfer: [canvas] }
    );

    return {
      docRef: ref,
      viewId: result.viewId,
      scale,
    };
  },

  resetRendering: function (
    ref: DocumentRef,
    viewId: ViewId,
    canvas: OffscreenCanvas
  ): Promise<void> {
    throw new Error('Function not implemented.');
  },

  stopRendering: function (ref: DocumentRef, viewId: ViewId): Promise<void> {
    throw new Error('Function not implemented.');
  },

  setScrollTop: async function (
    ref: DocumentRef,
    viewId: ViewId,
    yPx: number
  ): Promise<void> {
    tileRenderer[ref]?.[viewId]?.postMessage({
      t: 's',
      y: yPx,
    } as ToTileRenderer);
  },

  setVisibleHeight: async function (
    ref: DocumentRef,
    viewId: ViewId,
    heightPx: number
  ): Promise<void> {
    tileRenderer[ref]?.[viewId]?.postMessage({
      t: 'r',
      h: heightPx,
    } as ToTileRenderer);
  },
  dispatchCommand: async function (
    ref: DocumentRef,
    viewId: ViewId,
    command: string,
    args?: any,
    notifyWhenFinished?: boolean
  ): Promise<void> {
    await lokPromise;
    byRef(ref)?.dispatchCommand(
      viewId,
      command,
      typeof args === 'string' ? args : JSON.stringify(args),
      notifyWhenFinished
    );
  },
  removeText: async function (
    ref: DocumentRef,
    viewId: ViewId,
    charsBefore: number,
    charsAfter: number
  ): Promise<void> {
    await lokPromise;
    byRef(ref)?.removeText(
      viewId,
      0 /* window id is pretty much always 0 (the default window) */,
      charsBefore,
      charsAfter
    );
  },
  setClientVisibleArea: async function (
    ref: DocumentRef,
    viewId: ViewId,
    x: number,
    y: number,
    width: number,
    height: number
  ): Promise<void> {
    await lokPromise;
    byRef(ref)?.setClientVisibleArea(viewId, x, y, width, height);
  },
};

// this is used by imported scripts to register their handlers
self.registerExtension = (newHandlers) => {
  Object.assign(handler, newHandlers);
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

onmessage = async <K extends keyof Message = keyof Message>({
  data,
}: MessageEvent<ToWorker<K>>) => {
  const { i, f, a } = data;
  const message: FromWorker<K> = {
    f,
    i,
    r: await handler[f](...a),
  };
  if (isTransferable(message)) {
    postMessage(message, { transfer: [message.r] });
  } else {
    postMessage(message);
  }
};
