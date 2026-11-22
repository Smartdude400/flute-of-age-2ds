// rewritten: native (from flute-of-age/src/code/z_cheap_proc.inc.c, included by z_actor.c)
// Draws one display list with the current matrix and the "25" opaque/translucent preset.
#include "engine/gfx3d.h"
#include "sys_matrix.h"
#include "play_state.h"

void Gfx_DrawDListOpa(PlayState* play, DListRef dlist) {
    (void)play;
    Gfx3D_ResetMaterialState(0);
    Gfx3D_LoadMtxF(Matrix_GetCurrent());
    Gfx3D_DrawDL((u32)(uintptr_t)(dlist));
}

void Gfx_DrawDListXlu(PlayState* play, DListRef dlist) {
    (void)play;
    Gfx3D_ResetMaterialState(1);
    Gfx3D_LoadMtxF(Matrix_GetCurrent());
    Gfx3D_DrawDL((u32)(uintptr_t)(dlist));
}
