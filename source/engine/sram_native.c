// engine/sram_native.c (rewritten: native)
// Replaces the original cartridge SRAM driver (src/code/z_ss_sram.c over the PI bus). The game's
// save logic (z_sram.c: layout, checksums, slots) is unchanged and addresses this 32 KiB buffer,
// which is backed by sdmc:/3ds/foa/save.sra: loaded on first access, every write is written through.
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "engine/sram_native.h"
#include "engine/log.h"

#define SAVE_DIR "sdmc:/3ds/foa"
#define SAVE_PATH SAVE_DIR "/save.sra"

static u8 sSram[SRAM_NATIVE_SIZE] __attribute__((aligned(8)));
static bool sLoaded;
static bool sDirty;

static void load(void) {
    sLoaded = true;
    FILE* f = fopen(SAVE_PATH, "rb");
    if (f == NULL) {
        memset(sSram, 0, sizeof(sSram)); // blank cartridge
        return;
    }
    size_t n = fread(sSram, 1, sizeof(sSram), f);
    fclose(f);
    log_printf("sram: loaded %u bytes\n", (unsigned)n);
}

static void write_through(s32 addr, size_t size) {
    FILE* f = fopen(SAVE_PATH, "r+b");
    if (f == NULL) {
        mkdir("sdmc:/3ds", 0777);
        mkdir(SAVE_DIR, 0777);
        f = fopen(SAVE_PATH, "w+b");
        if (f == NULL) {
            log_printf("sram: cannot create %s\n", SAVE_PATH);
            return;
        }
        fwrite(sSram, 1, sizeof(sSram), f); // whole image once, then ranges
    } else {
        fseek(f, addr, SEEK_SET);
        fwrite(&sSram[addr], 1, size, f);
    }
    fclose(f);
}

void SsSram_ReadWrite(s32 addr, void* dramAddr, size_t size, s32 direction) {
    if (!sLoaded) {
        load();
    }
    if (addr < 0 || (u32)addr + size > SRAM_NATIVE_SIZE) {
        log_printf("sram: out of range %ld+%u\n", (long)addr, (unsigned)size);
        return;
    }
    if (direction == SRAM_DIR_READ) {
        memcpy(dramAddr, &sSram[addr], size);
    } else {
        memcpy(&sSram[addr], dramAddr, size);
        sDirty = true;
        write_through(addr, size);
    }
}

bool SramNative_IsDirty(void) {
    return sDirty;
}

u8* SramNative_Data(void) {
    if (!sLoaded) {
        load();
    }
    return sSram;
}
