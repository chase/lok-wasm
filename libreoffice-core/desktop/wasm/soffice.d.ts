/// <reference types="emscripten" />

type SetClipbaordItem = {
  mimeType: string;
  buffer: string | ArrayBuffer;
};

type GetClipbaordItem =
  | {
      mimeType: 'text/plain' | 'text/html';
      text: string;
    }
  | {
      mimeType: string;
      data: Uint8Array;
    };

export type DocumentRef = number & {};

export type TileRenderData = {
  readonly viewId: number;
  readonly tileSize: number;
  /** `_Atomic int32_t` */
  state: Int32Array;
  /** `uint32_t[4]` */
  tileTwips: Uint32Array;
  /** `uint8_t[]` */
  paintedTile: Uint8Array;
  /** `_Atomic int32_t` */
  pendingFullPaint: Int32Array;
  /** `_Atomic int32_t` */
  hasInvalidations: Int32Array;
  /** `_Atomic uint32_t[MAX_INVALIDATION_STACK][4]` */
  invalidationStack: Uint32Array;
  /** `_Atomic int32_t` */
  invalidationStackHead: Int32Array;
  /** `_Atomic uint32_t` */
  docWidthTwips: Uint32Array;
  /** `_Atomic uint32_t` */
  docHeightTwips: Uint32Array;
};

/** Embind Document class, see main_wasm.cxx */
export declare class Document {
  constructor(path: string): void;
  delete(): void;

  valid(): boolean;
  saveAs(path: string, format?: string, filterOptions?: string): boolean;
  getParts(): number;
  getPartRectangles(): string;
  paintTile(
    tileWidthPx: number,
    tileHeightPx: number,
    xTwips: number,
    yTwips: number
  ): Uint8Array;
  getDocumentSize(): [widthTwips: number, heightTwips: number];
  initializeForRendering(args: string): void;
  getViewId(): number;
  newView(): number;
  ref(): DocumentRef;
  postKeyEvent(
    viewId: number,
    type: number,
    charCode: number,
    keyCode: number
  ): void;
  postTextInputEvent(viewId: number, windowId: number, text: string): void;
  postMouseEvent(
    viewId: number,
    type: number,
    x: number,
    y: number,
    count: number,
    buttons: number,
    modifiers: number
  ): void;
  setTextSelection(viewId: number, type: number, x: number, y: Number): void;
  setClipboard(viewId: number, items: SetClipbaordItem[]): boolean;
  getClipboard(viewId: number, mimeTypes: string[]): GetClipbaordItem[];
  paste(viewId: number, mimeType: string, data: string | ArrayBuffer): void;
  setGraphicSelection(viewId: number, type: number, x: number, y: number): void;
  resetSelection(viewId: number): void;
  getCommandValues(viewId: number, command: string): string;
  subscribe(viewId: number, type: number): void;
  unsubscribe(viewId: number, type: number): void;
  startTileRenderer(viewId: number, tileSize: number): TileRenderData;
  setClientVisibleArea(
    viewId: number,
    x: number,
    y: number,
    width: number,
    heght: number
  ): void;
  setCurrentView(viewId: number): void;
  dispatchCommand(
    viewId: number,
    command: string,
    args?: string,
    notifyWhenFinished?: boolean
  ): void;
  removeText(
    viewId: number,
    windowId: number,
    charsBefore: number,
    charsAfter: number,
  ): void;
}

declare global {
  interface WorkerGlobalScope {
    /** returns the Document by the associated DocumentRef */
    byRef(ref: DocumentRef): Document | undefined;
    /** registers new methods for Document */
    registerExtension(handlers: {
      [name: string]: (ref: DocumentRef, ...args: any[]) => Promise<any>;
    }): void;
  }
}

interface Module extends EmscriptenModule {
  /** indicate whether to load fccache with the module */
  withFcCache?: boolean;

  /** mounts the blob at a file path with `name` and returns the file path */
  mountBlob(name: string, blob: Blob): string;

  /** unmounts the current blob */
  unmountBlob(): void;

  /** reads and unlinks a temporary file at path */
  readUnlink(path: string): Uint8Array;

  Document: typeof Document;

  /** preloads LOK */
  preload(): void;

  /** makes it so that all accelerator configuration files are handled as if they are on macOS */
  setIsMacOSForConfig(): void;

  /** creates the font config cache so that it can be exported to optimize load times */
  createFontConfigCache(): void;

  getDirectoryFiles(dir: string): Array<{ name: string; buffer: ArrayBuffer }>;

  /** handlers for subscribed document events */
  callbackHandlers: {
    callback(ref: DocumentRef, type: number, payload: string): void;
  };
}
declare const LOK: EmscriptenModuleFactory<Module>;
export default LOK;
