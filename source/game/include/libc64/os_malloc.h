// adapted from flute-of-age/include/libc64/os_malloc.h (ultra64.h -> game_types.h)
#ifndef LIBC64_OS_MALLOC_H
#define LIBC64_OS_MALLOC_H

#include "game_types.h"

struct ArenaNode;

typedef struct Arena {
    /* 0x00 */ struct ArenaNode* head;
    /* 0x04 */ void* start;
    // N64 layout (the GameCube branch with a platform lock queue is not ported)
    /* 0x08 */ u32 size;
    /* 0x0C */ u8 allocFailures;
} Arena; // size = 0x10 (N64), size = 0x24 (GC)

typedef struct ArenaNode {
    /* 0x00 */ s16 magic;
    /* 0x02 */ s16 isFree;
    /* 0x04 */ u32 size;
    /* 0x08 */ struct ArenaNode* next;
    /* 0x0C */ struct ArenaNode* prev;
#if PLATFORM_N64 || DEBUG_FEATURES
    /* 0x10 */ const char* filename;
    /* 0x14 */ int line;
    /* 0x18 */ s32 threadId; // native: debug metadata, always 0 (single-threaded)
    /* 0x1C */ Arena* arena;
    /* 0x20 */ u64 time;     // native: debug metadata, always 0
    /* 0x28 */ u8 unk_28[0x30-0x28]; // probably padding
#endif
} ArenaNode; // size = 0x30 (N64 and GC debug), size = 0x10 (GC retail)

#if PLATFORM_N64
// native: single-threaded ARM9 game loop and no allocation from IRQ handlers -> no masking needed
#define DECLARE_INTERRUPT_MASK
#define CLEAR_INTERRUPTS() (void)0
#define DISABLE_INTERRUPTS() (void)0
#define RESTORE_INTERRUPTS() (void)0
#else
#define DECLARE_INTERRUPT_MASK
#define CLEAR_INTERRUPTS() (void)0
#define DISABLE_INTERRUPTS() (void)0
#define RESTORE_INTERRUPTS() (void)0
#endif

void __osMallocInit(Arena* arena, void* start, s32 size);
void __osMallocCleanup(Arena* arena);
s32 __osMallocIsInitialized(Arena* arena);
void* __osMalloc(Arena* arena, u32 size);
void* __osMallocR(Arena* arena, u32 size);
void __osFree(Arena* arena, void* ptr);
void* __osRealloc(Arena* arena, void* ptr, u32 newSize);
void ArenaImpl_GetSizes(Arena* arena, u32* outMaxFree, u32* outFree, u32* outAlloc);
s32 __osCheckArena(Arena* arena);

#if PLATFORM_N64 || DEBUG_FEATURES
void* __osMallocDebug(Arena* arena, u32 size, const char* file, int line);
void* __osMallocRDebug(Arena* arena, u32 size, const char* file, int line);
void __osFreeDebug(Arena* arena, void* ptr, const char* file, int line);
void* __osReallocDebug(Arena* arena, void* ptr, u32 newSize, const char* file, int line);
#endif

#if !PLATFORM_N64 && DEBUG_FEATURES
void __osDisplayArena(Arena* arena);
extern u32 __osMalloc_FreeBlockTest_Enable;
#endif

#if PLATFORM_N64
extern u32 gTotalAllocFailures;
#endif

#endif

_Static_assert(sizeof(ArenaNode) == 0x30, "ArenaNode must keep the N64 size (arena exhaustion behaviour)");
