// This file is meant to be included only by `../execute.c`.
// It also serves as documentation of the ISA.

// Operands are given in-order and described with the form `addrmode<name>`.
// Addressing modes are:
//   * `r`: operand is stored in the given register (varlen integer)
//   * `imm`: operand is stored directly in the code (varlen integer)
//   * `word`: operand is stored directly in the code (big-endian word)
//   * `byte`: operand is stored directly in the code (as a single signed byte)
// Some operands are given as arrays, in which case they are written as `len *
// format`, where `len` is the name of a previously-given operand, and format is
// as above.

// 0x00 HCF
// "Halt and Catch Fire"
// Immediately exit with error code -1 (255).
//
// This is not meant to be used as a real opcode, it's just here as a sentinel
// value in case execution gets out of bounds. I chose all-zeros because I
// expect that to be common.
static inline
void halt(Machine* self) {
  // FIXME this should cleanup properly? (just so valgrind doesn't complain
  self->shouldHalt = true;
  self->exitcode = -1;
}

// 0x02 MOV r<dst>, r<src>
static inline
void move(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst] = self->top->r[src];
}

// 0x03 MOV r<dst>, imm<src>
static inline
void moveImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits = imm;
}

// 0x04 LD r<dst>, r<src>
// Load a word from the address in src and place it in dst.
static inline
void load(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst] = *self->top->r[src].wptr;
}

// 0x06 ST r<dst>, r<src>
// Store contents of the src register into memory pointed to by dst register.
static inline
void store(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  *self->top->r[dst].wptr = self->top->r[src];
}

// 0x0A LEA r<dst>, r<src>
// "Load Effective Address" stores the address of src into dst
// That is, the destination will contain a pointer allowing reads/writes to the
// src register.
static inline
void lea(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].wptr = &self->top->r[src];
}

// 0x0B LIA r<dst>, word<offset>
// "Load IP-relative Address" stores `ip + offset` into dst.
// The `ip` used is at the start of this instruction.
static inline
void lia(Machine* self) {
  byte* here = self->ip - 1;
  size_t dst = readVarint(&self->ip);
  ptrdiff_t offset = readWord(&self->ip);
  self->top->r[dst].bptr = here + offset;
}

// 0x0C LDB r<dst>, r<src>
// Load a byte from the address in src and place it in dst.
static inline
void loadByte(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].byte.low = *self->top->r[src].bptr;
}

// 0x0E STB r<dst>, r<src>
// Store contents of the src register into memory pointed to by dst register.
static inline
void storeByte(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  *self->top->r[dst].bptr = self->top->r[src].byte.low;
}

// 0x10 ADD r<dst>, r<src>
static inline
void add(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits += self->top->r[src].bits;
}

// 0x11 ADD r<dst>, imm<src>
static inline
void addImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits += imm;
}

// 0x12 SUB r<dst>, r<src>
static inline
void sub(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits -= self->top->r[src].bits;
}

// 0x13 SUB r<dst>, imm<src>
static inline
void subImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits -= imm;
}

// 0x17 NEG r<dst>, r<src>
// arithmetic negate
static inline
void neg(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].sbits = -self->top->r[src].sbits;
}

// 0x18 MUL r<dst>, r<src>
// unsigned single-width multiply
static inline
void mul(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits *= self->top->r[src].bits;
}

// 0x19 MUC r<dst-high>, r<dst-low>, r<src>
// unsigned double-width multiply
// dstHigh:dstLow <- dstLow * src
static inline
void muc(Machine* self) {
  size_t dstHigh = readVarint(&self->ip);
  size_t dstLow = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  ulong a = self->top->r[dstLow].bits;
  ulong b = self->top->r[src].bits;
  ulong r = a * b;
  self->top->r[dstLow].bits = (uintptr_t)r;
  self->top->r[dstHigh].bits = (uintptr_t)(r >> sizeof(uintptr_t));
}

// 0x1A IMUL r<dst>, r<src>
// signed single-width multiply
static inline
void imul(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].sbits *= self->top->r[src].sbits;
}

// 0x1B IMUC r<dst-high>, r<dst-low>, r<src>
// signed double-width multiply
static inline
void imuc(Machine* self) {
  // size_t dstHigh = readVarint(&self->ip);
  // size_t dstLow = readVarint(&self->ip);
  // size_t src = readVarint(&self->ip);
  // slong a = self->top->r[dstLow].sbits;
  // slong b = self->top->r[src].sbits;
  // slong r = a * b;
  fprintf(stderr, "oh jeez, I don't actually know what's normal for a signed double-width multiply\n");
  exit(-1);
  // I guess the low size-1 bits should go in low, plus the sign bit, and the high size-1 bits in high plus its own sign bit?
  // self->top->r[dstLow].bits = (uintptr_t)r;
  // self->top->r[dstHigh].bits = (uintptr_t)(r >> sizeof(uintptr_t));
}

// 0x1C DIV r<dst>, r<src>
// unsigned divide
static inline
void divide(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t numer = self->top->r[dst].bits;
  uintptr_t denom = self->top->r[src].bits;
  self->top->r[dst].bits = numer / denom;
}

// 0x1D DVR r<dst>, r<rem>, r<src>
// unsigned divide with remainder
// dst <- dst / src ; rem <- dst % src
static inline
void divrem(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t rem = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t numer = self->top->r[dst].bits;
  uintptr_t denom = self->top->r[src].bits;
  uintptr_t d = numer / denom;
  uintptr_t r = numer % denom;
  self->top->r[dst].bits = d;
  self->top->r[rem].bits = r;
}

// 0x1E IDIV r<dst>, r<src>
// signed divide (rount to zero)
static inline
void idivide(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  intptr_t numer = self->top->r[dst].sbits;
  intptr_t denom = self->top->r[src].sbits;
  // C is round-to-zero, and I see no reason to change that if the remander is immaterial
  self->top->r[dst].sbits = numer / denom;
}


// 0x1F IDVR r<dst>, r<rem>, r<src>
// signed divide with remainder (round to -∞)
static inline
void idivrem(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t rem = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  intptr_t numer = self->top->r[dst].sbits;
  intptr_t denom = self->top->r[src].sbits;
  // FIXME check that denom /= 0, but then what?
  intptr_t d = numer / denom;
  intptr_t r = numer % denom;
  // since C is round-to-zero, but modular arithmetic would prefer round-to-minus-infinity
  if (r < 0) {
    d -= 1;
    r += denom;
  }
  self->top->r[dst].sbits = d;
  self->top->r[rem].sbits = r;
}


/************************************
 Bit Fiddling
 ************************************/

// 0x30 OR r<dst>, r<src>
static inline
void bitOr(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits |= self->top->r[src].bits;
}

// 0x31 OR r<dst>, imm<src>
static inline
void bitOrImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits |= imm;
}

// 0x32 XOR r<dst>, r<src>
static inline
void xor(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits ^= self->top->r[src].bits;
}

// 0x33 XOR r<dst>, imm<src>
static inline
void xorImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits ^= imm;
}

// 0x34 AND r<dst>, r<src>
static inline
void bitAnd(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits &= self->top->r[src].bits;
}

// 0x35 AND r<dst>, imm<src>
static inline
void bitAndImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits &= imm;
}

// 0x37 INV r<dst>, r<src>
// bitwise invert
static inline
void inv(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits = ~self->top->r[src].bits;
}

static uintptr_t shiftMask = CHAR_BIT * sizeof(word) - 1;

// 0x38 SZR r<dst> r<src> r<amt>
// Shift right, zero-extending.
static inline
void szr(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  size_t amt = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].bits >> (self->top->r[amt].bits & shiftMask);
}

// 0x39 SZR r<dst> r<src> imm<amt>
// Shift right immediate, zero-extending.
static inline
void szrImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t amt = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].bits >> (amt & shiftMask);
}

// 0x3A SAR r<dst> r<src> r<amt>
// Shift right, sign-extending (i.e. arithmetic).
static inline
void sar(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  size_t amt = readVarint(&self->ip);
  self->top->r[dst].sbits = self->top->r[src].sbits >> (self->top->r[amt].bits & shiftMask);
}

// 0x3B SAR r<dst> r<src> imm<amt>
// Shift right immediate, sign-extending (i.e. arithmetic).
static inline
void sarImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t amt = readVarint(&self->ip);
  self->top->r[dst].sbits = self->top->r[src].sbits >> (amt & shiftMask);
}

// 0x3C SHL r<dst> r<src> r<amt>
// Shift left.
static inline
void shl(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  size_t amt = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].bits << (self->top->r[amt].bits & shiftMask);
}

// 0x3D SHL r<dst> r<src> imm<amt>
// Shift right immediate.
static inline
void shlImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t amt = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].bits << (amt & shiftMask);
}

// 0x3E ROL r<dst> r<src> r<amt>
// Rotate (circular shift) left.
static inline
void rol(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  size_t amtReg = readVarint(&self->ip);
  uintptr_t amt = self->top->r[amtReg].bits & shiftMask;
  uintptr_t x = self->top->r[src].bits;
  self->top->r[dst].bits = (x << amt) | (x >> (-amt & shiftMask));
}

// 0x3F ROL r<dst> r<src> imm<amt>
// Rotate (circular shift) left immediate.
static inline
void rolImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t amt = readVarint(&self->ip);
  uintptr_t x = self->top->r[src].bits;
  self->top->r[dst].bits = (x << amt) | (x >> (-amt & shiftMask));
}


/************************************
 Memory Buffers
 ************************************/


// 0x40 NEW r<dst> r<src>
// allocate src bytes and retain pointer to them in dst
static inline
void vmAlloc(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bptr = malloc(self->top->r[src].bits);
}

// 0x41 FREE r<ptr>
// Free `NEW`-allocated pointer and clear register.
//
// If passed a register containing NULL, this is a no-op.
static inline
void vmFree(Machine* self) {
  size_t src = readVarint(&self->ip);
  byte* ptr = self->top->r[src].bptr;
  if (ptr != NULL) {
    free(ptr);
    self->top->r[src].bptr = NULL;
  }
}


/************************************
 Comparisons
 ************************************/


// 0x50 BIT byte<i>, r<dst>, r<src>
// test bit i (0 is least-significant bit)
static inline
void bitTest(Machine* self) {
  byte i = *self->ip++;
  uintptr_t mask = 1 << i; // FIXME undefined behavoir if i bigger than word size
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits = (self->top->r[src].bits & mask) ? 1 : 0;
}

// 0x52 ANY r<dst>, imm<n>, n * r<src...>
// set dst to 1 if any of src... are non-zero (clear otherwise)
static inline
void any(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t n = readVarint(&self->ip);
  size_t i = 0;
  bool result = 0;
  for (; i < n; ++i) {
    size_t src = readVarint(&self->ip);
    if (self->top->r[src].bits) {
      result = 1;
      break;
    }
  }
  for (; i < n; ++i) {
    readVarint(&self->ip);
  }
  self->top->r[dst].bits = result;
}

// 0x53 ALL r<dst>, imm<n>, n * r<src...>
// set dst to 1 if all of src... are non-zero (clear otherwise)
static inline
void all(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t n = readVarint(&self->ip);
  size_t i = 0;
  bool result = 1;
  for (; i < n; ++i) {
    size_t src = readVarint(&self->ip);
    if (!self->top->r[src].bits) {
      result = 0;
      break;
    }
  }
  for (; i < n; ++i) {
    readVarint(&self->ip);
  }
  self->top->r[dst].bits = result;
}

// 0x54 EQ r<dst>, r<src1>, r<src2>
// set when equal (clear otherwise)
static inline
void setEq(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src1 = readVarint(&self->ip);
  size_t src2 = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src1].sbits == self->top->r[src2].sbits ? 1 : 0;
}

// 0x55 EQ r<dst>, r<src>, imm<const>
static inline
void setEqImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  intptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].sbits == imm ? 1 : 0;
}

// 0x56 NE r<dst>, r<src1>, r<src2>
// set when not equal (clear otherwise)
static inline
void setNeq(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src1 = readVarint(&self->ip);
  size_t src2 = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src1].sbits != self->top->r[src2].sbits ? 1 : 0;
}

// 0x57 NE r<dst>, r<src>, imm<const>
static inline
void setNeqImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  intptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].sbits != imm ? 1 : 0;
}

// 0x58 BL r<dst>, r<src1>, r<src2>
// set when `src1 < src2` (clear otherwise)
static inline
void setBelow(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src1 = readVarint(&self->ip);
  size_t src2 = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src1].bits < self->top->r[src2].bits ? 1 : 0;
}

// 0x59 BL r<dst>, r<src>, imm<const>
static inline
void setBelowImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].bits < imm ? 1 : 0;
}

// 0x5A BLE r<dst>, r<src1>, r<src2>
// set when `src1 <= src2` (clear otherwise)
static inline
void setBelowEq(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src1 = readVarint(&self->ip);
  size_t src2 = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src1].bits <= self->top->r[src2].bits ? 1 : 0;
}

// 0x5B BLE r<dst>, r<src>, imm<const>
static inline
void setBelowEqImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].bits <= imm ? 1 : 0;
}

// 0x5C LT r<dst>, r<src1>, r<src2>
// set when `src1 < src2` (clear otherwise)
static inline
void setLt(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src1 = readVarint(&self->ip);
  size_t src2 = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src1].sbits < self->top->r[src2].sbits ? 1 : 0;
}

// 0x5D LT r<dst>, r<src>, imm<const>
static inline
void setLtImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  intptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].sbits < imm ? 1 : 0;
}

// 0x5E LTE r<dst>, r<src1>, r<src2>
// set when `src1 <= src2` (clear otherwise)
static inline
void setLte(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src1 = readVarint(&self->ip);
  size_t src2 = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src1].sbits <= self->top->r[src2].sbits ? 1 : 0;
}

// 0x5F LTE r<dst>, r<src>, imm<const>
static inline
void setLteImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  intptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits = self->top->r[src].sbits <= imm ? 1 : 0;
}


/************************************
 Jumps
 ************************************/


// 0x70 JMPR r<src>
// computed jump (jump to address held in addr register)
static inline
void computedJump(Machine* self) {
  size_t src = readVarint(&self->ip);
  self->ip = self->top->r[src].bptr;
}

// 0x72 CJMP r<cond>, word<offset>
// if cond is non-zero, jump to `start address of this instruction + offset`
static inline
void cjump(Machine* self) {
  byte* here = self->ip - 1;
  size_t cond = readVarint(&self->ip);
  ptrdiff_t offset = readWord(&self->ip);
  if (self->top->r[cond].bits) {
    self->ip = here + offset;
  }
}

// 0x73 ZJMP r<cond>, word<offset>
// if cond is zero, jump to `start address of this instruction + offset`
static inline
void zjump(Machine* self) {
  byte* here = self->ip - 1;
  size_t cond = readVarint(&self->ip);
  ptrdiff_t offset = readWord(&self->ip);
  if (!self->top->r[cond].bits) {
    self->ip = here + offset;
  }
}



/************************************
 Subroutines
 ************************************/


// 0x81 JAL word<offset>, imm<n>, n * r<src>
// Jump and link to `ip + offset`.
//
// Save the address of the next instruction into the callee's register zero.
// Copy the contents of the n src registers of the caller into registers 1-n of
// the callee. Jump to `ip + offset`.
//
// As with all functions, the target address should be a word stating the size
// of the callee's stack frame. The actual code that is entered should occur
// immediately after that word.
static inline
void jal(Machine* self) {
  // accumulate information about callee
  byte* here = self->ip - 1; // WARNING have to backup one because the opcode has already been read
  ptrdiff_t offset = readWord(&self->ip);
  byte* tgt = here + offset;
  size_t calleeSize_words = readWord(&tgt);
  // setup callee stack frame
  StackFrame* callee = malloc(sizeof(StackFrame) + sizeof(word) * calleeSize_words);
  callee->prev = self->top;
  size_t argument_count = readVarint(&self->ip);
  for(size_t i = 1; i <= argument_count; ++i) {
    size_t src = readVarint(&self->ip);
    callee->r[i] = self->top->r[src];
  }
  callee->r[0] = (word)self->ip;
  // push callee frame and jump
  self->top = callee;
  self->ip = tgt;
}

// 0x83 JAR
// "Jump-and-relink".
// As JAL, but the next stack frame will return not to this frame, but the previous one.
// That is, this implements a tail call.
static inline
void jar(Machine* self) {
  byte* here = self->ip - 1;
  ptrdiff_t offset = readWord(&self->ip);
  byte* tgt = here + offset;
  size_t calleeSize_words = readWord(&tgt);
  StackFrame* callee = malloc(sizeof(StackFrame) + sizeof(word) * calleeSize_words);
  callee->prev = self->top->prev; // <-- this is different from jal
  size_t argument_count = readVarint(&self->ip);
  for(size_t i = 1; i <= argument_count; ++i) {
    size_t src = readVarint(&self->ip);
    callee->r[i] = self->top->r[src];
  }
  callee->r[0] = self->top->r[0]; // <-- this is different from jal
  free(self->top); // <-- this is an extra step relative to jal
  self->top = callee;
  self->ip = tgt;
}

// 0x84 RET imm<n>, n * r<src...>
// Initialize n return values from <src...> registers, then jump to the contents
// of register zero and destroy this stack frame.
//
// Internally, there's a machine-wide array of return values (all word-sized).
// This is grown as necessary, then by reading from the callee frame. This
// instruction cleans up the callee frame immediately. The address returned to
// should be an `into` instruction, unless the caller needs none of the return
// values.
static inline
void ret(Machine* self) {
  // determine jump location
  byte* tgt = self->top->r[0].bptr;
  // setup return values
  size_t retarray_count = readVarint(&self->ip);
  if (retarray_count > self->retarray.cap) {
    self->retarray.bufp = realloc(self->retarray.bufp, sizeof(word)*retarray_count);
    self->retarray.cap = retarray_count;
  }
  for (size_t i = 0; i < retarray_count; ++i) {
    size_t src = readVarint(&self->ip);
    self->retarray.bufp[i] = self->top->r[src];
  }
  // pop stack
  StackFrame* callee = self->top;
  self->top = self->top->prev;
  free(callee);
  // perform jump
  self->ip = tgt;
}

// 0x85 INTO imm<n>, n * r<dst>
// Collect n return values, placing them in-order into the dst registers.
//
// See the `ret` instruction.
static inline
void into(Machine* self) {
  size_t retarray_count = readVarint(&self->ip);
  for (size_t i = 0; i < retarray_count; ++i) {
    size_t dst = readVarint(&self->ip);
    self->top->r[dst] = self->retarray.bufp[i];
  }
}

// 0x86 EXIT r<src>
// Stop the virtual machine, exiting with the error code stored in src.
static inline
void exit_(Machine* self) {
  size_t ecReg = readVarint(&self->ip);
  self->exitcode = self->top->r[ecReg].byte.low;
  self->shouldHalt = true;
}

// used internally for testing while I develop
static inline
void test(Machine* self) {
  size_t src = readVarint(&self->ip);
  fprintf(stdout, "result is %lx\n", self->top->r[src].bits);
}
