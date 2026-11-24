// adapted from flute-of-age/include/src/overlays/actors/ovl_En_Kusa/z_en_kusa.h (ultra64.h -> game_types.h)
#ifndef Z_EN_KUSA_H
#define Z_EN_KUSA_H

#include "game_types.h"
#include "actor.h"

struct EnKusa;

typedef void (*EnKusaActionFunc)(struct EnKusa*, struct PlayState*);

typedef enum EnKusaType {
    /* 0 */ ENKUSA_TYPE_0,
    /* 1 */ ENKUSA_TYPE_1,
    /* 2 */ ENKUSA_TYPE_2
} EnKusaType;

typedef struct EnKusa {
    /* 0x000 */ Actor actor;
    /* 0x14C */ EnKusaActionFunc actionFunc;
    /* 0x150 */ ColliderCylinder collider;
    /* 0x19C */ s16 timer;
    /* 0x19E */ s8 requiredObjectSlot;
} EnKusa; // size = 0x1A0

#endif
