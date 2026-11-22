// (rewritten: native)
#pragma once
#include <stddef.h>
// Native replacement of the original "system arena" heap. Backed by the 2DS linear (GPU-visible) heap,
// so game memory that holds converted geometry (object space, room buffers) can be read by the GPU
// directly, as the original's RSP read vertices from RDRAM. Allocations are zero-filled so results
// never depend on stale memory contents.
void* SystemArena_Malloc(size_t size);
void SystemArena_Free(void* ptr);
size_t SystemArena_HighWater(void);
