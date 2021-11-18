#!/usr/bin/env python3

import sys
from os import path
import json

def main():
  filepath = path.join(path.dirname(__file__), "..", "src", "execute", "opcodes.c")
  opcodes = []
  with open(filepath, "rt") as fp:
    for instr in loop(fp):
      opcodes.append(int(instr['opcode'], 0))
      print(json.dumps(instr))
  undefined = list(reportUndefined(opcodes))
  print("unused opcodes: {}".format(undefined), file=sys.stderr)


def reportUndefined(defined):
  defined = sorted(defined)
  last = -1
  def out():
    start, end = last + 1, opcode - 1
    if start == end:
      yield hex(start)
    else:
      yield "{}-{}".format(hex(start), hex(end))
  for opcode in defined:
    if opcode != last + 1:
      yield from out()
    last = opcode
  if last != 255:
    opcode = 256
    yield from out()


def loop(fp):
  mode = 'search'
  instr = dict()
  for line in fp.readlines():
    if mode == 'search':
      if line.startswith("// 0x"):
        mode = 'summary'
        line = line[2:].strip().split(" ")
        args = " ".join(line[2:])
        instr = {
          'opcode': line[0],
          'mnemonic': line[1],
          'args': [a.strip() for a in args.split(',')],
          'summary': "",
          'details': "",
        }
    elif mode == 'summary':
      if not line.startswith("//"):
        mode = 'search'
        yield instr
      else:
        line = line[2:].strip()
        if line:
          instr['summary'] += line + "\n"
        else:
          mode = 'details'
    elif mode == 'details':
      if not line.startswith("//"):
        mode = 'search'
        yield instr
      else:
        line = line[2:].strip()
        instr['details'] += line + "\n"

if __name__ == "__main__":
  main()
