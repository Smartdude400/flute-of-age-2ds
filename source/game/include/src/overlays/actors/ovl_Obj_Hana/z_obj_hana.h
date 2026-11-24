// adapted from flute-of-age/include/src/overlays/actors/ovl_Obj_Hana/z_obj_hana.h (ultra64.h -> game_types.h)
#ifndef Z_OBJ_HANA_H
#define Z_OBJ_HANA_H

#include "game_types.h"
#include "actor.h"

struct ObjHana;

typedef struct ObjHana {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ ColliderCylinder collider;
} ObjHana; // size = 0x0198

#endif
