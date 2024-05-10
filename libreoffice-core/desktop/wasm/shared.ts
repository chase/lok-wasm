export type Id = number & {};
export type DocumentRef = number & {};
export type ViewId = number & {};
import type { CallbackType } from './lok_enums';
import type { TileRenderData } from './soffice';
export type GlobalMessage = {
  /** load the document with the file name `name` and content `blob`
  @returns the corresponding document on success, null otherwise */
  load(name: string, blob: Blob): DocumentRef | null;
  importScript(url: string): void;
  preload(): void;
  setIsMacOSForConfig(): void;
};

type Rectangle = {
  x: number;
  y: number;
  width: number;
  height: number;
};

export type RectanglePx = Rectangle & {};
export type RectangleTwips = Rectangle & {};
export type SetClipbaordItem = {
  mimeType: string;
  buffer: string | ArrayBuffer;
};

export type GetClipbaordItem =
  | {
      mimeType: 'text/plain' | 'text/html';
      text: string;
    }
  | {
      mimeType: string;
      data: Uint8Array;
    };

export type InitializeForRenderingOptions = Partial<{
  autoSpellcheck: boolean;
  author: string;
}>;

export type TileRendererData = {
  readonly docRef: number;
  readonly viewId: number;
  readonly scale: number;
};

export type TileDim = 256 | 512 | 1024 | 2048;

export type DocumentMethods = {
  /** closes the document */
  close(): void;
  /** returns a copy of the document in the provided `format` */
  save(format: 'docx' | 'pdf'): ArrayBuffer;
  /** returns the id of the new view created */
  newView(): number;
  /** returns the number of pages */
  parts(): number;
  /** returns the rectangles of each page in twips (1/12 pt) */
  partRectanglesTwips(): RectangleTwips[];
  /** returns document size in twips */
  documentSize(): [widthTwips: number, heightTwips: number];
  /** initializes the document for tiled rendering and returns the view ID, must be called immediately after loading */
  initializeForRendering(args?: InitializeForRenderingOptions): ViewId;
};

export type DocumentWithViewMethods = {
  postKeyEvent(type: number, charCode: number, keyCode: number): void;
  postTextInput(windowId: number, text: string): void;
  postMouseEvent(
    type: number,
    x: number,
    y: number,
    count: number,
    buttons: number,
    modifiers: number
  ): void;
  setTextSelection(type: number, x: number, y: Number): void;
  setClipboard(items: SetClipbaordItem[]): boolean;
  getClipboard(mimeTypes: string[]): GetClipbaordItem[];
  paste(mimeType: string, data: string | ArrayBuffer): void;
  setGraphicSelection(type: number, x: number, y: number): void;
  resetSelection(): void;
  getCommandValues(command: string): any;
  subscribe(callbacktype: CallbackType): void;
  unsubscribe(callbackType: CallbackType): void;
  dispatchCommand(
    command: string,
    args?: any,
    notifyWhenFinished?: boolean
  ): void;
  removeText(charsBefore: number, charsAfter: number): void;
  setClientVisibleArea(
    x: number,
    y: number,
    width: number,
    height: number
  ): void;

  startRendering(
    canvases: OffscreenCanvas[],
    tileSize: TileDim,
    /** Non-negative float, 1.0 is unchange, less than 1.0 is smaller, greater than 1.0 is larger */
    scale: number,
    /** scroll top position in pixels */
    yPosPx?: number
  ): TileRendererData;

  setScrollTop(yPx: number): number;
  setVisibleHeight(heightPx: number): void;
  setZoom(scale: number, dpi: number): void;

  /** TODO: implement, used to set a new scale or set a new offscreen cavnas */
  resetRendering(
    canvas: OffscreenCanvas[],
    /** Non-negative float, 1.0 is unchanged, less than 1.0 is smaller, greater than 1.0 is larger */
    scale: number
  ): void;
  /** TODO: implement */
  stopRendering(): void;
};

type DocumentMessage = {
  [K in keyof DocumentMethods]: (
    ref: DocumentRef,
    ...args: Parameters<DocumentMethods[K]>
  ) => ReturnType<DocumentMethods[K]>;
};
type DocumentWithViewMessage = {
  [K in keyof DocumentWithViewMethods]: (
    ref: DocumentRef,
    viewId: ViewId,
    ...args: Parameters<DocumentWithViewMethods[K]>
  ) => ReturnType<DocumentWithViewMethods[K]>;
};
export type Message = GlobalMessage & DocumentMessage & DocumentWithViewMessage;

export type AsyncMessage = {
  [K in keyof Message]: (
    ...args: Parameters<Message[K]>
  ) => Promise<ReturnType<Message[K]>>;
};

export type CallbackHandler = (payload: string) => void;
export type DocumentClientBase = {
  readonly ref: DocumentRef;
  readonly viewId: ViewId;
  on(type: CallbackType, handler: CallbackHandler): void;
  off(type: CallbackType, handler: CallbackHandler): void;
  newView(): Promise<DocumentClient | null>;
};
export type DocumentClient<
  E extends { [k: string]: (...args: any) => any } = {},
> = {
  [K in keyof DocumentMethods]: (
    ...args: Parameters<DocumentMethods[K]>
  ) => Promise<ReturnType<DocumentMethods[K]>>;
} & {
  [K in keyof DocumentWithViewMethods]: (
    ...args: Parameters<DocumentWithViewMethods[K]>
  ) => Promise<ReturnType<DocumentWithViewMethods[K]>>;
} & DocumentClientBase &
  E;

export type ToWorker<K extends keyof Message = keyof Message> = {
  f: K;
  i: Id;
  a: Parameters<Message[K]>;
};

export type WorkerCallback = {
  f: 'c';
  d: DocumentRef;
  t: number;
  p: string;
};

export type FromWorker<K extends keyof Message = keyof Message> = {
  f: K;
  i: Id;
  r: ReturnType<Message[K]>;
};

export type ToTileRenderer =
  | {
      /** initialize */
      t: 'i';
      c: OffscreenCanvas[];
      d: TileRenderData;
      /** absolute scale */
      s: number;
      /** top position in pixels */
      y: number;
      /** dpi */
      dpi: number;
    }
  | {
      /** scroll */
      t: 's';
      /** view height in pixels */
      y: number;
    }
  | {
      /** resize */
      t: 'r';
      /** height */
      h: number;
    }
  | {
      /** zoom */
      t: 'z';
      /** absolute scale */
      s: number;
      /** dpi */
      d: number;
      /** scrollTop position in px */
      y: number;
    };

export type Ref<T> = {
  current?: T;
};
