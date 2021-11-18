#include "execute.h"

#include "execute/opcodes.c"

void cycle(Machine* self) {
  switch (*self->ip++) {
    // 0x00: halt and catch fire in case ip goes out-of-bounds
    case 0x00: halt(self); break;
    // case 0x01: ???(self); break;
    // 0x02 - 0x0F: loads, stores, moves, and address calculation
    case 0x02: move(self); break;
    case 0x03: moveImm(self); break;
    case 0x04: load(self); break;
    // case 0x05 // load with immediate offset?
    case 0x06: store(self); break;
    // case 0x07 // store with immediate offset?
    // case 0x08: ???(self); break;
    // case 0x09: ???(self); break;
    case 0x0A: lea(self); break;
    case 0x0B: lia(self); break;
    case 0x0C: loadByte(self); break;
    // case 0x0D // load byte with immediate offset?
    case 0x0E: storeByte(self); break;
    // case 0x0F // store byte with immediate offset?

    // 0x10 - 0x1F: arithmetic
    case 0x10: add(self); break;
    case 0x11: addImm(self); break;
    case 0x12: sub(self); break;
    case 0x13: subImm(self); break;
    // case 0x14 adc
    // case 0x15 cc (clear carry) // FIXME would be a set carry
    // case 0x16 subb ?
    case 0x17: neg(self); break;
    case 0x18: mul(self); break;
    case 0x19: muc(self); break;
    case 0x1A: imul(self); break;
    case 0x1B: imuc(self); break;
    case 0x1C: divide(self); break;
    case 0x1D: divrem(self); break;
    case 0x1E: idivide(self); break;
    case 0x1F: idivrem(self); break;

    // 0x20-0x3F: bit fiddling
    case 0x30: bitOr(self); break;
    case 0x31: bitOrImm(self); break;
    case 0x32: xor(self); break;
    case 0x33: xorImm(self); break;
    case 0x34: bitAnd(self); break;
    case 0x35: bitAndImm(self); break;
    // case 0x36: ???(self); break;
    case 0x37: inv(self); break;
    case 0x38: szr(self); break;
    case 0x39: szrImm(self); break;
    case 0x3A: sar(self); break;
    case 0x3B: sarImm(self); break;
    case 0x3C: shl(self); break;
    case 0x3D: shlImm(self); break;
    case 0x3E: rol(self); break;
    case 0x3F: rolImm(self); break;

    // 0x40 - 0x4F: memory operations
    case 0x40: vmAlloc(self); break;
    case 0x41: vmFree(self); break;
    // mcpy r<dst>, r<src>, r<len>
    // mset r<dst>, r<len>, r<src>, r<srcLen> // cycle through tsrcLen chars of src, filling dst up to len chars
    // mcmp r<dst>, r<src1>, r<src2>, r<len>
    // mcat r<dst>, imm<n>, r<lengths -> word[n]>, r<strs -> byte*[n]>
    // mbrk r<dst>, r<src>, r<len>, r<chrs>, r<numChrs> // like C strpbrk
    // mspn r<dst>, r<src>, r<len>, r<chrs>, r<numChrs> // like C strcspn
    // mstr // find an infix like C strstr
    // // split, join, and strip are likely useful string functions
    // // perhaps mcat --> mimp (implode), and add mexp (explode)

    // 0x50-0x5F tests
    case 0x50: bitTest(self); break;
    // case 0x50: ???(self); break; boolean not?
    case 0x52: any(self); break;
    case 0x53: all(self); break;
    case 0x54: setEq(self); break;
    case 0x55: setEqImm(self); break;
    case 0x56: setNeq(self); break;
    case 0x57: setNeqImm(self); break;
    case 0x58: setBelow(self); break;
    case 0x59: setBelowImm(self); break;
    case 0x5A: setBelowEq(self); break;
    case 0x5B: setBelowEqImm(self); break;
    case 0x5C: setLt(self); break;
    case 0x5D: setLtImm(self); break;
    case 0x5E: setLte(self); break;
    case 0x5F: setLteImm(self); break;

    // 0x60 - 0x6F: conditioned operations
    // cmov, cld, cst
    // zcmov, zcld, zcst

    // 0x70 - 0x7F: jumps
    case 0x70: computedJump(self); break;
    case 0x72: cjump(self); break;
    case 0x73: zjump(self); break;


    // case 0x80: jalr(self); break;
    case 0x81: jal(self); break;
    // case 0x82: jarr(self); break; // jump and re-link (register)
    case 0x83: jar(self); break;
    case 0x84: ret(self); break;
    case 0x85: into(self); break;
    case 0x86: exit_(self); break;
    // 0x87
    // string operations

    // ... 0xC0-0xFF i/o
    case 0xFF: test(self); break;
    default: {
      fprintf(stderr, "unexpected opcode %x\n", *(self->ip - 1));
      exit(-1);
    } break;
  }
}
