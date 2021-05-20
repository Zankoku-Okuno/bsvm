#include "execute.h"

#include <stdio.h>

#include "execute/opcodes.c"

void cycle(Machine* self) {
  switch (*self->ip++) {
    // 00-3F basic intructions (movement, arithmetic, logic)
    case 0x00: halt(self); break;
    case 0x02: move(self); break;
    case 0x03: moveImm(self); break;
    // case 0x04:
    // case 0x05:
    case 0x06: load(self); break;
    // case 0x07 // load with immediate offset?
    case 0x08: store(self); break;
    // case 0x09 // store with immediate offset?
    // 0x0A-0x0F LDB &c


    case 0x10: add(self); break;
    case 0x11: addImm(self); break;
    case 0x12: sub(self); break;
    case 0x13: subImm(self); break;
    case 0x14: mul(self); break;
    case 0x15: dmul(self); break;
    case 0x16: imul(self); break;
    case 0x17: dimul(self); break;
    // case 0x18 adc
    // case 0x19 adci
    // case 0x1A subb
    // case 0x1B subbi
    // case 0x1C div
    case 0x1D: divrem(self); break;
    // case 0x1E idiv
    case 0x1F: idivrem(self); break;


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
