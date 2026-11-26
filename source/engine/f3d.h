// engine/f3d.h (native) - runtime display-list compiler.
// The original F3DEX2 display lists of the asset files (raw, big-endian: romfs/vrom/<vrom>.raw) are walked when
// the game draws them, with the actual segment bindings and the texture state (tiles, texture-memory loads,
// palette) left by the lists and texture loads drawn before them in the same buffer, as on the original. The
// result is a mesh program (engine/pmesh.h) executed by the renderer like the offline-compiled ones; it is cached
// by list address and entry texture state. Replaces the offline compiler for drawing (tools/dl2pica.py), whose
// fixed, guessed entry state misread materials.
#pragma once
#include "engine/types.h"
#include "engine/dsobj.h"
#include "engine/pmesh.h"

extern bool gF3DEnabled; // runtime compiler on (else the offline-compiled meshes are drawn)

// List at segAddr (a segmented address; its segment bound to an object with raw data), drawn into buffer buf.
// false: not available (no raw data, or the list cannot be compiled): the caller falls back.
bool F3D_Resolve(u32 segAddr, int buf, const DsObj** outObj, const PMeshHdr** outMesh);
// A texture block load issued by game code (gDPLoadTextureBlock and the like) into buffer buf: following lists
// see it in texture memory, as on the original.
void F3D_NoteTextureBlock(int buf, int tile, const void* img, int fmt, int siz, int w, int h, int pal, int cms,
                          int cmt, int masks, int maskt);
void F3D_BeginFrame(void); // texture state of every buffer back to the frame start
void F3D_FlushCache(void); // forget compiled lists and textures (asset memory freed or reused)
void F3D_ReleaseRetired(void); // free what was flushed and is no longer used (GPU idle: at replay)
void F3D_Stats(u32* compiled, u32* cached, u32* failed);
