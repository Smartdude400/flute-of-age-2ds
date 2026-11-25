// n64dd.h (rewritten: native)
// The original declared the 64DD disk drive interface (driver state, LEO commands, disk callbacks). The 2DS
// has no disk drive: the game code sees a retail N64 without one (n64dd_native.c), so only the symbols the
// ported game code tests or calls on its disk paths are declared.
#ifndef N64DD_H
#define N64DD_H

#include "game_types.h"

extern u8 D_80121212; // a 64DD disk is present (always 0 here)

s32 func_801C70FC(void);
void func_801C7268(void);
void n64dd_SetDiskVersion(s32 arg0);

#endif
