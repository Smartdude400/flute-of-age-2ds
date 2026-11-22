// adapted from flute-of-age/include/cutscene_flags.h (ultra64.h -> game_types.h)
#ifndef CUTSCENE_FLAGS_H
#define CUTSCENE_FLAGS_H

#include "game_types.h"

struct PlayState;

void CutsceneFlags_UnsetAll(struct PlayState* play);
void CutsceneFlags_Set(struct PlayState* play, s16 flag);
void CutsceneFlags_Unset(struct PlayState* play, s16 flag);
s32 CutsceneFlags_Get(struct PlayState* play, s16 flag);

#endif
