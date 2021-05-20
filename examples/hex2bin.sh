#!/bin/sh
set -e

case "$1" in
  *'.hex')
    <"$1" sed 's/#.*//' | xxd -r -p >"${1%%hex}bsvm"
  ;;
  *) exit 1 ;;
esac
