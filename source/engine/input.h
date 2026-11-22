// (rewritten: native)
#pragma once
#include "engine/types.h"
void input_init(void);          // starts the per-retrace sampling thread
u32 input_pending(void);        // samples waiting
void input_consume(int count);  // apply `count` retrace samples to the polled inputs
u32 input_dropped(void);
u32 input_consumed(void);       // retrace samples applied since boot (the game's time base)
