// engine/crc32.c (rewritten: native) - standard CRC-32 (IEEE 802.3), matches Python zlib.crc32
#include "engine/crc32.h"

static uint32_t sTable[256];

uint32_t crc32_calc(const void* data, size_t len) {
    if (sTable[1] == 0) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) {
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            sTable[i] = c;
        }
    }
    const uint8_t* p = data;
    uint32_t crc = 0xFFFFFFFFu;
    while (len--) {
        crc = sTable[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}
