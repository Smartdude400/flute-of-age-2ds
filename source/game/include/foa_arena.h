// adapted from flute-of-age/include/foa_arena.h (ultra64.h -> game_types.h)
#ifndef FOA_ARENA_H
#define FOA_ARENA_H

#include "game_types.h"

extern s32 gFoaArenaLogSeverity;

void* FoaArena_Malloc(u32 size);
void* FoaArena_MallocR(u32 size);
void* FoaArena_Realloc(void* ptr, u32 newSize);
void FoaArena_Free(void* ptr);
void* FoaArena_Calloc(u32 num, u32 size);
void FoaArena_GetSizes(u32* outMaxFree, u32* outFree, u32* outAlloc);
void FoaArena_Check(void);
void FoaArena_Init(void* start, u32 size);
void FoaArena_Cleanup(void);
s32 FoaArena_IsInitialized(void);

#if DEBUG_FEATURES
void FoaArena_CheckPointer(void* ptr, u32 size, const char* name, const char* action);
void* FoaArena_MallocDebug(u32 size, const char* file, int line);
void* FoaArena_MallocRDebug(u32 size, const char* file, int line);
void* FoaArena_ReallocDebug(void* ptr, u32 newSize, const char* file, int line);
void FoaArena_FreeDebug(void* ptr, const char* file, int line);
void FoaArena_Display(void);

#define FOA_ARENA_MALLOC(size, file, line) FoaArena_MallocDebug(size, file, line)
#define FOA_ARENA_MALLOC_R(size, file, line) FoaArena_MallocRDebug(size, file, line)
#define FOA_ARENA_FREE(size, file, line) FoaArena_FreeDebug(size, file, line)

#else

#define FOA_ARENA_MALLOC(size, file, line) FoaArena_Malloc(size)
#define FOA_ARENA_MALLOC_R(size, file, line) FoaArena_MallocR(size)
#define FOA_ARENA_FREE(size, file, line) FoaArena_Free(size)

#endif

#endif
