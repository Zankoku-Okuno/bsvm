#ifndef TYPES_H
#define TYPES_H

#include "common.h"


typedef struct Program Program;
struct Program {
  byte* code;
  size_t codeSize_bytes;
  ptrdiff_t entrypoint; // offset into `self.code` to begin execution
  // TODO symbol table for disassebly/debugging
  // TODO comments so disassembly can include them
};


typedef struct Machine Machine;
typedef struct StackFrame StackFrame;

struct Machine {
  byte* ip;
  StackFrame* top;
  struct {
    size_t len;
    word* at;
  } global;
  struct retarray {
    size_t cap;
    word* bufp;
  } retarray;
  struct environ {
    size_t argc;
    char** argv; // a read-only borrow
  } environ;
  bool shouldHalt;
  int exitcode;
  Program* program; // a read-only borrow
};
int initMachine(Machine* out, Program* prog, size_t argc, char** argv);
void destroyMachine(Machine* machine);

struct StackFrame {
  StackFrame* prev;
  word r[]; // `r` for register
};
void destroyStack(StackFrame* top);


int32_t readI32(byte** ipp);
uint32_t readU32(byte** ipp);
uintptr_t readWordOld(byte** ipp);

uintptr_t readVarint(byte** ipp);


#endif
