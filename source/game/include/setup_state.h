// adapted from flute-of-age/include/setup_state.h (ultra64.h -> game_types.h)
#ifndef SETUP_STATE_H
#define SETUP_STATE_H

#include "game.h"

typedef struct SetupState {
    /* 0x00 */ GameState state;
} SetupState; // size = 0xA4

void Setup_Init(GameState* thisx);
void Setup_Destroy(GameState* thisx);

#endif
