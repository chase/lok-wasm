import type { Document, TileRenderData } from "./soffice";

const NOT_CHROMIUM = navigator.userAgent.indexOf('Chrome/') === -1;

type Rect = [
	/** x */ x: number,
	/** y */ y: number,
	/** w */ w: number,
	/** h */ h: number,
];
const RECT_SIZE = 4;
const MAX_PAINTED_TILES_PER_ITER = 32;

type TileIndexRange = [start: number, endInclusive: number];

const DEBUG = false;

/** 15 = 1440 twips-per-inch / 96 dpi */
const LOK_INTERNAL_TWIPS_TO_PX = 15;

// At 256px, enough for 2 8K displays, about 260 MB of video memory
const POOL_SIZE = 2000;

export function renderer(doc: Document) {
	let d: TileRenderData;
	let activeCanvas: OffscreenCanvas;
	let ctx: OffscreenCanvasRenderingContext2D;
	let canvases: [OffscreenCanvas];
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
	let didScroll = false;
	let firstPaint = true;

	const visibleInvalidations: Rect[] = [];
	const nonVisibleInvalidations: Rect[] = [];

	let zoomResetTimeout: number | undefined;

	function scroll(/** scroll top in pixels */ y: number) {
		// scroll
		if (!activeCanvas) return;

		// even though technically data.y is in css pixels
		// we need to treat it as physical pixels because
		// it represents the amount we have scrolled through the canvas
		// document which is rendered in physical pixels
		scheduledTopTwips = pxToTwips(y);

		if (scheduledTopTwips !== renderedTopTwips) {
			didScroll = true;
			activeCanvas = canvases[activeCanvasIndex];
			Atomics.store(d.isVisibleAreaPainted, 0, 0);
		}
	}

	function resizeHeight(/** height */ h: number) {
		if (!activeCanvas) return;
		scheduledHeightPx = cssPxToPx(h, dpi);
		scheduledHeightTwips = pxToTwips(h);
		Atomics.store(d.scrollHeightTwips, 0, scheduledHeightTwips);
		const newHeight = Math.floor(scheduledHeightTwips / tileDimTwips);
		if (newHeight !== renderedHeightTwips) {
			Atomics.store(d.isVisibleAreaPainted, 0, 0);
		}
	}

	// FIXME: should 'w' be unused?
	function resizeWidth(/** width */ w: number) {
		let newDocWidthTwips = Atomics.load(d.docWidthTwips, 0);

		// A change in the width of the document most likely requires a full reset
		if (newDocWidthTwips !== docWidthTwips) {
			docWidthTwips = newDocWidthTwips;
			widthTileStride = Math.ceil(docWidthTwips / tileDimTwips);
			Atomics.store(d.widthTileStride, 0, widthTileStride);
			scheduledWidthPx = twipsToPx(docWidthTwips, dpi);
			Atomics.store(d.isVisibleAreaPainted, 0, 0);
		}
	}

	function handleZoom(data: {
		/** absolute scale */
		s: number;
		/** dpi */
		d: number;
	}) {
		if (!activeCanvas) return;
		// Clear the previously scheduled zoom reset
		if (zoomResetTimeout) clearTimeout(zoomResetTimeout);

		zoom(data.s, data.d);
	}

	/** Converts a CSS pixel to twips */
	function cssPxToTwips(px: number, dpi: number): number {
		return (px * scaledTwips) / dpi;
	}

	/** Converts a physical pixel to twips */
	function pxToTwips(px: number): number {
		return px * scaledTwips;
	}

	/** Converts a CSS pixel to a physical pixel */
	function cssPxToPx(px: number, dpi: number): number {
		return px * dpi;
	}

	/** Converts twips to physical pixels */
	function twipsToPx(twips: number, dpi: number): number {
		return (twips / scaledTwips) * dpi;
	}

	function getContext(canvas: OffscreenCanvas) {
		let ctx = canvas.getContext("2d");
		// default is true, which makes things blurry
		//https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D/imageSmoothingEnabled
		ctx.imageSmoothingEnabled = false;
		return ctx;
	}

	function zoom(in_scale: number, in_dpi: number) {
		console.log('zoom', in_scale, in_dpi);
		docWidthTwips = Atomics.load(d.docWidthTwips, 0);
		docHeightTwips = Atomics.load(d.docHeightTwips, 0);

		scaledTwips =
			clipToNearest8PxZoom(d.tileSize, 1 / in_scale) * LOK_INTERNAL_TWIPS_TO_PX;

		tileDimTwips = Math.ceil(cssPxToTwips(d.tileSize, dpi));
		widthTileStride = Math.ceil(docWidthTwips / tileDimTwips);
		Atomics.store(d.widthTileStride, 0, widthTileStride);
		Atomics.store(d.tileDimTwips, 0, tileDimTwips);
		// This is in physical pixels
		scheduledHeightPx = (activeCanvas.height * in_dpi) / dpi;
		scheduledHeightTwips = pxToTwips(activeCanvas.height);
		Atomics.store(d.scrollHeightTwips, 0, scheduledHeightTwips);
		Atomics.store(d.pendingFullPaint, 0, 1);

		scheduledWidthPx = twipsToPx(docWidthTwips, in_dpi);

		scale = in_scale;
		dpi = in_dpi;
	}

	function initialize(data: {
		c: [OffscreenCanvas];
		d: TileRenderData;
		/** absolute scale */
		s: number;
		/** top position in pixels */
		y: number;
		/** dpi */
		dpi: number;
	}) {
		d = data.d;
		canvases = data.c;
		activeCanvas = data.c[0];
		ctx = getContext(activeCanvas);

		ctx.clearRect(0, 0, activeCanvas.width, activeCanvas.height);

		scale = data.s;
		dpi = data.dpi;
		zoom(data.s, dpi);
		scheduledTopTwips = cssPxToTwips(data.y, dpi);
	}

	function prepareForRendering() {
		const visibleTop = scheduledTopTwips;
		const visibleHeight = scheduledHeightTwips;

		const rangesToRender = rectToTileIndexRanges([
			0,
			visibleTop,
			docWidthTwips,
			visibleHeight,
		]);
		if (renderedHeightTwips !== scheduledHeightTwips) {
			canvases[0].height = scheduledHeightPx;
		}
		if (activeCanvas.width !== scheduledWidthPx) {
			canvases[0].width = scheduledWidthPx;
		}
		renderedHeightTwips = visibleHeight;
		renderedTopTwips = visibleTop;

		return rangesToRender;
	}

  // NOTE: strictly speaking, this should never need to paint anymore, it's just a catch-all
	// precondition: prepareForRendering must be run prior
	/**
	 * @returns true if needs render, false otherwise
	 */
	function paintMissingVisibleTileRanges(rangesToRender: TileIndexRange[]) {
		let needsRender = false;
		for (let y = 0; y < rangesToRender.length && !shouldPausePaint(); ++y) {
			const [start, endInclusive] = rangesToRender[y];
			let runStart: number | null = null;
			for (let x = start; x <= endInclusive; ++x) {
				const hasImage = tileRing.has(tileIndexToTileRingIndex.get(x));
				if (hasImage) {
					// range has ended
					if (runStart != null) {
						batchedBlockingPaintTiles(runStart, x - 1, visibleRingTiles);
						runStart = null;
						needsRender = true;
					}
				} else {
					if (runStart != null) runStart = x;
				}
			}

			// remaining ranges
			if (runStart != null) {
				batchedBlockingPaintTiles(runStart, endInclusive, visibleRingTiles);
				needsRender = true;
			}
		}

		return needsRender;
	}

	// paints visible tiles and transfer to
	function paintAndRender(): boolean {
		const invalidations = removeContainedAdjacentRects(drainInvalidations());
		let needsRender = hasUpdatedVisibleArea();
		const rangesToPaint = prepareForRendering();
		const visibleTop = renderedTopTwips;
		const visibleHeight = renderedHeightTwips;

		if (Atomics.load(d.pendingFullPaint, 0) === 1) {
			// clear invalidations, because the whole area will be repainted
			visibleInvalidations.length = 0;
			nonVisibleInvalidations.length = 0;

			const newVisibleRingTiles = new Set<number>();

			// all tiles are invalid
			validTiles.clear();

			// effectively paints by rows of tiles, so there isn't any odd-looking tearing if painting is paused
			for (let y = 0; y < rangesToPaint.length && !shouldPausePaint(); ++y) {
				const [start, endInclusive] = rangesToPaint[y];
				batchedBlockingPaintTiles(start, endInclusive, newVisibleRingTiles);
			}

			visibleRingTiles = newVisibleRingTiles;

			// FIXME: This doesn't work correctly anymore
			// On Chrome: causes an infinite scroll loop, jumps back to top
			// On FF/Safari: just clears the whole area
			//
			// clearNonVisibleTiles();

			needsRender = true;
		} else if (invalidations.length != 0 || didScroll) {
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
					for (
						let y = 0;
						y < rangesToPaint.length && !shouldPausePaint();
						++y
					) {
						const [start, endInclusive] = rangesToPaint[y];
						batchedBlockingPaintTiles(start, endInclusive, newVisibleRingTiles);
						needsRender = true;
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
				let runStart: number | null = null;

				for (let x = start; x <= endInclusive; ++x) {
					const ringIndex = tileIndexToTileRingIndex.get(x);
					const valid =
						ringIndex != null &&
						validTiles.has(x) &&
						tileRingIndexToTileIndex.get(ringIndex) === x;

					if (valid) {
						newVisibleRingTiles.add(ringIndex);

						// the range is closed, paint the batch and reset the run
						if (runStart != null) {
							runStart = null;
							needsRender = true;
							batchedBlockingPaintTiles(runStart, x - 1, newVisibleRingTiles);
						}
					} else if (runStart == null) {
						runStart = x;
					}
				}

				// close last range if there's one open
				if (runStart != null) {
					needsRender = true;
					batchedBlockingPaintTiles(
						runStart,
						endInclusive,
						newVisibleRingTiles,
					);
				}
			}

			visibleInvalidations.length = 0;
			visibleRingTiles.clear();
			visibleRingTiles = newVisibleRingTiles;
		}

		needsRender ||= paintMissingVisibleTileRanges(rangesToPaint);

    // TODO: figure out why Firefox/Safari don't persist frames and
    // requires a new render every frame?
		if (NOT_CHROMIUM || needsRender || hasUpdatedVisibleArea()) {
			render(rangesToPaint);
		}

		return needsRender;
	}

	function hasUpdatedVisibleArea(): boolean {
		return (
			scheduledTopTwips !== renderedTopTwips ||
			scheduledHeightTwips !== renderedHeightTwips
		);
	}

	// renders the visible GPU textures to the canvas
	function render(rangesToRender: TileIndexRange[]) {
	  const offset = twipsToPx(renderedTopTwips % tileDimTwips, dpi);
		for (let y = 0; y < rangesToRender.length && !shouldPausePaint(); ++y) {
			const [start, endInclusive] = rangesToRender[y];
			for (let x = start; x <= endInclusive; ++x) {
				const [xCoord] = tileIndexToGridCoord(x);
				const img: ImageData = tileRing.get(tileIndexToTileRingIndex.get(x));
				if (!img) {
				  console.error('missing', x, y);
					continue;
				}
				const dstX: number = xCoord * d.tileSize;
				const dstY: number = y * d.tileSize - offset;
				ctx.beginPath();
				ctx.clearRect(dstX, dstY, d.tileSize, d.tileSize);
				ctx.putImageData(DEBUG ? addBorder(img) : img, dstX, dstY);

				if (DEBUG) {
					let timestamp = tileRingIndexToTime.get(tileIndexToTileRingIndex.get(x));
					ctx.font = "12px Arial";
					ctx.fillStyle = "blue";
					ctx.fillText(
						`${timestamp} (${xCoord}, ${y}) ${x}`,
						dstX + 5,
						dstY + 15,
					);
				}
				ctx.closePath();
			}
		}
		Atomics.store(d.isVisibleAreaPainted, 0, 1);

		Atomics.store(d.pendingFullPaint, 0, 0);
		didScroll = false;
		firstPaint = false;
	}

	function shouldPausePaint(): boolean {
		return (
			(scheduledTopTwips !== renderedTopTwips && renderedTopTwips !== -1) ||
			(scheduledHeightTwips !== renderedHeightTwips &&
				renderedHeightTwips !== -1)
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

	// TODO: figure out why Collabora disabled their time limit for draining events
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
			subarrayPos + RECT_SIZE,
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
		const [x,y,w,h] = rect;
		const result: TileIndexRange[] = [];
		const r = x + w;
		const x0 = Math.floor(x / tileDimTwips);
		const x1 = Math.floor(r / tileDimTwips);
		const b = Math.min(y + h, docHeightTwips);
		const y0 = Math.floor(y / tileDimTwips);
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

	/** sets the tileRingIndex to the next non-visible tile */
	function seekNonVisibleRingIndex() {
		const origin = tileRingIndex;
		tileRingIndex = ((tileRingIndex + 1) | 0) % POOL_SIZE;
		while (visibleRingTiles.has(tileRingIndex) && tileRingIndex != origin) {
			tileRingIndex = ((tileRingIndex + 1) | 0) % POOL_SIZE;
		}
		if (tileRingIndex === origin) {
			console.error("Texture pool too small for visible tiles");
		}
	}

	function blockingPaintTiles(
		startTileIndex: number,
		endTileIndex: number,
		ringIndexSet: Set<number>,
	): void {
	  const startTime = Date.now();
		const tileAllocSize =
			(d.paintedTiles.byteLength / MAX_PAINTED_TILES_PER_ITER) | 0;

		doc.paintTiles(d.viewId, startTileIndex, endTileIndex);

		for (
			let tileIndex = startTileIndex;
			tileIndex <= endTileIndex;
			tileIndex++
		) {
			const byteOffset = tileAllocSize * (tileIndex - startTileIndex);
			seekNonVisibleRingIndex();

			tileRingIndexToTileIndex.set(tileRingIndex, tileIndex);
			tileIndexToTileRingIndex.set(tileIndex, tileRingIndex);
			tileRingIndexToTime.set(tileRingIndex, Date.now() - startTime);
			tileRing.set(
				tileRingIndex,
				new ImageData(
					new Uint8ClampedArray(d.paintedTiles.slice(byteOffset, byteOffset + tileAllocSize)),
					d.tileSize,
					d.tileSize,
				),
			);
			validTiles.add(tileIndex);

			ringIndexSet.add(tileRingIndex);
		}
	}

	function batchedBlockingPaintTiles(
		start: number,
		endInclusive: number,
		ringIndexSet: Set<number>,
	): void {
		for (
			let batchStart = start;
			batchStart <= endInclusive;
			batchStart += MAX_PAINTED_TILES_PER_ITER
		) {
			const batchEnd = Math.min(
				batchStart + MAX_PAINTED_TILES_PER_ITER - 1,
				endInclusive,
			);

			blockingPaintTiles(batchStart, batchEnd, ringIndexSet);
		}
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

	// FIXME: this is clearing visible tiles
	function clearNonVisibleTiles() {
		const indices = tileRing.keys();
		for (const tileRingIndex of indices) {
			// but visible tiles shouldn't be cleared so there is something to paint
			if (visibleRingTiles.has(tileRingIndex)) continue;

			tileIndexToTileRingIndex.delete(
				tileRingIndexToTileIndex.get(tileRingIndex),
			);
			tileRing.delete(tileRingIndex);
			tileRingIndexToTileIndex.delete(tileRingIndex);
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

	return {
		initialize,
		scroll,
		resizeHeight,
		resizeWidth,
		zoom: handleZoom,
		paintAndRender
	};
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

function isContainedOrEqual(a: Rect, b: Rect): boolean {
	return (
		a[0] >= b[0] &&
		a[1] >= b[1] &&
		a[0] + a[2] <= b[0] + b[2] &&
		a[1] + a[3] <= b[1] + b[3]
	);
}

export function clipToNearest8PxZoom(w: number, s: number): number {
	const scaledWidth: number = Math.ceil(w * s);
	const mod: number = scaledWidth % 8;
	if (mod === 0) return s;

	return Math.abs((scaledWidth - mod) / w - s) <
		Math.abs((scaledWidth + 8 - mod) / w - s)
		? (scaledWidth - mod) / w
		: (scaledWidth + 8 - mod) / w;
}
