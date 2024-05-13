import { twipsToCssPx } from '@lok';
import { DocumentClient } from '@lok/shared';
import { Accessor, Signal, createSignal } from 'solid-js';
import { getOrCreateDPISignal } from './twipConversion';

export const DEFAULT_ZOOM = 0.8;
export const MAX_ZOOM = 3;
export const MIN_ZOOM = 0.375;

// Stepping by 1/8th seems to keep text relatively crisp
export const ZOOM_STEP = 0.125;

const docZooms = new WeakMap<DocumentClient, Signal<number>>();

export function getOrCreateZoomSignal(
  doc: Accessor<DocumentClient>
): Signal<number> {
  const doc_ = doc();
  let result = docZooms.get(doc_);
  if (result == null) {
    result = createSignal(DEFAULT_ZOOM);
    docZooms.set(doc_, result);
  }

  return result;
}

export function setZoom(doc: Accessor<DocumentClient>, level: number) {
  const [, set] = getOrCreateZoomSignal(doc);
  const getDpi = getOrCreateDPISignal();
  const dpi = getDpi();
  doc().setZoom(level, dpi);
  set(level);
}

/**
 * @param offset offset from the current zoom level
 * @returns the new zoom level
 * @example
 * // zoom out by 10%
 * updateZoom(doc, -0.1);
 * // zoom in by 10%
 * updateZoom(doc, 0.1);
 */
export function updateZoom(
  doc: Accessor<DocumentClient>,
  offset: number
): number {
  const [zoom] = getOrCreateZoomSignal(doc);
  const roundedZoom = Math.round((zoom() + offset) / Epsilon) * Epsilon;
  const newZoom = Math.min(MAX_ZOOM, Math.max(MIN_ZOOM, roundedZoom));

  setZoom(doc, newZoom);
  return newZoom;
}

/*
 * @param fit the kind of fit
 * @param marginPx the total amount to add to the width of the document
 * @returns the new zoom level
 */
export async function zoomToFit(
  doc: Accessor<DocumentClient>,
  container: { width: number; height: number },
  fit: 'width' | 'height' | 'widthMaxDefault',
  marginPx: number = 20
): Promise<number> {
  const [getZoom] = getOrCreateZoomSignal(doc);
  const zoom = getZoom();
  const { width, height } = container;
  let scaleFactor = 1;
  if (fit === 'width' || fit === 'widthMaxDefault') {
    const [docWidth] = await doc().documentSize();
    scaleFactor = (width - marginPx) / twipsToCssPx(docWidth, zoom);
  } else if (fit === 'height') {
    const rects = await doc().partRectanglesTwips();
    if (rects.length === 0) {
      console.error('no pages?');
    }
    scaleFactor = height / twipsToCssPx(rects[0].height + rects[0].y * 2, zoom);
  }
  const newZoom =
    fit === 'widthMaxDefault'
      ? Math.min(DEFAULT_ZOOM, scaleFactor * zoom)
      : scaleFactor * zoom;

  setZoom(doc, newZoom);

  return newZoom;
}

// 3 decimal places
const Epsilon = 0.001;
function fp_less_than(A: number, B: number) {
  return A - B < Epsilon && Math.abs(A - B) > Epsilon;
}

function fp_greater_than(A: number, B: number) {
  return A - B > Epsilon && Math.abs(A - B) > Epsilon;
}
export function canZoomIn(doc: Accessor<DocumentClient>) {
  const [zoom] = getOrCreateZoomSignal(doc);
  return fp_less_than(zoom(), MAX_ZOOM);
}

export function canZoomOut(doc: Accessor<DocumentClient>) {
  const [zoom] = getOrCreateZoomSignal(doc);
  return fp_greater_than(zoom(), MIN_ZOOM);
}

export function scale<T extends number[]>(a: T, scale: number): T {
  return a.map((x) => x * scale) as any;
}
