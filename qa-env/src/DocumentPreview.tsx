import { TILE_DIM_PX, twipsToCssPx } from '@lok';
import {
  DocumentClient,
  RectanglePx,
  RectangleTwips,
  ViewId,
} from '@lok/shared';
import { createEffect, createSignal, For, JSX, onCleanup } from 'solid-js';
import { ScrollArea } from './OfficeDocument/ScrollArea';
import { frameThrottle } from './OfficeDocument/frameThrottle';
//@ts-ignore
import { observedSize } from './OfficeDocument/OfficeDocument';
import { getOrCreateDPISignal } from './OfficeDocument/twipConversion';

interface Props extends Omit<JSX.HTMLAttributes<HTMLDivElement>, 'onScroll'> {
  doc: DocumentClient;
  mainViewId: ViewId;
}

const DEFAULT_AND_MAX_PAGES_PANE_ZOOM = 0.25;
const BORDER_WIDTH = 1;

function calcCanvasHeight(heightPx: number | undefined) {
  return heightPx
    ? (Math.ceil(heightPx / TILE_DIM_PX) + 1) * TILE_DIM_PX
    : undefined;
}

type Dimensions = [number, number];

const didInitialRender = new WeakSet<DocumentClient>();
export function DocumentPreview(props: Props) {
  let scrollAreaRef: HTMLDivElement | undefined;
  const [canvas0, setCanvas0] = createSignal<HTMLCanvasElement | undefined>();
  const [canvas1, setCanvas1] = createSignal<HTMLCanvasElement | undefined>();
  const [containerHeight, _setContainerHeight] = createSignal<
    number | undefined
  >();
  const [docSizeTwips, setDocSizeTwips] = createSignal<
    Dimensions | undefined
  >();

  const canvasHeight = () => {
    return containerHeight() ? calcCanvasHeight(containerHeight()) : undefined;
  };

  const [rectsTwips, setRectsTwips] = createSignal<RectanglePx[] | undefined>();
  let activeCanvas = 0;
  createEffect(async () => {
    setDocSizeTwips(await props.doc.documentSize());
    setRectsTwips(await props.doc.partRectanglesTwips());
  });

  const docSizePx = () => {
    return docSizeTwips()?.map((i) =>
      twipsToCssPx(i, DEFAULT_AND_MAX_PAGES_PANE_ZOOM)
    );
  };

  createEffect(async () => {
    if (didInitialRender.has(props.doc)) return;
    const width = docSizePx()?.[0];
    const height = canvasHeight();
    const dpi = getOrCreateDPISignal();
    const canvas0_ = canvas0();
    const canvas1_ = canvas1();
    if (!width || !height || !canvas0_ || !canvas1_ || !props.doc) return;
    canvas0_.width = width;
    canvas0_.height = height;
    canvas1_.width = width;
    canvas1_.height = height;

    props.doc.startRenderingPreview(
      [
        canvas0_.transferControlToOffscreen(),
        canvas1_.transferControlToOffscreen(),
      ],
      props.mainViewId,
      256,
      0.25,
      dpi(),
      0
    );

    didInitialRender.add(props.doc);
  });

  function scaleRectCssPx(rect: RectangleTwips, zoom: number): RectanglePx {
    return {
      x: twipsToCssPx(rect.x, zoom),
      y: twipsToCssPx(rect.y, zoom),
      height: twipsToCssPx(rect.height, zoom),
      width: twipsToCssPx(rect.width, zoom),
    };
  }

  const rectsPx = () => {
    return rectsTwips()
      ?.map((rect) => scaleRectCssPx(rect, DEFAULT_AND_MAX_PAGES_PANE_ZOOM))
      .filter((rect) => rect.width && rect.height);
  };

  onCleanup(() => {
    props.doc.stopRenderingPreview();
    didInitialRender.delete(props.doc);
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
        class="w-full flex flex-1 justify-center relative overflow-hidden h-full"
        use:observedSize={[props.doc, _setContainerHeight]}
      >
        {docSizePx() && canvasHeight && (
          <>
            <canvas
              ref={setCanvas0}
              class="absolute top-0 pointer-events-none"
              style={{
                'object-position': 'top center',
                'transform-origin': 'top center',
                width: `${docSizePx()![0]}px`,
                height: `${canvasHeight()!}px`,
              }}
            />
            <canvas
              ref={setCanvas1}
              class="absolute top-0 pointer-events-none"
              style={{
                'object-position': 'top center',
                'transform-origin': 'top center',
                width: `${docSizePx()![0]}px`,
                height: `${canvasHeight()}px`,
                display: 'none',
              }}
            />
          </>
        )}
      </div>
      <ScrollArea
        onScroll={handleScroll}
        ref={scrollAreaRef}
        class="absolute left-0 top-0"
        noPadding={true}
      >
        {docSizePx() && (
          <div
            class="relative mx-auto outline-none"
            style={{
              width: `${docSizePx()![0]}px`,
              height: `${docSizePx()![1]}px`,
            }}
          >
            <For each={rectsPx()}>
              {(rect, idx) => (
                <div
                  class="border border-zinc-200 mx-auto absolute top-0 left-0 hover:border-blue-300 hover:cursor-pointer"
                  style={{
                    transform: `translate(${rect.x + 8}px, ${rect.y - 1}px)`,
                    width: `${rect.width + BORDER_WIDTH * 2}px`,
                    height: `${rect.height + BORDER_WIDTH * 2}px`,
                  }}
                >
                  <div class="absolute right-2 bottom-2 select-none">
                    <p class="text-gray-500 text-2xl drop-shadow-sm">
                      {idx() + 1}
                    </p>
                  </div>
                </div>
              )}
            </For>
          </div>
        )}
      </ScrollArea>
    </>
  );
}
