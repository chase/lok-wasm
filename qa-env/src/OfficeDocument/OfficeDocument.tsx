import { TILE_DIM_PX, twipsToCssPx } from '@lok';
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
import { frameThrottle } from './frameThrottle';
import { createDocEventSignal } from './docEventSignal';
import { Shortcut, createKeyHandler } from './vclKeys';
import { getOrCreateFocusedSignal } from './focus';
import { getOrCreateZoomSignal } from './zoom';
import { getOrCreateDPISignal } from './twipConversion';

const OBSERVED_SIZE_DEBOUNCE = 100; //ms

const BORDER_WIDTH = 1;

interface Props extends Omit<JSX.HTMLAttributes<HTMLDivElement>, 'onScroll'> {
  doc: DocumentClient;
  scrollAreaRef?: (ref: HTMLDivElement) => void;
  ignoreShortcuts?: Shortcut[];
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

function calcCanvasHeight(heightPx: number | undefined) {
  return heightPx
    ? (Math.ceil(heightPx / TILE_DIM_PX) + 1) * TILE_DIM_PX
    : undefined;
}

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
    const canvasHeight = calcCanvasHeight(height);
    if (debounce) clearTimeout(debounce);
    debounce = setTimeout(async () => {
      setHeight(height);
      await doc.setVisibleHeight(canvasHeight!);
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

function scaleRectCssPx(rect: RectangleTwips, zoom: number): RectanglePx {
  return {
    x: twipsToCssPx(rect.x, zoom),
    y: twipsToCssPx(rect.y, zoom),
    height: twipsToCssPx(rect.height, zoom),
    width: twipsToCssPx(rect.width, zoom),
  }
}

const didInitialRender = new WeakSet<DocumentClient>();

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
  const [canvas0, setCanvas0] = createSignal<HTMLCanvasElement | undefined>();
  const [canvas1, setCanvas1] = createSignal<HTMLCanvasElement | undefined>();
  let activeCanvas = 0;

  const cursorType = createDocEventSignal(
    () => props.doc,
    CallbackType.MOUSE_POINTER
  );
  const canvasHeight = () =>
    containerHeight() ? calcCanvasHeight(containerHeight()) : undefined;

  createEffect(async () => {
    setDocSizeTwips(await props.doc.documentSize());
    setRectsTwips(
      await props.doc.partRectanglesTwips()
    );
  });

  const docSizePx = () => {
    const [getZoom] = getOrCreateZoomSignal(() => props.doc);
    const zoom = getZoom();
    return docSizeTwips()?.map((i) => twipsToCssPx(i, zoom));
  }
  const rectsPx = () => {
    const [getZoom] = getOrCreateZoomSignal(() => props.doc);
    const zoom = getZoom();
    return rectsTwips()
      ?.map(rect => scaleRectCssPx(rect, zoom))
      .filter((rect) => rect.width && rect.height);
  }

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
    const height = canvasHeight();
    if (height) props.doc.setVisibleHeight(height);
  });

  createEffect(async () => {
    if (didInitialRender.has(props.doc)) return;
    const width = docSizePx()?.[0];
    const height = canvasHeight();
    const canvas0_ = canvas0();
    const canvas1_ = canvas1();
    if (!width || !height || !canvas0_ || !canvas1_ || !props.doc)
      return;
    const [getZoom] = getOrCreateZoomSignal(() => props.doc);
    const zoom = getZoom();
    const getDpi = getOrCreateDPISignal();
    const dpi = getDpi();
    const scaledWidth = Math.floor(width * dpi);
    const scaledHeight = Math.floor(height * dpi);
    canvas0_.width = scaledWidth;
    canvas0_.height = scaledHeight;
    canvas1_.width = scaledWidth;
    canvas1_.height = scaledHeight;
    await props.doc.startRendering(
      [
        canvas0_.transferControlToOffscreen(),
        canvas1_.transferControlToOffscreen(),
      ],
      256,
      zoom,
      dpi
    );
    didInitialRender.add(props.doc);
  });

  const [focused, setFocused] = getOrCreateFocusedSignal(() => props.doc);

  const keyhandler = createMemo(() => {
    const result = createKeyHandler(() => props.doc, focused);
    if (props.ignoreShortcuts) {
      result.ignoreShortcuts(props.ignoreShortcuts);
    }
    return result;
  });

  const handleScroll = frameThrottle(async (yPx, xPx) => {
    handleScroll.cancel();
    const c0 = canvas0();
    const c1 = canvas1();
    if (!c0 || !c1) return;
    const previousCanvas = activeCanvas;
    activeCanvas = await props.doc.setScrollTop(yPx);
    const c = activeCanvas === 0 ? c0 : c1;
    c.style.willChange = 'transform';
    c.style.transform = `translate3d(-${xPx}px, -${Math.floor(yPx) % TILE_DIM_PX}px, 0)`;
    c.style.willChange = '';
    if (previousCanvas !== activeCanvas) {
      c.style.display = '';
      const priorC = previousCanvas === 0 ? c0 : c1;
      priorC.style.display = 'none';
    }
  });

  return (
    <>
      <div
        class="flex flex-1 justify-center relative overflow-hidden"
        use:observedSize={[props.doc, setContainerHeight]}
      >
        {docSizePx() && canvasHeight() && (
          <>
            <canvas
              ref={setCanvas0}
              class="absolute top-0 pointer-events-none"
              style={{
                // TODO: object-fit should dynamically set while zooming to use fill so we get "zooming" for free until the render actually finishes
                // 'object-fit': 'cover',
                'object-position': 'top left',
                'transform-origin': 'top center',
                width: `${docSizePx()![0]}px`,
                height: `${canvasHeight()!}px`,
              }}
            />
            <canvas
              ref={setCanvas1}
              class="absolute top-0 pointer-events-none"
              style={{
                // TODO: object-fit should dynamically set while zooming to use fill so we get "zooming" for free until the render actually finishes
                'object-fit': 'cover',
                'object-position': 'top left',
                'transform-origin': 'top center',
                width: `${docSizePx()![0]}px`,
                height: `${canvasHeight()}px`,
                display: 'none',
              }}
            />
          </>
        )}
        <ScrollArea
          class="absolute top-0"
          onScroll={handleScroll}
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
              oncapture:mousemove={frameThrottle((e: MouseEvent) => {
                vclMouse.handleMouseMove(props.doc, e);
              })}
              oncontextmenu={(e) => {
                e.preventDefault();
              }}
              onFocus={() => setFocused(true)}
              onBlur={() => setFocused(false)}
              onKeyDown={keyhandler().handleKeyEvent}
              onKeyUp={keyhandler().handleKeyEvent}
              tabIndex={-1}
            >
              <Cursor doc={props.doc} class="bg-sky-300" />
              <Selection doc={props.doc} />
              <For each={rectsPx()}>
                {(rect) => (
                  <div
                    class="border border-zinc-200 mx-auto absolute top-0 left-0 pointer-events-none"
                    style={{
                      transform: `translate(${rect.x}px, ${rect.y - 1}px)`,
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
