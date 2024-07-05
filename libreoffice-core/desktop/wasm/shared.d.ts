export type Id = number & {};
export type ViewId = number & {};
import type { CallbackType } from './lok_enums';
import type {
  Document,
  RootComment,
  Comment,
  FindAllOptions,
  HeaderFooterRect,
  ITextRanges,
  OutlineItem,
  ParagraphStyle,
  ParagraphStyleList,
  RectArray,
  TileRenderData,
  DocumentRef,
  ExpandedPart,
} from './soffice';
export type GlobalMessage = {
  /** load the document with the file name `name` and content `blob`
  @returns the corresponding document on success, null otherwise */
  load(name: string, blob: Blob): DocumentRef | null;
  loadFromExpandedParts(name: string, data: Array<ExpandedPart>, readOnly: boolean): DocumentRef | null;
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
  setDocumentWidth(widthTwips: number): void;
  setZoom(scale: number, dpi: number): void;

  /** TODO: implement, used to set a new scale or set a new offscreen cavnas */
  resetRendering(
    canvas: OffscreenCanvas[],
    /** Non-negative float, 1.0 is unchanged, less than 1.0 is smaller, greater than 1.0 is larger */
    scale: number
  ): void;
  /** TODO: implement */
  stopRendering(): void;
  // NOTE: Disabled until unoembind startup cost is under 1s
  // getXComponent(): void;

  comments(ids?: number[]): Array<Comment | RootComment>;
  addComment(text: string): void;
  replyComment(parentId: number, text: string): void;
  updateComment(id: number, text: string): void;
  deleteCommentThreads(parentIds: number[]): void;
  deleteComment(commentId: number): void;
  resolveCommentThread(parentId: number): void;
  resolveComment(commentId: number): void;
  sanitize(options: {
    documentMetadata?: boolean;
    trackChangesAccept?: boolean;
    trackChangesReject?: boolean;
    comments?: boolean;
  }): void;

  /** if inside of header/footer returns the type and its area rectangle, otherwise undefined */
  headerFooterRect(): HeaderFooterRect | undefined;

  getPropertyValue(property: string): any;
  setPropertyValue(property: string, value: any): void;
  saveCurrentSelection(): void;
  restoreCurrentSelection(): void;

  getSelectionText(): string | undefined;

  /** gets a paragraph style with the given name and returns the requested properties of that style if found, undefined otherwise */
  getParagraphStyle<T extends string[]>(
    name: string,
    properties: T
  ): ParagraphStyle<T>;

  /** get a list of all paragraph styles */
  paragraphStyles: Document['paragraphStyles'];

  getOutline(): OutlineItem[];
  gotoOutline(index: number): RectArray;

  /** adds an undo item and returns the external id number to track it */
  addExternalUndo(): number;
  getNextUndoId(): number;
  getNextRedoId(): number;
  getUndoCount(): number;
  getRedoCount(): number;
  undo(count: number): void;
  redo(count: number): void;

  setAuthor(author: string): void;

  getExpandedPart(path: string): {path: string, content: ArrayBuffer} | null;
  listExpandedParts(): Array<{path: string, sha: string}>;
  getRedlineTextRange(id: number): RectArray[] | undefined;
};

/** methods that forward to a class weakly bound to the Document that forwards calls */
export type ResolverToForwardingMethod = {
  _find: {
    findAll(text: string, options?: FindAllOptions | undefined): ITextRanges;
  };
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

export type ForwardedResolver = keyof ResolverToForwardingMethod;

export type ForwardingId<K extends ForwardedResolver = ForwardedResolver> = [
  DocumentRef,
  ViewId,
  K,
];

/** resolves all methods in ResolverToForwardingMethod */
export type ForwardingMethod<
  K extends keyof ResolverToForwardingMethod = keyof ResolverToForwardingMethod,
  M extends ResolverToForwardingMethod[K] = ResolverToForwardingMethod[K],
  F extends M[keyof M] & ((...args: any) => any) = Extract<
    M[keyof M],
    (...args: any) => any
  >,
> = F;

export type ForwardedResolverMap<
  K extends keyof ResolverToForwardingMethod = keyof ResolverToForwardingMethod,
> = {
  [T in K]: (id: ForwardingId<K>) => ReturnType<ForwardingMethod<K>>;
};

export type ForwardingMethodHandler<
  C,
  F extends (...args: any[]) => any = (...args: any[]) => any,
> = (
  doc: C,
  docRef: DocumentRef,
  viewId: ViewId,
  ...args: Parameters<F>
) => ForwardingId;

export type ForwardingMethodHandlers<
  C,
  K extends ForwardedResolver = ForwardedResolver,
  M extends ResolverToForwardingMethod[K] = ResolverToForwardingMethod[K],
  FK extends keyof M = keyof M,
> = {
  [T in FK]: M[FK] extends (...args: any) => any
    ? ForwardingMethodHandler<C, M[FK]>
    : never;
};

export type ForwardedMethodMap<
  K extends ForwardedResolver = ForwardedResolver,
> = {
  [T in K]: {
    [X in keyof ReturnType<ForwardingMethod<K>>]: true;
  };
};

export type ForwardingMessage<
  K extends ForwardedResolver = ForwardedResolver,
  M extends ResolverToForwardingMethod[K] = ResolverToForwardingMethod[K],
  FK extends keyof M = keyof M,
> = {
  [T in FK]: M[FK] extends (...args: infer P) => infer R
    ? (docRef: DocumentRef, viewId: ViewId, ...args: P) => ForwardingId
    : never;
};

export type ForwardedMethodMessage<
  K extends ForwardedResolver = ForwardedResolver,
  F extends ForwardingMethod<K> = ForwardingMethod<K>,
  R extends ReturnType<F> = ReturnType<F>,
  RK extends keyof R = keyof R,
> = {
  [T in K]: (
    method: RK,
    fwd: ForwardingId,
    ...args: Parameters<R[RK]>
  ) => ReturnType<R[RK]>;
};

type DocumentMessageUnion = DocumentMessage & DocumentWithViewMessage;

export type Message = GlobalMessage &
  DocumentMessageUnion &
  ForwardingMessage &
  ForwardedMethodMessage;

export type GlobalMethod = {
  [K in keyof GlobalMessage]: (
    ...args: Parameters<GlobalMessage[K]>
  ) => ReturnType<GlobalMessage[K]>;
};

type Tail<T> = T extends [any, ...infer Rest] ? Rest : never;

export type DocumentMethodHandler<C> = {
  [K in keyof DocumentMessageUnion]: (
    doc: C,
    ...args: Tail<Parameters<DocumentMessageUnion[K]>>
  ) => ReturnType<DocumentMessageUnion[K] | Promise<DocumentMessageUnion[K]>>;
};

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
  /** after painting is idle, calls the callback, returns the method to unregister the callback */
  afterIdle(callback: () => any): () => void;
  /** !!!USE SPARINGLY: Impacts performance!!! Calls the callback, returns the method to unregister the callback */
  afterPaint(callback: () => any): () => void;
};

export type AsyncFunctions<T extends { [K: string]: (...args: any) => any }> = {
  [K in keyof T]: (...args: Parameters<T[K]>) => Promise<ReturnType<T[K]>>;
};

type FlatForwardMethod<
  K extends keyof ResolverToForwardingMethod = keyof ResolverToForwardingMethod,
  M extends ResolverToForwardingMethod[K] = ResolverToForwardingMethod[K],
> = {
  [FK in keyof M]: M[FK];
};

export type DocumentClient = {
  [K in keyof Omit<DocumentMethods, 'newView'>]: (
    ...args: Parameters<DocumentMethods[K]>
  ) => Promise<ReturnType<DocumentMethods[K]>>;
} & {
  [K in keyof DocumentWithViewMethods]: (
    ...args: Parameters<DocumentWithViewMethods[K]>
  ) => Promise<ReturnType<DocumentWithViewMethods[K]>>;
} & {
  [K in keyof FlatForwardMethod]: (
    ...args: Parameters<FlatForwardMethod[K]>
  ) => Promise<AsyncFunctions<ReturnType<FlatForwardMethod[K]>>>;
} & DocumentClientBase;

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

export type KeysMessage = {
  f: '_keys';
  keys: string[];
  forwarded: {
    [K in keyof ForwardedMethodMap]: string[];
  };
};

export type FromWorker<K extends keyof Message = keyof Message> = {
  f: K;
  i: Id;
  r: ReturnType<Message[K]>;
};

export type ForwardingFromWorker<
  K extends ForwardedResolver = ForwardedResolver,
  M extends ResolverToForwardingMethod[K] = ResolverToForwardingMethod[K],
  FK extends keyof M = keyof M,
> = FromWorker<FK & keyof Message> & {
  fwd: true;
};

export type ForwardedFromWorker<
  K extends ForwardedResolver = ForwardedResolver,
> = FromWorker<K> & {
  m: keyof ReturnType<ForwardingMethod<K>>;
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
    }
  | {
      /** width change */
      t: 'w';
      /** width */
      w: number;
  };

export type Ref<T> = {
  current?: T;
};
