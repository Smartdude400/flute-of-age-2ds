// (rewritten: native)
// engine/res.h - RomFS resource loader with manifest CRC verification.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool res_init(void);                         // mounts RomFS and parses /manifest.txt
void* res_load(const char* path, size_t* outSize); // malloc'd; NULL on failure
bool res_verify(const char* path, const void* data, size_t size); // CRC check vs manifest
int res_manifest_count(void);
