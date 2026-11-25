// n64dd_native.c (rewritten: native)
// The 64DD disk drive does not exist on the 2DS: the game sees the state of a retail N64 without one
// (no disk inserted, D_80121212 == 0), so its 64DD paths are never taken.
#include "n64dd.h"

u8 D_80121212 = 0;
n64ddStruct_80121220* B_80121220 = NULL;

void n64dd_SetDiskVersion(s32 arg0) {
    (void)arg0; // only called with a disk present
}

// 64DD calls on paths guarded by D_80121212 (never taken without a disk)
s32 func_801C70FC(void) {
    return 0;
}

void func_801C7268(void) {
}
