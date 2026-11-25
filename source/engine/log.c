// engine/log.c (rewritten: native)
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "engine/log.h"

static bool sConsoleReady;
static FILE* sFile;
static char sLogBuf[256];

void log_init(void) {
    // the log goes to the SD card and the debugger; the bottom screen belongs to the game (menu and HUD)
    sFile = fopen("sdmc:/foa_log.txt", "w");
}

void log_console(void) {
    if (!sConsoleReady) {
        consoleInit(GFX_BOTTOM, NULL);
        sConsoleReady = true;
    }
}

void log_printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sLogBuf, sizeof(sLogBuf), fmt, ap);
    va_end(ap);
    size_t n = strlen(sLogBuf);
    svcOutputDebugString(sLogBuf, n);
    if (sFile != NULL) {
        fputs(sLogBuf, sFile);
        fflush(sFile);
    }
    if (sConsoleReady) {
        fputs(sLogBuf, stdout);
    }
}
