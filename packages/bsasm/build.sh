#!/bin/sh
set -e

HERE="$(realpath "$(dirname "$0")")"
cd "$HERE"


BSASM=../../scripts/bsasm.py
BIN="./bin"
LIB=../stdlib/src
SRC=./src
"$BSASM" \
    "$LIB/isa.bS" \
    "$LIB/Char.bS" \
    "$LIB/Print.bS" \
    "$LIB/ByteSlice.bS" \
    "$LIB/ByteBuf.bS" \
    "$LIB/ArrayBuf.bS" \
    "$LIB/File.bS" \
    "$SRC/types.bS" \
    "$SRC/parse.bS" \
    "$SRC/strings.bS" \
    "$SRC/main.bS"

mkdir -p "$BIN"
mv "$SRC/main.bsvm" "$BIN/bsasm"
chmod +x "$BIN/bsasm"
