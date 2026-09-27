#!/bin/bash

START_DIR=$(pwd)
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
TMP_DIR=$(mktemp -d)

UPLOAD_RESET_SRC="$SCRIPT_DIR/tools/upload-reset.c"
UPLOAD_RESET_BIN="$TMP_DIR/upload-reset"

clean() {
    cd "$START_DIR" || exit 1
    rm -rf "$TMP_DIR"
}

if [ ! -f "$UPLOAD_RESET_SRC" ]; then
    echo "Error: missing $UPLOAD_RESET_SRC"
    rm -rf "$TMP_DIR"
    exit 1
fi

if ! command -v gcc >/dev/null 2>&1; then
    echo "Error: gcc not found (needed to build upload-reset)"
    rm -rf "$TMP_DIR"
    exit 1
fi

echo "Building upload-reset..."
if ! gcc -Wall -Wextra -Wpedantic -std=c11 -o "$UPLOAD_RESET_BIN" "$UPLOAD_RESET_SRC"; then
    echo "Error: failed to compile upload-reset"
    rm -rf "$TMP_DIR"
    exit 1
fi

"$UPLOAD_RESET_BIN" /dev/ttyACM0 1500
status=$?
if [ "$status" -ne 0 ]; then
    echo "Error: Failed to reset to original bootloader"
    clean
    exit 1
fi

clean
echo "Successfully reset to original bootloader"
