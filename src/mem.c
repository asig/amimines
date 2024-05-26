#include "mem.h"

#include <proto/exec.h>

struct Block {
    void *mem;
    ULONG sz;
    void *next;
} Block;

struct Block *memRoot = NULL;

void memregister(void *mem, ULONG sz) {
    struct Block *b = AllocMem(sizeof(Block), 0);
    b->mem = mem;
    b->sz = sz;
    b->next = memRoot;
    memRoot = b;
}

void memFreeAll() {
    struct Block *cur = memRoot;
    while (cur) {
        struct Block *next = cur->next;
        FreeMem(cur->mem, cur->sz);
        FreeMem(cur, sizeof(struct Block));
        cur = next;
    }
    memRoot = NULL;
}
