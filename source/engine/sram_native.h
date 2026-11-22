// engine/sram_native.h (rewritten: native)
#pragma once
#include "engine/types.h"
#include <stddef.h>
#define SRAM_NATIVE_SIZE 0x8000
#define SRAM_DIR_READ 0
#define SRAM_DIR_WRITE 1
void SsSram_ReadWrite(s32 addr, void* dramAddr, size_t size, s32 direction); // addr = offset in save memory
bool SramNative_IsDirty(void);
u8* SramNative_Data(void);
