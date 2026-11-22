// test/viewer.c (rewritten: native TEST HARNESS, not game code)
// Phase 3b milestone: the child player skeleton driven by the ADAPTED game animation code
// (SkelAnime_InitAren / ArenAnimation_Update / AnimTaskQueue_Update / SkelAnime_DrawFlexLod)
// on converted data. The camera here is a fixed test camera (render-only).
#include "engine/types.h"
#include <stdio.h>
#include "play_state.h"
#include "player.h"
#include "sys_matrix.h"
#include "engine/asset.h"
#include "engine/gfx3d.h"
#include "engine/gfx2d.h"
#include "engine/log.h"
#include "assets_agg/gameplay_keep.h"
#include "assets_agg/object_aren_child.h"
#include "test/viewer.h"

DsObj* Vrom_Load(u32 vromStart);
static Vec3s sJointTable[PLAYER_LIMB_BUF_COUNT];
static Vec3s sMorphTable[PLAYER_LIMB_BUF_COUNT];
static SkelAnime sSkel;
static s16 sYaw;
static int sAnimIndex;

static void* const kAnims[] = {
    &gPlayerAnim_aren_normal_wait,
    &gPlayerAnim_aren_normal_run,
    &gPlayerAnim_aren_normal_walk,
};

static void perspective(MtxF* m, f32 fovyDeg, f32 aspect, f32 n, f32 f) {
    f32 c = 1.0f / tanf(fovyDeg * (3.14159265f / 360.0f));
    memset(m, 0, sizeof(*m));
    m->mf[0][0] = c / aspect;
    m->mf[1][1] = c;
    m->mf[2][2] = -(f + n) / (f - n);
    m->mf[2][3] = -1.0f;
    m->mf[3][2] = -2.0f * f * n / (f - n);
}

static void look_at(MtxF* m, Vec3f eye, Vec3f at) {
    Vec3f z = { eye.x - at.x, eye.y - at.y, eye.z - at.z };
    f32 l = sqrtf(z.x * z.x + z.y * z.y + z.z * z.z);
    z.x /= l; z.y /= l; z.z /= l;
    Vec3f x = { z.z, 0, -z.x }; // up = +Y
    l = sqrtf(x.x * x.x + x.z * x.z);
    x.x /= l; x.z /= l;
    Vec3f y = { z.y * x.z, z.z * x.x - z.x * x.z, -z.y * x.x };
    MtxF r = { 0 };
    r.mf[0][0] = x.x; r.mf[1][0] = x.y; r.mf[2][0] = x.z;
    r.mf[0][1] = y.x; r.mf[1][1] = y.y; r.mf[2][1] = y.z;
    r.mf[0][2] = z.x; r.mf[1][2] = z.y; r.mf[2][2] = z.z;
    r.mf[3][0] = -(x.x * eye.x + x.y * eye.y + x.z * eye.z);
    r.mf[3][1] = -(y.x * eye.x + y.y * eye.y + y.z * eye.z);
    r.mf[3][2] = -(z.x * eye.x + z.y * eye.y + z.z * eye.z);
    r.mf[3][3] = 1.0f;
    *m = r;
}

void Viewer_Init(PlayState* play) {
    Matrix_Init(&play->state);
    Asset_SetSegment(4, Vrom_Load(GAMEPLAY_KEEP_VROM));
    Asset_SetSegment(6, Vrom_Load(OBJECT_AREN_CHILD_VROM));
    SkelAnime_InitAren(play, &sSkel, (FlexSkeletonHeader*)&gArenChildSkel, (ArenAnimationHeader*)kAnims[0], 9,
                       sJointTable, sMorphTable, PLAYER_LIMB_BUF_COUNT);
    log_printf("[viewer] skeleton limbs=%d dLists=%d anim frames=%d\n", sSkel.limbCount, sSkel.dListCount,
               (int)sSkel.endFrame + 1);
}

void Viewer_Main(PlayState* play) {
    Input* in = &play->state.input[0];
    if (in->press.button & 0x8000 /* BTN_A */) {
        sAnimIndex = (sAnimIndex + 1) % 3;
        ArenAnimation_Change(play, &sSkel, kAnims[sAnimIndex], 1.0f, 0.0f, Animation_GetLastFrame(kAnims[sAnimIndex]),
                             ANIMMODE_LOOP, -6.0f);
        log_printf("[viewer] anim %d\n", sAnimIndex);
    }
    sYaw += in->cur.stick_x * 8;

    // --- update (adapted game code)
    AnimTaskQueue_Reset(&play->animTaskQueue);
    ArenAnimation_Update(play, &sSkel);
    AnimTaskQueue_Update(play, &play->animTaskQueue);

    // --- draw
    MtxF proj, view;
    perspective(&proj, 60.0f, 4.0f / 3.0f, 10.0f, 4000.0f);
    look_at(&view, (Vec3f){ 0, 30, 90 }, (Vec3f){ 0, 22, 0 });
    Gfx3D_SetupFrame(6 << 3, 10 << 3, 14 << 3);
    Gfx3D_SetCamera(&proj, &view);
    static const u8 amb[3] = { 90, 90, 90 };
    static const s8 dirs[2][3] = { { 40, -80, -60 }, { -60, -20, 40 } };
    static const u8 cols[2][3] = { { 230, 230, 220 }, { 90, 90, 110 } };
    Gfx3D_SetLights(amb, 2, dirs, cols);
    Gfx3D_SetSegmentTex(8, Asset_FindTexture(&gArenChildEyesOpenTex));
    Gfx3D_SetSegmentTex(9, Asset_FindTexture(&gArenChildMouthClosedTex));

    Vec3s rot = { 0, sYaw, 0 };
    Matrix_SetTranslateRotateYXZ(0.0f, 0.0f, 0.0f, &rot);
    Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
    // as Player_DrawImpl does: the tunic colour is the environment colour of the body lists
    extern Color_RGB8 sTunicColors[];
    Gfx3D_SetEnvColor(sTunicColors[0].r, sTunicColors[0].g, sTunicColors[0].b, 0);
    SkelAnime_DrawFlexLod(play, sSkel.skeleton, sSkel.jointTable, sSkel.dListCount, NULL, NULL, NULL, 0);

    Gfx2D_Text(8, 8, 255, 255, 255, 255, "PLAYER MODEL TEST (A: next anim)");
    char buf[48];
    snprintf(buf, sizeof(buf), "frame %d/%d", (int)sSkel.curFrame, (int)sSkel.endFrame);
    Gfx2D_Text(8, 20, 200, 200, 200, 255, buf);
}
