import { CallbackType } from '@lok/lok_enums';
import { DocumentClient } from '@lok/shared';
import { Accessor, createSignal } from 'solid-js';
import { registerCleanup } from './cleanup';
import { frameThrottle } from './frameThrottle';

type PayloadTransform<T> = (payload: string) => Exclude<T, Function>;

export function createDocEventSignal<T>(
  doc: Accessor<DocumentClient>,
  type: CallbackType,
  transform: PayloadTransform<T>,
  skipUndefined?: true
): Accessor<Exclude<T, Function> | undefined>;

export function createDocEventSignal(
  doc: Accessor<DocumentClient>,
  type: CallbackType,
  transform?: undefined,
  skipUndefined?: undefined
): Accessor<string | undefined>;

export function createDocEventSignal<T>(
  doc: Accessor<DocumentClient>,
  type: CallbackType,
  transform?: PayloadTransform<T>,
  skipUndefined?: true
): Accessor<Exclude<T, Function> | string | undefined> {
  const [payload, setPayload] = createSignal<any>();
  const callback = frameThrottle(
    transform
      ? (payload: string) => {
          const transformed = transform(payload);
          if (skipUndefined && transform == null) return;
          setPayload(transformed);
        }
      : setPayload
  );
  doc().on(type, callback);
  registerCleanup(doc(), (doc_) => {
    doc_.off(type, callback);
  });
  return payload;
}

export function createInitalizedDocEventSignal<T>(
  doc: Accessor<DocumentClient>,
  type: CallbackType,
  transform: (payload: string) => T | undefined,
  initial: Exclude<T, Function> | Promise<Exclude<T, Function>>
): Accessor<T> {
  const [payload, setPayload] =
    initial instanceof Promise
      ? createSignal<T>(undefined as T)
      : createSignal<T>(initial);
  if (initial instanceof Promise) {
    initial.then(setPayload);
  }
  const callback = frameThrottle(
    transform
      ? (payload: string) => {
          const transformed = transform(payload);
          if (transformed == null) return;
          // yes, this Exclude is redundant, no TypeScript isn't smart enough to expand this
          setPayload(transformed as Exclude<T, Function>);
        }
      : setPayload
  );
  doc().on(type, callback);
  registerCleanup(doc(), (doc_) => {
    doc_.off(type, callback);
  });
  return payload;
}
