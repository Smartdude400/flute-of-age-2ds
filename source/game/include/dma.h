// dma.h (rewritten: native). The original declared the ROM DMA manager. The port has no ROM DMA:
// converted data is loaded from RomFS by engine code, and the game-level RomFile type (used as a file
// identifier in scene/object tables) is provided.
// DMA_REQUEST_SYNC / DMA_REQUEST_ASYNC (same arguments as the original macros): the copy is done at once
// by RomBind_Set from the raw ROM files kept in RomFS (tools/build_rawdata.py), or recorded as a binding
// for data the renderer converts itself (engine/rom_bind.h). Request / queue arguments are not used: the
// callers wait for the transfer right after starting it.
#ifndef DMA_H
#define DMA_H
#include "romfile.h"
#include "engine/rom_bind.h"
#define DMA_REQUEST_SYNC(ram, vrom, size, file, line) RomBind_Set((ram), (uintptr_t)(vrom), (size))
#define DMA_REQUEST_ASYNC(req, ram, vrom, size, unk5, queue, msg, file, line) \
    RomBind_Set((ram), (uintptr_t)(vrom), (size))
#endif
