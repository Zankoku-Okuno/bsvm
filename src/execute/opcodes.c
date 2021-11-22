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

// 0x01 OFF r<dst>, imm<src>
// Add src * sizeof(word) to dst
static inline
void offset(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t imm = readVarint(&self->ip);
  self->top->r[dst].bits += sizeof(word) * imm;
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

// 0x05 LD r<dst>, r<src>, imm<off>
// Load a word from the address in src + off*sizeof(word) and place it in dst.
static inline
void loadOff(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  size_t imm = readVarint(&self->ip);
  self->top->r[dst] = self->top->r[src].wptr[imm];
}

// 0x06 ST r<dst>, r<src>
// Store contents of the src register into memory pointed to by dst register.
static inline
void store(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  *self->top->r[dst].wptr = self->top->r[src];
}

// 0x07 ST r<dst>, imm<off>, r<src>
// Store a word from src into the address at dst + off*sizeof(word).
static inline
void storeOff(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t imm = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].wptr[imm] = self->top->r[src];
}

// 0x08 LDG r<dst>, imm<ix>
// Load global value.
static inline
void ldGlobal(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t ix = readVarint(&self->ip);
  if (ix >= self->global.len) {
    self->global.at = realloc(self->global.at, sizeof(word*) * ix);
  }
  self->top->r[dst].bits = self->global.at[ix].bits;
}

// 0x09 STG imm<ix>, reg<src>
// Store global value.
static inline
void stGlobal(Machine* self) {
  size_t ix = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  if (ix >= self->global.len) {
    self->global.at = realloc(self->global.at, sizeof(word*) * ix);
  }
  self->global.at[ix].bits = self->top->r[src].bits;
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

// 0x0B LIA r<dst>, i32<offset>
// "Load IP-relative Address" stores `ip + offset` into dst.
// The `ip` used is at the start of this instruction.
static inline
void lia(Machine* self) {
  byte* here = self->ip - 1;
  size_t dst = readVarint(&self->ip);
  ptrdiff_t offset = readI32(&self->ip);
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

// 0x14 ADC r<carry>, r<dst>, r<src>
static inline
void adc(Machine* self) {
  size_t carry = readVarint(&self->ip);
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t val0 = self->top->r[dst].bits;
  uintptr_t val = val0 + self->top->r[src].bits
                + (self->top->r[carry].bits ? 1 : 0);
  self->top->r[dst].bits = val;
  self->top->r[carry].bits = val < val0 ? 1 : 0;
}

// 0x16 SBB r<borrow>, r<dst>, r<src>
static inline
void sbb(Machine* self) {
  size_t borrow = readVarint(&self->ip);
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t val0 = self->top->r[dst].bits;
  uintptr_t val = val0 - self->top->r[src].bits
                - (self->top->r[borrow].bits ? 1 : 0);
  self->top->r[dst].bits = val;
  self->top->r[borrow].bits = val > val0 ? 1 : 0;
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


// 0x40 NEW r<dst>, r<src>
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

// 0x42 RNEW r<ptr>, r<src>
// Reallocate a `NEW`-allocated pointer to be a new size.
static inline
void vmRealloc(Machine* self) {
  size_t ptr = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[ptr].bptr = realloc(self->top->r[ptr].bptr, self->top->r[src].bits);
}



// 0x44 MMOV r<dst>, r<src>, r<len>
// Copy len bytes from src to dst.
//
// This works even when the src/dst memory regions overlap.
static inline
void memMove(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  size_t len = readVarint(&self->ip);
  memmove(self->top->r[dst].bptr, self->top->r[src].bptr, self->top->r[len].bits);
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

// 0x51 NOT r<dst>, r<src>
// Logical not.
//
// Store 1 in dst if src is zero, esle store 0 in dst.
static inline
void not(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits = (self->top->r[src].bits == 0) ? 1 : 0;
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
 Conditioned Operations
 ************************************/

// 0x60 CMOV r<cond>, r<dst>, r<src>
// Move src to dst when cond is non-zero.
static inline
void cmov(Machine* self) {
  size_t cond = readVarint(&self->ip);
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  if (self->top->r[cond].bits != 0) {
    self->top->r[dst].bits = self->top->r[src].bits;
  }
}

// 0x61 CMOV r<cond>, r<dst>, imm<src>
// Move src to dst when cond is non-zero.
static inline
void cmovi(Machine* self) {
  size_t cond = readVarint(&self->ip);
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  if (self->top->r[cond].bits != 0) {
    self->top->r[dst].bits = imm;
  }
}

// 0x62 ZMOV r<cond>, r<dst>, r<src>
// Move src to dst when cond is zero.
static inline
void zmov(Machine* self) {
  size_t cond = readVarint(&self->ip);
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  if (self->top->r[cond].bits == 0) {
    self->top->r[dst].bits = self->top->r[src].bits;
  }
}

// 0x63 ZMOV r<cond>, r<dst>, imm<src>
// Move src to dst when cond is zero.
static inline
void zmovi(Machine* self) {
  size_t cond = readVarint(&self->ip);
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  if (self->top->r[cond].bits == 0) {
    self->top->r[dst].bits = imm;
  }
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

// 0x71 JMP imm<off>
// unconditional jump
static inline
void jump(Machine* self) {
  byte* here = self->ip - 1;
  ptrdiff_t offset = readI32(&self->ip);
  self->ip = here + offset;
}

// 0x72 CJMP r<cond>, i32<offset>
// if cond is non-zero, jump to `start address of this instruction + offset`
static inline
void cjump(Machine* self) {
  byte* here = self->ip - 1;
  size_t cond = readVarint(&self->ip);
  ptrdiff_t offset = readI32(&self->ip);
  if (self->top->r[cond].bits) {
    self->ip = here + offset;
  }
}

// 0x73 ZJMP r<cond>, i32<offset>
// if cond is zero, jump to `start address of this instruction + offset`
static inline
void zjump(Machine* self) {
  byte* here = self->ip - 1;
  size_t cond = readVarint(&self->ip);
  ptrdiff_t offset = readI32(&self->ip);
  if (!self->top->r[cond].bits) {
    self->ip = here + offset;
  }
}



/************************************
 Subroutines
 ************************************/

// 0x80 JAL r<tgt>, imm<n>, n * r<src>
// Jump and link to address stored in tgt register.
//
// As 0x81, but with a register source rather than an immediate offset.
static inline
void jalr(Machine* self) {
  // accumulate information about callee
  size_t reg = readI32(&self->ip);
  byte* tgt = self->top->r[reg].bptr;
  size_t calleeSize_words = readU32(&tgt);
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

// 0x81 JAL i32<offset>, imm<n>, n * r<src>
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
  ptrdiff_t offset = readI32(&self->ip);
  byte* tgt = here + offset;
  size_t calleeSize_words = readU32(&tgt);
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

// 0x82 JAR r<tgt>, imm<n>, n * r<src>
// Jump and re-link to address stored in tgt register.
//
// As 0x83, but with a register source rather than an immediate offset.
static inline
void jarr(Machine* self) {
  size_t reg = readI32(&self->ip);
  byte* tgt = self->top->r[reg].bptr;
  size_t calleeSize_words = readU32(&tgt);
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

// 0x83 JAR
// "Jump-and-relink".
// As JAL, but the next stack frame will return not to this frame, but the previous one.
// That is, this implements a tail call.
static inline
void jar(Machine* self) {
  byte* here = self->ip - 1;
  ptrdiff_t offset = readI32(&self->ip);
  byte* tgt = here + offset;
  size_t calleeSize_words = readU32(&tgt);
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

// 0xC0 STRM r<dst> imm<id>
// Load a file handle into the destination based on the id: either
//   standard input (id = 0),
//   standard outut (id = 1), or
//   standard error (id = 3).
// Other values of id are leave the destination register undefined
static inline
void strm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t id = readVarint(&self->ip);
  FILE* res;
  switch (id) {
    case 0: res = stdin; break;
    case 1: res = stdout; break;
    case 2: res = stderr; break;
    default: res = NULL; break;
  }
  self->top->r[dst].fptr = res;
}

// 0xC1 ENV r<dst>, r<src>
// Lookup the variable name from src in the process' environment.
// Store pointer to the result in the dst register.
// Both strings are NUL-terminated.
// If the environment does not define the given variable, the dst is set to zero.


// 0xC2 ARGC r<dst>
// Load number of arguments (including bytecode filepath) into dst.
static inline
void getArgc(Machine* self) {
  size_t dst = readVarint(&self->ip);
  self->top->r[dst].bits = self->environ.argc;
}

// 0xC3 ARGV r<dst>, r<ix>
// Create a handle to a copy of the ix-th argument.
// The handle is two words: a length and a pointer to a NUL-terminated string of that length.
// The pointer is freshly-allocated, so should be freed when finished with it.
// The handle is written to the contents of the pointer stored in dst.
static inline
void getArgv(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t ix = readVarint(&self->ip);
  char* nulstrp = self->environ.argv[self->top->r[ix].bits];
  size_t len = strlen(nulstrp);
  byte* new = malloc(len + 1);
  memcpy(new, nulstrp, len+1);
  word* tgt = self->top->r[dst].wptr;
  tgt[0].bits = len;
  tgt[1].bptr = new;
}

// 0xD0 OPEN imm<mode>, r<dst>, r<src>
// Opens the file names in src and places the file pointer in dst.
// The src register should have a pointer to a NUL-terminated string.
// The mode argument should be:
//    0 for read,
//    1 for write (create file if it doesn't exist),
//    2 for read/write (create file if it doesn't exist).
// If there is an error, zero is stored in dst.
static inline
void openFile(Machine* self) {
  size_t mode = readVarint(&self->ip);
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  switch (mode) {
    case 0: {
      self->top->r[dst].fptr = fopen((char*)self->top->r[src].bptr, "r");
    } break;
    case 1: {
      self->top->r[dst].fptr = fopen((char*)self->top->r[src].bptr, "w");
    } break;
    case 2: {
      FILE* file = fopen((char*)self->top->r[src].bptr, "a+");
      fseek(file, 0, SEEK_SET);
      self->top->r[dst].fptr = file;
    } break;
    default: {
      self->top->r[dst].fptr = NULL;
    }; break;
  }
}

// 0xD1 CLOS r<fp>
// Close a file.
// This may or may not successfully flush any buffered output.
static inline
void closeFile(Machine* self) {
  size_t fp = readVarint(&self->ip);
  fclose(self->top->r[fp].fptr);
}

// 0xD2 GET r<dst>, r<fp>, r<src>
// Read bytes from the file handle in fp to a buffer pointed to by dst.
// The src register contains the maximum number of bytes that should be read.
// At the end of the operation, store the number of bytes actually read in src.
// If there was an error, store `-read_bytes - 1` in src.
static inline
void getBytes(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t fp = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  FILE* file = self->top->r[fp].fptr;
  size_t req_read = self->top->r[src].bits;
  size_t real_read = fread(self->top->r[dst].bptr, 1, req_read, file);
  if (req_read == real_read) {
    self->top->r[src].sbits = real_read;
  }
  else if (feof(file)) {
    self->top->r[src].sbits = real_read;
    clearerr(file);
  }
  else {
    self->top->r[src].sbits = -real_read - 1;
  }
}

// 0xD3 PUT r<fp>, r<src>
// Write bytes from src to a file handle stored in fp.
// The src register should contain a pointer to a packed struct with:
//   a word specifying how many bytes to write, and
//   a pointer to the bytes to be written.
// The number of bytes actually written is then stored in src.
static inline
void putBytes(Machine* self) {
  size_t fp = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  word* str = self->top->r[src].wptr;
  size_t written = fwrite(str[1].bptr, 1, str[0].bits, self->top->r[fp].fptr);
  self->top->r[src].sbits = written;
  }

// 0xD4 GETB r<dst>, r<fp>
// Read a byte from the file pointer and store it in dst.
// If at the end of the file, store 256 in dst.
// If an error occured, store a value less than 0 in dst.
static inline
void getByte(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t fp = readVarint(&self->ip);
  FILE* file = self->top->r[fp].fptr;
  int res = getc(file);
  if (res == EOF) {
    if (feof(file)) {
      self->top->r[dst].sbits = 256;
      clearerr(file);
    }
    else {
      self->top->r[dst].sbits = -2;
      clearerr(file);
    }
  }
  else {
    self->top->r[dst].sbits = res;
  }
}

// 0xD5 PUTB r<fp>, r<src>
// Write the low byte from the src register to the file pointer.
// On error, store a value less than zero in src.
static inline
void putByte(Machine* self) {
  size_t fp = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  int res = putc(self->top->r[src].byte.low, self->top->r[fp].fptr);
  if (res == EOF) {
    self->top->r[src].sbits = -2;
  }
}

// 0xD7 FLUS r<fp>, r<err>
// Flush any buffered output for the file.
// Store 1 in err if there is an error, otherwise store 0 there.
static inline
void flushFile(Machine* self) {
  size_t fp = readVarint(&self->ip);
  size_t err = readVarint(&self->ip);
  if (fflush(self->top->r[fp].fptr) != EOF) {
    self->top->r[err].bits = 0;
  }
  else {
    self->top->r[err].bits = 1;
  }
}

// 0xD8 TELL r<fp>, r<dst>
// Store location within file into dst., or -1 on error.
static inline
void tellFile(Machine* self) {
  size_t fp = readVarint(&self->ip);
  size_t dst = readVarint(&self->ip);
  self->top->r[dst].sbits = ftell(self->top->r[fp].fptr);
}

// 0xD9 SEEK imm<whence>, r<fp>, r<src>
// Adjust position in file by a signed offset.
// The new position depends on the value of whence:
//    0 — the value in src,
//    1 — current position plus src, or
//    2 — end of file plus src.
// If there is an error, store 1 in src, else 0.
static inline
void seekFile(Machine* self) {
  int whence = readVarint(&self->ip);
  size_t fp = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  switch (whence) {
    case 0: whence = SEEK_SET; break;
    case 1: whence = SEEK_CUR; break;
    case 2: whence = SEEK_END; break;
    default: {
      self->top->r[src].bits = 1;
    } return;
  }
  int res = fseek(self->top->r[fp].fptr, self->top->r[src].sbits, whence);
  self->top->r[src].bits = (res == 0) ? 0 : 1;
}
