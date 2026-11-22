// adapted from flute-of-age/src/code/z_debug.c (Regs_Init only; the on-screen register editor
// and debug camera text are debug-build features and are not ported)
#include "regs.h"
#include "engine/system_arena.h"

RegEditor* gRegEditor;

void Regs_Init(void) {
    s32 i;

    gRegEditor = SystemArena_Malloc(sizeof(RegEditor));
    gRegEditor->regPage = 0;
    gRegEditor->regGroup = 0;
    gRegEditor->regCur = 0;
    gRegEditor->dPadInputPrev = 0;
    gRegEditor->inputRepeatTimer = 0;
    for (i = 0; i < (s32)(sizeof(gRegEditor->data) / sizeof(gRegEditor->data[0])); i++) {
        gRegEditor->data[i] = 0;
    }
}
