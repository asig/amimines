#ifndef MEM_H
#define MEM_H

#include <proto/exec.h>

void registerMem(void *mem, ULONG sz);
void freeAllMem();

#endif