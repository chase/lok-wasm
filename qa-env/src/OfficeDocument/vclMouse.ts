import { DocumentClient } from '@lok/shared';
import { eventModifiers } from './vclKeys';

const MOUSE_MOVE_INTERVAL_MS = 200;

// these are true globals, unless the system supports multiple non-gesture pointers
let mouseIsDown = false;

// Taken from LibreOfficeKitEnums.h
const VCL_MOUSE_DOWN = 0;
const VCL_MOUSE_UP = 1;
const VCL_MOUSE_MOVE = 2;

const VCL_MOUSE_MIDDLE = 2;
const VCL_MOUSE_RIGHT = 4;

function domMouseButtonsToVclButtons(evt: MouseEvent) {
  const { buttons } = evt;
  let newButtons = buttons & 1; // left is the same
  newButtons |= buttons & 2 ? VCL_MOUSE_RIGHT : 0; // right
  newButtons |= buttons & 4 ? VCL_MOUSE_MIDDLE : 0; // middle
  return newButtons;
}

export function handleMouseDown(doc: DocumentClient, evt: MouseEvent) {
  mouseIsDown = true;
  doc.postMouseEvent(
    VCL_MOUSE_DOWN,
    evt.offsetX * 15,
    evt.offsetY * 15,
    1,
    domMouseButtonsToVclButtons(evt),
    eventModifiers(evt)
  );
}

export function handleMouseUp(doc: DocumentClient, evt: MouseEvent) {
  mouseIsDown = false;
  doc.postMouseEvent(
    VCL_MOUSE_UP,
    evt.offsetX * 15,
    evt.offsetY * 15,
    0,
    domMouseButtonsToVclButtons(evt),
    eventModifiers(evt)
  );
}

let lastMouseMove = Date.now();
export function handleMouseMove(doc: DocumentClient, evt: MouseEvent) {
  if (!mouseIsDown && lastMouseMove + MOUSE_MOVE_INTERVAL_MS > Date.now()) {
    return;
  }

  doc.postMouseEvent(
    VCL_MOUSE_MOVE,
    evt.offsetX * 15,
    evt.offsetY * 15,
    0,
    domMouseButtonsToVclButtons(evt),
    eventModifiers(evt)
  );
  lastMouseMove = Date.now();
}
