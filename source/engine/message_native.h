// engine/message_native.h (rewritten: native)
// Message text loading. The original DMA'd message bytes from the ROM text files into the message
// buffer; the bytes (the game's own text encoding with control codes) are decoded by the translated
// game logic in z_message.c. Here they come from the converted copy of the same file on NitroFS.
#pragma once
#include "engine/types.h"
void MessageNative_LoadText(void* dst, uintptr_t fileVromStart, u32 offset, u32 size);
