// engine/hud.c (rewritten: native)
// Minimal native HUD: reads the same game state the original Interface_Draw displayed and renders it
// with the native 2D layer. Layout is rearranged for the DS (allowed presentation change).
#include <stdio.h>
#include "engine/hud.h"
#include "engine/gfx2d.h"
#include "play_state.h"
#include "save.h"

void Hud_Draw(struct PlayState* play) {
    (void)play;
    char buf[40];
    s16 health = gSaveContext.save.info.playerData.health;
    s16 cap = gSaveContext.save.info.playerData.healthCapacity;
    snprintf(buf, sizeof(buf), "HP %d/%d", health / 16, cap / 16);
    Gfx2D_Text(16, 12, 255, 80, 80, 255, buf);
    snprintf(buf, sizeof(buf), "RUPEES %d", gSaveContext.save.info.playerData.crowns);
    Gfx2D_Text(16, 222, 120, 255, 120, 255, buf);
}
