#include "execute.h"

#include <stdio.h>

#include "execute/opcodes.c"

void cycle(Machine* self) {
  switch (*self->ip++) {
    case 0x00: halt(self); break;
    case 0x03: moveImm(self); break; // TODO opcode not fixed
    case 0x05: addImm(self); break; // TODO opcode not fixed
    // ...
    // case 0x80: jalr(self); break;
    case 0x81: jal(self); break;
    // case 0x82: rjalr(self); break;
    // case 0x83: rjal(self); break;
    case 0x84: ret(self); break;
    case 0x85: into(self); break;
    case 0x86: _exit(self); break;
    // 0x87
    // ...
    case 0xFF: test(self); break;
    default: {
      fprintf(stderr, "unexpected opcode %x\n", *(self->ip - 1));
      exit(-1);
    } break;
  }
}
