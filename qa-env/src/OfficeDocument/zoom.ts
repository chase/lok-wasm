import { DocumentClient } from '@lok/shared';
import { Accessor, Signal, createSignal } from 'solid-js';

const DEFAULT_ZOOM = 1;

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
