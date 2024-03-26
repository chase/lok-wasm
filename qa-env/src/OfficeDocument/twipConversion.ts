import { conversionTable } from '@lok';
import { Accessor, createMemo, createSignal, onCleanup } from 'solid-js';

let cleanupDPIQuery: () => void | undefined;
function createDPIQuery() {
  const [signal, setSignal] = createSignal(window.devicePixelRatio);
  const updateDPIWatch = () => {
    window.devicePixelRatio;
    let dpiMediaQuery = matchMedia(
      `(resolution: ${window.devicePixelRatio}dppx)`
    );
    dpiMediaQuery.addEventListener('change', updateDPIWatch);
    cleanupDPIQuery = () => {
      dpiMediaQuery.removeEventListener('change', updateDPIWatch);
    };
    setSignal(window.devicePixelRatio);
  };

  updateDPIWatch();

  onCleanup(() => {
    cleanupDPIQuery?.();
  });
  return signal;
}

let lazyDPISignal: Accessor<number>;
export function getOrCreateDPISignal() {
  if (lazyDPISignal == null) {
    lazyDPISignal = createDPIQuery();
  }
  return lazyDPISignal;
}

export type ConversionTable = ReturnType<typeof conversionTable>;

const PRECISION = 0.0001;

export function createConversionTable(
  zoom: Accessor<number>
): Accessor<ConversionTable> {
  const dpi = getOrCreateDPISignal();
  return createMemo(
    (prev: ConversionTable) => {
      const newTable = conversionTable(zoom(), dpi());
      if (Math.abs(prev.scale - newTable.scale) > PRECISION) {
        return newTable;
      }
      return prev;
    },
    conversionTable(zoom(), dpi())
  );
}
