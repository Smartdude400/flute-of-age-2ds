// (rewritten: native)
// engine/gfx2d.h - native 2D layer on the PICA200. Coordinates are in the game's logical 320x240
// screen space, drawn 1:1 into the centred game viewport (engine/screen.h).
#pragma once
#include "engine/types.h"
#include "gfx.h"
#include "libu64/gfxprint.h"

bool Gfx2D_Init(void);                 // loads the font texture and the 2D shader
void Gfx2D_Begin(u8 r, u8 g, u8 b);    // fills the game frame with a colour (original frame clear)
void Gfx2D_Text(int x, int y, u8 r, u8 g, u8 b, u8 a, const char* text);
void Gfx2D_Rect(int x, int y, int w, int h, u8 r, u8 g, u8 b, u8 a);
void Gfx2D_FillScreen(u8 r, u8 g, u8 b, s16 a); // Environment_FillScreen equivalent
void Gfx2D_End(void);
void Gfx2D_Begin3DOrtho(void); // (DS-era call, no-op)
void Gfx2D_ExecFill(u8 r, u8 g, u8 b, u8 a); // replay-time full-frame fill (engine/gfx3d.c)
