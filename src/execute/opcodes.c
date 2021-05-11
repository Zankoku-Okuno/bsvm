// This file is meant to be included only by `../execute.c`.
// It also serves as documentation of the ISA.


// HALT 0x00
// immediately exit with error code -1 (255)
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


static inline
void moveImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  word imm = readVarint(&self->ip);
  self->top->r[dst] = imm;
}


static inline
void addImm(Machine* self) {
  size_t dst = readVarint(&self->ip);
  word imm = readVarint(&self->ip);
  self->top->r[dst] += imm;
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
  byte* tgt = (byte*)self->top->r[link];
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
  self->exitcode = self->top->r[ecReg];
  self->shouldHalt = true;
}

// used internally for testing while I develop
static inline
void test(Machine* self) {
  word src = readVarint(&self->ip);
  fprintf(stdout, "result is %lx\n", self->top->r[src]);
}
