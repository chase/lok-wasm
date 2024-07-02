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

export type RectArray = [x: number, y: number, width: number, height: number];

export type OutlineItem = {
  id: number;
  parent: number;
  text: string;
};

export declare const enum LayoutStatus {
  INVISIBLE,
  VISIBLE,
  INSERTED,
  DELETED,
  NONE,
  HIDDEN,
}

export type Comment = {
  id: number;
  parentId: number;
  author: string;
  text: string;
  resolved: boolean;
  dateTime: string;
};

export type RootComment = Comment & {
  parentId: 0;
  anchorPos: RectArray;
  textRange: RectArray[];
  bottomTwips: number;
  layoutStatus: LayoutStatus;
};

export type SanitizeOptions = Partial<{
  documentMetadata: boolean;
  trackChangesAccept: boolean;
  trackChangesReject: boolean;
  comments: boolean;
}>;

export type HeaderFooterRect = {
  type: 'header' | 'footer';
  rect: RectArray;
};

export type ParagraphStyle<T extends readonly string[], K = T[number]> = {
  name: string;
} & {
  [PK in K]: any;
};

export type ParagraphStyleList<T extends readonly string[]> = {
  userDefined: ParagraphStyle<T>[];
  used: ParagraphStyle<T>[];
  other: ParagraphStyle<T>[];
};

export type FindAllOptions = Partial<{
  /** the case of letters is important for the match */
  caseSensitive: boolean;
  /** only complete words are matched */
  wholeWords: boolean;
  /** the mode used to make matches.
   *
   * undefined: a normal text search.
   * wildcard: `*` for any sequence of characters (including empty), `?` for exactly one character. `\` escapes these characters.
   * regex: a regular expression-based search.
   * similar: an approximate match/fuzzy search.
   **/
  mode: 'wildcard' | 'regex' | 'similar';
}>;

export type TextRangeDescription = [
  before: string,
  term: string,
  after: string,
];

export type ITextRanges = {
  length(): number;
  rect(index: number): RectArray[];
  rects(
    startYPosTwips: number,
    endYPosTwips: number
  ): Array<{
    i: number;
    rect: RectArray[];
  }>;
  isCursorAt(index: number): boolean;
  indexAtCursor(): number;
  moveCursorTo(index: number, end: boolean, select: boolean): void;
  description(index: number): TextRangeDescription | undefined;
  descriptions(
    startIndex: number,
    endIndex: number
  ): Array<{
    i: number;
    desc: TextRangeDescription;
  }>;
  replace(index: number, text: string): void;
  replaceAll(text: string): void;
};

export declare class ExpandedDocPart {
  constructor(path: string, content: string): void;
}

export declare class ExpandedDocument {
  constructor(): void;
  addPart(path: string, name: ArrayBuffer): void;
  delete(): void;
}

export type ExpandedPart = { path: string; content: ArrayBuffer };

/** Embind ocument class, see main_wasm.cxx */
export declare class Document {
  constructor(path: string): void;
  constructor(expanded: ExpandedDocument, name: string): void;
  delete(): void;

  valid(): boolean;
  saveAs(path: string, format?: string, filterOptions?: string): boolean;
  getParts(): number;
  pageRects(): RectArray[];
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
  stopTileRenderer(viewId: number): void;
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
    charsAfter: number
  ): void;
  // NOTE: Disabled until unoembind startup cost is under 1s
  // getXComponent(viewId: number): any;

  comments(ids: number[]): Comment[];
  addComment(text: string): void;
  replyComment(parentId: number, text: string): void;
  updateComment(id: number, text: string): void;
  deleteCommentThreads(parentIds: number[]): void;
  deleteComment(commentId: number): void;
  resolveCommentThread(parentId: number): void;
  resolveComment(commentId: number): void;
  sanitize(options: SanitizeOptions): void;

  headerFooterRect(): HeaderFooterRect | undefined;

  getPropertyValue(property: string): any;
  setPropertyValue(property: string, value: any): void;

  saveCurrentSelection(): void;
  restoreCurrentSelection(): void;
  getSelectionText(): string | undefined;
  getParagraphStyle(
    name: string,
    properties: string[]
  ): undefined | Record<string, any>;
  paragraphStyles<T extends readonly string[]>(
    properties: T
  ): ParagraphStyleList<T>;

  findAll(text: string, options: FindAllOptions | undefined): ITextRanges;
  getOutline(): OutlineItem[];
  gotoOutline(index: number): RectArray;
  setAuthor(author: string): void;
  setAuthor(author: string): void;
  getExpandedPart(path: string): ExpandedPart | null;
  listExpandedParts(): Array<ExpandedPart>;

  addExternalUndo(): number;
  getNextUndoId(): number;
  getNextRedoId(): number;
  getUndoCount(): number;
  getRedoCount(): number;
  undo(count: number): void;
  redo(count: number): void;

  getRedlineTextRange(id: number): RectArray[] | undefined;
}

// NOTE: Disabled until unoembind startup cost is under 1s
// declare global {
//   interface WorkerGlobalScope {
//     /** registers new methods for Document */
//     registerExtension(handlers: {
//       [name: string]: (doc: Document, viewId: number, ...args: any[]) => Promise<any>;
//     }): void;
//   }
// }

export interface Module extends EmscriptenModule {
  /** indicate whether to load fccache with the module */
  withFcCache?: boolean;

  /** mounts the blob at a file path with `name` and returns the file path */
  mountBlob(name: string, blob: Blob): string;

  /** unmounts the current blob */
  unmountBlob(): void;

  /** reads and unlinks a temporary file at path */
  readUnlink(path: string): Uint8Array;

  Document: typeof Document;
  ExpandedPart: typeof ExpandedPart;
  ExpandedDocument: typeof ExpandedDocument;

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
