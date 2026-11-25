// adapted from flute-of-age/include/src/overlays/actors/ovl_Bg_Spot00_Hanebasi/z_bg_spot00_hanebasi.h (ultra64.h -> game_types.h)
#ifndef Z_BG_SPOT00_HANEBASI_H
#define Z_BG_SPOT00_HANEBASI_H

#include "game_types.h"
#include "actor.h"
#include "light.h"

struct BgSpot00Hanebasi;

typedef void (*BgSpot00HanebasiActionFunc)(struct BgSpot00Hanebasi*, struct PlayState*);

typedef struct BgSpot00Hanebasi {
    /* 0x0000 */ DynaPolyActor dyna;
    /* 0x0164 */ BgSpot00HanebasiActionFunc actionFunc;
    /* 0x0168 */ s16 destAngle;
    /* 0x016C */ LightNode* lightNode;
    /* 0x0170 */ LightInfo lightInfo;
} BgSpot00Hanebasi; // size = 0x0180

#endif
