import { acceptCanvasTransfer, loadDocument } from '@lok';
import type { DocumentClient } from '@lok/shared';
import './App.css';
import { Show, createSignal } from 'solid-js';
import { CallbackType } from '@lok/lok_enums';
import { OfficeDocument, containerHeight } from './OfficeDocument/OfficeDocument';
import { cleanup } from './OfficeDocument/cleanup';

const [loading, setLoading] = createSignal(false);
const [getDoc, setDoc] = createSignal<DocumentClient | null>(null);

async function fileOpen(files: FileList | null) {
  if (!files || !files[0]) return;
  const name = files[0].name;
  const blob = files[0].slice();
  setLoading(true);
  const oldDoc = getDoc();
  if (oldDoc) {
    cleanup(oldDoc);
  }
  const offscreen = document?.querySelector("canvas")?.transferControlToOffscreen();
  console.log('offscreen', offscreen);
  if (offscreen) {
    console.log('transfering canvas', offscreen);
    acceptCanvasTransfer(offscreen)
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
  doc.on(CallbackType.TEXT_SELECTION, console.log);
  // doc.on(CallbackType.STATE_CHANGED, (payload) => console.log(payload));
}

export const [canvas, setCanvas] = createSignal<HTMLCanvasElement | undefined>();

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
      </div>
      <Show when={loading()}>
        <div class="loadsection">
          <span class="loader" />
        </div>
      </Show>
      <Show when={getDoc()} keyed>
        <OfficeDocument doc={getDoc()!} />
      </Show>
      <canvas
        
        ref={setCanvas}
        id="canvas"
        class="sticky top-0 pointer-events-none"
        style={{
          // TODO: object-fit should dynamically set while zooming to use fill so we get "zooming" for free until the render actually finishes
          'object-fit': 'none',
          'object-position': 'top left',
          width: `$800px`,
          'max-height': `${containerHeight()!}px`,
        }}
      />
    </>
  );
}

export default App;
