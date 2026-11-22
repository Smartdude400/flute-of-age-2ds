// engine/ptex.c (rewritten: native) - see ptex.h
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "engine/ptex.h"
#include "engine/log.h"

bool PTex_Load(const char* path, C3D_Tex* out) {
    char full[96];
    snprintf(full, sizeof(full), "romfs:%s", path);
    FILE* f = fopen(full, "rb");
    if (f == NULL) {
        log_printf("ptex: %s missing\n", full);
        return false;
    }
    PTexHeader h;
    bool ok = fread(&h, sizeof(h), 1, f) == 1 && memcmp(h.magic, "PTEX", 4) == 0 &&
              C3D_TexInit(out, h.width, h.height, (GPU_TEXCOLOR)h.format);
    if (ok) {
        ok = h.dataSize <= out->size && fread(out->data, 1, h.dataSize, f) == h.dataSize;
        if (ok) {
            C3D_TexFlush(out);
        } else {
            C3D_TexDelete(out);
        }
    }
    fclose(f);
    if (!ok) {
        log_printf("ptex: %s invalid\n", full);
    }
    return ok;
}
