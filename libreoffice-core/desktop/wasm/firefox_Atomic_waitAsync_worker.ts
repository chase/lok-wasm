// TODO: Drop the pollyfill when Firefox supports it: https://bugzilla.mozilla.org/show_bug.cgi?id=1884148
onmessage = ({ data }: { data: [Int32Array, number, Int32Array] }) => {
  if (Atomics.load(data[2], 0) === 4 /** quit */) self.close();
  Atomics.wait(data[0], 0, data[1]);
  postMessage(null);
  if (Atomics.load(data[2], 0) === 4 /** quit */) self.close();
};
