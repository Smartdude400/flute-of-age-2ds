// adapted from flute-of-age/include/src/overlays/actors/ovl_Obj_Makekinsuta/z_obj_makekinsuta.h (ultra64.h -> game_types.h)
#ifndef Z_OBJ_MAKEKINSUTA_H
#define Z_OBJ_MAKEKINSUTA_H

#include "game_types.h"
#include "actor.h"

struct ObjMakekinsuta;

typedef void (*ObjMakekinsutaActionFunc)(struct ObjMakekinsuta*, struct PlayState*);

typedef struct ObjMakekinsuta {
    /* 0x000 */ Actor actor;
    /* 0x14C */ ObjMakekinsutaActionFunc actionFunc;
    /* 0x150 */ s16 timer;
    /* 0x152 */ s16 unk_152;
} ObjMakekinsuta; // size = 0x154

#endif
