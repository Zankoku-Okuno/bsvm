#!/bin/sh
set -e

case "$1" in
  *'.hex')
    xxd -r -p "$1" >"${1%%hex}bsvm"
  ;;
  *) exit 1 ;;
esac
