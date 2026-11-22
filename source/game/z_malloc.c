// adapted from flute-of-age/src/code/z_malloc.c
#include "libc64/os_malloc.h"
#include "printf.h"
#include "translation.h"

#define LOG_SEVERITY_NOLOG 0
#define LOG_SEVERITY_ERROR 2
#define LOG_SEVERITY_VERBOSE 3

Arena sFoaArena;

#if DEBUG_FEATURES
s32 gFoaArenaLogSeverity = LOG_SEVERITY_ERROR;

void FoaArena_CheckPointer(void* ptr, u32 size, const char* name, const char* action) {
    if (ptr == NULL) {
        if (gFoaArenaLogSeverity >= LOG_SEVERITY_ERROR) {
            PRINTF(T("%s: %u バイトの%sに失敗しました\n", "%s: %u bytes %s failed\n"), name, size, action);
#if PLATFORM_GC
            __osDisplayArena(&sFoaArena);
#endif
        }
    } else if (gFoaArenaLogSeverity >= LOG_SEVERITY_VERBOSE) {
        PRINTF(T("%s: %u バイトの%sに成功しました\n", "%s: %u bytes %s succeeded\n"), name, size, action);
    }
}

#define FOA_ARENA_CHECK_POINTER(ptr, size, name, action) FoaArena_CheckPointer(ptr, size, name, action)
#else
#define FOA_ARENA_CHECK_POINTER(ptr, size, name, action) (void)0
#endif

void* FoaArena_Malloc(u32 size) {
    void* ptr = __osMalloc(&sFoaArena, size);

    // TODO re-evaluate "secure" as a translation (in this file and others using "確保")
    FOA_ARENA_CHECK_POINTER(ptr, size, "elara_malloc", T("確保", "Secure"));
    return ptr;
}

#if DEBUG_FEATURES
void* FoaArena_MallocDebug(u32 size, const char* file, int line) {
    void* ptr = __osMallocDebug(&sFoaArena, size, file, line);

    FOA_ARENA_CHECK_POINTER(ptr, size, "elara_malloc_DEBUG", T("確保", "Secure"));
    return ptr;
}
#endif

void* FoaArena_MallocR(u32 size) {
    void* ptr = __osMallocR(&sFoaArena, size);

    FOA_ARENA_CHECK_POINTER(ptr, size, "elara_malloc_r", T("確保", "Secure"));
    return ptr;
}

#if DEBUG_FEATURES
void* FoaArena_MallocRDebug(u32 size, const char* file, int line) {
    void* ptr = __osMallocRDebug(&sFoaArena, size, file, line);

    FOA_ARENA_CHECK_POINTER(ptr, size, "elara_malloc_r_DEBUG", T("確保", "Secure"));
    return ptr;
}
#endif

void* FoaArena_Realloc(void* ptr, u32 newSize) {
    ptr = __osRealloc(&sFoaArena, ptr, newSize);
    FOA_ARENA_CHECK_POINTER(ptr, newSize, "elara_realloc", T("再確保", "Re-securing"));
    return ptr;
}

#if DEBUG_FEATURES
void* FoaArena_ReallocDebug(void* ptr, u32 newSize, const char* file, int line) {
    ptr = __osReallocDebug(&sFoaArena, ptr, newSize, file, line);
    FOA_ARENA_CHECK_POINTER(ptr, newSize, "elara_realloc_DEBUG", T("再確保", "Re-securing"));
    return ptr;
}
#endif

void FoaArena_Free(void* ptr) {
    __osFree(&sFoaArena, ptr);
}

#if DEBUG_FEATURES
void FoaArena_FreeDebug(void* ptr, const char* file, int line) {
    __osFreeDebug(&sFoaArena, ptr, file, line);
}
#endif

void* FoaArena_Calloc(u32 num, u32 size) {
    void* ret;
    u32 n = num * size;

    ret = __osMalloc(&sFoaArena, n);
    if (ret != NULL) {
        memset(ret, 0, n);
    }

    FOA_ARENA_CHECK_POINTER(ret, n, "elara_calloc", T("確保", "Secure"));
    return ret;
}

#if PLATFORM_GC && DEBUG_FEATURES
void FoaArena_Display(void) {
    PRINTF(T("エララヒープ表示\n", "Elara heap display\n"));
    __osDisplayArena(&sFoaArena);
}
#endif

void FoaArena_GetSizes(u32* outMaxFree, u32* outFree, u32* outAlloc) {
    ArenaImpl_GetSizes(&sFoaArena, outMaxFree, outFree, outAlloc);
}

void FoaArena_Check(void) {
    __osCheckArena(&sFoaArena);
}

void FoaArena_Init(void* start, u32 size) {
#if DEBUG_FEATURES
    gFoaArenaLogSeverity = LOG_SEVERITY_NOLOG;
#endif
    __osMallocInit(&sFoaArena, start, size);
}

void FoaArena_Cleanup(void) {
#if DEBUG_FEATURES
    gFoaArenaLogSeverity = LOG_SEVERITY_NOLOG;
#endif
    __osMallocCleanup(&sFoaArena);
}

s32 FoaArena_IsInitialized(void) {
    return __osMallocIsInitialized(&sFoaArena);
}
