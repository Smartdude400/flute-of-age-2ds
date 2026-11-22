// engine/player_native.c (rewritten: native) - see player_native.h
#include "engine/player_native.h"
#include "engine/object_native.h"
#include "engine/asset.h"
#include "player.h"

void PlayerNative_LoadGetItemObject(struct Player* player, u32 vromStart, u32 size) {
    u32 used = 0;
    if (player->giDsObj != NULL) {
        DsObj_Free(player->giDsObj);
    }
    player->giDsObj = Vrom_LoadInto(vromStart, player->giObjectSegment, size, &used);
    player->giObjectLoadPending = 1;
}
