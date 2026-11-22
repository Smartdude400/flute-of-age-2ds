// engine/player_anim.c (rewritten: native)
// Frame record = limbCount Vec3s + one s16 (face/expression data): (6 * limbCount + 2) bytes, already
// little-endian (tools/build_object.py). Reads straight into the game's frame table.
#include <stdio.h>
#include "engine/player_anim.h"
#include "engine/log.h"

static FILE* sFile;

void PlayerAnim_ReadFrame(u32 fileOffset, s32 frame, s32 limbCount, Vec3s* frameTable) {
    if (sFile == NULL) {
        sFile = fopen("romfs:/anim/aren_animetion.bin", "rb");
        if (sFile == NULL) {
            log_printf("player anim file missing\n");
            return;
        }
    }
    size_t size = sizeof(Vec3s) * limbCount + 2;
    fseek(sFile, (long)(fileOffset + size * frame), SEEK_SET);
    if (fread(frameTable, 1, size, sFile) != size) {
        log_printf("player anim read failed @%lu\n", (unsigned long)(fileOffset + size * frame));
    }
}
