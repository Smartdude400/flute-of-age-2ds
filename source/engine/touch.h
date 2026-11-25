// engine/touch.h (native) - touch screen (2DS layout: the menu and HUD are on the bottom screen).
// Elements the game draws on the bottom screen are tagged while recording (Gfx3D_TouchTag); the renderer keeps
// the screen box of every tagged element it drew; touches are hit-tested against the previous frame's boxes.
// Buttons act as the N64 button while touched; other targets are delivered as taps (Touch_TakeTap).
#pragma once
#include "engine/types.h"

enum {
    TOUCH_NONE,
    TOUCH_BTN_B,      // HUD B button
    TOUCH_BTN_CLEFT,  // HUD C buttons
    TOUCH_BTN_CDOWN,
    TOUCH_BTN_CRIGHT,
    TOUCH_BTN_START,  // pause menu START icon
    TOUCH_BTN_Z,      // pause menu page arrows (Z / R switch pages)
    TOUCH_BTN_R,
    TOUCH_ITEM_SLOT = 16,  // + item grid slot (24)
    TOUCH_EQUIP_SLOT = 40, // + equipment slot (equipment type * 4 + value, 16)
    TOUCH_TAG_COUNT = 56
};

// recorded into the current buffer: elements drawn after it belong to the tag (TOUCH_NONE ends it)
void Gfx3D_TouchTag(int tag);
// tag of the smallest element under (x, y) on the bottom screen in the last drawn frame, or TOUCH_NONE
int Gfx3D_TouchFind(int x, int y);
// a tap started this game frame on a tag in [first, first + count): its index, else -1 (taken once)
s32 Touch_TakeTap(int first, int count);
