// segmented_address.h (rewritten: native). Asset handles are resolved by engine/asset.h.
#ifndef SEGMENTED_ADDRESS_H
#define SEGMENTED_ADDRESS_H
#include "engine/asset.h"
// Segment number / offset of an asset handle (handles carry the original segmented address)
#define SEGMENT_NUMBER(a) ((((uintptr_t)(a)) >> 24) & 0xF)
#define SEGMENT_OFFSET(a) (((uintptr_t)(a)) & 0x00FFFFFF)
#endif
