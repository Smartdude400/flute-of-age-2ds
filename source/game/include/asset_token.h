// asset_token.h (rewritten: native)
// Asset handles. A symbol from an object/scene (e.g. gArenChildSkel) is an opaque token whose
// "address" is the original segmented address; the native engine resolves it against the
// converted DS data of the object loaded for that segment (engine/dsobj.c, gfx3d.c).
// Works both as `&gArenChildSkel` and as a decayed pointer `gArenChildDL`.
#ifndef ASSET_TOKEN_H
#define ASSET_TOKEN_H
#include <stdint.h>
typedef unsigned char AssetToken[1];
#define ASSET_TOKEN(addr) (*(AssetToken*)(uintptr_t)(addr))
#define ASSET_ADDR(ptr) ((uint32_t)(uintptr_t)(ptr))
#endif
