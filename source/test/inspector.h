// test/inspector.h (native TOOL, make INSPECT=1): asset inspector hooks, see inspector.c
#pragma once
#include "engine/types.h"
struct PlayState;
struct Input;
// runner: this frame's input; true = the game must not see it
bool Inspector_Input(struct Input* input);
// Play_Draw: once per frame first; then the view override before View_Apply; OBJECTS mode draws instead of
// the scene (true = Play_Draw draws nothing else)
void Inspector_PlayFrame(struct PlayState* play);
void Inspector_OverrideView(struct PlayState* play);
bool Inspector_DrawObjects(struct PlayState* play);
// Play_DrawOverlayElements: the inspector panel replaces the HUD on the bottom screen
void Inspector_DrawUI(struct PlayState* play);
