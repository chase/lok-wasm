#!/bin/bash

set -e

BASE_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE:-$0}")"/.. &>/dev/null && pwd)
FILE_PACKAGER="$BASE_DIR/emsdk/upstream/emscripten/tools/file_packager"
FCCACHE_FILE="soffice_fccache.data"
METADATA_SUFFIX=".js.metadata"
FCCAHCE_OUT="$BASE_DIR/libreoffice-core/instdir/program/$FCCACHE_FILE"
QA_ENV_DIR="$BASE_DIR/qa-env"

# Add emscripten toolchain
source "$BASE_DIR/emsdk/emsdk_env.sh"
cd "$BASE_DIR/libreoffice-core"
echo "Building LOK"
make || exit 1
echo "Generating FcCache"
# shellcheck disable=SC2046
$FILE_PACKAGER "$FCCAHCE_OUT" --preload $(cd "$QA_ENV_DIR" && node generate_fccache.mjs) \
  --js-output="$QA_ENV_DIR/workdir/$FCCACHE_FILE.js" \
  --separate-metadata && mv "$QA_ENV_DIR/workdir/$FCCACHE_FILE$METADATA_SUFFIX" "$FCCAHCE_OUT$METADATA_SUFFIX"
