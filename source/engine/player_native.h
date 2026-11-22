// engine/player_native.h (rewritten: native)
#pragma once
#include "engine/types.h"
struct Player;
// Get-item model object: the original DMA'd the object file into player->giObjectSegment and polled a
// message queue; here the converted object is loaded at once and reported through giObjectLoadPending.
void PlayerNative_LoadGetItemObject(struct Player* player, u32 vromStart, u32 size);
