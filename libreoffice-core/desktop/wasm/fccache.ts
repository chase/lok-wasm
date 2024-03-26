type RunResult = Array<{ name: string; buffer: ArrayBuffer }>;
export function run() {
  return new Promise<RunResult>((resolve) => {
    const worker = new Worker(new URL('./fccache_worker.js', import.meta.url), {
      type: 'module',
    });
    worker.onerror = () => {
      resolve([]);
    };
    worker.onmessage = ({
      data,
    }: MessageEvent<{
      files: RunResult;
    }>) => {
      resolve(data.files);
    };
  });
}
