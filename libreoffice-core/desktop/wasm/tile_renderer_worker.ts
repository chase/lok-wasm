import { TileRenderData } from './soffice';
import { ToTileRenderer } from './shared';

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

const DEBUG = false;
const LARGE_POOL_SIZE = 2000;
const SMALL_POOL_SIZE = 1000;

const RECT_SIZE = 4;
const LOK_INTERNAL_TWIPS_TO_PX = 15;

let workerData: TileRenderData;
// Changes frequently
let docWidthTwips: number;
// Changes infrequently
let docHeightTwips: number;
// Is the state machine currently running?
let running = false;
let pendingStateChange: boolean = false;
let invalidations: Rect[] = [];
let startTimestamp: number;

let mainView: RenderedView;
// previewView is optional
let previewView: RenderedView | undefined = undefined;

function updateDocSize() {
  docWidthTwips = Atomics.load(workerData.docWidthTwips, 0);
  docHeightTwips = Atomics.load(workerData.docHeightTwips, 0);
}

class RenderedView {
  readonly viewId: number;
  readonly canvases: OffscreenCanvas[];
  readonly POOL_SIZE: number;
  private pendingFullPaint: Int32Array;

  activeCanvas: OffscreenCanvas;
  activeCanvasIndex: number = 0;
  ctx: OffscreenCanvasRenderingContext2D;

  scale: number = 1;
  dpi: number = 1;
  scaledTwips: number = LOK_INTERNAL_TWIPS_TO_PX;
  tileSize: number = 256;
  tileDimTwips: number;
  widthTileStride: number;

  /** the scheduled height to be painted/rendered **/
  scheduledHeightPx: number = -1;
  scheduledHeightTwips: number = -1;
  scheduledWidthPx: number = -1;
  scheduledTopTwips: number = -1;
  /** the rendered height that was previously painted/rendered **/
  renderedTileTop: number = -1;
  renderedTopTwips: number = -1;
  renderedHeightTwips: number = -1;
  didScroll = false;
  didZoom = false;

  visibleInvalidations: Rect[] = [];
  nonVisibleInvalidations: Rect[] = [];

  missingRects: Rect[] = [];

  /** contains a set of all the valid tile indices */
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

  pendingPartialPaint: boolean = false;
  needsRender: boolean = false;

  paintTimes: number[] = [];
  idleAreaPaint = false;

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
    pendingFullPaint,
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
    pendingFullPaint: Int32Array;
  }) {
    this.viewId = viewId;
    this.canvases = canvases;
    this.scale = scale;
    this.dpi = dpi;
    this.tileSize = tileSize;
    this.POOL_SIZE = poolSize;
    this.tileTwips = tileTwips;
    this.paintedTile = paintedTile;
    this.activeCanvas = canvases[0];
    this.ctx = getContext(this.activeCanvas);
    this.pendingFullPaint = pendingFullPaint;

    this.zoom(scale, dpi);

    this.scheduledTopTwips = (yPos ?? 0) * this.scaledTwips;
  }

  isPendingFullPaint(): boolean {
    return Atomics.load(this.pendingFullPaint, 0) === 1;
  }

  setIsPendingFullPaint(val: boolean) {
    Atomics.store(this.pendingFullPaint, 0, val ? 1 : 0);
  }

  commitInvalidations(invalidations: Rect[]) {
    // Rebalance invalidations
    invalidations.push(...this.visibleInvalidations);
    invalidations.push(...this.nonVisibleInvalidations);
    this.visibleInvalidations.length = 0;
    this.nonVisibleInvalidations.length = 0;
    for (const invalidation of invalidations) {
      commitVisibleAndNonVisible(
        this,
        invalidation,
        this.scheduledTopTwips,
        this.scheduledHeightTwips
      );
    }
  }

  reset() {
    this.visibleInvalidations.length = 0;
    this.nonVisibleInvalidations.length = 0;
    this.missingRects.length = 0;
    this.validTiles.clear();
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
      this.ctx = getContext(this.activeCanvas);
      setState(RenderState.IDLE, this.viewId);
      if (!running) stateMachine();
    } else {
      postMessage({ s: this.activeCanvasIndex });
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

  resize(h: number): boolean {
    if (!this.activeCanvas) return false;

    this.scheduledHeightPx = h * this.dpi;
    this.scheduledHeightTwips = h * this.scaledTwips;

    return true;
  }
}

let zoomResetTimeout: number;

onmessage = ({ data }: { data: ToTileRenderer }) => {
  switch (data.t) {
    case 'i': {
      // initialize
      startTimestamp = Date.now();
      workerData = data.d;
      mainView = new RenderedView({
        viewId: data.m.viewId,
        canvases: data.m.canvases,
        scale: data.m.scale,
        yPos: data.m.y,
        tileSize: data.d.tileSize,
        dpi: data.dpi,
        poolSize: LARGE_POOL_SIZE,
        paintedTile: data.m.paintedTile,
        tileTwips: data.m.tileTwips,
        pendingFullPaint: data.m.pendingFullPaint,
      });

      pendingStateChange = true;
      setState(RenderState.IDLE, mainView.viewId);
      stateMachine();

      break;
    }
    case 's': {
      const isMainView = data.viewId === mainView.viewId;
      const view = isMainView ? mainView : previewView;

      view.scroll(data.y);
      break;
    }
    case 'r': {
      const isMainView = data.viewId === mainView.viewId;
      const view = isMainView ? mainView : previewView;
      if (!view) {
        console.error(
          'tried to perform resize on non-existent view, viewId:',
          data.viewId
        );
        break;
      }
      const shouldResize = view.resize(data.h);
      if (!shouldResize) break;
      setState(RenderState.IDLE, view.viewId);
      if (!running) stateMachine();
      break;
    }
    case 'z': {
      const isMainView = data.viewId === mainView.viewId;
      const view = isMainView ? mainView : previewView;
      view.didZoom = true;
      view.zoom(data.s, data.d);
      view.reset();
      if (zoomResetTimeout) clearTimeout(zoomResetTimeout);
      zoomResetTimeout = setTimeout(() => {
        view.reset();
        setState(RenderState.RESET, view.viewId);
        Atomics.wait(workerData.state, 0, RenderState.RESET); // wait for reset to finish
        if (!running) stateMachine();
      });
      break;
    }
    case 'previewStart': {
      // reset the worker data to omit the preview view
      workerData.previewPaintedTile = data.d.paintedTile;
      workerData.previewTileTwips = data.d.tileTwips;
      workerData.previewViewId = data.d.viewId;
      workerData.previewTileSize = data.d.tileSize;
      workerData.previewPendingFullPaint = data.d.pendingFullPaint;
      previewView = new RenderedView({
        viewId: data.p.viewId,
        canvases: data.p.canvases,
        scale: data.p.scale,
        yPos: data.p.y,
        tileSize: data.d.tileSize,
        dpi: mainView.dpi, // DPI should be the same for both views
        poolSize: SMALL_POOL_SIZE, // Document preview has a smaller pool size
        paintedTile: data.p.paintedTile,
        tileTwips: data.p.tileTwips,
        pendingFullPaint: data.p.pendingFullPaint,
      });

      if (!running) {
        setState(RenderState.IDLE, previewView.viewId);
        stateMachine();
      }
      break;
    }
    case 'previewStop': {
      // Clear the previewView and reset the worker data
      previewView = undefined;
      workerData.previewPaintedTile = undefined;
      workerData.previewTileTwips = undefined;
      workerData.previewViewId = undefined;
      workerData.previewTileSize = undefined;
      workerData.previewPendingFullPaint = undefined;
      break;
    }
  }
};

function getState(): RenderState {
  return Atomics.load(workerData.state, 0);
}

function setState(state: RenderState, viewId?: number): void {
  // Optionally set a new active view
  // specifically used for painting
  if (viewId) {
    Atomics.store(workerData.activeViewId, 0, viewId);
  }

  pendingStateChange = true;
  Atomics.store(workerData.state, 0, state);
  Atomics.notify(workerData.state, 0);
}

function fullPaint(view: RenderedView) {
  DEBUG && console.log('FULL_PAINT');
  let didFinishPaint = true;
  view.visibleInvalidations.length = 0;
  view.nonVisibleInvalidations.length = 0;

  const rangesToPaint = rectToTileIndexRanges(
    [0, view.scheduledTopTwips, docWidthTwips, view.scheduledHeightTwips],
    view.tileDimTwips,
    view.widthTileStride
  );

  const newVisibleRingTiles = new Set<number>();

  // all tiles are invalid
  view.validTiles.clear();

  // effectively paints by rows of tiles, so there isn't any odd-looking tearing if painting is paused
  for (let y = 0; y < rangesToPaint.length; ++y) {
    if (shouldPausePaint(view)) {
      view.setIsPendingFullPaint(true);
      didFinishPaint = false;
      break;
    }
    const [start, endInclusive] = rangesToPaint[y];
    for (let x = start; x <= endInclusive; ++x) {
      newVisibleRingTiles.add(blockingPaintTile(view, x));
    }
  }

  if (didFinishPaint) {
    view.visibleRingTiles.clear();
    view.visibleRingTiles = newVisibleRingTiles;
  }

  clearNonVisibleTiles(view);

  view.needsRender = true;

  if (didFinishPaint) {
    view.setIsPendingFullPaint(false);
  }
}

function partialPaint(view: RenderedView) {
  DEBUG && console.log('PARTIAL_PAINT');
  let didFinishPaint = true;
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
      for (let y = 0; y < rangesToPaint.length; ++y) {
        if (shouldPausePaint(view)) {
          view.pendingPartialPaint = true;
          didFinishPaint = false;
          break;
        }
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

  for (let y = 0; y < visibleRangesToPaint.length; ++y) {
    if (shouldPausePaint(view)) {
      view.pendingPartialPaint = true;
      didFinishPaint = false;
      break;
    }
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
  view.pendingPartialPaint = false;

  if (hasUpdatedVisibleArea(view)) {
    view.needsRender = true;
  }
}

function render(view: RenderedView) {
  DEBUG && console.log('RENDER');
  const visibleTop = view.scheduledTopTwips;
  const visibleHeight = view.scheduledHeightTwips;

  const rangesToRender = rectToTileIndexRanges(
    [0, visibleTop, docWidthTwips, visibleHeight],
    view.tileDimTwips,
    view.widthTileStride
  );

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

  for (let y = 0; y < rangesToRender.length && !shouldPausePaint(view); ++y) {
    const [start, endInclusive] = rangesToRender[y];
    for (let x = start; x <= endInclusive; ++x) {
      const [xCoord] = tileIndexToGridCoord(view, x);
      const img: ImageData = view.tileRing.get(
        view.tileIndexToTileRingIndex.get(x)
      );
      if (!img) {
        if (
          view.missingRects.filter((r) => r[0] === xCoord * view.tileSize)
            .length === 0
        ) {
          view.missingRects.push([
            xCoord * view.tileSize,
            y * view.tileSize,
            view.tileSize,
            view.tileSize,
          ]);
          view.pendingPartialPaint = true;
        }
        continue;
      }
      const dstX: number = xCoord * view.tileSize;
      const dstY: number = y * view.tileSize;
      view.ctx.beginPath();
      view.ctx.putImageData(DEBUG ? addBorder(img) : img, dstX, dstY);
      if (DEBUG) {
        let timestamp = view.tileRingIndexToTileIndex.get(x);
        view.ctx.font = '12px Arial';
        view.ctx.fillStyle = 'blue';
        view.ctx.fillText(
          `${timestamp - startTimestamp} (${xCoord}, ${y})`,
          dstX + 5,
          dstY + 15
        );
      }
      view.ctx.closePath();
    }
  }
  view.needsRender = false;
}

let previewInvalidationTimeout: number;
let missingTimeout: number;

function stateMachine() {
  running = true;
  while (pendingStateChange) {
    pendingStateChange = false;
    const isMainActive =
      Atomics.load(workerData.activeViewId, 0) === mainView.viewId;
    const mPendingFullPaint = mainView.isPendingFullPaint();
    const pPendingFullPaint = previewView && previewView?.isPendingFullPaint();
    invalidations = removeContainedAdjacentRects(drainInvalidations());
    mainView.commitInvalidations(invalidations);
    if (previewView) {
      previewView.commitInvalidations(invalidations);
    }
    switch (getState()) {
      case RenderState.IDLE: {
        DEBUG && console.log('IDLE');
        switch (isMainActive) {
          case true: {
            if (mPendingFullPaint) {
              fullPaint(mainView);
              setState(RenderState.RENDERING, mainView.viewId);
              break;
            }

            if (mainView.missingRects.length > 0) {
              mainView.commitInvalidations(mainView.missingRects);
              mainView.missingRects.length = 0;
            }

            partialPaint(mainView);

            if (mainView.needsRender) {
              setState(RenderState.RENDERING, mainView.viewId);
              break;
            }
          }
          case false: {
            if (!previewView) break;
            if (pPendingFullPaint) {
              fullPaint(previewView);
              setState(RenderState.RENDERING, previewView.viewId);
              break;
            }

            if (previewView.missingRects.length > 0) {
              previewView.commitInvalidations(previewView.missingRects);
              previewView.missingRects.length = 0;
            }

            partialPaint(previewView);

            if (previewView.needsRender) {
              setState(RenderState.RENDERING, previewView.viewId);
              break;
            }

            // Always give priority back to the main view
            Atomics.store(workerData.activeViewId, 0, mainView.viewId);
          }
        }
        break;
      }
      case RenderState.TILE_PAINT:
        // owned by wasm_extensions.cxx, so just wait for a state change
        break;
      case RenderState.RENDERING: {
        let viewToRender: RenderedView = mainView;
        let otherView: RenderedView | undefined = previewView;
        if (
          previewView &&
          Atomics.load(workerData.activeViewId, 0) === previewView?.viewId
        ) {
          viewToRender = previewView;
          otherView = mainView;
        }

        render(viewToRender);

        if (viewToRender.didZoom) {
          viewToRender.didZoom = false;
        }

        if (viewToRender.didScroll) {
          viewToRender.renderedTileTop = Math.floor(
            viewToRender.renderedTopTwips / viewToRender.tileDimTwips
          );
          postMessage({ s: viewToRender.activeCanvasIndex })
          viewToRender.didScroll = false;
        }

        // If there where any missing tiles during rendering,
        // we need to repaint them and then re-render the view.
        if (viewToRender.missingRects.length > 0) {
          if (missingTimeout) clearTimeout(missingTimeout);
          missingTimeout = setTimeout(() => {
            setState(RenderState.IDLE, viewToRender.viewId);
            if (!running) stateMachine();
          });
          break;
        }

        // unset pending full paint for the view.
        if (viewToRender.isPendingFullPaint()) {
          viewToRender.setIsPendingFullPaint(false);
        }

        if (otherView && otherView.isPendingFullPaint()) {
          setState(RenderState.IDLE, otherView.viewId);
        } else if (otherView && otherView.didScroll) {
          setState(RenderState.IDLE, otherView.viewId);
        }

        // Always give priority back to the main view
        Atomics.store(workerData.activeViewId, 0, mainView.viewId);
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
  mainView.idleAreaPaint = true;
  if (!pendingInvalidate) {
    afterInvalidate().then((shouldRun) => {
      if (shouldRun) {
        mainView.idleAreaPaint = false;
        pendingStateChange = true;
        mainView.pendingPartialPaint = true;
        setState(RenderState.IDLE, mainView.viewId);
        if (!running) {
          stateMachine();
          maybeInvalidatePreview();
        }
      } else {
        setState(RenderState.IDLE, mainView.viewId);
        if (!running) {
          stateMachine();
        }
      }
    });

    if (idleDebounceTimeout) clearTimeout(idleDebounceTimeout);
  }
  running = false;
  idleDebounceTimeout = setTimeout(
    paintNonVisibleAreasWhileIdle,
    Math.max(trimmedMean(mainView.paintTimes), 10)
  );
}

function maybeInvalidatePreview() {
  if (!previewView) return;
  if (previewInvalidationTimeout) {
    clearTimeout(previewInvalidationTimeout);
  }
  // Try to schedule a new paint for invalidations for
  // the preview view. If no new main view invalidations are fired
  // this should trigger a paint for the preview view
  previewInvalidationTimeout = setTimeout(() => {
    if (Atomics.load(workerData.state, 0) !== RenderState.IDLE) {
      maybeInvalidatePreview();
    }

    previewView.pendingPartialPaint = true;
    setState(RenderState.IDLE, previewView.viewId);
    if (!running) stateMachine();
  }, 200);
}

let pendingInvalidate: boolean;
// TODO: Drop the polyfill when Firefox supports it: https://bugzilla.mozilla.org/show_bug.cgi?id=1884148
const afterInvalidate: () => Promise<boolean> = Atomics.waitAsync
  ? async function () {
      if (pendingInvalidate) return Promise.resolve(false);
      const res = Atomics.waitAsync(workerData.hasInvalidations, 0, 0);
      if (res.async) {
        pendingInvalidate = true;
        return res.value.then(() => {
          pendingInvalidate = false;
          return true;
        });
      }
      pendingInvalidate = false;
      return Promise.resolve(false);
    }
  : /** FireFox polyfill or other cases where Atomics.waitAsync doesn't exist  */
    function () {
      if (pendingInvalidate) return Promise.resolve(false);
      // don't spin up the worker if the state already doesn't match, saving about 6ms
      if (Atomics.load(workerData.hasInvalidations, 0) != 0) {
        pendingInvalidate = false;
        return Promise.resolve(false);
      }

      // the overhead for launching the worker is about 5ms, which is better than polling every 10ms
      // but we pool based on the state so that it just has overhead once. there is a cap of about 20 workers,
      // so this doesn't scale if the number of states grows larger than 5
      const waitOnInvalidatePolyfillWorker =
        globalThis.waitOnInvalidatePolyfillWorker ??
        new Worker(
          new URL('./firefox_Atomic_waitAsync_worker.js', import.meta.url)
        );
      if (!globalThis.waitOnInvalidatePolyfillWorker)
        globalThis.waitOnInvalidatePolyfillWorker =
          waitOnInvalidatePolyfillWorker;
      const promise = new Promise<boolean>((resolve) => {
        function resolver() {
          pendingInvalidate = false;
          resolve(true);
          waitOnInvalidatePolyfillWorker.removeEventListener(
            'message',
            resolver
          );
        }
        waitOnInvalidatePolyfillWorker.addEventListener('message', resolver);
      });
      waitOnInvalidatePolyfillWorker.postMessage([
        workerData.hasInvalidations,
        0,
        workerData.state,
      ]);
      pendingInvalidate = true;
      return promise;
    };

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
  view.tileRingIndexToTileIndex.set(tileIndex, Date.now());
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

function shouldPausePaint(view: RenderedView): boolean {
  // Main view should always be painted / rendered
  // in higher priority than the preview view
  if (view.viewId !== mainView.viewId) {
    const pause =
      mainView.needsRender ||
      mainView.isPendingFullPaint() ||
      mainView.didScroll;
    return pause;
  }

  const shouldPausePaint =
    Atomics.load(workerData.state, 0) === RenderState.RESET ||
    (view.scheduledTopTwips !== view.renderedTopTwips &&
      view.renderedTopTwips !== -1 &&
      view.scheduledTopTwips !== -1) ||
    (view.scheduledHeightTwips !== view.renderedHeightTwips &&
      view.renderedHeightTwips !== -1 &&
      view.scheduledHeightTwips !== -1);

  if (shouldPausePaint && DEBUG) {
    console.log('PAUSING');
  }

  return shouldPausePaint;
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

function hasUpdatedVisibleArea(view: RenderedView): boolean {
  return (
    view.scheduledTopTwips !== view.renderedTopTwips ||
    view.scheduledHeightTwips !== view.renderedHeightTwips
  );
}

let idleDebounceTimeout: number;
function paintNonVisibleAreasWhileIdle(view: RenderedView): void {
  if (!view?.idleAreaPaint) return;
  const visibleHeight = view.renderedHeightTwips;
  const docHeight = Atomics.load(workerData.docHeightTwips, 0);
  const visibleTop = view.renderedTopTwips;
  // heuristic is one whole visible area below the fold, primarily for zooming out, but also helps with scrolling a little
  const lowerFoldTop = visibleTop + visibleHeight;
  const lowerFoldHeight = Math.min(docHeight - lowerFoldTop, visibleHeight);
  const rangesToPaint = rectToTileIndexRanges(
    [0, lowerFoldTop, docWidthTwips, lowerFoldHeight],
    view.tileDimTwips,
    view.widthTileStride
  );

  let didPaint = false;
  for (let y = 0; y < rangesToPaint.length; ++y) {
    const [start, endInclusive] = rangesToPaint[y];
    for (let x = start; x <= endInclusive; ++x) {
      if (running || !view.idleAreaPaint) {
        return;
      }
      if (!view.validTiles.has(x)) {
        blockingPaintTile(view, x);
        didPaint = true;
      }
    }
  }

  if (didPaint) {
    if (previewInvalidationTimeout) {
      clearTimeout(previewInvalidationTimeout);
    }
    idleDebounceTimeout = setTimeout(
      () => paintNonVisibleAreasWhileIdle(mainView),
      Math.max(trimmedMean(view.paintTimes), 10)
    );
  }
}

function removeContainedAdjacentRects(rects: Rect[]): Rect[] {
  const result: Rect[] = [];

  for (let i = 0; i < rects.length; i++) {
    if (
      i === 0 ||
      !(
        isContainedOrEqual(rects[i], rects[i - 1]) &&
        isContainedOrEqual(rects[i], result.at(-1))
      )
    ) {
      result.push(rects[i]);
    }
  }

  return result;
}

function isContainedOrEqual(a: Rect, b: Rect): boolean {
  return (
    a[0] >= b[0] &&
    a[1] >= b[1] &&
    a[0] + a[2] <= b[0] + b[2] &&
    a[1] + a[3] <= b[1] + b[3]
  );
}

const PERCENT_OUTLIER = 0.2;
const MINIMUM_TIME_SAMPLES = 10; // Allows 1, 20% outliers to be trimmed from either side
const MAXIMUM_TIME_SAMPLES = 20; // Allows 2, 20% outliers to be trimmed from either side
// TODO: sample tile paint times then run paints in the background during idle
/** attempts to calculate a mean time per paint while excluding outliers */
function trimmedMean(input: number[]): number {
  if (input.length === 0) return 0;
  // skips work when there aren't enough viable samples
  if (input.length < MINIMUM_TIME_SAMPLES)
    return input.reduce((acc, val) => acc + val) / input.length;

  // sort the input array in-place, since mostly-sorted data performs better and it the side effect has no impact outside of this
  input.sort((a, b) => a - b);

  // calculate the number of elements to trim from each end
  const trimCount = Math.floor((input.length * PERCENT_OUTLIER) / 2);

  // trim the elements from both ends and calculate the mean of the remaining elements
  const trimmedArray = input.slice(trimCount, input.length - trimCount);

  // reduces work to the maximum number of time samples
  if (input.length > MAXIMUM_TIME_SAMPLES) {
    // permanently apply the previous trim
    input.splice(0, trimCount);
    input.splice(-trimCount, trimCount);
  }

  const sum = trimmedArray.reduce((acc, val) => acc + val, 0);

  return sum / trimmedArray.length;
}

function getContext(canvas: OffscreenCanvas) {
  let ctx = canvas.getContext("2d");
  // default is true, which makes things blurry
  //https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D/imageSmoothingEnabled
  ctx.imageSmoothingEnabled = false;
  return ctx;
}

function addBorder(imageData: ImageData) {
  const width = imageData.width;
  const height = imageData.height;
  const data = imageData.data;
  const red = 0;
  const green = 0;
  const blue = 255;
  const alpha = 255;

  for (let x = 0; x < width; x++) {
    const index = (x + 0 * width) * 4;
    data[index] = red;
    data[index + 1] = green;
    data[index + 2] = blue;
    data[index + 3] = alpha;
  }

  for (let x = 0; x < width; x++) {
    const index = (x + (height - 1) * width) * 4;
    data[index] = red;
    data[index + 1] = green;
    data[index + 2] = blue;
    data[index + 3] = alpha;
  }

  for (let y = 0; y < height; y++) {
    const index = (0 + y * width) * 4;
    data[index] = red;
    data[index + 1] = green;
    data[index + 2] = blue;
    data[index + 3] = alpha;
  }

  for (let y = 0; y < height; y++) {
    const index = (width - 1 + y * width) * 4;
    data[index] = red;
    data[index + 1] = green;
    data[index + 2] = blue;
    data[index + 3] = alpha;
  }

  return imageData;
}
