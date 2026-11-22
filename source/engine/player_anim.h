// (rewritten: native)
// engine/player_anim.h - streams player animation frames from the converted aren_animetion file.
#pragma once
#include "game_types.h"
#include "z_math.h"
// fileOffset: byte offset inside the converted file (= original offset into aren_animetion).
void PlayerAnim_ReadFrame(u32 fileOffset, s32 frame, s32 limbCount, Vec3s* frameTable);
