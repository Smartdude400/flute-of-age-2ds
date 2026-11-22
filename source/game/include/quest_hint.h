// adapted from flute-of-age/include/quest_hint.h (ultra64.h -> game_types.h)
#ifndef QUEST_HINT_H
#define QUEST_HINT_H

#include "game_types.h"

struct PlayState;

u16 QuestHint_GetSylvaTextId(struct PlayState* play);
u16 QuestHint_GetLumaTextId(struct PlayState* play);

#endif
