// engine/rom_bind.h (rewritten: native)
// Records which original ROM data (by vrom) a game buffer was filled with, where the original code
// DMA'd texture data into a game-owned buffer (minimap, skybox). The buffer keeps its original
// allocation (memory accounting); the native renderer looks up the converted asset by vrom.
#pragma once
#include "engine/types.h"
void RomBind_Set(const void* dst, uintptr_t vromStart, u32 size);
uintptr_t RomBind_Get(const void* dst, u32* size); // 0 if nothing bound
