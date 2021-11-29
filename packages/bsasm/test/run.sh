#!/bin/sh
set -e

HERE="$(realpath "$(dirname "$0")")"
cd "$HERE"


../build.sh

BSVM=../../../bin/bsvm
BSASM=../../../scripts/bsasm.py
INPUT=.
OUTPUT=.
BIN=../bin

if [ "$1" = "--valgrind" ]; then
    BSVM="valgrind --error-exitcode=100 $BSVM"
    shift
fi

inFile="$INPUT/delme.bS" # TODO
$BSVM "$BIN/bsasm" "$inFile" >"$OUTPUT/delme.actual" # TODO
if ! diff "$OUTPUT/delme.expected" "$OUTPUT/delme.actual"; then # TODO
    exit 1
fi
