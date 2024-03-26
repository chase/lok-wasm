import { CallbackType } from '@lok/lok_enums';
import { DocumentClient } from '@lok/shared';
import { Accessor } from 'solid-js';
import {
    createDocEventSignal,
    createInitalizedDocEventSignal,
} from './docEventSignal';

const cursorVisibleMap = new WeakMap<DocumentClient, Accessor<boolean>>();
export function getOrCreateCursorVisible(
  doc: Accessor<DocumentClient>
): Accessor<boolean> {
  const doc_ = doc();
  let result = cursorVisibleMap.get(doc_);
  if (result == null) {
    result = createInitalizedDocEventSignal(
      () => doc_,
      CallbackType.CURSOR_VISIBLE,
      (payload) => payload === 'true',
      true
    );
  }
  return result;
}

export type CursorRectTwips = [
  x: number,
  y: number,
  width: number,
  height: number,
];

type ParsedInvalidateVisibleCursor = {
  viewId: string;
  rectangle: string;
  misspelledWord: 1 | 0;
};

const toInt = (x: string) => Number.parseInt(x);

const cursorPositionMap = new WeakMap<
  DocumentClient,
  Accessor<CursorRectTwips | undefined>
>();
export function getOrCreateCursorPosition(
  doc: Accessor<DocumentClient>
): Accessor<CursorRectTwips | undefined> {
  const doc_ = doc();
  let result = cursorPositionMap.get(doc_);
  if (result == null) {
    result = createDocEventSignal(
      doc,
      CallbackType.INVALIDATE_VISIBLE_CURSOR,
      (payload) => {
        const { viewId } = doc_;
        const event: ParsedInvalidateVisibleCursor = JSON.parse(payload);
        // if the view ID doesn't match, don't set the signal
        if (viewId !== Number(event.viewId)) return undefined;

        const rect = event.rectangle.split(', ').map(toInt);
        return rect as CursorRectTwips;
      },
      true
    );
  }
  return result;
}
