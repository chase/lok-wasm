import { DocumentClient } from '@lok/shared';

const cleanupMap = new WeakMap<
  DocumentClient,
  Array<(doc: DocumentClient) => void>
>();

export function registerCleanup(
  doc: DocumentClient,
  fn: (doc: DocumentClient) => void
) {
  let registry = cleanupMap.get(doc);
  if (registry == null) {
    registry = [];
    cleanupMap.set(doc, registry);
  }
  registry.push(fn);
}

export function cleanup(doc: DocumentClient) {
  const registry = cleanupMap.get(doc);
  if (registry == null) {
    return;
  }
  for (const action of registry) {
    action(doc);
  }
}
