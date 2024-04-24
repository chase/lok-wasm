import type {
  Message,
  FromWorker,
  DocumentRef,
  ToWorker,
  RectangleTwips,
  SetClipbaordItem,
  GetClipbaordItem,
  WorkerCallback,
  InitializeForRenderingOptions,
  ViewId,
  TileRendererData,
  TileDim,
  ToTileRenderer,
  AsyncGlobalMethod,
  AsyncDocumentMethod,
} from './shared';
import LOK from './soffice';
import type { Document } from './soffice';
// NOTE: Disabled until unoembind startup cost is under 1s
// import { init_unoembind_uno } from './bindings_uno';

const docMap: Record<DocumentRef, Document> = {};
const byRef = (ref: DocumentRef) => docMap[ref];
const tileRenderer: Record<DocumentRef, Record<ViewId, Worker>> = {};

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

const globalHandler: AsyncGlobalMethod = {
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
  preload: async function (): Promise<void> {
    (await lokPromise).preload();
  },
};

const handler: AsyncDocumentMethod<Document> = {
  newView: async function (doc: Document): Promise<DocumentRef> {
    return doc.newView();
  },

  close: async function (doc: Document): Promise<void> {
    doc.delete();
  },

  save: async function (doc: Document, format: string): Promise<ArrayBuffer> {
    const { readUnlink } = await lokPromise;
    const tmpFile = `/${Date.now()}.${format}`;
    // Optional arguments as emscripten can be undefined,
    // but the number of parameters must match the binding signature
    // so for optional parameters, we have to pass undefined
    // https://github.com/emscripten-core/emscripten/pull/21076/files
    doc.saveAs(`file://${tmpFile}`, format, undefined);
    // only buffer is transferable
    return readUnlink(tmpFile).buffer;
  },

  parts: async function (doc: Document): Promise<number> {
    return doc.getParts();
  },

  partRectanglesTwips: async function (
    doc: Document
  ): Promise<RectangleTwips[]> {
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
    doc: Document
  ): Promise<[widthTwips: number, heightTwips: number]> {
    return doc.getDocumentSize();
  },

  initializeForRendering: async function (
    doc: Document,
    args: InitializeForRenderingOptions = {}
  ): Promise<number> {
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
    doc: Document,
    viewId: ViewId,
    type: number,
    charCode: number,
    keyCode: number
  ): Promise<void> {
    return doc.postKeyEvent(viewId, type, charCode, keyCode);
  },

  postTextInput: async function (
    doc: Document,
    viewId: ViewId,
    windowId: number,
    text: string
  ): Promise<void> {
    return doc.postTextInputEvent(viewId, windowId, text);
  },

  postMouseEvent: async function (
    doc: Document,
    viewId: ViewId,
    type: number,
    x: number,
    y: number,
    count: number,
    buttons: number,
    modifiers: number
  ): Promise<void> {
    return doc.postMouseEvent(viewId, type, x, y, count, buttons, modifiers);
  },

  setTextSelection: async function (
    doc: Document,
    viewId: ViewId,
    type: number,
    x: number,
    y: Number
  ): Promise<void> {
    return doc.setTextSelection(viewId, type, x, y);
  },

  setClipboard: async function (
    doc: Document,
    viewId: ViewId,
    items: SetClipbaordItem[]
  ): Promise<boolean> {
    return doc.setClipboard(viewId, items);
  },

  getClipboard: async function (
    doc: Document,
    viewId: ViewId,
    mimeTypes: string[]
  ): Promise<GetClipbaordItem[]> {
    return doc.getClipboard(viewId, mimeTypes);
  },

  paste: async function (
    doc: Document,
    viewId: ViewId,
    mimeType: string,
    data: string | ArrayBuffer
  ): Promise<void> {
    return doc.paste(viewId, mimeType, data);
  },

  setGraphicSelection: async function (
    doc: Document,
    viewId: ViewId,
    type: number,
    x: number,
    y: number
  ): Promise<void> {
    return doc.setGraphicSelection(viewId, type, x, y);
  },

  resetSelection: async function (
    doc: Document,
    viewId: ViewId
  ): Promise<void> {
    return doc.resetSelection(viewId);
  },

  getCommandValues: async function (
    doc: Document,
    viewId: ViewId,
    command: string
  ): Promise<any> {
    const result = doc.getCommandValues(viewId, command);
    return result.startsWith('{') ? JSON.parse(result) : result;
  },

  subscribe: async function (
    doc: Document,
    viewId: ViewId,
    callbacktype: number
  ): Promise<void> {
    doc.subscribe(viewId, callbacktype);
  },

  unsubscribe: async function (
    doc: Document,
    viewId: ViewId,
    callbacktype: number
  ): Promise<void> {
    doc.unsubscribe(viewId, callbacktype);
  },

  startRendering: async function (
    doc: Document,
    viewId: ViewId,
    canvas: OffscreenCanvas,
    tileSize: TileDim,
    scale: number,
    yPos: number = 0
  ): Promise<TileRendererData> {
    await lokPromise;
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
    doc: Document,
    viewId: ViewId,
    canvas: OffscreenCanvas
  ): Promise<void> {
    throw new Error('Function not implemented.');
  },

  stopRendering: function (doc: Document, viewId: ViewId): Promise<void> {
    throw new Error('Function not implemented.');
  },

  setScrollTop: async function (
    doc: Document,
    viewId: ViewId,
    yPx: number
  ): Promise<void> {
    tileRenderer[doc.ref()]?.[viewId]?.postMessage({
      t: 's',
      y: yPx,
    } as ToTileRenderer);
  },

  setVisibleHeight: async function (
    doc: Document,
    viewId: ViewId,
    heightPx: number
  ): Promise<void> {
    tileRenderer[doc.ref()]?.[viewId]?.postMessage({
      t: 'r',
      h: heightPx,
    } as ToTileRenderer);
  },

  dispatchCommand: async function (
    doc: Document,
    viewId: ViewId,
    command: string,
    args?: any,
    notifyWhenFinished?: boolean
  ): Promise<void> {
    doc.dispatchCommand(
      viewId,
      command,
      typeof args === 'string' ? args : JSON.stringify(args),
      notifyWhenFinished
    );
  },

  removeText: async function (
    doc: Document,
    viewId: ViewId,
    charsBefore: number,
    charsAfter: number
  ): Promise<void> {
    doc.removeText(
      viewId,
      0 /* window id is pretty much always 0 (the default window) */,
      charsBefore,
      charsAfter
    );
  },

  setClientVisibleArea: async function (
    doc: Document,
    viewId: ViewId,
    x: number,
    y: number,
    width: number,
    height: number
  ): Promise<void> {
    doc.setClientVisibleArea(viewId, x, y, width, height);
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

onmessage = async <K extends keyof Message = keyof Message>({
  data,
}: MessageEvent<ToWorker<K>>) => {
  const { i, f, a } = data;
  let r: Awaited<ReturnType<Message[K]>>;
  const docHandler: (doc: Document, ...args: any[]) => Promise<any> =
    handler[f as keyof AsyncDocumentMethod<Document>];
  if (docHandler) {
    const [ref, ...rest] = a;
    const doc = byRef(ref as DocumentRef);
    if (!doc) {
      console.error('doc ref missing');
      return;
    }
    await lokPromise;
    r = await docHandler(doc, ...rest);
  } else {
    r = await (
      globalHandler[f as keyof AsyncGlobalMethod] as (
        ...args: any[]
      ) => Promise<any>
    )(...a);
  }
  const message: FromWorker<K> = {
    f,
    i,
    r,
  };
  if (isTransferable(message)) {
    postMessage(message, { transfer: [message.r] });
  } else {
    postMessage(message);
  }
};
