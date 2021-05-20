#include "types.h"


int initMachine(Machine* out, Program* prog) {
  // setup code and instruction pointer
  out->program = prog;
  out->ip = prog->code + prog->entrypoint;
  size_t startFrameRegisters_count = readWord(&out->ip);
  // setup main stack frame
  out->top = malloc(sizeof(StackFrame) + sizeof(word) * startFrameRegisters_count);
  out->top->prev = NULL;
  if (out->top == NULL) { return 1; }
  // setup retarray
  out->retarray.cap = 8;
  out->retarray.bufp = malloc(sizeof(word) * out->retarray.cap);
  out->shouldHalt = false;
  out->exitcode = -1;
  return 0;
}
void destroyMachine(Machine* machine) {
  destroyStack(machine->top);
  free(machine->top);
  machine->top = NULL;
  free(machine->retarray.bufp);
  machine->retarray.bufp = NULL;
  machine->retarray.cap = 0;
}

void destroyStack(StackFrame* top) {
  if (top->prev != NULL) { destroyStack(top->prev); }
  free(top->prev);
  top->prev = NULL;
}


uintptr_t readWord(byte** ipp) {
  uintptr_t out = 0;
  byte* ip = *ipp;
  for (int i = 0; i < sizeof(word); ++i) {
    out = (out << 8) + *ip++;
  }
  *ipp = ip;
  return out;
}

uintptr_t readVarint(byte** ipp) {
  byte* ip = *ipp;
  uintptr_t out = (*ip & 0x40) ? ~0 : 0;
  do {
    out = (out << 7) + (*ip & 0x7F);
  } while (*ip++ & 0x80);
  *ipp = ip;
  return out;
}
