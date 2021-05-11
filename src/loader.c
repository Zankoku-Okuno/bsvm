#include "common.h"
#include "types.h"
#include <stdio.h>

#include "loader.h"


int readProgram(Program* out, const char* filename) {
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) { goto badexit; }
  // first 8 bytes are a magic number
  {
    char magic[8];
    size_t read_bytes = fread(magic, 1, 8, fp);
    if (read_bytes != 8) { goto badexit; }
    if (strncmp((const char*)&magic, "BsvmExe1", 8) != 0) { goto badexit; }
  }
  // next 4 bytes hold a the size of the code section in bytes, big-endian
  {
    size_t codeSize = 0;
    for (int i = 0; i < 4; ++i) {
      byte c = fgetc(fp);
      if (feof(fp)) { goto badexit; }
      codeSize = (codeSize << 8) + c;
    }
    out->codeSize_bytes = codeSize;
  }
  // next 4 bytes hold the byte offset into the code section of the entrypoint, big-endian
  {
    size_t entrypoint = 0;
    for (int i = 0; i < 4; ++i) {
      byte c = fgetc(fp);
      if (feof(fp)) { goto badexit; }
      entrypoint = (entrypoint << 8) + c;
    }
    out->entrypoint = entrypoint;
  }
  // TODO more header info (size/location of symtab and comments
  // next `out->codeSize_bytes` is the bytecode
  {
    byte* codebuf = malloc(sizeof(byte) * out->codeSize_bytes);
    size_t read_bytes = fread(codebuf, 1, out->codeSize_bytes, fp);
    if (read_bytes != out->codeSize_bytes) { free(codebuf); goto badexit; }
    out->code = codebuf;
  }
  fclose(fp);
  return 0;
  badexit: {
    fclose(fp);
    return -1;
  }
}

void fputProgram(FILE* fp, const Program* prog) {
  fprintf(fp, "Program {\n");
  fprintf(fp, "  codeSize_bytes = %ld\n", prog->codeSize_bytes);
  fprintf(fp, "  entrypoint = %ld\n", prog->entrypoint);
  fprintf(fp, "}\n");
}
