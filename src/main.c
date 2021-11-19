#include "common.h"

#include "types.h"
#include "loader.h"
#include "execute.h"


int main(int argc, char** argv) {
  if (argc < 1) {
    fprintf(stderr, "usage: bsvm <bytecode file> <args to program...>\n");
    return 1;
  }
  Program prog;
  Machine machine;
  // fprintf(stderr, "reading...\n");
  if (readProgram(&prog, argv[1])) {
    fprintf(stderr, "[ERROR] when reading program\n");
    return -1;
  }
  // fprintf(stderr, "initializing...\n");
  initMachine(&machine, &prog, argc-1, argv+1);
  // fprintf(stderr, "executing...\n");
  // TODO I'm debating whether to use longjmp instead of testing a boolean every time
  while(!machine.shouldHalt) {
    // fprintf(stderr, "[INFO] %08lx: %02x\n", machine.ip - prog.code, *machine.ip);
    cycle(&machine);
  }
  destroyMachine(&machine);
  free(machine.program->code);
  return machine.exitcode;
}
