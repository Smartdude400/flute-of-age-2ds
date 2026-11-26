// adapted from flute-of-age/include/src/overlays/actors/ovl_Item_Flute/z_item_flute.h (ultra64.h -> game_types.h)
#ifndef Z_ITEM_FLUTE_H
#define Z_ITEM_FLUTE_H

#include "game_types.h"
#include "actor.h"

struct ItemFlute;

typedef void (*ItemFluteActionFunc)(struct ItemFlute*, struct PlayState*);

typedef struct ItemFlute {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ ItemFluteActionFunc actionFunc;
    /* 0x0150 */ s16 spinRotOffset;
} ItemFlute; // size = 0x0154

#endif
