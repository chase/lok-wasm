import { DocumentClient } from '@lok/shared';
import { Accessor, Signal, createSignal } from 'solid-js';

export const [ hasFocus, setHasFocus ] = createSignal<boolean>(false);
export const [ focusRef, setFocusRef ] = createSignal<HTMLDivElement | null>(null);

const signals = new WeakMap<DocumentClient, Signal<boolean>>();

export function getOrCreateFocusedSignal(
  doc: Accessor<DocumentClient>
): Signal<boolean> {
  const doc_ = doc();
  let result = signals.get(doc_);
  if (result == null) {
    result = createSignal(false);
    signals.set(doc_, result);
  }

  return result;
}
