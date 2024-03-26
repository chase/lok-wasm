import { CallbackType } from './lok_enums';
import {
  WorkerCallback,
  DocumentClient,
  DocumentRef,
  FromWorker,
  Id,
  Message,
  Ref,
  ToWorker,
  CallbackHandler,
  DocumentMethods,
  ViewId,
  DocumentClientBase,
  DocumentWithViewMethods,
} from './shared';

/** rendered tile size in pixels */
const TILE_DIM_PX = 256;
/** 15 = 1440 twips-per-inch / 96 dpi */
const LOK_INTERNAL_TWIPS_TO_PX = 15;

/** pulled from tile_renderer_worker
TODO: should probably be split into a separate file */
function clipToNearest8PxZoom(w: number, s: number): number {
  const scaledWidth: number = Math.ceil(w * s);
  const mod: number = scaledWidth % 8;
  if (mod === 0) return s;

  return Math.abs((scaledWidth - mod) / w - s) <
    Math.abs((scaledWidth + 8 - mod) / w - s)
    ? (scaledWidth - mod) / w
    : (scaledWidth + 8 - mod) / w;
}

/** Calculates the conversions for twips provided a `zoom` level and `dpi` */
export function conversionTable(zoom: number, dpi: number) {
  const scale = clipToNearest8PxZoom(TILE_DIM_PX, zoom * dpi);
  const TILE_DIM_TWIPS = Math.floor(
    TILE_DIM_PX * LOK_INTERNAL_TWIPS_TO_PX * scale
  );
  return {
    zoom,
    dpi,
    scale,
    twipsToPx: TILE_DIM_PX / TILE_DIM_TWIPS,
    pxToTwips: TILE_DIM_TWIPS / TILE_DIM_PX,
  };
}

/** CSS pixels are DPI indepdendent */
export function twipsToCssPx(twips: number, zoom: number) {
  return Math.ceil(twips / zoom / LOK_INTERNAL_TWIPS_TO_PX);
}

/** CSS pixels are DPI indepdendent */
export function cssPxToTwips(twips: number, zoom: number) {
  return Math.ceil(twips * zoom * LOK_INTERNAL_TWIPS_TO_PX);
}

const worker: Ref<Worker> = {};

const counter: Ref<Id> = { current: 0 };

class Future<T> {
  _settled = false;
  promise: Promise<T>;
  resolve: (value: T | PromiseLike<T>) => void;

  constructor() {
    this.promise = new Promise<T>((resolve) => {
      this.resolve = resolve;
    });
  }
}

const UNUSED_ID: Id = -1;

const callIdToFuture: Record<Id, Future<any>> = {};
type CallbackHandlers = {
  -readonly [K in keyof typeof CallbackType]?: Set<CallbackHandler>;
};
const subscribedEvents: { [K: number]: CallbackHandlers } = {};

function messageIsCallback<K extends keyof Message = keyof Message>(
  data: FromWorker<K> | WorkerCallback
): data is WorkerCallback {
  return data.f === 'c';
}

async function handleMessage<K extends keyof Message = keyof Message>({
  data,
}: MessageEvent<FromWorker<K> | WorkerCallback>) {
  if (messageIsCallback(data)) {
    subscribedEvents[data.d]?.[data.t]?.forEach((handler) => handler(data.p));
  } else if (data.i !== UNUSED_ID) {
    callIdToFuture[data.i].resolve(data.r);
    callIdToFuture[data.i] = undefined;
  }
}

function loadWorkerOnce() {
  if (worker.current) {
    return worker.current;
  }

  worker.current = new Worker(new URL('./worker.js', import.meta.url), {
    type: 'module',
  });
  worker.current.onmessage = handleMessage;

  return worker.current;
}

function registerFuture<T>(): [Id, Future<T>] {
  // technically, this could hit Number.SAFE_INTEGER_MAX and overflow around but that means 2**53-1 calls
  counter.current = Math.max(0, (counter.current + 1) | 0);
  const i = ++counter.current;
  callIdToFuture[i] = new Future<T>();
  return [i, callIdToFuture[i]];
}

const WITH_VIEW_ID: Record<
  keyof DocumentWithViewMethods | (string & {}),
  true
> = {
  postKeyEvent: true,
  postMouseEvent: true,
  postWindowExtTextInputEvent: true,
  postTextInput: true,
  setTextSelection: true,
  setClipboard: true,
  getClipboard: true,
  paste: true,
  setGraphicSelection: true,
  resetSelection: true,
  getCommandValues: true,
  subscribe: true,
  unsubscribe: true,
  dispatchCommand: true,
  removeText: true,
  setClientVisibleArea: true,
  startRendering: true,
  setScrollTop: true,
  setVisibleHeight: true,
  resetRendering: true,
  stopRendering: true,
};

const workerProxyHandler: ProxyHandler<{ ref: DocumentRef; viewId: ViewId }> = {
  get(
    target,
    prop: keyof DocumentMethods | keyof DocumentWithViewMethods | (string & {})
  ) {
    if (prop in target) return target[prop];
    // `then` is excluded to avoid trying to resolve the client as a Promise
    if (prop !== 'then' && typeof prop === 'string') {
      target[prop] = function (...args: any[]) {
        const [i, future] = registerFuture();
        let transfers: { transfer: Transferable[] } | undefined;
        if (prop === 'startRendering' || prop === 'resetRendering') {
          transfers = {
            transfer: [
              (
                args as
                  | Parameters<DocumentWithViewMethods['startRendering']>
                  | Parameters<DocumentWithViewMethods['resetRendering']>
              )[0],
            ],
          };
        }
        loadWorkerOnce().postMessage(
          {
            f: prop,
            i,
            a: WITH_VIEW_ID[prop]
              ? [target.ref, target.viewId, ...args]
              : [target.ref, ...args],
          } as ToWorker,
          transfers
        );
        // automatically assign the view ID after intializing for rendering so that rendering calls have a valid target
        if (prop === 'initializeForRendering') {
          future.promise.then(
            (viewId: FromWorker<'initializeForRendering'>['r']) => {
              target.viewId = viewId;
            }
          );
        }
        return future.promise;
      };
      return target[prop];
    }
    return undefined;
  },
};

function documentClient<T extends DocumentClient>(
  ref: DocumentRef | null
): T | null {
  if (!ref) return null;
  const clientObject: DocumentClientBase = {
    ref,
    // this is set after the first initializeForRendering call
    viewId: -1,
    on(type: CallbackType, handler: CallbackHandler) {
      if (subscribedEvents[ref] == null)
        subscribedEvents[ref] = { [type]: new Set() };
      if (subscribedEvents[ref][type] == null)
        subscribedEvents[ref][type] = new Set();
      const subscribedEvent = subscribedEvents[ref][type];
      if (subscribedEvent.size === 0) {
        loadWorkerOnce().postMessage({
          f: 'subscribe',
          i: UNUSED_ID,
          a: [ref, this.viewId ?? -1, type],
        } as ToWorker);
      }
      subscribedEvent.add(handler);
    },
    off(type: CallbackType, handler: CallbackHandler) {
      const subscribedEvent = subscribedEvents[ref]?.[type];
      if (subscribedEvent != null) return;

      subscribedEvent.delete(handler);
      if (subscribedEvent.size === 0) {
        loadWorkerOnce().postMessage({
          f: 'unsubscribe',
          i: UNUSED_ID,
          a: [ref, this.viewId ?? -1, type],
        } as ToWorker);
      }
    },
    newView(): DocumentClient {
      // TODO: implement this
      throw new Error('Function not implemented.');
    },
  };
  return new Proxy(clientObject, workerProxyHandler) as T;
}

export async function loadDocument<T extends DocumentClient = DocumentClient>(
  name: string,
  blob: Blob
): Promise<T | null> {
  const [i, future] = registerFuture<DocumentRef | null>();
  const message: ToWorker = {
    f: 'load',
    i,
    a: [name, blob],
  };
  loadWorkerOnce().postMessage(message);
  return future.promise.then(documentClient<T>);
}

export async function loadDocumentFromArrayBuffer<
  T extends DocumentClient = DocumentClient,
>(
  name: string,
  arrayBuffer: ArrayBufferLike,
  type: string = 'application/octet-stream'
): Promise<DocumentClient | null> {
  // TODO: this might be slower than transfering the ArrayBuffer to the worker and setting it there?
  return loadDocument<T>(name, new Blob([arrayBuffer], { type }));
}

/** imports a script at `url` that registers an extension inside of the worker */
export async function importScript(url: string) {
  const [i, future] = registerFuture<DocumentRef | null>();
  const message: ToWorker = {
    f: 'importScript',
    i,
    a: [url],
  };
  loadWorkerOnce().postMessage(message);
  return future.promise.then(documentClient);
}

export async function preload() {
  const message: ToWorker = {
    f: 'preload',
    i: UNUSED_ID,
    a: [],
  };
  loadWorkerOnce().postMessage(message);
}

declare global {
  interface ImportMeta {
    readonly hot?: {
      readonly data: Partial<{
        worker: Worker;
        counter: Id;
      }>;

      accept(): void;
      dispose(cb: (data: any) => void): void;
    };
  }
}

if (import.meta.hot) {
  if (import.meta.hot.data.worker) {
    worker.current = import.meta.hot.data.worker;
  }
  if (import.meta.hot.data.counter) {
    counter.current = import.meta.hot.data.counter;
  }
  import.meta.hot.accept();
  import.meta.hot.dispose(() => {
    import.meta.hot.data.worker = worker.current;
    import.meta.hot.data.counter = counter.current;
  });
}
