// adapted from flute-of-age/include/src/overlays/actors/ovl_Elf_Msg/z_elf_msg.h (ultra64.h -> game_types.h)
#ifndef Z_ELF_MSG_H
#define Z_ELF_MSG_H

#include "game_types.h"
#include "actor.h"

struct ElfMsg;

typedef void (*ElfMsgActionFunc)(struct ElfMsg*, struct PlayState*);

typedef struct ElfMsg {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ ElfMsgActionFunc actionFunc;
} ElfMsg; // size = 0x0150

#endif
