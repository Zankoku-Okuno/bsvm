#!/bin/sh
set -e

HERE="$(realpath "$(dirname "$0")")"
cd "$HERE"


BIN="./bin"
LIB=../stdlib
SRC=./src
BSASM=../scripts/bsasm.py
"$BSASM" \
    "$LIB/isa.bS" \
    "$LIB/ByteSlice.bS" \
    "$LIB/ByteBuf.bS" \
    "$LIB/ArrayBuf.bS" \
    "$LIB/Print.bS" \
    "$SRC/strings.bS" \
    "$SRC/main.bS"

mkdir -p "$BIN"
mv "$SRC/main.bsvm" "$BIN/bsasm"
chmod +x "$BIN/bsasm"
