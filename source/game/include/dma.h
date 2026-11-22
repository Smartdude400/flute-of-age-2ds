// dma.h (rewritten: native). The original declared the ROM DMA manager. The DS port has no ROM DMA:
// converted data is loaded from NitroFS by engine code. Only the game-level RomFile type
// (used as a file identifier in scene/object tables) is provided.
#ifndef DMA_H
#define DMA_H
#include "romfile.h"
#endif
