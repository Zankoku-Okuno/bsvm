// This file is meant to be included only by `../execute.c`.
// It also serves as documentation of the ISA.


// HCF 0x00
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

// MOV 0x02 reg<dst> reg<src>
static inline
void move(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst] = self->top->r[src];
}

// MOVI 0x03 reg<dst> imm<src>
static inline
void moveImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits = imm;
}

// LD 0x06 reg<dst> reg<src>
// Load a word from the address in src and place it in dst.
static inline
void load(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst] = *self->top->r[src].wptr;
}

// ST 0x06 reg<dst> reg<src>
// Store contents of the src register into memory pointed to by dst register.
static inline
void store(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  *self->top->r[dst].wptr = self->top->r[src];
}


static inline
void add(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits += self->top->r[src].bits;
}

static inline
void addImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits += imm;
}

static inline
void sub(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits -= self->top->r[src].bits;
}

static inline
void subImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  uintptr_t imm = readVarint(&self->ip);
  self->top->r[dst].bits -= imm;
}

// 0x14 MUL reg<dst> reg<src>
// unsigned single-width multiply
static inline
void mul(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].bits *= self->top->r[src].bits;
}

// 0x15 DMUL reg<dst-high> reg<dst-low> reg<src>
// unsigned double-width multiply
// dstHigh:dstLow <- dstLow * src
static inline
void dmul(Machine* self) {
  size_t dstHigh = readVarint(&self->ip);
  size_t dstLow = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  ulong a = self->top->r[dstLow].bits;
  ulong b = self->top->r[src].bits;
  ulong r = a * b;
  self->top->r[dstLow].bits = (uintptr_t)r;
  self->top->r[dstHigh].bits = (uintptr_t)(r >> sizeof(uintptr_t));
}

// 0x16 IMUL reg<dst> reg<src>
// signed single-width multiply
static inline
void imul(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  self->top->r[dst].sbits *= self->top->r[src].sbits;
}

// 0x17 DIMUL reg<dst-high> reg<dst-low> reg<src>
// signed double-width multiply
static inline
void dimul(Machine* self) {
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

// 0x1D DIVREM reg<dst> reg<rem> reg<src>
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

// 0x1D DIVMOD reg<dst> reg<rem> reg<src>
// unsigned divide with remainder
// dst <- dst / src ; rem <- dst % src
static inline
void divmod(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t rem = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  uintptr_t numer = self->top->r[dst].bits;
  uintptr_t denom = self->top->r[src].bits;
  // FIXME check that denom /= 0, but then what?
  uintptr_t d = numer / denom;
  uintptr_t r = numer % denom;
  self->top->r[dst].bits = d;
  self->top->r[rem].bits = r;
}

// 0x1F IDIVMOD reg<dst> reg<rem> reg<src>
// signed divide with remainder
static inline
void idivmod(Machine* self) {
  size_t dst = readVarint(&self->ip);
  size_t rem = readVarint(&self->ip);
  size_t src = readVarint(&self->ip);
  intptr_t numer = self->top->r[dst].sbits;
  intptr_t denom = self->top->r[src].sbits;
  // FIXME check that denom /= 0, but then what?
  intptr_t d = numer / denom;
  intptr_t r = numer % denom;
  // since C is round-to-zero, but modular arithmetic woudl prefer round-to-minus-infinity
  if (r < 0) {
    d -= 1;
    r += denom;
  }
  self->top->r[dst].sbits = d;
  self->top->r[rem].sbits = r;
}

// 0x81 JAL word<offset>, imm<n>, n * reg<src> 
// Jump and link to `ip + offset`.
//
// Save the address of the next instruction into the callee's register zero.
// Copy the contents of the n src registerrs of the caller into registers 1-n of
// the callee. Jump to `ip + offset`.
//
// As with all functions, the target address should be a word stating the size
// of the callee's stack frame. The actuall code that is entered should occur
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

// 0x84 RET reg<link>, imm<n>, n * reg<src...>
// Initialize n return values from <src...> registers, then jump to the contents
// of the <link> register and destroy this stack frame.
//
// Internally, there's a machine-wide array of return values (all word-sized).
// This is grown as necessary, then by reading from the callee frame. This
// instruction cleans up the callee frame immediately. The address returned to
// should be an `into` instruction, unless the caller needs none of the return
// values.
static inline
void ret(Machine* self) {
  // determine jump location
  size_t link = readVarint(&self->ip);
  byte* tgt = self->top->r[link].bptr;
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

// 0x85 INTO imm<n>, n * reg<dst>
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

// 0x86 EXIT reg<src>
// Stop the virtual machine, exiting with the error code stored in src.
static inline
void _exit(Machine* self) {
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
