#!/bin/sh
set -e

here="$(realpath "$(dirname "$0")")"
cd "$here"

mkdir -p bin

gcc -std=c11 -I src -Wall -Werror src/*.c -o bin/bsvm
