// adapted from flute-of-age/include/memory_utils.h (ultra64.h -> game_types.h)
#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include "game_types.h"

void* MemCopy(void* dest, void* src, s32 size);
void* MemSet(void* dest, s32 val, s32 size);

#endif
