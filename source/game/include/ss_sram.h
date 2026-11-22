// adapted from flute-of-age/include/ss_sram.h (ultra64.h -> game_types.h)
#ifndef SS_SRAM_H
#define SS_SRAM_H

#include "stddef.h"
#include "game_types.h"

void SsSram_Init(s32 addr, u8 handleType, u8 handleDomain, u8 handleLatency, u8 handlePageSize, u8 handleRelDuration,
    u8 handlePulse, u32 handleSpeed);
void SsSram_Dma(void* dramAddr, size_t size, s32 direction);
void SsSram_ReadWrite(s32 addr, void* dramAddr, size_t size, s32 direction);

#endif
