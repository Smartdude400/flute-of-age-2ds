// engine/system_arena.c (rewritten: native) - see system_arena.h
#include <3ds.h>
#include <string.h>
#include "engine/system_arena.h"

static size_t sInUse, sHighWater;

void* SystemArena_Malloc(size_t size) {
    void* p = linearMemAlign(size, 0x80);
    if (p != NULL) {
        memset(p, 0, size);
        sInUse += linearGetSize(p);
        if (sInUse > sHighWater) {
            sHighWater = sInUse;
        }
    }
    return p;
}

void SystemArena_Free(void* ptr) {
    if (ptr != NULL) {
        sInUse -= linearGetSize(ptr);
        linearFree(ptr);
    }
}

size_t SystemArena_HighWater(void) {
    return sHighWater;
}
