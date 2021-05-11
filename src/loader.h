#ifndef LOADER_H
#define LOADER_H

#include "types.h"



int readProgram(Program* out, const char* filename);

void fputProgram(FILE* fp, const Program* prog);

#endif
