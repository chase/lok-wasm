import type { TileRenderData } from './soffice';
import type { ToTileRenderer } from './shared';

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
const RECT_SIZE = 4;

type TileIndexRange = [start: number, endInclusive: number];

const DEBUG = false;
const START_TIMESTAMP = Date.now();

/** 15 = 1440 twips-per-inch / 96 dpi */
const LOK_INTERNAL_TWIPS_TO_PX = 15;

// At 256px, enough for 2 8K displays, about 260 MB of video memory
const POOL_SIZE = 2000;

let d: TileRenderData;
let activeCanvas: OffscreenCanvas;
let ctx: OffscreenCanvasRenderingContext2D;
let canvases: OffscreenCanvas[];
let activeCanvasIndex: number = 0;
let tileRingIndexToTime: Map<number, number> = new Map();

/** contains a set of all the valid tile indices */
const validTiles: Set<number> = new Set();
/** maps a tile index to a tileRing index */
const tileIndexToTileRingIndex: Map<number, number> = new Map();
/** maps a tileRing index to a tile index */
const tileRingIndexToTileIndex: Map<number, number> = new Map();
/** a ring buffer that shouldn't allocate more than POOL_SIZE */
let tileRing: Map<number, ImageData> = new Map();
/** contains a set of all visible tile ring indices */
let visibleRingTiles: Set<number> = new Set();
/** ring index that wraps on POOL_SIZE */
let tileRingIndex = 0;
let tileDimTwips: number;
let docWidthTwips: number;
let docHeightTwips: number;
/** the number of tiles that make up a horizontal stride for the width of the document */
let widthTileStride: number;
let renderedTopTwips: number = -1;
let renderedHeightTwips: number = -1;
let scheduledTopTwips: number = -1;
let scheduledHeightTwips: number = -1;
let scaledTwips: number = LOK_INTERNAL_TWIPS_TO_PX;
let scale: number = 1;
let dpi: number = 1;
let scheduledWidthPx: number = -1;
let scheduledHeightPx: number = -1;
let renderedTileTop: number = 0;
let didScroll = false;

const visibleInvalidations: Rect[] = [];
const nonVisibleInvalidations: Rect[] = [];

let pendingStateChange = false;
let running = false;
let idleAreaPaint = false;
let zoomResetTimeout: number | undefined;

onmessage = ({ data }: { data: ToTileRenderer }) => {
  switch (data.t) {
    case 'i': // initialize
      initialize(data);
      break;
    case 's': // scroll
      if (!activeCanvas) return;
      idleAreaPaint = false;
      scheduledTopTwips = data.y * scaledTwips;
      // Checks that the tile row has changed since the last scroll
      if (Math.floor(scheduledTopTwips / tileDimTwips) !== renderedTileTop) {
        didScroll = true;
        activeCanvasIndex ^= 1;
        activeCanvas = canvases[activeCanvasIndex];
        ctx = getContext(activeCanvas);
        setState(RenderState.IDLE);
        if (!running) stateMachine();
      } else {
        if (!didScroll) postMessage({ s: activeCanvasIndex });
      }
      break;
    case 'r': // resize
      if (!activeCanvas) return;
      idleAreaPaint = false;
      scheduledHeightPx = data.h * dpi;
      scheduledHeightTwips = data.h * scaledTwips;
      setState(RenderState.IDLE);
      if (!running) stateMachine();
      break;
    case 'z': // zoom
      if (!activeCanvas) return;
      // Clear the previously scheduled zoom reset
      if (zoomResetTimeout) clearTimeout(zoomResetTimeout);

      idleAreaPaint = false;
      zoom(data.s, data.d);

      // Ensure we debounce a reset in combination with shouldStopPaint
      zoomResetTimeout = setTimeout(() => {
        setState(RenderState.RESET);
        Atomics.wait(d.state, 0, RenderState.RESET); // wait for reset to finish
        if (!running) stateMachine();
      });

      break;
  }
};

function getContext(canvas: OffscreenCanvas) {
  let ctx = canvas.getContext("2d");
  // default is true, which makes things blurry
  //https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D/imageSmoothingEnabled
  ctx.imageSmoothingEnabled = false;
  return ctx;
}

function zoom(in_scale: number, in_dpi: number) {
  docWidthTwips = Atomics.load(d.docWidthTwips, 0);
  docHeightTwips = Atomics.load(d.docHeightTwips, 0);

  scaledTwips =
    clipToNearest8PxZoom(d.tileSize, 1 / (in_scale * in_dpi)) *
    LOK_INTERNAL_TWIPS_TO_PX;

  tileDimTwips = Math.ceil(d.tileSize * scaledTwips);
  widthTileStride = Math.ceil(docWidthTwips / tileDimTwips);
  scheduledHeightPx = (activeCanvas.height * in_dpi) / dpi;
  scheduledHeightTwips = activeCanvas.height * scaledTwips;
  scheduledWidthPx = docWidthTwips / scaledTwips;

  // Set this as a reference for the new position for the next scroll event
  renderedTileTop = Math.floor(scheduledTopTwips / tileDimTwips);

  scale = in_scale;
  dpi = in_dpi;
}

function initialize(data: ToTileRenderer & { t: 'i' }) {
  d = data.d;
  canvases = data.c;
  activeCanvas = data.c[0];
  ctx = getContext(activeCanvas);

  ctx.clearRect(0, 0, activeCanvas.width, activeCanvas.height);

  scale = data.s;
  dpi = data.dpi;
  zoom(data.s, dpi);
  scheduledTopTwips = data.y * scaledTwips;

  pendingStateChange = true;
  stateMachine();
}

// while LOK is idle, paints visible tiles first then non-visible tiles to textures
function idle() {
  const invalidations = removeContainedAdjacentRects(drainInvalidations());
  const visibleTop = scheduledTopTwips;
  const visibleHeight = scheduledHeightTwips;
  let needsRender = false;

  if (Atomics.load(d.pendingFullPaint, 0) === 1) {
    // clear invalidations, because the whole area will be repainted
    visibleInvalidations.length = 0;
    nonVisibleInvalidations.length = 0;

    const rangesToPaint = rectToTileIndexRanges([
      0,
      visibleTop,
      docWidthTwips,
      visibleHeight,
    ]);

    const newVisibleRingTiles = new Set<number>();

    // all tiles are invalid
    validTiles.clear();

    // effectively paints by rows of tiles, so there isn't any odd-looking tearing if painting is paused
    for (let y = 0; y < rangesToPaint.length && !shouldPausePaint(); ++y) {
      const [start, endInclusive] = rangesToPaint[y];
      for (let x = start; x <= endInclusive; ++x) {
        newVisibleRingTiles.add(blockingPaintTile(x));
      }
    }
    visibleRingTiles.clear();
    visibleRingTiles = newVisibleRingTiles;

    clearNonVisibleTiles();

    needsRender = true;
  } else {
    // rebalance visible and non-visible
    invalidations.push(...visibleInvalidations);
    invalidations.push(...nonVisibleInvalidations);
    visibleInvalidations.length = 0;
    nonVisibleInvalidations.length = 0;

    for (const invalidation of invalidations) {
      commitVisibleAndNonVisible(invalidation, visibleTop, visibleHeight);
    }

    const newVisibleRingTiles = new Set<number>();

    if (visibleInvalidations.length != 0) {
      const tileRangesToPaint = [];
      // first do a pass and mark the invalid tiles, to prevent uncessary work by invalidating/painting overlapping areas
      for (const visibleInvalidation of visibleInvalidations) {
        const rangesToPaint = rectToTileIndexRanges(visibleInvalidation);
        markInvalid(rangesToPaint);
        tileRangesToPaint.push(rangesToPaint);
      }

      for (const rangesToPaint of tileRangesToPaint) {
        // effectively paints by rows of tiles, so there isn't any odd-looking tearing if painting is paused
        for (let y = 0; y < rangesToPaint.length && !shouldPausePaint(); ++y) {
          const [start, endInclusive] = rangesToPaint[y];
          for (let x = start; x <= endInclusive; ++x) {
            if (!validTiles.has(x)) {
              newVisibleRingTiles.add(blockingPaintTile(x));
              needsRender = true;
            }
          }
        }
      }
    }

    const visibleRangesToPaint = rectToTileIndexRanges([
      0,
      visibleTop,
      docWidthTwips,
      visibleHeight,
    ]);

    for (
      let y = 0;
      y < visibleRangesToPaint.length && !shouldPausePaint();
      ++y
    ) {
      const [start, endInclusive] = visibleRangesToPaint[y];
      for (let x = start; x <= endInclusive; ++x) {
        const ringIndex = tileIndexToTileRingIndex.get(x);
        if (
          ringIndex != null &&
          validTiles.has(x) &&
          tileRingIndexToTileIndex.get(ringIndex) === x
        ) {
          newVisibleRingTiles.add(ringIndex);
        } else {
          newVisibleRingTiles.add(blockingPaintTile(x));
          needsRender = true;
        }
      }
    }

    visibleInvalidations.length = 0;
    visibleRingTiles.clear();
    visibleRingTiles = newVisibleRingTiles;
  }

  if (needsRender || hasUpdatedVisibleArea()) {
    setState(RenderState.RENDERING);
  }
}

function hasUpdatedVisibleArea(): boolean {
  return (
    scheduledTopTwips !== renderedTopTwips ||
    scheduledHeightTwips !== renderedHeightTwips
  );
}

// renders the visible GPU textures to the canvas
function rendering() {
  const visibleTop = scheduledTopTwips;
  const visibleHeight = scheduledHeightTwips;

  const rangesToRender = rectToTileIndexRanges([
    0,
    visibleTop,
    docWidthTwips,
    visibleHeight,
  ]);
  // TODO: mark missing and invalidate
  ctx.clearRect(0, 0, activeCanvas.width, activeCanvas.height);
  if (renderedHeightTwips !== scheduledHeightTwips) {
    canvases[0].height = scheduledHeightPx;
    canvases[1].height = scheduledHeightPx;
  }
  if (activeCanvas.width !== scheduledWidthPx) {
    canvases[0].width = scheduledWidthPx;
    canvases[1].width = scheduledWidthPx;
  }
  renderedHeightTwips = visibleHeight;
  renderedTopTwips = visibleTop;

  for (let y = 0; y < rangesToRender.length && !shouldPausePaint(); ++y) {
    const [start, endInclusive] = rangesToRender[y];
    for (let x = start; x <= endInclusive; ++x) {
      // TODO: mark missing and invalidate
      const [xCoord] = tileIndexToGridCoord(x);
      const img: ImageData = tileRing.get(tileIndexToTileRingIndex.get(x));
      if (!img) {
        console.error('missing texture at ', x, xCoord, y);
        continue;
      }
      const dstX: number = xCoord * d.tileSize;
      const dstY: number = y * d.tileSize;
      ctx.beginPath();
      ctx.putImageData(DEBUG ? addBorder(img) : img, dstX, dstY);
      if (DEBUG) {
        let timestamp = tileRingIndexToTime.get(x);
        console.log(timestamp, START_TIMESTAMP);
        ctx.font = '12px Arial';
        ctx.fillStyle = 'blue';
        ctx.fillText(
          `${timestamp - START_TIMESTAMP} (${xCoord}, ${y})`,
          dstX + 5,
          dstY + 15
        );
      }
      ctx.closePath();
    }
  }

  Atomics.store(d.pendingFullPaint, 0, 0);

  setState(RenderState.IDLE);
  if (didScroll) {
    renderedTileTop = Math.floor(renderedTopTwips / tileDimTwips);
    postMessage({ s: activeCanvasIndex });
    didScroll = false;
  }
}

let pendingInvalidate: boolean;
// TODO: Drop the polyfill when Firefox supports it: https://bugzilla.mozilla.org/show_bug.cgi?id=1884148
const afterInvalidate: () => Promise<boolean> = Atomics.waitAsync
  ? async function () {
      if (pendingInvalidate) return Promise.resolve(false);
      const res = Atomics.waitAsync(d.hasInvalidations, 0, 0);
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
      if (Atomics.load(d.hasInvalidations, 0) != 0) {
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
        d.hasInvalidations,
        0,
        d.state,
      ]);
      pendingInvalidate = true;
      return promise;
    };

function stateMachine() {
  running = true;
  while (pendingStateChange) {
    pendingStateChange = false;
    switch (getState()) {
      case RenderState.IDLE:
        idle();
        break;
      case RenderState.TILE_PAINT:
        // owned by wasm_extensions.cxx, so just wait for a state change
        break;
      case RenderState.RENDERING:
        rendering();
        break;
      case RenderState.RESET:
        // owned by wasm_extensions.cxx, so just wait for a state change
        break;
      case RenderState.QUIT:
        self.close();
    }
  }
  idleAreaPaint = true;
  if (!pendingInvalidate) {
    afterInvalidate().then((shouldRun) => {
      idleAreaPaint = false;
      pendingStateChange ||= shouldRun;
      if (shouldRun && !running) {
        stateMachine();
      }
    });
    if (idleDebounceTimeout) clearTimeout(idleDebounceTimeout);
  }
  running = false;
  idleDebounceTimeout = setTimeout(
    paintNonVisibleAreasWhileIdle,
    Math.max(trimmedMean(paintTimes), 10)
  );
}

let idleDebounceTimeout: number;
function paintNonVisibleAreasWhileIdle(): void {
  if (!idleAreaPaint) return;
  const visibleHeight = renderedHeightTwips;
  const docHeight = Atomics.load(d.docHeightTwips, 0);
  const visibleTop = renderedTopTwips;
  // heuristic is one whole visible area below the fold, primarily for zooming out, but also helps with scrolling a little
  const lowerFoldTop = visibleTop + visibleHeight;
  const lowerFoldHeight = Math.min(docHeight - lowerFoldTop, visibleHeight);
  const rangesToPaint = rectToTileIndexRanges([
    0,
    lowerFoldTop,
    docWidthTwips,
    lowerFoldHeight,
  ]);

  let didPaint = false;
  for (let y = 0; y < rangesToPaint.length; ++y) {
    const [start, endInclusive] = rangesToPaint[y];
    for (let x = start; x <= endInclusive; ++x) {
      if (running || !idleAreaPaint) {
        return;
      }
      if (!validTiles.has(x)) {
        blockingPaintTile(x);
        didPaint = true;
      }
    }
  }

  if (didPaint) {
    idleDebounceTimeout = setTimeout(
      paintNonVisibleAreasWhileIdle,
      Math.max(trimmedMean(paintTimes), 10)
    );
  }
}

function shouldPausePaint(): boolean {
  return (
    Atomics.load(d.state, 0) === RenderState.RESET ||
    (scheduledTopTwips !== renderedTopTwips && renderedTopTwips !== -1) ||
    (scheduledHeightTwips !== renderedHeightTwips && renderedHeightTwips !== -1)
  );
}

function commitVisibleAndNonVisible(rect: Rect, y: number, h: number) {
  const [rx, ry, rw, rh] = rect;
  const b = y + h;
  const rb = ry + rh;

  // Calculate the overlap
  const overlapTop = Math.max(ry, y);
  const overlapBottom = Math.min(rb, b);

  // Check if no overlap
  if (overlapTop >= overlapBottom) {
    nonVisibleInvalidations.push(rect);
    return;
  }

  visibleInvalidations.push([rx, overlapTop, rw, overlapBottom - overlapTop]);

  // Above the overlap
  if (ry < overlapTop) {
    nonVisibleInvalidations.push([rx, ry, rw, overlapTop - ry]);
  }

  // Below the overlap
  if (rb > overlapBottom) {
    nonVisibleInvalidations.push([rx, overlapBottom, rw, rb - overlapBottom]);
  }
}

function hasInvalidations(): boolean {
  return Atomics.load(d.hasInvalidations, 0) === 1;
}

// TODO: does this need to be time-limited?
function drainInvalidations(): Rect[] {
  const result: Rect[] = [];
  let invalidation: Rect | undefined;
  do {
    invalidation = takeInvalidation();
    if (invalidation != null) result.push(invalidation);
  } while (invalidation != null);
  Atomics.store(d.hasInvalidations, 0, 0);
  return result;
}

// Operates on a fixed-size thread-safe stack, JS-side is always SEQ_CST so there's some perf loss
function takeInvalidation(): Rect | undefined {
  if (!hasInvalidations()) return undefined;

  const head = Atomics.load(d.invalidationStackHead, 0);
  if (head < 0) return undefined;

  const result = new Array<number>(RECT_SIZE) as Rect;

  const subarrayPos = head * RECT_SIZE;
  const invalidRect = d.invalidationStack.subarray(
    subarrayPos,
    subarrayPos + RECT_SIZE
  );

  for (let i = 0; i < RECT_SIZE; ++i) {
    result[i] = Atomics.load(invalidRect, i);
  }
  const newHead = head - 1;
  Atomics.store(d.invalidationStackHead, 0, newHead);
  if (newHead < 0) {
    Atomics.store(d.hasInvalidations, 0, 1);
    Atomics.notify(d.hasInvalidations, 0);
  }
  return result;
}

function rectToTileIndexRanges(rect: Rect): TileIndexRange[] {
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

function tileIndexToGridCoord(tileIndex: number): [x: number, y: number] {
  const y = (tileIndex / widthTileStride) | 0;
  const x = tileIndex % widthTileStride;
  return [x, y];
}

function tileIndexToTwipsRect(tileIndex: number): Rect {
  const [x, y] = tileIndexToGridCoord(tileIndex);
  return [x * tileDimTwips, y * tileDimTwips, tileDimTwips, tileDimTwips];
}

/** sets the tileRingIndex to the next non-visible tile */
function seekNonVisibleRingIndex() {
  const origin = tileRingIndex;
  tileRingIndex = ((tileRingIndex + 1) | 0) % POOL_SIZE;
  while (visibleRingTiles.has(tileRingIndex) && tileRingIndex != origin) {
    tileRingIndex = ((tileRingIndex + 1) | 0) % POOL_SIZE;
  }
  if (tileRingIndex === origin) {
    console.error('Texture pool too small for visible tiles');
  }
}

let paintTimes: number[] = [];
/** blocks the worker thread while painting a tile at tileIndex, returning the corresponding tileRingIndex */
function blockingPaintTile(tileIndex: number): number {
  Atomics.wait(d.state, 0, RenderState.TILE_PAINT); // wait for existing paint to finish if necessary
  const start = Date.now();
  const rect = tileIndexToTwipsRect(tileIndex);
  for (let i = 0; i < RECT_SIZE; ++i) {
    Atomics.store(d.tileTwips, i, rect[i]);
  }
  setState(RenderState.TILE_PAINT);
  Atomics.wait(d.state, 0, RenderState.TILE_PAINT); // wait to finish
  seekNonVisibleRingIndex();

  tileRingIndexToTileIndex.set(tileRingIndex, tileIndex);
  tileIndexToTileRingIndex.set(tileIndex, tileRingIndex);
  tileRingIndexToTime.set(tileRingIndex, Date.now());
  tileRing.set(
    tileRingIndex,
    new ImageData(
      new Uint8ClampedArray(
        d.paintedTile,
        d.paintedTile.byteOffset,
        d.paintedTile.byteLength
      ),
      d.tileSize,
      d.tileSize
    )
  );
  validTiles.add(tileIndex);
  paintTimes.push(Date.now() - start);

  return tileRingIndex;
}

function markInvalid(ranges: TileIndexRange[]) {
  for (const range of ranges) {
    const [start, endInclusive] = range;
    for (let i = start; i <= endInclusive; ++i) {
      validTiles.delete(i);
      tileIndexToTileRingIndex.delete(i);
    }
  }
}

function clearNonVisibleTiles() {
  const indices = tileRing.keys();
  for (const tileRingIndex of indices) {
    // but visible tiles shouldn't be cleared so there is something to paint
    if (visibleRingTiles.has(tileRingIndex)) continue;

    tileIndexToTileRingIndex.delete(
      tileRingIndexToTileIndex.get(tileRingIndex)
    );
    tileRing.delete(tileRingIndex);
    tileRingIndexToTileIndex.delete(tileRingIndex);
  }
}

function setState(state: RenderState): void {
  pendingStateChange = true;
  Atomics.store(d.state, 0, state);
  Atomics.notify(d.state, 0);
}

function getState(): RenderState {
  return Atomics.load(d.state, 0);
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
