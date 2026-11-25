// adapted from flute-of-age/src/code/z_vr_box_draw.c
// Skybox drawing. The matrix work on the game's matrix stack is kept as in the original (it changes the
// current matrix); the display lists become native draws of the face quad lists built by
// Skybox_CalculateFace* (z_vr_box.c) with the converted face textures of the bound sky files.
#include "skybox.h"
#include "attributes.h"
#include "sys_matrix.h"
#include "engine/gfx3d.h"
#include "engine/skybox_native.h"

static MtxF sSkyboxDrawMatrix;

void Skybox_UpdateMatrix(SkyboxContext* skyboxCtx, f32 x, f32 y, f32 z) {
    Matrix_Translate(x, y, z, MTXMODE_NEW);
    Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);
    Matrix_RotateX(skyboxCtx->rot.x, MTXMODE_APPLY);
    Matrix_RotateY(skyboxCtx->rot.y, MTXMODE_APPLY);
    Matrix_RotateZ(skyboxCtx->rot.z, MTXMODE_APPLY);
    sSkyboxDrawMatrix = *Matrix_GetCurrent(); // was MATRIX_TO_MTX into the draw matrix
}

// one original face display list (skyboxCtx->dListBuf[index]) -> native face draw
static void draw_face_list(SkyboxContext* skyboxCtx, s32 index, bool twoFiles) {
    const SkyFaceList* l = (const SkyFaceList*)((const u8*)skyboxCtx->dListBuf + index * SKYFACE_LIST_BYTES);
    const struct DsTexture* t0 = SkyboxNative_FaceTexture(skyboxCtx->staticSegments[0], l->face);
    const struct DsTexture* t1 = twoFiles ? SkyboxNative_FaceTexture(skyboxCtx->staticSegments[1], l->face) : t0;
    Gfx3D_DrawSky(skyboxCtx->roomVtx, l->idx, l->numIdx, t0, t1);
}

static void Skybox_DrawImpl(SkyboxContext* skyboxCtx, struct GraphicsContext* gfxCtx, s16 skyboxId, s16 blend, f32 x,
                            f32 y, f32 z) {
    (void)gfxCtx;
    Gfx3D_SetupDLTo(GFX_BUF_OPA, 40); // Gfx_SetupDL_40Opa

    // segments 7 / 8 / 9 = the two texture files and the palettes: resolved per face below
    GFX_TO(OPA, Gfx3D_SetPrimColor(0, 0, 0, blend));

    Matrix_Translate(x, y, z, MTXMODE_NEW);
    Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);
    Matrix_RotateX(skyboxCtx->rot.x, MTXMODE_APPLY);
    Matrix_RotateY(skyboxCtx->rot.y, MTXMODE_APPLY);
    Matrix_RotateZ(skyboxCtx->rot.z, MTXMODE_APPLY);
    sSkyboxDrawMatrix = *Matrix_GetCurrent();
    GFX_TO(OPA, Gfx3D_LoadMtxF(&sSkyboxDrawMatrix));

    if (skyboxCtx->drawType != SKYBOX_DRAW_128) {
        // 256x256 textures, per-face palettes (converted per face); 2, 3 or 4 faces
        draw_face_list(skyboxCtx, 0, false); // -z face upper
        draw_face_list(skyboxCtx, 1, false); // -z face lower
        draw_face_list(skyboxCtx, 2, false); // +x face upper
        draw_face_list(skyboxCtx, 3, false); // +x face lower

        if (skyboxId != SKYBOX_BAZAAR) {
            if (skyboxId < SKYBOX_SYLVAN_SHOP || skyboxId > SKYBOX_BOMBMOUSE_SHOP) {
                // Skip remaining faces for most shop skyboxes
                draw_face_list(skyboxCtx, 4, false); // +z face upper
                draw_face_list(skyboxCtx, 5, false); // +z face lower

                if (skyboxCtx->drawType != SKYBOX_DRAW_256_3FACE) {
                    draw_face_list(skyboxCtx, 6, false); // -x face upper
                    draw_face_list(skyboxCtx, 7, false); // -x face lower
                }
            }
        }
    } else {
        // 128x128 and 128x64 textures, two sky files blended by the prim alpha; 5 or 6 faces
        draw_face_list(skyboxCtx, 0, true); // -z face
        draw_face_list(skyboxCtx, 2, true); // +z face
        draw_face_list(skyboxCtx, 4, true); // -x face
        draw_face_list(skyboxCtx, 6, true); // +x face
        draw_face_list(skyboxCtx, 8, true); // +y face
        if (skyboxId == SKYBOX_CUTSCENE_MAP) {
            // Only draw the bottom face in the cutscene map
            draw_face_list(skyboxCtx, 10, true); // -y face
        }
    }
}

// native (60 Hz presentation): the sky's matrices are matched with its previous frame's (engine/gfx3d.h)
void Skybox_Draw(SkyboxContext* skyboxCtx, struct GraphicsContext* gfxCtx, s16 skyboxId, s16 blend, f32 x, f32 y,
                 f32 z) {
    u32 interpCtx = Gfx3D_InterpGetContext();

    Gfx3D_InterpSetContext(0xB0000000u | (u32)skyboxId);
    Skybox_DrawImpl(skyboxCtx, gfxCtx, skyboxId, blend, x, y, z);
    Gfx3D_InterpSetContext(interpCtx);
}

void Skybox_Update(UNUSED SkyboxContext* skyboxCtx) {
}
