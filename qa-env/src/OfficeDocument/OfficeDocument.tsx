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
  on,
} from 'solid-js';
import { CallbackType } from '@lok/lok_enums';
import { ScrollArea } from './ScrollArea';
import { Cursor } from './Cursor';
import { Selection } from './Selection';
import * as vclMouse from './vclMouse';
import { createDocEventSignal } from './docEventSignal';
import { Shortcut, createKeyHandler } from './vclKeys';
import { getOrCreateFocusedSignal } from './focus';
import { frameThrottle } from './frameThrottle';
import { getOrCreateZoomSignal } from './zoom';
import { getOrCreateDPISignal } from './twipConversion';

// These give us good scaling behavior until the render actually finishes
/** Cover on Zoom In, will stretch the image to fit as the canvas size changes */
const ZOOM_IN_CANVAS_FIT = 'cover';
/** Contain on Zoom Out and after Zoom In, squeezes the image to fit as the canvas size changes */
const CANVAS_FIT = 'contain';
const OBSERVED_SIZE_DEBOUNCE = 100; //ms

const BORDER_WIDTH = 1;

interface Props extends Omit<JSX.HTMLAttributes<HTMLDivElement>, 'onScroll'> {
  doc: DocumentClient;
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
  };
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
  let canvasSwitchLock = false;

  const cursorType = createDocEventSignal(
    () => props.doc,
    CallbackType.MOUSE_POINTER
  );
  const canvasHeight = () =>
    containerHeight() ? calcCanvasHeight(containerHeight()) : undefined;

  createEffect(async () => {
    setDocSizeTwips(await props.doc.documentSize());
    setRectsTwips(await props.doc.partRectanglesTwips());
  });

  const docSizePx = () => {
    const [getZoom] = getOrCreateZoomSignal(() => props.doc);
    const zoom = getZoom();
    return docSizeTwips()?.map((i) => twipsToCssPx(i, zoom));
  };

  const rectsPx = () => {
    const [getZoom] = getOrCreateZoomSignal(() => props.doc);
    const zoom = getZoom();
    return rectsTwips()
      ?.map((rect) => scaleRectCssPx(rect, zoom))
      .filter((rect) => rect.width && rect.height);
  };

  createEffect(() => {
    const callback = async () => {
      setDocSizeTwips(await props.doc.documentSize());
      setRectsTwips(await props.doc.partRectanglesTwips());
      // This is required to re-render the document with the new width
      // if it has actually changed, if the width remains the same nothing
      // will happen
      await props.doc.setDocumentWidth(docSizePx()![0]);
    };
    props.doc.on(CallbackType.DOCUMENT_SIZE_CHANGED, callback);
    onCleanup(() => {
      props.doc.off(CallbackType.DOCUMENT_SIZE_CHANGED, callback);
    });
  });

  const [getZoom] = getOrCreateZoomSignal(() => props.doc);

  const didZoomIn = createMemo(
    on(
      getZoom,
      (zoom, lastZoom) => {
        return lastZoom != null && zoom > lastZoom;
      },
      { defer: true }
    ),
    false
  );

  // resets the object fit to 'contain' so that it isn't blurry after the
  // paint for zooming in is finished
  createEffect(() => {
    props.doc.afterIdle(() => {
      if (didZoomIn()) {
        canvas0()!.style.objectFit = CANVAS_FIT;
        canvas1()!.style.objectFit = CANVAS_FIT;
      }
    });
  });

  onCleanup(() => {
    props.doc.stopRendering();
  });

  createEffect(async () => {
    if (didInitialRender.has(props.doc)) return;
    const width = docSizePx()?.[0];
    const height = canvasHeight();
    const canvas0_ = canvas0();
    const canvas1_ = canvas1();
    if (!width || !height || !canvas0_ || !canvas1_ || !props.doc) return;
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
    const canvases = [
      canvas0_.transferControlToOffscreen(),
      canvas1_.transferControlToOffscreen(),
    ];
    didInitialRender.add(props.doc);
    await props.doc.startRendering(canvases, 256, zoom, dpi);
  });

  const [focused, setFocused] = getOrCreateFocusedSignal(() => props.doc);

  const keyhandler = createMemo(() => {
    const result = createKeyHandler(() => props.doc, focused);
    if (props.ignoreShortcuts) {
      result.ignoreShortcuts(props.ignoreShortcuts);
    }
    return result;
  });

  const [scrollPos, setScrollPos] = createSignal<{ x: number; y: number }>();

  /// Make the scroll responsive to the changing zoom level
  /// Only update scroll when zoom has actually changed
  createEffect(
    on(
      getZoom,
      (newZoom, prev) => {
        const pos = scrollPos();
        if (!pos || !scrollAreaRef) return;

        const scalePos = (p: number) => Math.floor((p / (prev || 1)) * newZoom);

        const newX = scalePos(pos.x);
        const newY = scalePos(pos.y);

        scrollAreaRef.scrollLeft = newX;
        scrollAreaRef.scrollTop = newY;

        setScrollPos({ x: newX, y: newY });
      },
      { defer: true }
    )
  );

  const handleScroll = async (yPx: number, xPx: number) => {
    setScrollPos({ x: xPx, y: yPx });
    const c0 = canvas0();
    const c1 = canvas1();
    if (!c0 || !c1) return;
    const previousCanvas = activeCanvas;
    const dpi = getOrCreateDPISignal();
    const scaledTileDim = TILE_DIM_PX / dpi();
    // because this function is async, scroll events have a race condition that can cause flickering
    // if switching the canvas isn't explicitly blocked until an ongoing setScrollTop resolves
    if (canvasSwitchLock) {
      // during the lock, the old active canvas is still displayed until the previous setScrollTop resolves
      const c = activeCanvas === 0 ? c1 : c0;
      c.style.transform = `translate3d(-${xPx}px, -${Math.floor(yPx) % scaledTileDim}px, 0)`;
      return;
    }
    canvasSwitchLock = true;
    activeCanvas = await props.doc.setScrollTop(yPx);
    canvasSwitchLock = false;

    const c = activeCanvas === 0 ? c0 : c1;
    // xPx/yPx is technically in css pixels, but it is referring to the position
    // of the document rendered on the canvas which is in physical pixels
    // so the offset should be scaled down aswell.
    c.style.transform = `translate3d(-${xPx}px, -${Math.floor(yPx) % scaledTileDim}px, 0)`;
    if (previousCanvas !== activeCanvas) {
      const c = activeCanvas === 0 ? c0 : c1;
      c.style.display = 'block';
      const priorC = previousCanvas === 0 ? c0 : c1;
      priorC.style.display = 'none';
    }
  };

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
                'object-fit': didZoomIn() ? ZOOM_IN_CANVAS_FIT : CANVAS_FIT,
                'object-position': 'top center',
                'image-rendering': 'crisp-edges',
                'transform-origin': 'top center',
                width: `${docSizePx()![0]}px`,
                height: `${canvasHeight()!}px`,
                display: 'block',
              }}
            />
            <canvas
              ref={setCanvas1}
              class="absolute top-0 pointer-events-none"
              style={{
                'object-fit': didZoomIn() ? ZOOM_IN_CANVAS_FIT : CANVAS_FIT,
                'object-position': 'top center',
                'image-rendering': 'crisp-edges',
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
                };
                const callback = frameThrottle((evt) => {
                  vclMouse.handleMouseMove(props.doc, evt);
                });

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
