import { ToTileRenderer } from './shared';
import { TileRenderData} from './soffice';

/** From lib/wasm_extensions.hxx */
enum RenderState {
  IDLE = 0,
  TILE_PAINT = 1,
  RENDERING = 2,
  RESET = 3,
  QUIT = 4,
}

type Rect = [
  /** x */ x: number,
  /** y */ y: number,
  /** w */ w: number,
  /** h */ h: number,
];

type TileIndexRange = [start: number, endInclusive: number];

const RECT_SIZE = 4;
const LOK_INTERNAL_TWIPS_TO_PX = 15;

let workerData: TileRenderData;
// Changes frequently
let docWidthTwips: number;
// Changes infrequently
let docHeightTwips: number;

// Is the state machine currently running?
let running = false;

let mainView: RenderedView;

// previewView is optional
let previewView: RenderedView | undefined = undefined;

let pendingStateChange: boolean = false;

let invalidations: Rect[] = [];

function updateDocSize() {
  docWidthTwips = Atomics.load(workerData.docWidthTwips, 0);
  docHeightTwips = Atomics.load(workerData.docHeightTwips, 0);
}


class RenderedView {
  readonly viewId: number;
  readonly canvases: OffscreenCanvas[];
  readonly POOL_SIZE: number;
  activeCanvas: OffscreenCanvas;
  activeCanvasIndex: number = 0;
  ctx: OffscreenCanvasRenderingContext2D;

  scale: number = 1;
  dpi: number = 1;

  scaledTwips: number = 1;
  tileSize: number = 256;
  tileDimTwips: number = 0;
  widthTileStride: number = 0;

  scheduledHeightPx: number = -1;
  scheduledHeightTwips: number = -1;
  scheduledWidthPx: number = -1;
  scheduledTopTwips: number = -1;
  renderedTileTop: number = 0;
  renderedTopTwips: number = 0;
  renderedHeightTwips: number = 0;
  didScroll = false;

  visibleInvalidations: Rect[] = [];
  nonVisibleInvalidations: Rect[] = [];

  validTiles: Set<number> = new Set();
  /** maps a tile index to a tileRing index */
  tileIndexToTileRingIndex: Map<number, number> = new Map();
  /** maps a tileRing index to a tile index */
  tileRingIndexToTileIndex: Map<number, number> = new Map();
  /** a ring buffer that shouldn't allocate more than POOL_SIZE */
  tileRing: Map<number, ImageData> = new Map();
  /** contains a set of all visible tile ring indices */
  visibleRingTiles: Set<number> = new Set();
  /** ring index that wraps on POOL_SIZE */
  tileRingIndex = 0;

  pendingFullPaint: boolean = false;
  needsRender: boolean = false;

  paintTimes: number[] = [];

  paintedTile: Uint8Array;
  tileTwips: Uint32Array;


  constructor({
    viewId,
    canvases,
    scale,
    yPos,
    dpi,
    tileSize,
    poolSize,
    paintedTile,
    tileTwips,
  }: {
    viewId: number;
    canvases: OffscreenCanvas[];
    scale: number;
    yPos: number;
    dpi: number;
    tileSize: number;
    poolSize: number;
    tileTwips: Uint32Array;
    paintedTile: Uint8Array;
  }) {
    this.viewId = viewId;
    this.canvases = canvases;
    this.scale = scale;
    this.dpi = dpi;
    this.tileSize = tileSize;
    this.POOL_SIZE = poolSize;
    this.tileTwips = tileTwips;
    this.paintedTile = paintedTile;
    this.scheduledTopTwips = yPos * this.scaledTwips;
    this.activeCanvas = canvases[0];
    this.ctx = this.activeCanvas.getContext('2d');

    this.zoom(scale, dpi);


  }

  /// Returns true if the view has scroll sufficiently past the current tile
  /// to require switching canvases
  scroll(y: number): boolean {
    if (!this.activeCanvas) return;
    this.scheduledTopTwips = y * this.scaledTwips;
    if (
      Math.floor(this.scheduledTopTwips / this.tileDimTwips) !==
      this.renderedTileTop
    ) {
      this.didScroll = true;
      this.activeCanvasIndex ^= 1;
      this.activeCanvas = this.canvases[this.activeCanvasIndex];
      this.ctx = this.activeCanvas.getContext('2d');
    }

    return this.didScroll;
  }

  zoom(scale: number, dpi: number) {
    updateDocSize();

    let previousDpi = this.dpi;

    this.scaledTwips =
      clipToNearest8PxZoom(this.tileSize, 1 / (scale * dpi)) *
      LOK_INTERNAL_TWIPS_TO_PX;

    this.tileDimTwips = Math.ceil(this.tileSize * this.scaledTwips);
    this.widthTileStride = Math.ceil(docWidthTwips / this.tileDimTwips);
    this.scheduledHeightPx = (this.activeCanvas.height * dpi) / previousDpi;
    this.scheduledHeightTwips = this.activeCanvas.height * this.scaledTwips;
    this.scheduledWidthPx = docWidthTwips / this.scaledTwips;
  }

  resize(h: number) {
    if (!this.activeCanvas) return;

    this.scheduledHeightPx = h * this.dpi;
    this.scheduledTopTwips = h * this.scaledTwips;
  }
}

onmessage = ({ data }: { data: ToTileRenderer }) => {
  switch (data.t) {
    case 'i': { // initialize
      console.log(`Initializing Tile Renderer with`);
      console.log(data, data.d);

      workerData = data.d;
      mainView = new RenderedView({
        viewId: data.m.viewId,
        canvases: data.m.canvases,
        scale: data.m.scale,
        yPos: data.m.y,
        tileSize: 256,
        dpi: data.dpi,
        poolSize: 2000,
        paintedTile: data.m.paintedTile,
        tileTwips: data.m.tileTwips,
      });


      if (data.p) {
        previewView = new RenderedView({
          viewId: data.p.viewId,
          canvases: data.p.canvases,
          scale: data.p.scale,
          yPos: data.p.y,
          tileSize: 256,
          dpi: data.dpi,
          poolSize: 500, // Document preview has a smaller pool size
          paintedTile: data.p.paintedTile,
          tileTwips: data.p.tileTwips,
        });
      }

      pendingStateChange = true;
      stateMachine();

      break;
    }
    case 's': {
      const isMainView = data.viewId === mainView.viewId;
      console.log(`Received scroll message for view ${data.viewId}`);
      const view = isMainView ? mainView : previewView;

      const didScroll = view.scroll(data.y);

      if (didScroll) {
        setState(RenderState.IDLE, view.viewId);
        if (!running) stateMachine();
      } else {
        postMessage({ s: view.activeCanvasIndex });
      }
      break;
    }
    case 'r': {
      const isMainView = data.viewId === mainView.viewId;
      console.log(`Received resize message for view ${data.viewId}`);
      const view = isMainView ? mainView : previewView;
      view.resize(data.h);
      setState(RenderState.IDLE, view.viewId);
      if (!running) stateMachine();
      break;
    }
    case 'z': {
      const isMainView = data.viewId === mainView.viewId;
      console.log(`Received zoom message for view ${data.viewId}`);
      const view = isMainView ? mainView : previewView;
      view.zoom(data.s, data.d);
      setState(RenderState.RESET, view.viewId);
      Atomics.wait(workerData.state, 0, RenderState.RESET); // wait for reset to finish
      if (!running) stateMachine();
      break;
    }
  }
};

function getState(): RenderState {
  return Atomics.load(workerData.state, 0);
}

function setState(state: RenderState, viewId?: number): void {
  pendingStateChange = true;
  Atomics.store(workerData.state, 0, state);
  Atomics.notify(workerData.state, 0);

  // Optionally set a new active view
  // specifically used for painting
  if (viewId) {
    Atomics.store(workerData.activeViewId, 0, viewId);
  }
}

function fullPaint(view: RenderedView) {
  console.log(`fullPaint for view ${view.viewId}`);
  view.visibleInvalidations.length = 0;
  view.nonVisibleInvalidations.length = 0;
  view.pendingFullPaint = false;

  const rangesToPaint = rectToTileIndexRanges(
    [0, view.scheduledTopTwips, docWidthTwips, view.scheduledHeightTwips],
    view.tileDimTwips,
    view.widthTileStride
  );

  const newVisibleRingTiles = new Set<number>();

  // all tiles are invalid
  view.validTiles.clear();

  // effectively paints by rows of tiles, so there isn't any odd-looking tearing if painting is paused
  for (let y = 0; y < rangesToPaint.length /* && !shouldPausePaint() */; ++y) {
    const [start, endInclusive] = rangesToPaint[y];
    for (let x = start; x <= endInclusive; ++x) {
      newVisibleRingTiles.add(blockingPaintTile(view, x));
    }
  }
  view.visibleRingTiles.clear();
  view.visibleRingTiles = newVisibleRingTiles;

  clearNonVisibleTiles(view);

  view.needsRender = true;
}

function partialPaint(view: RenderedView) {
  console.log("partialPaint for view", view.viewId);
  // rebalance visible and non-visible
  // invalidations.push(...visibleInvalidations);
  // invalidations.push(...nonVisibleInvalidations);
  view.visibleInvalidations.length = 0;
  view.nonVisibleInvalidations.length = 0;

  for (const invalidation of invalidations) {
    commitVisibleAndNonVisible(
      view,
      invalidation,
      view.scheduledTopTwips,
      view.scheduledHeightTwips
    );
  }

  const newVisibleRingTiles = new Set<number>();

  if (view.visibleInvalidations.length != 0) {
    const tileRangesToPaint = [];
    // first do a pass and mark the invalid tiles, to prevent uncessary work by invalidating/painting overlapping areas
    for (const visibleInvalidation of view.visibleInvalidations) {
      const rangesToPaint = rectToTileIndexRanges(
        visibleInvalidation,
        view.tileDimTwips,
        view.widthTileStride
      );
      markInvalid(view, rangesToPaint);
      tileRangesToPaint.push(rangesToPaint);
    }

    for (const rangesToPaint of tileRangesToPaint) {
      // effectively paints by rows of tiles, so there isn't any odd-looking tearing if painting is paused
      for (
        let y = 0;
        y < rangesToPaint.length /*&& !shouldPausePaint()*/;
        ++y
      ) {
        const [start, endInclusive] = rangesToPaint[y];
        for (let x = start; x <= endInclusive; ++x) {
          if (!view.validTiles.has(x)) {
            newVisibleRingTiles.add(blockingPaintTile(view, x));
            view.needsRender = true;
          }
        }
      }
    }
  }

  const visibleRangesToPaint = rectToTileIndexRanges(
    [0, view.scheduledTopTwips, docWidthTwips, view.scheduledHeightTwips],
    view.tileDimTwips,
    view.widthTileStride
  );

  for (
    let y = 0;
    y < visibleRangesToPaint.length /*&& !shouldPausePaint()*/;
    ++y
  ) {
    const [start, endInclusive] = visibleRangesToPaint[y];
    for (let x = start; x <= endInclusive; ++x) {
      const ringIndex = view.tileIndexToTileRingIndex.get(x);
      if (
        ringIndex != null &&
        view.validTiles.has(x) &&
        view.tileRingIndexToTileIndex.get(ringIndex) === x
      ) {
        newVisibleRingTiles.add(ringIndex);
      } else {
        newVisibleRingTiles.add(blockingPaintTile(view, x));
        view.needsRender = true;
      }
    }
  }

  view.visibleInvalidations.length = 0;
  view.visibleRingTiles.clear();
  view.visibleRingTiles = newVisibleRingTiles;
}

function render(view: RenderedView) {
  console.log(`rendering view ${view.viewId}`);
  const visibleTop = view.scheduledTopTwips;
  const visibleHeight = view.scheduledHeightTwips;

  const rangesToRender = rectToTileIndexRanges([
    0,
    visibleTop,
    docWidthTwips,
    visibleHeight,
  ], view.tileDimTwips, view.widthTileStride);

  // TODO: mark missing and invalidate
  view.ctx.clearRect(0, 0, view.activeCanvas.width, view.activeCanvas.height);
  if (view.renderedHeightTwips !== view.scheduledHeightTwips) {
    view.canvases[0].height = view.scheduledHeightPx;
    view.canvases[1].height = view.scheduledHeightPx;
  }
  if (view.activeCanvas.width !== view.scheduledWidthPx) {
    view.canvases[0].width = view.scheduledWidthPx;
    view.canvases[1].width = view.scheduledWidthPx;
  }
  view.renderedHeightTwips = visibleHeight;
  view.renderedTopTwips = visibleTop;

    for (let y = 0; y < rangesToRender.length /*&& !shouldPausePaint()*/; ++y) {
    const [start, endInclusive] = rangesToRender[y];
    for (let x = start; x <= endInclusive; ++x) {
      // TODO: mark missing and invalidate
      const [xCoord] = tileIndexToGridCoord(view, x);
      const img: ImageData = view.tileRing.get(view.tileIndexToTileRingIndex.get(x));
      if (!img) {
        console.error('missing texture at ', x, xCoord, y);
        continue;
      }
      const dstX: number = xCoord * view.tileSize;
      const dstY: number = y * view.tileSize;
      view.ctx.beginPath();
      view.ctx.putImageData(img, dstX, dstY);
      view.ctx.closePath();
    }
  }
  view.pendingFullPaint = false;
  view.needsRender = false;
}

function stateMachine() {
  running = true;
  while (pendingStateChange) {
    pendingStateChange = false;
    switch (getState()) {
      case RenderState.IDLE: {
        // Invalidations are shared between the main and preview views
        invalidations = drainInvalidations();

        if (Atomics.load(workerData.pendingFullPaint, 0) === 1) {
          mainView.pendingFullPaint = true;
          if (previewView) previewView.pendingFullPaint = true;
        }

        // Prioritize painting and rendering the main view
        // Only paint the preview view if the main view does not need a render
        if (mainView.pendingFullPaint) {
          fullPaint(mainView);
        } else {
          partialPaint(mainView);
        }

        if (mainView.needsRender /* || hasUpdatedVisibleArea */) {
          setState(RenderState.RENDERING, mainView.viewId);
        } else if (previewView) {
          // if nothing needs to be done on the main view, check the preview view
          if (previewView.pendingFullPaint) {
            fullPaint(previewView);
          } else {
            partialPaint(previewView);
          }

          if (previewView.needsRender /* || hasUpdatedVisibleArea */) {
            setState(RenderState.RENDERING, previewView.viewId);
          }
        }
        break;
      }
      case RenderState.TILE_PAINT:
        // owned by wasm_extensions.cxx, so just wait for a state change
        break;
      case RenderState.RENDERING: {
        console.log("rendering");
        let viewToRender: RenderedView = mainView;
        let isRenderingPreview = false;
        if (Atomics.load(workerData.activeViewId, 0) === previewView?.viewId) {
          viewToRender = previewView;
          isRenderingPreview = true;
        }

        render(viewToRender);
        let pendingFullPaint = mainView.pendingFullPaint || (previewView && previewView.pendingFullPaint);
        if (!pendingFullPaint) {
          Atomics.store(workerData.pendingFullPaint, 0, 0);
        }

        setState(RenderState.IDLE, mainView.viewId);
        // // if (didScroll) {
        // //   renderedTileTop = Math.floor(renderedTopTwips / tileDimTwips);
        // //   postMessage({ s: activeCanvasIndex });
        // //   didScroll = false;
        // // }
        break;
      }
      case RenderState.RESET:
        // owned by wasm_extensions.cxx, so just wait for a state change
        break;
      case RenderState.QUIT:
        self.close();
        break;
    }
  }

  running = false;
}
function blockingPaintTile(view: RenderedView, tileIndex: number): number {
  Atomics.wait(workerData.state, 0, RenderState.TILE_PAINT); // wait for existing paint to finish if necessary
  const start = Date.now();
  const rect = tileIndexToTwipsRect(view, tileIndex);
  for (let i = 0; i < RECT_SIZE; ++i) {
    Atomics.store(view.tileTwips, i, rect[i]);
  }
  setState(RenderState.TILE_PAINT);
  Atomics.wait(workerData.state, 0, RenderState.TILE_PAINT); // wait to finish
  seekNonVisibleRingIndex(view);

  view.tileRingIndexToTileIndex.set(view.tileRingIndex, tileIndex);
  view.tileIndexToTileRingIndex.set(tileIndex, view.tileRingIndex);
  view.tileRing.set(
    view.tileRingIndex,
    new ImageData(
      new Uint8ClampedArray(
        view.paintedTile,
        view.paintedTile.byteOffset,
        view.paintedTile.byteLength
      ),
      view.tileSize,
      view.tileSize
    )
  );
  view.validTiles.add(tileIndex);
  view.paintTimes.push(Date.now() - start);

  return view.tileRingIndex;
}

function tileIndexToGridCoord(
  view: RenderedView,
  tileIndex: number
): [x: number, y: number] {
  const y = (tileIndex / view.widthTileStride) | 0;
  const x = tileIndex % view.widthTileStride;
  return [x, y];
}

function tileIndexToTwipsRect(view: RenderedView, tileIndex: number): Rect {
  const [x, y] = tileIndexToGridCoord(view, tileIndex);
  return [
    x * view.tileDimTwips,
    y * view.tileDimTwips,
    view.tileDimTwips,
    view.tileDimTwips,
  ];
}

function seekNonVisibleRingIndex(view: RenderedView) {
  const origin = view.tileRingIndex;
  view.tileRingIndex = ((view.tileRingIndex + 1) | 0) % view.POOL_SIZE;
  while (
    view.visibleRingTiles.has(view.tileRingIndex) &&
    view.tileRingIndex != origin
  ) {
    view.tileRingIndex = ((view.tileRingIndex + 1) | 0) % view.POOL_SIZE;
  }
  if (view.tileRingIndex === origin) {
    console.error('Texture pool too small for visible tiles');
  }
}

function markInvalid(view: RenderedView, ranges: TileIndexRange[]) {
  for (const range of ranges) {
    const [start, endInclusive] = range;
    for (let i = start; i <= endInclusive; ++i) {
      view.validTiles.delete(i);
      view.tileIndexToTileRingIndex.delete(i);
    }
  }
}

function clearNonVisibleTiles(view: RenderedView) {
  const indices = view.tileRing.keys();
  for (const tileRingIndex of indices) {
    // but visible tiles shouldn't be cleared so there is something to paint
    if (view.visibleRingTiles.has(tileRingIndex)) continue;

    view.tileIndexToTileRingIndex.delete(
      view.tileRingIndexToTileIndex.get(tileRingIndex)
    );
    view.tileRing.delete(tileRingIndex);
    view.tileRingIndexToTileIndex.delete(tileRingIndex);
  }
}

function commitVisibleAndNonVisible(
  view: RenderedView,
  rect: Rect,
  y: number,
  h: number
) {
  const [rx, ry, rw, rh] = rect;
  const b = y + h;
  const rb = ry + rh;

  // Calculate the overlap
  const overlapTop = Math.max(ry, y);
  const overlapBottom = Math.min(rb, b);

  // Check if no overlap
  if (overlapTop >= overlapBottom) {
    view.nonVisibleInvalidations.push(rect);
    return;
  }

  view.visibleInvalidations.push([
    rx,
    overlapTop,
    rw,
    overlapBottom - overlapTop,
  ]);

  // Above the overlap
  if (ry < overlapTop) {
    view.nonVisibleInvalidations.push([rx, ry, rw, overlapTop - ry]);
  }

  // Below the overlap
  if (rb > overlapBottom) {
    view.nonVisibleInvalidations.push([
      rx,
      overlapBottom,
      rw,
      rb - overlapBottom,
    ]);
  }
}

function rectToTileIndexRanges(
  rect: Rect,
  tileDimTwips: number,
  widthTileStride: number
): TileIndexRange[] {
  const result: TileIndexRange[] = [];
  const r = rect[0] + rect[2];
  const x0 = Math.floor(rect[0] / tileDimTwips);
  const x1 = Math.floor(r / tileDimTwips);
  const b = rect[1] + rect[3];
  const y0 = Math.floor(rect[1] / tileDimTwips);
  const y1 = Math.floor(b / tileDimTwips);
  for (let y = y0; y <= y1; ++y) {
    result.push([x0 + widthTileStride * y, x1 + widthTileStride * y]);
  }
  return result;
}

function hasInvalidations(): boolean {
  return Atomics.load(workerData.hasInvalidations, 0) === 1;
}

// Operates on a fixed-size thread-safe stack, JS-side is always SEQ_CST so there's some perf loss
function takeInvalidation(): Rect | undefined {
  if (!hasInvalidations()) return undefined;

  const head = Atomics.load(workerData.invalidationStackHead, 0);
  if (head < 0) return undefined;

  const result = new Array<number>(RECT_SIZE) as Rect;

  const subarrayPos = head * RECT_SIZE;
  const invalidRect = workerData.invalidationStack.subarray(
    subarrayPos,
    subarrayPos + RECT_SIZE
  );

  for (let i = 0; i < RECT_SIZE; ++i) {
    result[i] = Atomics.load(invalidRect, i);
  }
  const newHead = head - 1;
  Atomics.store(workerData.invalidationStackHead, 0, newHead);
  if (newHead < 0) {
    Atomics.store(workerData.hasInvalidations, 0, 1);
    Atomics.notify(workerData.hasInvalidations, 0);
  }
  return result;
}

// TODO: does this need to be time-limited?
function drainInvalidations(): Rect[] {
  const result: Rect[] = [];
  let invalidation: Rect | undefined;
  do {
    invalidation = takeInvalidation();
    if (invalidation != null) result.push(invalidation);
  } while (invalidation != null);
  Atomics.store(workerData.hasInvalidations, 0, 0);
  return result;
}

function clipToNearest8PxZoom(w: number, s: number): number {
  const scaledWidth: number = Math.ceil(w * s);
  const mod: number = scaledWidth % 8;
  if (mod === 0) return s;

  return Math.abs((scaledWidth - mod) / w - s) <
    Math.abs((scaledWidth + 8 - mod) / w - s)
    ? (scaledWidth - mod) / w
    : (scaledWidth + 8 - mod) / w;
}
