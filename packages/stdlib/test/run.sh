#!/bin/sh
set -e

HERE="$(realpath "$(dirname "$0")")"
cd "$HERE"



BSVM=../../../bin/bsvm
BSASM=../../../scripts/bsasm.py
INPUT=./files
OUTPUT=./files

LIB=../src
STDLIB=""
for lib in isa Print ByteSlice ByteBuf; do
    STDLIB="$STDLIB $LIB/$lib.bS"
done

if [ "$1" = "--valgrind" ]; then
    BSVM="valgrind --error-exitcode=100 $BSVM"
    shift
fi
if [ "$#" = 0 ]; then
    suites="Print ByteSlice ByteBuf"
else
    suites=$@
fi

success=0
for suite in $suites; do
    echo >&2 "$suite"
    $BSASM $STDLIB "$suite.bS"
    inFile="$INPUT/$suite.input"
    if [ ! -f "$inFile" ]; then inFile=''; fi
    $BSVM "$suite.bsvm" "$inFile" >"$OUTPUT/$suite.actual"
    if ! diff "$OUTPUT/$suite.expected" "$OUTPUT/$suite.actual"; then
        success=$((success + 1))
    fi
done

exit "$success"
