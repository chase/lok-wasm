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
