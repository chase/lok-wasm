#!/bin/bash

EMSDK_VERSION=3.1.54
BASE_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE:-$0}")"/.. &>/dev/null && pwd)

# This prevents git from trying to track a massive amount of files, speeding up git status and other commands
find libreoffice-core/translations -type f -exec git update-index --assume-unchanged '{}' +
find libreoffice-core/dictionaries -type f -exec git update-index --assume-unchanged '{}' +
find libreoffice-core/helpcontent2 -type f -exec git update-index --assume-unchanged '{}' +

# Install qa-env
(cd $BASE_DIR/qa-env && npm install)

# Get the emsdk repo, this isn't a submodule because they're awkward to work with
git clone https://github.com/emscripten-core/emsdk.git || true
cd emsdk || exit 1
./emsdk install $EMSDK_VERSION
./emsdk activate $EMSDK_VERSION
for patch_file in "$BASE_DIR"/emsdk-patches/*.patch; do
  patch -p1 <"$patch_file"
done
