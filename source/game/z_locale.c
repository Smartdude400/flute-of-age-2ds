// adapted from flute-of-age/src/boot/z_locale.c
// The original reads the region from the cartridge header. The DS build is configured for the
// US ntsc-1.0 ROM, so the region is fixed at build time (OOT_REGION).
#include "z_locale.h"
#include "region.h"

s32 gCurrentRegion = 0;

void Locale_Init(void) {
    gCurrentRegion = OOT_REGION;
}

void Locale_ResetRegion(void) {
    gCurrentRegion = REGION_NULL;
}
