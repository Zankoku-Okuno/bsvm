#!/bin/sh
set -e

HERE="$(realpath "$(dirname "$0")")"
cd "$HERE"

./build.sh

GOLDEN=./golden
BSVM=../bin/bsvm
if [ "$1" = "--valgrind" ]; then
    BSVM="valgrind --error-exitcode=100 $BSVM"
fi


success=0

    echo >&2 "donothing.hex"
    set +e
        $BSVM ./donothing.hand-bsvm
        ec=$?
    set -e
    if [ "$ec" != 255 ]; then
        echo >&2 "[FAIL] unexpected error code ($ec), expecting 255"
        success=$((success + 1))
    fi

    echo >&2 "exit84.hex"
    set +e
        $BSVM ./exit84.hand-bsvm
        ec=$?
    set -e
    if [ "$ec" != 84 ]; then
        echo >&2 "[FAIL] unexpected error code ($ec), expecting 84"
        success=$((success + 1))
    fi

    echo >&2 "exit84.bS"
    set +e
        $BSVM ./exit84.bsvm
        ec=$?
    set -e
    if [ "$ec" != 84 ]; then
        echo >&2 "[FAIL] unexpected error code ($ec), expecting 84"
        success=$((success + 1))
    fi

    echo >&2 "factorial.bS"
    set +e
        $BSVM ./factorial.bsvm
        ec=$?
    set -e
    if [ "$ec" != 120 ]; then
        echo >&2 "[FAIL] unexpected error code ($ec), expecting 120"
        success=$((success + 1))
    fi

    echo >&2 "hello.bS"
    set +e
        $BSVM ./hello.bsvm > "$GOLDEN/hello.actual"
        ec=$?
    set -e
    if [ "$ec" != 0 ]; then
        echo >&2 "[FAIL] unexpected error code ($ec)"
        success=$((success + 1))
    elif ! diff "$GOLDEN/hello.expected" "$GOLDEN/hello.actual"; then
        echo >&2 "[FAIL] actual output does not match expected"
        success=$((success + 1))
    fi
    echo >&2 "hello.bS Joy"
    set +e
        $BSVM ./hello.bsvm Joy > "$GOLDEN/hello-joy.actual"
        ec=$?
    set -e
    if [ "$ec" != 0 ]; then
        echo >&2 "[FAIL] unexpected error code ($ec)"
        success=$((success + 1))
    elif ! diff "$GOLDEN/hello-joy.expected" "$GOLDEN/hello-joy.actual"; then
        echo >&2 "[FAIL] actual output does not match expected"
        success=$((success + 1))
    fi


    echo >&2 "sponge.bS"
    set +e
        $BSVM ./sponge.bsvm < "$GOLDEN/sponge.expected" > "$GOLDEN/sponge.actual"
        ec=$?
    set -e
    if [ "$ec" != 0 ]; then
        echo >&2 "[FAIL] unexpected error code ($ec)"
        success=$((success + 1))
    elif ! diff "$GOLDEN/sponge.expected" "$GOLDEN/sponge.actual"; then
        echo >&2 "[FAIL] actual output does not match expected"
        success=$((success + 1))
    fi


exit "$success"
