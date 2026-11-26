// adapted from flute-of-age/include/src/overlays/actors/ovl_En_Varkon_Mant/z_en_varkon_mant.h (ultra64.h -> game_types.h)
#ifndef Z_EN_VARKON_MANT_H
#define Z_EN_VARKON_MANT_H

#include "game_types.h"
#include "actor.h"

struct EnVarkonMant;

#define VARKON_MANT_NUM_JOINTS 12
#define VARKON_MANT_NUM_STRANDS 12

typedef struct MantStrand {
    /* 0x000 */ Vec3f root;                              // root position along the collar
    /* 0x00C */ Vec3f joints[VARKON_MANT_NUM_JOINTS];     // "joints" for deforming the cloak, stemming from root and propagating down the cloak
    /* 0x090 */ Vec3f rotations[VARKON_MANT_NUM_JOINTS];  // normal vector rotations, x and y only
    /* 0x120 */ Vec3f velocities[VARKON_MANT_NUM_JOINTS]; // velocities calculated as differences in position from last update
    /* 0x1B0 */ u8 torn[VARKON_MANT_NUM_JOINTS];          // guess: whether the joint is torn?
} MantStrand; // size = 0x1C8

typedef struct EnVarkonMant {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ MantStrand strands[VARKON_MANT_NUM_STRANDS];
    /* 0x16AC */ f32 minY; // minimum Y value possible for joints, for emulating collision with a floor
    /* 0x16B0 */ f32 backPush; // larger values push the cloak further outwards, negative is away from the actor
    /* 0x16B4 */ f32 backSwayMagnitude; // magnitude of backwards/forwards swaying
    /* 0x16B8 */ f32 sideSwayMagnitude; // magnitude of sideways swaying
    /* 0x16BC */ f32 attachRightArmTimer; // timer for the duration of which the cloak is attached to right forearm and left shoulder
    /* 0x16C0 */ f32 attachLeftArmTimer; // timer for the duration of which the cloak is attached to left forearm and right shoulder
    /* 0x16C4 */ f32 attachShouldersTimer; // timer for the duration of which the cloak is attached to both shoulders
    /* 0x16C8 */ f32 gravity; // strand gravity
    /* 0x16CC */ f32 baseYaw;
    /* 0x16D0 */ f32 minDist; // closest distance the cloak can get to the attached actor without being pushed away
    /* 0x16D4 */ Vec3f rightForearmPos;
    /* 0x16E0 */ Vec3f leftForearmPos;
    /* 0x16EC */ Vec3f rightShoulderPos;
    /* 0x16F8 */ Vec3f leftShoulderPos;
    /* 0x1704 */ u8 tearTimer; // tear the cloak for x many frames
    /* 0x1705 */ u8 updateHasRun;
    /* 0x1706 */ u8 frameTimer;
} EnVarkonMant; // size = 0x1708

#endif
