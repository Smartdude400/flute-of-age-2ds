// engine/res.c (rewritten: native) - RomFS resource loader with manifest CRC verification
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/res.h"
#include "engine/crc32.h"
#include "engine/log.h"

typedef struct {
    char path[56];
    uint32_t crc;
    uint32_t size;
} ManifestEntry;

static ManifestEntry* sManifest;
static int sManifestCount;

int res_manifest_count(void) {
    return sManifestCount;
}

static void* load_raw(const char* path, size_t* outSize) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* buf = malloc(size + 1);
    if (buf != NULL) {
        if (fread(buf, 1, size, f) != (size_t)size) {
            free(buf);
            buf = NULL;
        } else {
            ((char*)buf)[size] = 0;
        }
    }
    fclose(f);
    if (outSize) {
        *outSize = size;
    }
    return buf;
}

bool res_init(void) {
    if (R_FAILED(romfsInit())) {
        log_printf("romfsInit failed\n");
        return false;
    }
    size_t size;
    char* text = load_raw("romfs:/manifest.txt", &size);
    if (text == NULL) {
        log_printf("no manifest\n");
        return false;
    }
    int lines = 0;
    for (size_t i = 0; i < size; i++) {
        lines += text[i] == '\n';
    }
    sManifest = calloc(lines, sizeof(ManifestEntry));
    char* save = NULL;
    for (char* line = strtok_r(text, "\n", &save); line != NULL; line = strtok_r(NULL, "\n", &save)) {
        ManifestEntry* e = &sManifest[sManifestCount];
        unsigned long crc, sz;
        if (sscanf(line, "%lx %lu %55s", &crc, &sz, e->path) == 3) {
            e->crc = crc;
            e->size = sz;
            sManifestCount++;
        }
    }
    free(text);
    return true;
}

bool res_verify(const char* path, const void* data, size_t size) {
    for (int i = 0; i < sManifestCount; i++) {
        if (strcmp(sManifest[i].path, path) == 0) {
            uint32_t crc = crc32_calc(data, size);
            bool ok = (crc == sManifest[i].crc) && (size == sManifest[i].size);
            log_printf("verify %s crc=%08lx exp=%08lx %s\n", path, (unsigned long)crc,
                       (unsigned long)sManifest[i].crc, ok ? "OK" : "FAIL");
            return ok;
        }
    }
    log_printf("verify %s: not in manifest\n", path);
    return false;
}

void* res_load(const char* path, size_t* outSize) {
    char full[72];
    snprintf(full, sizeof(full), "romfs:%s", path);
    size_t size = 0;
    void* data = load_raw(full, &size);
    if (data == NULL) {
        log_printf("res_load %s: missing\n", path);
        return NULL;
    }
#ifndef NDEBUG
    if (!res_verify(path, data, size)) {
        free(data);
        return NULL;
    }
#endif
    if (outSize) {
        *outSize = size;
    }
    return data;
}
