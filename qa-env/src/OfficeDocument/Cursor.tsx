import { twipsToCssPx } from '@lok';
import { DocumentClient } from '@lok/shared';
import { JSX, Show, createMemo, splitProps } from 'solid-js';
import {
  getOrCreateCursorPosition,
  getOrCreateCursorVisible,
} from './cursorSignal';
import { getOrCreateZoomSignal } from './zoom';
import { InputHandler } from './InputHandler';
import { getOrCreateDPISignal } from './twipConversion';

const CURSOR_WIDTH_PX = 2;

interface Props extends Omit<JSX.HTMLAttributes<HTMLDivElement>, 'children'> {
  doc: DocumentClient;
  ref?: (ref: HTMLDivElement) => void;
}

export function Cursor(props: Props) {
  const [local, others] = splitProps(props, ['doc', 'class', 'ref']);
  const doc = () => local.doc;

  const cursorVisible = getOrCreateCursorVisible(doc);
  const cursorPosition = getOrCreateCursorPosition(doc);
  const [zoom] = getOrCreateZoomSignal(doc);
  const dpi = getOrCreateDPISignal();
  const pos = createMemo(() => {
    const zoom_ = zoom();
    return cursorPosition()?.map((x) => twipsToCssPx(x, zoom_));
  });

  return (
    <Show when={pos()}>
      <div
        {...others}
        class={`absolute top-0 left-0 ${local.class ?? ''}`}
        style={{
          visibility: cursorVisible() ? 'visible' : 'hidden',
          transform: `translate(${pos()![0]}px, ${pos()![1]}px)`,
          height: `${pos()![3]}px`,
          width: `${CURSOR_WIDTH_PX / dpi()}px`,
          'pointer-events': 'none',
        }}
        ref={local.ref}
      />
      <InputHandler doc={doc} pos={pos()!} />
    </Show>
  );
}
