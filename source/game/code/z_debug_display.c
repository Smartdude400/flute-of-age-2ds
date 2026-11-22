// rewritten: native (from flute-of-age/src/code/z_debug_display.c)
// Debug object list (sprites/polygons the debug build draws). The list logic is kept; objects come
// from the per-frame scratch pool (was GRAPH_ALLOC). Drawing is a presentation stub (debug-only
// models; retail builds never add objects).
#include "play_state.h"
#include "debug_display.h"
#include "engine/gfx3d.h"
#include "engine/stub.h"

static DebugDispObject* sDebugObjectListHead;

void DebugDisplay_Init(void) {
    sDebugObjectListHead = NULL;
}

DebugDispObject* DebugDisplay_AddObject(f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, f32 scaleX,
                                        f32 scaleY, f32 scaleZ, u8 red, u8 green, u8 blue, u8 alpha, s16 type,
                                        struct GraphicsContext* gfxCtx) {
    DebugDispObject* prevHead = sDebugObjectListHead;

    (void)gfxCtx;
    sDebugObjectListHead = Gfx3D_FrameAlloc(sizeof(DebugDispObject));

    sDebugObjectListHead->pos.x = posX;
    sDebugObjectListHead->pos.y = posY;
    sDebugObjectListHead->pos.z = posZ;
    sDebugObjectListHead->rot.x = rotX;
    sDebugObjectListHead->rot.y = rotY;
    sDebugObjectListHead->rot.z = rotZ;
    sDebugObjectListHead->scale.x = scaleX;
    sDebugObjectListHead->scale.y = scaleY;
    sDebugObjectListHead->scale.z = scaleZ;
    sDebugObjectListHead->color.r = red;
    sDebugObjectListHead->color.g = green;
    sDebugObjectListHead->color.b = blue;
    sDebugObjectListHead->color.a = alpha;
    sDebugObjectListHead->type = type;
    sDebugObjectListHead->next = prevHead;

    return sDebugObjectListHead;
}

void DebugDisplay_DrawObjects(PlayState* play) {
    DebugDispObject* dispObj = sDebugObjectListHead;

    (void)play;
    while (dispObj != NULL) {
        PRESENTATION_STUB("DebugDisplay_DrawObjects");
        dispObj = dispObj->next;
    }
}
