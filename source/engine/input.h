// (rewritten: native)
#pragma once
#include "engine/types.h"
void input_init(void);          // starts the per-retrace sampling thread
u32 input_pending(void);        // samples waiting
void input_consume(int count);  // apply `count` retrace samples to the polled inputs
u32 input_dropped(void);
u32 input_consumed(void);       // retrace samples applied since boot (the game's time base)
// 2DS keys (KEY_*) and touch of the current game frame's samples (scripted input included)
u32 input_keys_held(void);
u32 input_keys_pressed(void);
bool input_touch_held(s16* x, s16* y);
bool input_touch_tapped(s16* x, s16* y);
