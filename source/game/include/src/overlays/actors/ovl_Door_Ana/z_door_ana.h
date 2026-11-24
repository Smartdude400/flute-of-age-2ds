// adapted from flute-of-age/include/src/overlays/actors/ovl_Door_Ana/z_door_ana.h (ultra64.h -> game_types.h)
#ifndef Z_DOOR_ANA_H
#define Z_DOOR_ANA_H

#include "game_types.h"
#include "actor.h"

struct DoorAna;

typedef void (*DoorAnaActionFunc)(struct DoorAna*, struct PlayState*);

typedef struct DoorAna {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ ColliderCylinder collider;
    /* 0x0198 */ DoorAnaActionFunc actionFunc;
} DoorAna; // size = 0x019C

#endif
