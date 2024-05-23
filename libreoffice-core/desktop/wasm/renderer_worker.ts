type Rect = [
  /** x */ x: number,
  /** y */ y: number,
  /** w */ w: number,
  /** h */ h: number,
];

let canvas: OffscreenCanvas;
let ctx: OffscreenCanvasRenderingContext2D;

type TileIndexRange = [start: number, endInclusive: number];
let tileRing: Uint8Array;
let view: ViewData;

onmessage = ({ data }) => {
  switch (data.t) {
    case 'i': {
      const { newCanvas, newTileRing, view: newView} = data;
      canvas = newCanvas;
      tileRing = newTileRing;
      view = newView;
      ctx = canvas.getContext('2d');
      break;
    }
    case 'f': {
      if (!view) break;
      render();
      break;
    }
    case 'd' : {
      const viewData: ViewData = data.view;
      console.log('viewData', viewData);
      view = viewData;

      break;
    }
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

type ViewData = {
  scheduledTopTwips: number;
  scheduledHeightTwips: number;
  tileDimTwips: number;
  widthTileStride: number;
  scheduledWidthPx: number;
  scheduledHeightPx: number;
  docWidthTwips: number;
  tileIndexToTileRingIndex: Record<number, number>;
  tileSize: number;
}
function tileIndexToGridCoord(
  view: ViewData,
  tileIndex: number
): [x: number, y: number] {
  const y = (tileIndex / view.widthTileStride) | 0;
  const x = tileIndex % view.widthTileStride;
  return [x, y];
}
function getTileImageData(view: ViewData, tileRingIndex: number): ImageData {
  const tileRingOffset = tileRingIndex * view.tileSize * view.tileSize * 4;
  const tileRingDataUint32 = new Uint32Array(
    tileRing.buffer,
    tileRingOffset,
    view.tileSize * view.tileSize
  );

  const tileData = new Uint8ClampedArray(view.tileSize * view.tileSize * 4);

  for (let i = 0; i < tileRingDataUint32.length; ++i) {
    const pixelValue = Atomics.load(tileRingDataUint32, i);
    tileData[i * 4] = (pixelValue >> 24) & 0xff;
    tileData[i * 4 + 1] = (pixelValue >> 16) & 0xff;
    tileData[i * 4 + 2] = (pixelValue >> 8) & 0xff;
    tileData[i * 4 + 3] = pixelValue & 0xff;
  }

  return new ImageData(tileData, view.tileSize, view.tileSize);
}

function render() {
  let start = performance.now();
  const visibleTop = view.scheduledTopTwips;
  const visibleHeight = view.scheduledHeightTwips;

  const rangesToRender = rectToTileIndexRanges(
    [0, visibleTop, view.docWidthTwips, visibleHeight],
    view.tileDimTwips,
    view.widthTileStride
  );

  ctx.clearRect(0, 0, canvas.width, canvas.height);
  // canvas.height = view.scheduledHeightPx;
  // canvas.width = view.scheduledWidthPx;
  let renderedHeightTwips = visibleHeight;
  let renderedTopTwips = visibleTop;

  for (let y = 0; y < rangesToRender.length; ++y) {
    const [start, endInclusive] = rangesToRender[y];
    for (let x = start; x <= endInclusive; ++x) {
      const [xCoord] = tileIndexToGridCoord(view, x);
      const img: ImageData = getTileImageData(view,
        view.tileIndexToTileRingIndex[x]
      );
      if (!img) {
        continue;
      }
      const dstX: number = xCoord * view.tileSize;
      const dstY: number = y * view.tileSize;
      ctx.beginPath();
      ctx.putImageData(addBorder(img), dstX, dstY);
      ctx.closePath();
    }
  }
  let end = performance.now();

  self.postMessage({
    renderedTopTwips,
    renderedHeightTwips,
    time: end - start,
  })

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
