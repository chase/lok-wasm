import JsZip from "jszip";

export function downloadFile(name: string, buffer: ArrayBuffer, type: string) {
  const a = document.createElement('a');
  const blob = new Blob([buffer], {type: type})
  const url = URL.createObjectURL(blob);
  a.href = url;
  a.download = name;
  document.body.appendChild(a);
  a.click();
  setTimeout(function () {
    document.body.removeChild(a);
    window.URL.revokeObjectURL(url);
  }, 0);
}

export async function unzipDocxFile(blob: Blob): Promise<Array<{path: string, content: ArrayBuffer}>> {
  const zip = await JsZip.loadAsync(blob);

  const files = Promise.all(Object.keys(zip.files).map(async (key) => {
    const file = zip.files[key];
    return {
      path: key,
      content: new Uint8Array(await file.async('uint8array')).buffer,
    }
  }));

  return files;
}

export async function zipDocxFile(parts: Array<{path: string, content: ArrayBuffer}>): Promise<Blob> {
  const zip = new JsZip();

  for (let part of parts) {
    zip.file(part.path, part.content);
  }

  return await zip.generateAsync({type: 'blob'});
}
