// engine/screen.h (rewritten: native)
// Render targets. The game's 320x240 frame is drawn 1:1 in the centre of the 400x240 top screen (original
// aspect ratio and field of view; 40-pixel black bars left and right). The bottom screen (320x240) shows the
// pause menu and the HUD at their original coordinates (2DS layout, see Gfx3D_Screen).
#pragma once
#include "engine/types.h"
#define SCREEN_FB_W 400
#define SCREEN_FB_H 240
#define SCREEN_GAME_W 320
#define SCREEN_GAME_H 240
void Screen_Init(void);
void Screen_BeginFrame(void); // clears the whole target to black, selects the game viewport
void Screen_EndFrame(void);
void Screen_SetGameViewport(void); // the game area of the selected screen
enum { SCREEN_TOP, SCREEN_BOTTOM };
void Screen_Select(int screen); // draw on this screen from now on (within the frame)
int Screen_Current(void);
// draw on the screen again after an offscreen image (engine/gfx3d.c)
void Screen_DrawOnScreen(void);
// copy of the game area as drawn so far; the texture (512 x 256, u = logical x, v = logical y, first stored row
// = y 0) is built the next frame
#include <citro3d.h>
void Screen_CaptureFrame(void);
const C3D_Tex* Screen_CaptureTexture(void);
