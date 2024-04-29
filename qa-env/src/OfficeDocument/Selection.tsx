import { twipsToCssPx } from '@lok';
import { CallbackType } from '@lok/lok_enums';
import { DocumentClient } from '@lok/shared';
import { For, JSX, Show, createMemo, splitProps } from 'solid-js';
import { createDocEventSignal } from './docEventSignal';
import { getOrCreateZoomSignal } from './zoom';

interface Props
  extends Omit<JSX.HTMLAttributes<HTMLDivElement>, 'ref' | 'style'> {
  doc: DocumentClient;
}

export function Selection(props: Props) {
  const [local, others] = splitProps(props, ['class', 'doc']);
  const doc = () => local.doc;
  const [zoom] = getOrCreateZoomSignal(doc);
  const selection = createDocEventSignal(
    () => props.doc,
    CallbackType.TEXT_SELECTION,
    (payload) =>
      payload.length !== 0
        ? payload
            .split('; ')
            .map((x) =>
              x.split(', ').map((x) => parseInt(x))
            )
        : undefined
  );

  const scaledSelection = createMemo(() =>
    selection()?.map((rect) => rect.map((x) => twipsToCssPx(x, zoom()))));

    return (
    <Show when={scaledSelection()}>
      <div class="mix-blend-multiply opacity-20 pointer-events-none absolute top-0">
        <For each={scaledSelection()}>
          {(rect: number[]) => (
            <div
              class={`bg-blue-400 pointer-events-none absolute top-0 ${local.class ?? ''}`}
              style={{
                transform: `translate(${rect[0]}px, ${rect[1]}px)`,
                width: `${rect[2]}px`,
                height: `${rect[3]}px`,
              }}
              {...others}
            />
          )}
        </For>
      </div>
    </Show>
  );
}
