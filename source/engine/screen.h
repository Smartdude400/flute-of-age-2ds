// engine/screen.h (rewritten: native)
// Top-screen render target. The game's 320x240 frame is drawn 1:1 in the centre of the 400x240 top
// screen (original aspect ratio and field of view; 40-pixel black bars left and right).
#pragma once
#include "engine/types.h"
#define SCREEN_FB_W 400
#define SCREEN_FB_H 240
#define SCREEN_GAME_W 320
#define SCREEN_GAME_H 240
void Screen_Init(void);
void Screen_BeginFrame(void); // clears the whole target to black, selects the game viewport
void Screen_EndFrame(void);
void Screen_SetGameViewport(void);
