#!/bin/bash

# Check if fswatch is installed
if ! command -v fswatch &>/dev/null; then
  echo "fswatch could not be found"

  # Check if Homebrew is installed
  if command -v brew &>/dev/null; then
    echo "Installing fswatch using Homebrew without updating..."
    HOMEBREW_NO_AUTO_UPDATE=1 brew install fswatch
  else
    echo "Homebrew not found. Please install fswatch manually."
    exit 1
  fi
fi

set -e
BASE_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE:-$0}")"/.. &>/dev/null && pwd)

source "$BASE_DIR/emsdk/emsdk_env.sh"

WATCH_DIRS=("sw" "vcl" "desktop")

cd "$BASE_DIR/libreoffice-core"

declare -a pids

# Function to start fswatch for a given directory
start_fswatch() {
  echo "Watching $1"
  fswatch -r -0 -o --event Updated \
    --include '.*\.(cxx|ts)$' \
    --exclude '/tests/' \
    --exclude '/test/' \
    "$1" |
    while read -r -d "" event; do
      echo "$event detected in $1. Running 'make'..."
      make || echo "Error occurred"
      echo "== FINISHED =="
    done
  pids+=("$!")
}

cleanup() {
  echo "Stopping all fswatch processes..."
  for pid in "${pids[@]}"; do
    kill "$pid"
  done
}
trap cleanup SIGINT

# Start watching each directory in background
for dir in "${WATCH_DIRS[@]}"; do
  start_fswatch "$dir" &
done

wait
