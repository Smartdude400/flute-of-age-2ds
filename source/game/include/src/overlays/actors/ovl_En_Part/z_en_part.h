// adapted from flute-of-age/include/src/overlays/actors/ovl_En_Part/z_en_part.h (ultra64.h -> game_types.h)
#ifndef Z_EN_PART_H
#define Z_EN_PART_H

#include "game_types.h"
#include "actor.h"

struct EnPart;

typedef void (*EnPartActionFunc)(struct EnPart*, struct PlayState*);

typedef struct EnPart {
    /* 0x000 */ Actor actor;
    /* 0x14C */ u8 action;
    /* 0x14E */ s16 timer;
    /* 0x150 */ DListRef  displayList;
    /* 0x154 */ f32 rotZ;
    /* 0x158 */ f32 rotZSpeed;
} EnPart; // size = 0x015C

#endif
