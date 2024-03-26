# Important files

<!-- TIP: in neovim, you can use `gf` to go to the file linked if the cursor is between ( ) -->

## `libreoffice-core` 

The LibreOffice "core" from which LOK is built, forked from upstream with proprietary changes from Macro

[`solenv/gbuild/platform/EMSCRIPTEN_INTEL_GCC.mk`](./libreoffice-core/solenv/gbuild/platform/EMSCRIPTEN_INTEL_GCC.mk) - compile flags for emscripten

[`desktop/Library_sofficeapp.mk`](./libreoffice-core/desktop/Library_sofficeapp.mk) - Makefile for the original LibreOfficeKit (LOK) as a library

[`desktop/Executable_soffice_bin.mk`](./libreoffice-core/desktop/Executable_soffice_bin.mk) - Makefile for `sofficeapp.js` `sofficeapp.wasm`

[`desktop/source/app/main_wasm.cxx`](./libreoffice-core/desktop/source/app/main_wasm.cxx) - Emscripten bindings, most exported WASM classes/functions

[`desktop/wasm/`](./libreoffice-core/desktop/wasm/) - JavaScript/TypeScript for the WASM library, worker, and tiled renderer

[`desktop/inc/lib/init.hxx`](./libreoffice-core/desktop/inc/lib/init.hxx) 
[`desktop/source/lib/init.cxx`](./libreoffice-core/desktop/source/lib/init.cxx) - the implementation for the original LOK bindings

[`desktop/inc/lib/wasm_extensions.hxx`](./libreoffice-core/desktop/inc/lib/wasm_extensions.hxx)
[`desktop/source/lib/wasm_extensions.cxx`](./libreoffice-core/desktop/source/lib/wasm_extensions.cxx) - extensions to LibreOfficeDocument used by the Emscripten bindings

## `qa-env`
