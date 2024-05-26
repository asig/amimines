#ifndef MEM_H
#define MEM_H

#include <proto/exec.h>

void memRegister(void *mem, ULONG sz);
void memFreeAll();

#endif