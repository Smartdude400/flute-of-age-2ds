// n64dd.h (rewritten: native)
// The original declared the 64DD disk drive interface (driver state, LEO commands, disk callbacks). The 2DS
// has no disk drive: the game code sees a retail N64 without one (n64dd_native.c), so only the symbols the
// ported game code tests or calls on its disk paths are declared.
#ifndef N64DD_H
#define N64DD_H

#include "game_types.h"
#include "pause.h"

extern u8 D_80121212; // a 64DD disk is present (always 0 here)

struct PlayState;
// The 64DD's callback table (original member offsets; only the members the ported game code tests are typed)
typedef struct n64ddStruct_80121220 {
    void (*unk_00)(void);
    void (*unk_04_to_30[12])(void);
    void (*unk_34)(PauseMapMarksData**);
    void (*unk_38)(PauseMapMarksData**);
    void (*unk_3C)(void);
    void (*unk_40)(void);
    s32 (*unk_44)(struct PlayState*);
} n64ddStruct_80121220;
extern n64ddStruct_80121220* B_80121220; // NULL: no disk drive

s32 func_801C70FC(void);
void func_801C7268(void);
void n64dd_SetDiskVersion(s32 arg0);

#endif
