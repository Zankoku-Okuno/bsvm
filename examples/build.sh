#!/bin/sh
set -e

HERE="$(realpath "$(dirname "$0")")"
cd "$HERE"

UNHEX=./hex2bin.sh
BSASM=../scripts/bsasm.py
LIB=../packages/stdlib
SRC=.


"$UNHEX" "$SRC/donothing.hex"
"$UNHEX" "$SRC/exit84.hex"
"$BSASM" "$SRC/exit84.bS"
"$BSASM" "$SRC/factorial.bS"
"$BSASM" "$SRC/hello.bS"
"$BSASM" \
    "$LIB/isa.bS" \
    "$LIB/ByteBuf.bS" \
    "$SRC/sponge.bS"
