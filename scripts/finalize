#!/bin/bash

set -e
BASE_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE:-$0}")"/.. &>/dev/null && pwd)
BUILD_DIR="$BASE_DIR/build"

mkdir "$BUILD_DIR" &>/dev/null || true;

cd "$BASE_DIR/libreoffice-core/instdir/program";

echo 'Optimizing WASM...'
"$BASE_DIR"/emsdk/upstream/bin/wasm-opt \
  -o "$BUILD_DIR/soffice.wasm" \
  -O4 \
  --dce \
  --remove-unused-names \
  --enable-simd \
  --enable-bulk-memory \
  --enable-exception-handling \
  --enable-threads \
  soffice.wasm

echo 'Copying files...'
cp ./*.data ./*.js ./*.d.ts ./*.metadata "$BUILD_DIR"

cd "$BUILD_DIR"
rm fccache.* fccache_worker.* bindings_uno.* webgl2_draw_image.* || true;

echo 'Compressing files...'
brotli ./*.wasm ./*.data

for file in *.br; do
  mv -- "$file" "${file%.br}"
done
