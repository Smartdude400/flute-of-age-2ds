// adapted from flute-of-age/include/lifemeter.h (ultra64.h -> game_types.h)
#ifndef LIFEMETER_H
#define LIFEMETER_H

#include "game_types.h"

struct PlayState;

void Health_InitMeter(struct PlayState* play);
void Health_UpdateMeter(struct PlayState* play);
void Health_DrawMeter(struct PlayState* play);
void Health_UpdateBeatingHeart(struct PlayState* play);
u32 Health_IsCritical(void);

#endif
