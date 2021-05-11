#include "common.h"
#include <stdio.h>

#include "types.h"
#include "loader.h"
#include "execute.h"


int main(int argc, char* argv[]) {
  if (argc < 1) {
    fprintf(stderr, "usage: bsvm <bytecode file> <args to program...>\n");
    return 1;
  }
  Program prog;
  Machine machine;
  readProgram(&prog, argv[1]);
  initMachine(&machine, &prog);
  fprintf(stdout, "executing...\n");
  while(!machine.shouldHalt) {
    cycle(&machine);
  }
  destroyMachine(&machine);
  free(machine.program->code);
  return machine.exitcode;
}
