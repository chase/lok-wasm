import { loadDocument } from '@lok';
import throttle from 'lodash.throttle';
import { CallbackType } from '@lok/lok_enums';
import type { DocumentClient } from '@lok/shared';
import { Show, createSignal, onCleanup } from 'solid-js';
import './App.css';
import { OfficeDocument, setCanvasObjectFit } from './OfficeDocument/OfficeDocument';
import { cleanup } from './OfficeDocument/cleanup';
import { IS_MAC } from './OfficeDocument/isMac';
import { Shortcut } from './OfficeDocument/vclKeys';
import { updateZoom } from './OfficeDocument/zoom';
import { downloadFile } from './utils';

const [loading, setLoading] = createSignal(false);
const [getDoc, setDoc] = createSignal<DocumentClient | null>(null);
const getDocThrows = () => {
  const doc = getDoc();
  if (!doc) throw new Error('no doc');
  return doc;
};

async function fileOpen(files: FileList | null) {
  if (!files || !files[0]) return;
  const name = files[0].name;
  const blob = files[0].slice();
  setLoading(true);
  const oldDoc = getDoc();
  if (oldDoc) {
    cleanup(oldDoc);
  }
  const doc = await loadDocument(name, blob);
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

  // doc.on(CallbackType.STATE_CHANGED, (payload) => console.log(payload));
}
async function saveAsPDF(doc: DocumentClient | null) {
  if (!doc) return;
  const buffer = await doc.save("pdf")
  downloadFile("Pdf Export.pdf", buffer, "application/pdf");
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

/// This gives us good behavior until the render actually finishes
/// Cover on Zoom In, will stretch the image to fit as 
/// the canvas size changes 
const ZOOM_IN_CANVAS_FIT = "cover";
/// This gives us good behavior until the render actually finishes
/// Contain on Zoom Out, squeezes the image to fit as
/// the canvas size changes 
const ZOOM_OUT_CANVAS_FIT = "contain";

const throttledUpdateZoom = throttle(updateZoom, 50);

function registerGlobalKeys() {
  function callback(e: KeyboardEvent) {
    if (IS_MAC ? !e.metaKey : !e.ctrlKey) return;
    switch (e.key) {
      case '=':
        e.preventDefault();
        setCanvasObjectFit(ZOOM_IN_CANVAS_FIT)
        updateZoom(getDocThrows, 0.1);
        break;
      case '-':
        e.preventDefault();
        ///This gives us good behavior until the render actually finishes
        // Contain on Zoom Out, squeezes the image to fit as
        // the canvas size changes 
        setCanvasObjectFit(ZOOM_OUT_CANVAS_FIT)
        updateZoom(getDocThrows, -0.1);
        break;
    }
  }

  function wheelCallback(e: WheelEvent) {
    if (IS_MAC ? !e.metaKey : !e.ctrlKey) return;
    
    const isAccelerated = IS_MAC ? e.metaKey : e.ctrlKey;

    if (!isAccelerated) return;

    e.preventDefault();
    e.stopPropagation();

    // Scroll Up = Zoom In
    if (e.deltaY < 0) {
      setCanvasObjectFit(ZOOM_IN_CANVAS_FIT);
      throttledUpdateZoom(getDocThrows, 0.1);
    // Scroll Down = Zoom In
    } else if (e.deltaY > 0) {
      setCanvasObjectFit(ZOOM_OUT_CANVAS_FIT)
      throttledUpdateZoom(getDocThrows, -0.1);
    }
  }

  document.addEventListener('keydown', callback);
  // By default wheel events are passive and annot be prevented
  document.addEventListener('wheel', wheelCallback, {passive: false})

  onCleanup(() => {
    document.removeEventListener('keydown', callback);
    document.removeEventListener('wheel', wheelCallback);
  });
}

function App() {
  registerGlobalKeys();

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
        <OfficeDocument doc={getDoc()!} ignoreShortcuts={ignoredShortcuts} />
      </Show>
    </>
  );
}

export default App;
