#!/bin/bash

set -e

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE:-$0}")" &> /dev/null && pwd)

# Add emscripten toolchain
source "$SCRIPT_DIR/../emsdk/emsdk_env.sh"
cd "$SCRIPT_DIR/../libreoffice-core"
./autogen.sh --with-distro=CPWASM-LOKit
