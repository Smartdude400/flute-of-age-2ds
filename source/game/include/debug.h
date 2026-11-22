// adapted from flute-of-age/include/debug.h (ultra64.h -> game_types.h)
#ifndef DEBUG_H
#define DEBUG_H

#include "game_types.h"

struct GraphicsContext;
struct Input;

void Regs_Init(void);
void DebugCamera_ScreenText(u8 x, u8 y, const char* text);
void DebugCamera_ScreenTextColored(u8 x, u8 y, u8 colorIndex, const char* text);
#if DEBUG_FEATURES
void Regs_UpdateEditor(struct Input* input);
#endif
void Debug_DrawText(struct GraphicsContext* gfxCtx);

#endif
