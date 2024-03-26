import { run } from "@lok/fccache";
function downloadFile({
  name,
  buffer,
}: {
  name: string;
  buffer: ArrayBuffer;
}): void {
  const blob = new Blob([buffer], { type: "application/octet-stream" });
  const url = window.URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = name.split('/').at(-1)!;
  document.body.appendChild(a);
  a.click();
  window.URL.revokeObjectURL(url);
  document.body.removeChild(a);
}

window.run = async () => {
  const files = await run();
  files.forEach(downloadFile);
  return files.length;
};
