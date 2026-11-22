// adapted from flute-of-age/include/horse.h (ultra64.h -> game_types.h)
#ifndef HORSE_H
#define HORSE_H

#include "game_types.h"
#include "z_math.h"

struct PlayState;
struct Actor;
struct Player;

void Horse_ResetHorseData(struct PlayState* play);
void Horse_FixLakeEldenPosition(struct PlayState* play);
void Horse_InitPlayerHorse(struct PlayState* play, struct Player* player);
void Horse_RotateToPoint(struct Actor* actor, Vec3f* pos, s16 turnAmount);

#endif
