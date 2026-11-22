// fault.h (rewritten: native). The game's crash-reporting entry points, implemented by the native
// fault screen (engine/fault.c). The original fault thread / framebuffer drawer is not ported.
#ifndef FAULT_H
#define FAULT_H
#include "game_types.h"
void Fault_AddHungupAndCrash(const char* file, int line) __attribute__((noreturn));
void Fault_AddHungupAndCrashImpl(const char* exp1, const char* exp2) __attribute__((noreturn));
#define HUNGUP_AND_CRASH(file, line) Fault_AddHungupAndCrash(file, line)
#endif
