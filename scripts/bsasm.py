#!/usr/bin/env python3

import sys
import re
from os import path

def main():
  # FIXME not every argument is a file name
  asm = Asm()
  try:
    for fname in sys.argv[1:]:
      with open(fname, "rt") as fp:
        asm.file = fname
        asm.lineno = 0
        for line in fp.readlines():
          asm.asmLine(line)
    asm.finalize_function()
    for listener in asm.rewrites.values():
      for off, stuff in listener.items():
        size, endianness, expr = stuff
        try:
          val = expr(asm)
        except ForwardReference as exn:
          name, _ = exn.args
          raise AsmExn("undefined label: {}".format(name))
        asm.code[off:off+size] = val.to_bytes(size, endianness, signed=True)
  except AsmExn as exn:
    print("{} line {}: {}".format(asm.file, asm.lineno, exn), file=sys.stderr)
    exit(1)
  with open(path.splitext(fname)[0] + ".bsvm", "wb") as fp:
    fp.write(b"BsvmExe1")
    fp.write(len(asm.code).to_bytes(4, 'big'))
    fp.write((asm.entrypoint or 0).to_bytes(4, 'big'))
    fp.write(asm.code)
  print("ding!")

class AsmExn(Exception):
  pass

class Asm:
  def __init__(self):
    self.wordSize = None
    self.endianness = None
    self.lbltab = dict()
    self.consttab = dict()
    self.regtab = None
    # info about current location
    self.offset = 0
    self.file = None
    self.lineno = 0
    self.functionName = None
    self.functionAddr = None
    self.functionSize = None
    self.prevLbl = None
    # output
    self.entrypoint = None
    self.code = bytearray(b"")
    self.rewrites = dict() # Map[WaitOnLabelName, Map[Offset, Expr]]

  def asmLine(self, line):
    self.lineno += 1
    line = line.rstrip()
    if not line or re.match(r'\s*;', line):
      return
    elif line[0] == ' ':
      self.asmInstruction(line.lstrip())
    elif line[0] == '.':
      self.asmDirective(line[1:])
    else:
      self.asmLabel(line)

  LABEL_RE = r"(@?)([a-zA-Z0-9._-]+):"
  def asmLabel(self, line):
    # make sure it's well-formed
    m = re.fullmatch(Asm.LABEL_RE, line)
    if not m:
      raise AsmExn("invalid syntax")
    # extract the label name (expanding leading @-sign)
    lblname = m.group(2)
    if m.group(1):
      if not self.functionName:
        raise AsmExn("local label outside function")
      lblname = self.functionName + '.' + lblname
    else:
      self.prevLbl = lblname
    # add to label table
    self.add_label(lblname)
  def add_label(self, lblname):
    if lblname in self.lbltab:
      raise AsmExn("duplicate label {}".format(repr(lblname)))
    self.lbltab[lblname] = self.offset


  def asmDirective(self, line):
    # parse the directive apart
    directive = line.split(' ')[0]
    args = line[len(directive):].lstrip()
    try:
      f = getattr(self, "DIR_" + directive)
    except AttributeError:
      raise AsmExn("unknown directive .{}".format(directive))
    f(args)

  def asmInstruction(self, line):
    opcode = line.split(' ')[0]
    args = [ arg.strip() for arg in line[len(opcode):].split(',') if arg.strip() ]
    # print("{}: {}".format(opcode, repr(args))) # TODO
    try:
      f = getattr(self, "OP_" + opcode.lower())
    except AttributeError:
      raise AsmExn("unknown instruction {}".format(repr(opcode)))
    f(*args)
  
  def append(self, code):
    self.code += code
    self.offset += len(code)
  def suspend(self, waitOn, expr, size=4, endianness='big'):
    if waitOn not in  self.rewrites:
      self.rewrites[waitOn] = dict()
    self.rewrites[waitOn][self.offset] = (size, endianness, expr)
    self.append((0).to_bytes(size, 'big'))
  # TODO fill placeholder

  def arg(self, text, allow):
    # print("{} :: {}".format(repr(text), type(text)))
    if 'r' in allow:
      if re.match(r"^%[0-9]+$", text):
        r = int(text[1:])
        self.functionSize = max(self.functionSize, r + 1)
        return ('r', r)
      if re.match(r"^[a-zA-Z0-9._-]+$", text):
        if text in self.regtab:
          return ('r', self.regtab[text])
    if 'i' in allow:
      try:
        e = self.parseExpr(text)
      except ParseExn as exn:
        raise AsmExn("parse error: {}".format(*exn.args))
      return 'i', e(self)
    raise AsmExn("bad argument (expecting {}): {}".format(allow, text))
  def finalize_function(self):
    if self.functionAddr is not None:
      self.code[self.functionAddr:self.functionAddr+4] \
        = self.functionSize.to_bytes(4, 'big')

  def DIR_wordsize(self, args):
    if self.wordSize is not None:
      raise AsmExn("word size is already defined")
    if args == '8':
      self.wordSize = 8
    elif args == '4':
      self.wordSize = 4
    else:
      raise AsmExn("unsupported word size: {}".format(args))
    self.consttab["__wordSize__"] = self.wordSize
  def DIR_endian(self, args):
    if self.endianness is None:
      self.endianness = args
    else:
      raise AsmExn("endianness is already defined")
  def DIR_func(self, args):
    # finalize last function
    self.finalize_function()
    # extract arguments
    args = args.strip().split(' ')
    if len(args) == 0:
      raise AsmExn("missing function name")
    else:
      name, params = args[0], args[1:]
    # define function name/label
    if re.match(r"^[a-zA-Z0-9._-]+$", name):
      self.functionName = name
      self.add_label(name)
    else:
      raise AsmExn("bad function name: {}".format(name))
    # set up a suspended function header
    self.functionSize = 1 + len(params)
    self.functionAddr = self.offset
    self.append((0).to_bytes(4, 'big'))
    # initialize register names for parameters
    self.regtab = dict()
    for i, param in enumerate(params):
      if param == "_":
        pass
      elif re.match(r"^[a-zA-Z0-9._-]+$", param):
        self.regtab[param] = i + 1
  def DIR_def(self, args):
    tmp = args.strip().split(" ")
    if len(tmp) < 2:
      raise AsmExn("bad definition")
    name = tmp[0]
    body = args[len(name):].strip()
    _, value = self.arg(body, 'i')
    self.consttab[name] = value
  def DIR_word(self, args):
    args = [arg.strip() for arg in args.split(',')]
    for arg in args:
      try:
        _, imm = self.arg(arg, 'i')
        fref = None
      except ParseExn as exn:
        raise AsmExn("parse error: {}".format(*exn.args))
      except ForwardReference as exn:
        fref, expr = exn.args
      if self.wordSize is None:
        raise AsmExn("unknown word size")
      if self.endianness is None:
        raise AsmExn("unknown endianness")
      if fref is None:
        self.append(imm.to_bytes(self.wordSize, self.endianness, signed=True))
      else:
        self.suspend(fref, expr, size=self.wordSize, endianness=self.endianness)
  def DIR_ascii(self, args):
    text = ""
    while True:
      m = re.match(r"'((?:[^']+|'')+)'", args)
      if m:
        text += re.sub("''", "'", m.group(1))
        args = args[len(m.group(0)):].lstrip()
        if args and args[0] == ',':
          args = args[1:].lstrip()
          continue
        else:
          break
      m = re.match(r"[0-9]+", args)
      if m:
        text += chr(int(m.group(0)))
        args = args[len(m.group(0)):].lstrip()
        if args and args[0] == ',':
          args = args[1:].lstrip()
          continue
        else:
          break
      raise AsmExn("invalid ascii syntax: {}".format(repr(args)))
    self.append(text.encode('ascii'))

  def OP_hcf(self):
    self.append(b"\x00")
  # 0x01
  ###### Byte Pushing ######
  def OP_mov(self, a, b): self.op_reg_regimm(a, b, whenReg=0x02, whenImm=0x03)
  def OP_ld(self, a, b): self.op_reg_reg(a, b, 0x04)
  def OP_st(self, a, b): self.op_reg_reg(a, b, 0x05)
  # 0x06–0x09
  def OP_lea(self, a, b): self.op_reg_reg(a, b, 0x0A)
  def OP_lia(self, a, b): self.reg_off(a, b, opcode=0x0B)
  def OP_ldb(self, a, b): self.op_reg_reg(a, b, 0x0C)
  # 0x0D
  def OP_stb(self, a, b): self.op_reg_reg(a, b, 0x0E)
  # 0x0F
  ###### Arithmetic ######
  def OP_add(self, a, b): self.op_reg_regimm(a, b, whenReg=0x10, whenImm=0x11)
  def OP_sub(self, a, b): self.op_reg_regimm(a, b, whenReg=0x12, whenImm=0x13)
  def OP_adc(self, a, b, c): self.op_reg_reg_reg(a, b, c, 0x14)
  # 0x15
  def OP_sbb(self, a, b, c): self.op_reg_reg_reg(a, b, c, 0x16)
  def OP_neg(self, a, b): self.op_reg_reg(a, b, 0x17)
  def OP_mul(self, a, b): self.op_reg_reg(a, b, 0x18)
  def OP_muc(self, a, b, c): op_reg_reg_reg(a, b, c, 0x19)
  def OP_imul(self, a, b): self.op_reg_reg(a, b, 0x1A)
  def OP_imuc(self, a, b, c): op_reg_reg_reg(a, b, c, 0x1B)
  def OP_div(self, a, b): self.op_reg_reg(a, b, 0x1C)
  def OP_dvr(self, a, b, c): op_reg_reg_reg(a, b, c, 0x1D)
  def OP_idiv(self, a, b): self.op_reg_reg(a, b, 0x1E)
  def OP_idvr(self, a, b, c): op_reg_reg_reg(a, b, c, 0x1F)
  ###### Bit Fiddling ######
  # 0x20–0x2F
  def OP_or(self, a, b): self.op_reg_regimm(a, b, whenReg=0x30, whenImm=0x31)
  def OP_xor(self, a, b): self.op_reg_regimm(a, b, whenReg=0x32, whenImm=0x33)
  def OP_and(self, a, b): self.op_reg_regimm(a, b, whenReg=0x34, whenImm=0x35)
  # 0x36
  def OP_inv(self, a, b): self.op_reg_reg(a, b, 0x37)
  def OP_szr(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x38, whemImm=0x39)
  def OP_sar(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x3A, whemImm=0x3B)
  def OP_shl(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x3C, whemImm=0x3D)
  def OP_rot(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x3E, whemImm=0x3F)
  ###### Memory Buffers ######
  def OP_new(self, a, b): self.op_reg_reg(a, b, 0x40)
  def OP_free(self, a):
    _, src = self.arg(a, 'r')
    self.append(b"\x41" + mkVarint(src))
  # 0x42–4F
  ###### Comparisons ######
  def OP_bit(self, a, b, c):
    _, bit = self.arg(a, 'i')
    _, dst = self.arg(b, 'r')
    _, src = self.arg(b, 'r')
    if bit > 255:
      raise AsmExn("bit index out of bounds: {}".format(bit))
    self.append(b"\x50" + bit.to_bytes(1, 'big') + mkVarint(dst) + mkVarint(src))
  # 0x51
  def OP_any(self, a, *bs): self.op_reg_regs(a, *bs, opcode=0x52)
  def OP_all(self, a, *bs): self.op_reg_regs(a, *bs, opcode=0x53)
  def OP_eq(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x54, whenImm = 0x55)
  def OP_neq(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x56, whenImm = 0x57)
  def OP_bl(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x58, whenImm = 0x59)
  def OP_ble(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x5A, whenImm = 0x5B)
  def OP_lt(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x5C, whenImm = 0x5D)
  def OP_lte(self, a, b, c): self.op_reg_reg_regimm(a, b, c, whenReg=0x5E, whenImm = 0x5F)
  ###### Conditioned Operations ######
  # 0x60–0x6F
  ###### Jumps ######
  def OP_jmpr(self, a):
    _, src = self.arg(a, 'r')
    self.append(b"\x70" + mkVarint(src))
  # 0x71
  def OP_cjmp(self, a, b): self.reg_off(a, b, opcode=0x72)
  def OP_zjmp(self, a, b): self.reg_off(a, b, opcode=0x73)
  # 0x74–7F
  ###### Subroutines ######
  # 0x80
  def OP_jal(self, a, *bs): self.op_off_regs(a, *bs, opcode=0x81)
  # 0x82
  def OP_jar(self, a, *bs): self.op_off_regs(a, *bs, opcode=0x83)
  def OP_ret(self, *args): self.op_regs(*args, opcode=0x84)
  def OP_into(self, *args): self.op_regs(*args, opcode=0x85)
  def OP_exit(self, a):
    _, src = self.arg(a, 'r')
    self.append(b"\x86" + mkVarint(src))
  ###### String Operations ######
  ###### Input/Output ######
  def OP_strm(self, a, b): self.op_reg_imm(a, b, opcode=0xC0)
  def OP_fwr(self, a, b, c): self.op_reg_reg_reg(a, b, c, 0xD3)
  ###### Done wth Opcodes ######

  def op_reg_reg(self, a, b, opcode):
    _, dst = self.arg(a, 'r')
    _, src = self.arg(b, 'r')
    self.append(opcode.to_bytes(1, 'big') + mkVarint(dst) + mkVarint(src))
  def op_reg_regimm(self, a, b, *, whenReg, whenImm):
    _, dst = self.arg(a, 'r')
    sType, src = self.arg(b, 'ri')
    if sType == 'r':
      self.append(whenReg.to_bytes(1, 'big') + mkVarint(dst) + mkVarint(src))
    elif sType == 'i':
      self.append(whenImm.to_bytes(1, 'big') + mkVarint(dst) + mkVarint(src))
  def op_reg_imm(self, a, b, opcode):
    _, dst = self.arg(a, 'r')
    _, src = self.arg(b, 'i')
    self.append(opcode.to_bytes(1, 'big') + mkVarint(dst) + mkVarint(src))
  def op_reg_reg_reg(self, a, b, c, opcode):
    _, r1 = self.arg(a, 'r')
    _, r2 = self.arg(b, 'r')
    _, r3 = self.arg(c, 'r')
    self.append(opcode.to_bytes(1, 'big') + mkVarint(r1) + mkVarint(r2) + mkVarint(r3))
  def op_reg_reg_regimm(self, a, b, c, *, whenReg, whemImm):
    _, dst = self.arg(a, 'r')
    _, src = self.arg(b, 'r')
    aType, amt = self.arg(c, 'ri')
    if aType == 'r':
      self.append(whenReg.to_bytes(1, 'big') + mkVarint(dst) + mkVarint(src) + mkVarint(amt))
    elif aType == 'i':
      self.append(whemImm.to_bytes(1, 'big') + mkVarint(dst) + mkVarint(src) + mkVarint(amt))
  def op_regs(self, *args, opcode):
    n = len(args)
    srcs = [self.arg(a ,'r')[1] for a in args]
    instr = opcode.to_bytes(1, 'big') + mkVarint(n)
    for src in srcs:
      instr += mkVarint(src)
    self.append(instr)
  def op_reg_regs(self, a, *bs, opcode):
    _, dst = self.arg(a, 'r')
    n = len(bs)
    srcs = [self.arg(b ,'r')[1] for b in bs]
    instr = opcode.to_bytes(1, 'big') + mkVarint(dst) + mkVarint(n)
    for src in srcs:
      instr += mkVarint(src)
    self.append(instr)
  def reg_off(self, a, b, opcode):
    _, dst = self.arg(a, 'r')
    try:
      _, imm = self.arg(b, 'i')
      fref = None
    except ForwardReference as exn:
      fref, expr = exn.args
    if fref is None:
      imm -= self.offset
      self.append(opcode.to_bytes(1, 'big') + mkVarint(dst) + imm.to_bytes(4, 'big', signed=True))
    else:
      instrAddr = self.offset
      self.append(opcode.to_bytes(1, 'big') + mkVarint(dst))
      self.suspend(fref, lambda asm: expr(asm) - instrAddr)
  def op_off_regs(self, a, *bs, opcode):
    try:
      _, imm = self.arg(a, 'i')
      fref = None
    except ForwardReference as exn:
      fref, expr = exn.args
    n = len(bs)
    srcs = [self.arg(b, 'r')[1] for b in bs]
    instrAddr = self.offset
    self.append(opcode.to_bytes(1, 'big'))
    if fref is None:
      self.append((imm - instrAddr).to_bytes(4, 'big', signed=True))
    else:
      self.suspend(fref, lambda asm: expr(asm) - instrAddr)
    self.append(mkVarint(n))
    for src in srcs:
      self.append(mkVarint(src))
  
  def parseExpr(self, text):
    # sum ::= term ([+-] sum)?
    # term ::= e ([*/] term)>
    # e ::= int
    #    |  var
    #    |  label
    #    | "(" sum ")"
    toks = [ tok for tok in text.split(' ') if tok ]
    def takeKeyword(expect):
      nonlocal toks
      if not toks: return None
      if toks[0] == expect:
        toks = toks[1:]
        return expect
      else:
        return None
    def takeSum():
      nonlocal toks
      e = takeTerm()
      if e is None: return None
      op = takeKeyword("+")
      if op is not None:
        e2 = takeSum()
        if e2 is None:
          raise ParseExn("missing sum after plus")
        return lambda asm: e(asm) + e2(asm)
      op = takeKeyword("-")
      if op is not None:
        e2 = takeSum()
        if e2 is None:
          raise ParseExn("missing sum after minus")
        return lambda asm: e(asm) - e2(asm)
      return e
    def takeTerm():
      nonlocal toks
      e = takeExpr()
      if e is None: return None
      op = takeKeyword("*")
      if op is not None:
        e2 = takeTerm()
        if e2 is None:
          raise ParseExn("missing expression after times")
        return lambda asm: e(asm) + e2(asm)
      return e
    def takeExpr():
      nonlocal toks
      e = takeInt()
      if e is not None: return e
      e = takeVar()
      if e is not None: return e
      e = takeLabel()
      if e is not None: return e
      paren = takeKeyword("(")
      if paren is not None:
        e = takeSum()
        if e is None:
          raise ParseExn("missing expression after open paren")
        paren = takeKeyword(")")
        if paren is None:
          raise ParseExn("missing close paren")
        return e
      if not toks:
        raise ParseExn("expecting expression")
      raise ParseExn("bad token {}".format(toks[0]))
    def takeInt():
      nonlocal toks
      if not toks: return None
      tok = toks[0]
      if re.match(r"^[0-9]+$", tok):
        tok = int(tok)
      elif re.match(r"^[0-9a-fA-F]+h$", tok):
        tok = int(tok[:-1], base=16)
      else:
        return None
      toks = toks[1:]
      return lambda asm: tok
    def takeVar():
      nonlocal toks
      if not toks: return None
      tok = toks[0]
      if re.match(r"^\$[a-zA-Z0-9._-]+$", tok):
        name = tok[1:]
        toks = toks[1:]
        def out(asm):
          nonlocal name
          try:
            return asm.consttab[name]
          except KeyError:
            raise EvalExn("undefined constant {}".format(name))
        return out
    def takeLabel():
      nonlocal toks
      if not toks: return None
      tok = toks[0]
      if re.match (r"^[&@][a-zA-Z0-9._-]+$", toks[0]):
        name = tok[1:]
        if tok[0] == '@':
          if self.functionName is None:
            raise AsmExn("local label outside function")
          name = self.functionName + '.' + name
        toks = toks[1:]
      else:
        return None
      def out(asm):
        nonlocal name, wholeExpr
        try:
          return asm.lbltab[name]
        except KeyError:
          raise ForwardReference(name, wholeExpr)
      return out
    wholeExpr = takeSum()
    if wholeExpr is None:
      raise ParseExn("invalid expression")
    if toks:
      raise ParseExn("extra tokens: {}".format(toks))
    return wholeExpr

def mkVarint(n):
  out = b""
  while n >= 0x40:
    n, b = n >> 7, n & 0x7f
    out = (b + (0x80 if out else 0)).to_bytes(1, 'big') + out
  out = (n + (0x80 if out else 0)).to_bytes(1, 'big') + out
  return out

class ParseExn(Exception):
  pass
class EvalExn(Exception):
  pass
class ForwardReference(Exception):
  pass



if __name__ == "__main__":
  main()
