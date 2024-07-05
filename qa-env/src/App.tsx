import { loadDocument, loadDocumentFromExpandedParts } from '@lok';
import { CallbackType } from '@lok/lok_enums';
import type { DocumentClient } from '@lok/shared';
import { Show, createSignal, onCleanup } from 'solid-js';
import './App.css';
import { OfficeDocument } from './OfficeDocument/OfficeDocument';
import { cleanup } from './OfficeDocument/cleanup';
import { IS_MAC } from './OfficeDocument/isMac';
import { Shortcut } from './OfficeDocument/vclKeys';
import { ZOOM_STEP, updateZoom } from './OfficeDocument/zoom';
import { downloadFile, unzipDocxFile } from './utils';

const [loading, setLoading] = createSignal(false);
const [getDoc, setDoc] = createSignal<DocumentClient | null>(null);
const getDocThrows = () => {
  const doc = getDoc();
  if (!doc) throw new Error('no doc');
  return doc;
};

declare global {
  interface Window {
    // global for debugging
    d: DocumentClient | undefined;
    saveAsPDF(doc: DocumentClient | null): void;
  }
}

async function unzipLoadDoc(name: string, blob: Blob) {
  const parts = await unzipDocxFile(blob);
  return await loadDocumentFromExpandedParts(
    name,
    parts,
    (document.getElementById('readonly') as HTMLInputElement).checked
  );
}

let loadDocFunc = unzipLoadDoc;

async function fileOpen(files: FileList | null) {
  if (!files || !files[0]) return;
  const name = files[0].name;
  const blob = files[0].slice();
  setLoading(true);
  const oldDoc = getDoc();
  if (oldDoc) {
    cleanup(oldDoc);
  }
  const doc = await loadDocFunc(name, blob);
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
  doc.afterIdle(() => {
    console.log('did idle');
  });
  doc.afterPaint(() => {
    console.log('did paint');
  });
  window.d = doc;
}

window.saveAsPDF = async function saveAsPDF(doc: DocumentClient | null) {
  if (!doc) return;
  const buffer = await doc.save('pdf');
  downloadFile('Pdf Export.pdf', buffer, 'application/pdf');
};

const MOD = IS_MAC ? 'cmd' : 'ctrl';
const ignoredShortcuts: Shortcut[] = [
  {
    key: '=',
    modifiers: [MOD],
  },
  {
    key: '-',
    modifiers: [MOD],
  },
];

let zoomTimeout: ReturnType<typeof setTimeout>;

function registerGlobalKeys() {
  async function callback(e: KeyboardEvent) {
    if (IS_MAC ? !e.metaKey : !e.ctrlKey) return;
    switch (e.key) {
      case '+':
      case '=':
        e.preventDefault();
        if (zoomTimeout) clearTimeout(zoomTimeout);
        zoomTimeout = setTimeout(() => updateZoom(getDocThrows, ZOOM_STEP));
        break;
      case '_':
      case '-':
        e.preventDefault();
        if (zoomTimeout) clearTimeout(zoomTimeout);
        zoomTimeout = setTimeout(() => updateZoom(getDocThrows, -ZOOM_STEP));
        break;
    }
  }

  async function wheelCallback(e: WheelEvent) {
    if (!e.ctrlKey) return;

    e.preventDefault();
    e.stopPropagation();

    // Scroll Up = Zoom In
    if (e.deltaY < 0) {
      if (zoomTimeout) clearTimeout(zoomTimeout);
      zoomTimeout = setTimeout(() => updateZoom(getDocThrows, ZOOM_STEP));
      // Scroll Down = Zoom In
    } else if (e.deltaY > 0) {
      if (zoomTimeout) clearTimeout(zoomTimeout);
      zoomTimeout = setTimeout(() => updateZoom(getDocThrows, -ZOOM_STEP));
    }
  }

  document.addEventListener('keydown', callback);
  // By default wheel events are passive and annot be prevented
  document.addEventListener('wheel', wheelCallback, { passive: false });

  onCleanup(() => {
    document.removeEventListener('keydown', callback);
    document.removeEventListener('wheel', wheelCallback);
  });
}

function App() {
  registerGlobalKeys();

  return (
    <>
      <div class="flex bg-white w-full p-2 pl-3 gap-3 items-center border-solid border-b border-slate-300">
        <label>
          <input
            class="h-4 w-4"
            type="checkbox"
            checked
            onChange={(evt) =>
              (loadDocFunc = evt.target.checked ? unzipLoadDoc : loadDocument)
            }
          />{' '}
          Expanded
        </label>
        <label>
          <input class="h-4 w-4" id="readonly" type="checkbox" /> Read Only
        </label>
        <input
          type="file"
          accept=".docx"
          class="block flex-1 text-sm text-slate-400 rounded-md
        file:mr-4 file:py-2 file:px-4 file:rounded-md
        file:border-0 file:text-sm file:font-semibold
        file:bg-blue-50 file:text-blue-700
        hover:file:bg-blue-100 h-auto"
          onChange={(evt) => fileOpen(evt.target.files)}
        />
      </div>
      <Show when={loading()}>
        <div class="loadsection">
          <span class="loader" />
        </div>
      </Show>
      <Show when={getDoc()} keyed>
        <OfficeDocument doc={getDoc()!} ignoreShortcuts={ignoredShortcuts} />
      </Show>
    </>
  );
}

export default App;
