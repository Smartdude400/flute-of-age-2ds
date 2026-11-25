// engine/vtx.h (imported: include/ultra64/gbi.h lines 1010-1032)
// The original vertex record. Game code that builds vertices at runtime (the HUD's action button and
// beating heart) fills these by field; the native renderer reads them (engine/gfx3d.c Gfx3D_Vertices).
#ifndef ENGINE_VTX_H
#define ENGINE_VTX_H
#include "engine/types.h"

typedef struct Vtx_t {
    short          ob[3];   /* x, y, z */
    unsigned short flag;
    short          tc[2];   /* texture coord */
    unsigned char  cn[4];   /* color & alpha */
} Vtx_t;

/*
 * Vertex (set up for use with normals)
 */
typedef struct Vtx_tn {
    short          ob[3];   /* x, y, z */
    unsigned short flag;
    short          tc[2];   /* texture coord */
    signed char    n[3];    /* normal */
    unsigned char  a;       /* alpha  */
} Vtx_tn;

typedef union Vtx {
    Vtx_t  v;   /* Use this one for colors  */
    Vtx_tn n;   /* Use this one for normals */
    long long int force_structure_alignment;
} Vtx;

#endif
