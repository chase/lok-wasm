import { conversionTable } from '@lok';
import type { DocumentClient, RectanglePx, RectangleTwips } from '@lok/shared';
import {
  For,
  JSX,
  Setter,
  createEffect,
  createMemo,
  createSignal,
  onCleanup,
  onMount,
} from 'solid-js';
import { CallbackType } from '@lok/lok_enums';
import { ScrollArea } from './ScrollArea';
import { Cursor } from './Cursor';
import { Selection } from './Selection';
import * as vclMouse from './vclMouse';
import { createDocEventSignal } from './docEventSignal';
import { createKeyHandler } from './vclKeys';
import { getOrCreateFocusedSignal } from './focus';
import { frameThrottle } from './frameThrottle';

const OBSERVED_SIZE_DEBOUNCE = 100; //ms

const convert = conversionTable(1, window.devicePixelRatio);

/** TODO: remove this so a document can be replaced */
let rendered = false;
function rectToPx(rect: RectangleTwips): RectanglePx {
  return {
    width: Math.round(rect.width * convert.twipsToPx),
    height: Math.round(rect.height * convert.twipsToPx),
    x: Math.round(rect.x * convert.twipsToPx),
    y: Math.round(rect.y * convert.twipsToPx),
  };
}

const BORDER_WIDTH = 1;

interface Props extends Omit<JSX.HTMLAttributes<HTMLDivElement>, 'onScroll'> {
  doc: DocumentClient;
  scrollAreaRef?: (ref: HTMLDivElement) => void;
}

type Dimensions = [number, number];

declare module 'solid-js' {
  namespace JSX {
    interface Directives {
      observedSize: [DocumentClient, Setter<number | undefined>];
    }
    interface CustomCaptureEvents {
      mousedown: MouseEvent;
      mouseup: MouseEvent;
      mousemove: MouseEvent;
    }
  }
}
false && observedSize; // workaround for "unused" function warning with Solid directives

function observedSize(
  el: Element,
  setter: () => [DocumentClient, Setter<number | undefined>]
) {
  const [doc, setHeight] = setter();
  let debounce: ReturnType<typeof setTimeout>;
  const observer = new ResizeObserver((entries) => {
    const entry = entries[0];
    const height = entry.contentRect
      ? entry.contentRect.height
      : entry.contentBoxSize
        ? entry.contentBoxSize[0].blockSize
        : 0;
    if (debounce) clearTimeout(debounce);
    debounce = setTimeout(async () => {
      setHeight(height);
      await doc.setVisibleHeight(height);
    }, OBSERVED_SIZE_DEBOUNCE);
  });

  onMount(() => {
    observer.observe(el);
    setHeight(el.clientHeight);
  });

  onCleanup(() => {
    observer.disconnect();
  });
}

export function OfficeDocument(props: Props) {
  let scrollAreaRef: HTMLDivElement | undefined;
  /** TODO: add DPI observer and handle DPI change */
  /** TODO: add context loss for machine if left asleep */
  const [containerHeight, setContainerHeight] = createSignal<
    number | undefined
  >();
  const [docSizeTwips, setDocSizeTwips] = createSignal<
    Dimensions | undefined
  >();
  const [rectsTwips, setRectsTwips] = createSignal<RectanglePx[] | undefined>();
  const [canvas, setCanvas] = createSignal<HTMLCanvasElement | undefined>();
  const cursorType = createDocEventSignal(
    () => props.doc,
    CallbackType.MOUSE_POINTER
  );

  createEffect(async () => {
    setDocSizeTwips(await props.doc.documentSize());
    setRectsTwips(await props.doc.partRectanglesTwips());
  });

  const docSizePx = () =>
    docSizeTwips()?.map((i) => Math.round(i * convert.twipsToPx));
  const rectsPx = () =>
    rectsTwips()
      ?.map(rectToPx)
      .filter((rect) => rect.width && rect.height);

  createEffect(() => {
    const callback = async () => {
      setDocSizeTwips(await props.doc.documentSize());
      setRectsTwips(await props.doc.partRectanglesTwips());
    };
    props.doc.on(CallbackType.DOCUMENT_SIZE_CHANGED, callback);
    onCleanup(() => {
      props.doc.off(CallbackType.DOCUMENT_SIZE_CHANGED, callback);
    });
  });

  createEffect(() => {
    const height = containerHeight();
    if (height) props.doc.setVisibleHeight(height);
  });

  createEffect(async () => {
    const width = docSizePx()?.[0];
    const height = containerHeight();
    const canvas_ = canvas();
    if (!width || !height || rendered || !canvas_ || !props.doc) return;
    canvas_.width = width;
    canvas_.height = height;
    await props.doc.startRendering(
      canvas_.transferControlToOffscreen(),
      256,
      1
    );
    rendered = true;
  });

  const [focused, setFocused] = getOrCreateFocusedSignal(() => props.doc);

  const keyhandler = createMemo(() =>
    createKeyHandler(() => props.doc, focused)
  );

  return (
    <>
      <div
        class="flex flex-1 justify-center relative overflow-hidden"
        use:observedSize={[props.doc, setContainerHeight]}
      >
        <ScrollArea
          class="absolute top-0"
          onScroll={(yPx) => {
            props.doc.setScrollTop(yPx);
          }}
          ref={scrollAreaRef}
        >
          {docSizePx() && (
            <div
              class="relative mx-auto outline-none"
              style={{
                width: `${docSizePx()![0]}px`,
                height: `${docSizePx()![1]}px`,
                cursor: cursorType(),
              }}
              oncapture:mousedown={(e: MouseEvent) => {
                vclMouse.handleMouseDown(props.doc, e);
              }}
              oncapture:mouseup={(e: MouseEvent) => {
                vclMouse.handleMouseUp(props.doc, e);
              }}
              oncapture:mousemove={(e: MouseEvent) => {
                // Seems like on firefox the mousemove event object gets recycled
                // We can't pass a ref of the event to frameThrottle because it 
                // might get mutated / cleaned up by the time the callback gets invoked
                // Instead copying over what we need instead of doing a full clone
                const partialEvent: vclMouse.PartialMouseEvent = {
                      buttons: e.buttons,
                      offsetX: e.offsetX,
                      offsetY: e.offsetY,
                      shiftKey: e.shiftKey,
                      altKey: e.altKey,
                      metaKey: e.metaKey,
                      ctrlKey: e.ctrlKey,
                }
                const callback = frameThrottle((evt) => {
                    vclMouse.handleMouseMove(props.doc, evt);
                })

                callback(partialEvent);
              }}
              oncontextmenu={(e) => {
                e.preventDefault();
              }}
              onFocus={() => setFocused(true)}
              onBlur={() => setFocused(false)}
              onKeyDown={keyhandler().handleKeyEvent}
              onKeyUp={keyhandler().handleKeyEvent}
              tabIndex={-1}
            >
              {containerHeight() && (
                <canvas
                  ref={setCanvas}
                  class="sticky top-0 pointer-events-none"
                  style={{
                    // TODO: object-fit should dynamically set while zooming to use fill so we get "zooming" for free until the render actually finishes
                    'object-fit': 'none',
                    'object-position': 'top left',
                    width: `${docSizePx()![0]}px`,
                    'max-height': `${containerHeight()!}px`,
                  }}
                />
              )}
              <Cursor doc={props.doc} class="bg-sky-300" />
              <Selection doc={props.doc} />
              <For each={rectsPx()}>
                {(rect) => (
                  <div
                    class="border border-zinc-200 mx-auto absolute top-0 left-0 pointer-events-none"
                    style={{
                      transform: `translate(${rect.x}px, ${rect.y}px)`,
                      width: `${rect.width + BORDER_WIDTH * 2}px`,
                      height: `${rect.height + BORDER_WIDTH * 2}px`,
                    }}
                  ></div>
                )}
              </For>
            </div>
          )}
        </ScrollArea>
      </div>
    </>
  );
}
