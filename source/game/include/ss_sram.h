// adapted from flute-of-age/include/ss_sram.h (ultra64.h -> game_types.h)
#ifndef SS_SRAM_H
#define SS_SRAM_H

#include "stddef.h"
#include "game_types.h"

void SsSram_Init(s32 addr, u8 handleType, u8 handleDomain, u8 handleLatency, u8 handlePageSize, u8 handleRelDuration,
    u8 handlePulse, u32 handleSpeed);
void SsSram_Dma(void* dramAddr, size_t size, s32 direction);
// (libultra) uncached segment address -> physical; the game passes SRAM addresses (0xA8000000+) this way
#ifndef OS_K1_TO_PHYSICAL
#define OS_K1_TO_PHYSICAL(x) ((u32)(x) & 0x1FFFFFFF)
#endif
void SsSram_ReadWrite(s32 addr, void* dramAddr, size_t size, s32 direction);

#endif
