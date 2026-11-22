// engine/message_native.c (rewritten: native) - see message_native.h
#include <stdio.h>
#include <string.h>
#include "engine/message_native.h"
#include "engine/log.h"

void MessageNative_LoadText(void* dst, uintptr_t fileVromStart, u32 offset, u32 size) {
    char path[40];
    snprintf(path, sizeof(path), "romfs:/vrom/%08lX.bin", (unsigned long)fileVromStart);
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        log_printf("message: text file %s missing\n", path);
        memset(dst, 0, size);
        return;
    }
    fseek(f, (long)offset, SEEK_SET);
    size_t n = fread(dst, 1, size, f);
    fclose(f);
    if (n < size) {
        memset((u8*)dst + n, 0, size - n);
    }
}
