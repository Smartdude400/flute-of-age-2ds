// engine/rom_bind.c (rewritten: native) - see rom_bind.h
#include <stddef.h>
#include "engine/rom_bind.h"

#define MAX_BINDINGS 8
static struct { const void* dst; uintptr_t vrom; u32 size; } sBind[MAX_BINDINGS];

void RomBind_Set(const void* dst, uintptr_t vromStart, u32 size) {
    int slot = -1;
    for (int i = 0; i < MAX_BINDINGS; i++) {
        if (sBind[i].dst == dst) {
            slot = i;
            break;
        }
        if (sBind[i].dst == NULL && slot < 0) {
            slot = i;
        }
    }
    if (slot < 0) {
        slot = 0;
    }
    sBind[slot].dst = dst;
    sBind[slot].vrom = vromStart;
    sBind[slot].size = size;
}

uintptr_t RomBind_Get(const void* dst, u32* size) {
    for (int i = 0; i < MAX_BINDINGS; i++) {
        if (sBind[i].dst == dst) {
            if (size != NULL) {
                *size = sBind[i].size;
            }
            return sBind[i].vrom;
        }
    }
    return 0;
}
