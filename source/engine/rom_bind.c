// engine/rom_bind.c (rewritten: native) - see rom_bind.h
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/rom_bind.h"
#include "engine/log.h"

// ---- ROM files kept as raw bytes in RomFS (tools/build_rawdata.py, index romfs:/rawfiles.bin): where the
// original DMA'd from one of them into a game buffer, the same bytes are copied (font glyphs, text box
// textures, item icons). Each file is read once and kept in memory.
typedef struct { u32 vrom, size; u8* data; } RawFile;
static RawFile* sRaw;
static u32 sNumRaw;
static bool sRawLoaded;

static void raw_index(void) {
    sRawLoaded = true;
    FILE* f = fopen("romfs:/rawfiles.bin", "rb");
    if (f == NULL) {
        return;
    }
    u32 n = 0;
    if (fread(&n, 4, 1, f) == 1 && n < 256) {
        sRaw = calloc(n, sizeof(RawFile));
        for (u32 i = 0; sRaw != NULL && i < n; i++) {
            if (fread(&sRaw[i].vrom, 4, 1, f) != 1 || fread(&sRaw[i].size, 4, 1, f) != 1) {
                break;
            }
            sNumRaw = i + 1;
        }
    }
    fclose(f);
}

static bool raw_copy(const void* dst, uintptr_t vrom, u32 size) {
    if (!sRawLoaded) {
        raw_index();
    }
    for (u32 i = 0; i < sNumRaw; i++) {
        RawFile* r = &sRaw[i];
        if (vrom < r->vrom || vrom + size > r->vrom + r->size) {
            continue;
        }
        if (r->data == NULL) {
            char path[40];
            snprintf(path, sizeof(path), "romfs:/vrom/%08lX.bin", (unsigned long)r->vrom);
            FILE* f = fopen(path, "rb");
            r->data = malloc(r->size);
            if (f == NULL || r->data == NULL || fread(r->data, 1, r->size, f) != r->size) {
                log_printf("rom_bind: cannot read %s\n", path);
                free(r->data);
                r->data = NULL;
                if (f) fclose(f);
                return false;
            }
            fclose(f);
        }
        memcpy((void*)dst, r->data + (vrom - r->vrom), size);
        return true;
    }
    return false;
}

#define MAX_BINDINGS 8
static struct { const void* dst; uintptr_t vrom; u32 size; } sBind[MAX_BINDINGS];

void RomBind_Set(const void* dst, uintptr_t vromStart, u32 size) {
    if (raw_copy(dst, vromStart, size)) {
        return; // the buffer now holds the original bytes, as after the original DMA
    }
    int slot = -1;
    for (int i = 0; i < MAX_BINDINGS; i++) {
        if (sBind[i].dst == dst) {
            slot = i;
            break;
        }
        if (sBind[i].dst == NULL && slot < 0) {
            slot = i;
        }
    }
    if (slot < 0) {
        slot = 0;
    }
    sBind[slot].dst = dst;
    sBind[slot].vrom = vromStart;
    sBind[slot].size = size;
}

uintptr_t RomBind_Get(const void* dst, u32* size) {
    for (int i = 0; i < MAX_BINDINGS; i++) {
        if (sBind[i].dst == dst) {
            if (size != NULL) {
                *size = sBind[i].size;
            }
            return sBind[i].vrom;
        }
    }
    return 0;
}
