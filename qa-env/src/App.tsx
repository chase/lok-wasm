import { loadDocument, loadDocumentFromExpandedParts } from '@lok';
import type { DocumentClient } from '@lok/shared';
import './App.css';
import { Show, createSignal } from 'solid-js';
import { CallbackType } from '@lok/lok_enums';
import { OfficeDocument } from './OfficeDocument/OfficeDocument';
import { cleanup } from './OfficeDocument/cleanup';
import { downloadFile } from './utils';

const [loading, setLoading] = createSignal(false);
const [getDoc, setDoc] = createSignal<DocumentClient | null>(null);

async function fileOpen(files: FileList | null) {
  if (!files || !files[0]) return;
  const name = files[0].name;
  const blob = files[0].slice();
  const type = files[0].type;

  setLoading(true);
  const oldDoc = getDoc();
  if (oldDoc) {
    cleanup(oldDoc);
  }
  let doc;
  if (type !== "application/vnd.openxmlformats-officedocument.wordprocessingml.document"){
    const decoder = new TextDecoder();
    let content = decoder.decode(await blob.arrayBuffer());
    console.log(content)
    let parts = JSON.parse(content) as Array<{path: string, content: string}>;

    doc = await loadDocumentFromExpandedParts(parts)

  } else {
    doc = await loadDocument(name, blob);
  }

  if (!doc) {
    console.error('failure!');
    return;
  }
  await doc.initializeForRendering({
    author: 'Macro User',
  });
  setDoc(doc);
  setLoading(false);
  doc.on(CallbackType.ERROR, console.error);
  doc.on(CallbackType.TEXT_SELECTION, console.log);
  // doc.on(CallbackType.STATE_CHANGED, (payload) => console.log(payload));
}
async function saveAsPDF(doc: DocumentClient | null) {
  if (!doc) return;
  const buffer = await doc.save("pdf")
  downloadFile("Pdf Export.pdf", buffer, "application/pdf");
};

function App() {
  return (
    <>
      <div class="flex bg-white w-full p-2 rounded-md">
        <input
          type="file"
          accept=".docx"
          class="block w-full text-sm text-slate-500 rounded-md
        file:mr-4 file:py-2 file:px-4 file:rounded-md
        file:border-0 file:text-sm file:font-semibold
        file:bg-blue-50 file:text-blue-700
        hover:file:bg-blue-100 h-auto"
          onChange={(evt) => fileOpen(evt.target.files)}
        />
        <input
          type="file"
          accept=".json"
          class="block w-full text-sm text-slate-500 rounded-md
        file:mr-4 file:py-2 file:px-4 file:rounded-md
        file:border-0 file:text-sm file:font-semibold
        file:bg-blue-50 file:text-blue-700
        hover:file:bg-blue-100 h-auto"
          onChange={(evt) => fileOpen(evt.target.files)}
        />
      </div>
      <Show when={getDoc()}>
        <div class = "h-[70px] border-b border border-gray-300 flex items-center bg-gray-200 px-2">
          <button onClick={() => saveAsPDF(getDoc())}>
            Save As PDF
          </button>
        </div>
      </Show>
      <Show when={loading()}>
        <div class="loadsection">
          <span class="loader" />
        </div>
      </Show>
      <Show when={getDoc()} keyed>
        <OfficeDocument doc={getDoc()!} />
      </Show>
    </>
  );
}

export default App;
