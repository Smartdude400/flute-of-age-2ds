// engine/tev.h (rewritten: native) - colour combiner -> PICA200 texture combiner (see tev.c)
#pragma once
#include <citro3d.h>
#include "engine/pmesh.h"

typedef struct TevStage {
    u8 srcRgb[3], srcA[3], opRgb[3], opA[3];
    u8 funcRgb, funcA;
    u8 constRgb, constA; // constant kinds (tev.c K_*)
} TevStage;

typedef struct TevProgram {
    TevStage stage[6];
    u8 numStages;
    bool approx, overflow, constConflict;
} TevProgram;

void Tev_Compile(TevProgram* p, const GfxCombine* c, bool twoCycle);
void Tev_Apply(const TevProgram* p, const u8 prim[4], const u8 env[4], u8 primLod);
