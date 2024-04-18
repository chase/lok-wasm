import { DocumentClient } from '@lok/shared';
import { eventModifiers } from './vclKeys';
import { getOrCreateZoomSignal } from './zoom';
import { cssPxToTwips } from '@lok';

const MOUSE_MOVE_INTERVAL_MS = 200;

// these are true globals, unless the system supports multiple non-gesture pointers
let mouseIsDown = false;
let mouseDownButtons = 0;

// Taken from LibreOfficeKitEnums.h
const VCL_MOUSE_DOWN = 0;
const VCL_MOUSE_UP = 1;
const VCL_MOUSE_MOVE = 2;

const VCL_MOUSE_MIDDLE = 2;
const VCL_MOUSE_RIGHT = 4;

function domMouseButtonsToVclButtons(evt: PartialMouseEvent) {
  const { buttons } = evt;
  let newButtons = buttons & 1; // left is the same
  newButtons |= buttons & 2 ? VCL_MOUSE_RIGHT : 0; // right
  newButtons |= buttons & 4 ? VCL_MOUSE_MIDDLE : 0; // middle
  return newButtons;
}

function handleMouseEvent(
  doc: DocumentClient,
  evt: MouseEvent
): [x: number, y: number] {
  const [getZoom] = getOrCreateZoomSignal(() => doc);
  const zoom = getZoom();
  return [cssPxToTwips(evt.offsetX, zoom), cssPxToTwips(evt.offsetY, zoom)];
}

export function handleMouseDown(doc: DocumentClient, evt: MouseEvent) {
  mouseIsDown = true;
  mouseDownButtons = domMouseButtonsToVclButtons(evt);
  const [x, y] = handleMouseEvent(doc, evt);
  doc.postMouseEvent(
    VCL_MOUSE_DOWN,
    x,
    y,
    1,
    mouseDownButtons,
    eventModifiers(evt)
  );
}

export function handleMouseUp(doc: DocumentClient, evt: MouseEvent) {
  mouseIsDown = false;
  const [x, y] = handleMouseEvent(doc, evt);
  doc.postMouseEvent(
    VCL_MOUSE_UP,
    x,
    y,
    0,
    mouseDownButtons,
    eventModifiers(evt)
  );
}

export type PartialMouseEvent = Pick<MouseEvent,
  'metaKey' | 'altKey' | 'shiftKey' | 'ctrlKey' | 'offsetX' | 'offsetY' | 'buttons'>;

let lastMouseMove = Date.now();
export function handleMouseMove(doc: DocumentClient, evt: PartialMouseEvent) {
  if (!mouseIsDown && lastMouseMove + MOUSE_MOVE_INTERVAL_MS > Date.now()) {
    return;
  }

  const [x, y] = handleMouseEvent(doc, evt);
  doc.postMouseEvent(
    VCL_MOUSE_MOVE,
    x,
    y,
    0,
    domMouseButtonsToVclButtons(evt),
    eventModifiers(evt)
  );
  lastMouseMove = Date.now();
}
