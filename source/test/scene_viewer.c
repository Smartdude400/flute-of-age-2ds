// test/scene_viewer.c (rewritten: native TEST HARNESS, not game code)
// Phase 3c milestone: one full level area (all rooms of the first overworld scene) loaded from
// converted data and drawn by the ADAPTED Room_Draw (z_room.c). Free-fly test camera:
// D-pad move, L/R down/up, Y/X turn.
#include "engine/types.h"
#include <stdio.h>
#include "play_state.h"
#include "scene.h"
#include "room.h"
#include "sys_matrix.h"
#include "engine/asset.h"
#include "engine/gfx3d.h"
#include "engine/gfx2d.h"
#include "engine/log.h"
#include "engine/runner.h"
#include "engine/room_native.h"
#include "test/scene_viewer.h"
#include "skybox.h"
#include "save.h"

DsObj* Vrom_Load(u32 vromStart);

#define MAX_ROOMS 8
static Room sRooms[MAX_ROOMS];
static int sNumRooms;
static Vec3f sEye = { -200, 200, 1300 };
static s16 sYaw = (s16)0x8000;
static s16 sPitch = -0x600;
static EnvLightSettings* sEnv;

static void perspective(MtxF* m, f32 fovyDeg, f32 aspect, f32 n, f32 f) {
    f32 c = 1.0f / tanf(fovyDeg * (3.14159265f / 360.0f));
    memset(m, 0, sizeof(*m));
    m->mf[0][0] = c / aspect;
    m->mf[1][1] = c;
    m->mf[2][2] = -(f + n) / (f - n);
    m->mf[2][3] = -1.0f;
    m->mf[3][2] = -2.0f * f * n / (f - n);
}

static void view_matrix(MtxF* m) {
    // camera rotation: yaw around Y then pitch around X; view = inverse
    f32 cy = cosf(sYaw * (3.14159265f / 32768.0f)), sy = sinf(sYaw * (3.14159265f / 32768.0f));
    f32 cp = cosf(sPitch * (3.14159265f / 32768.0f)), sp = sinf(sPitch * (3.14159265f / 32768.0f));
    // camera basis (row vectors) for camera looking down -Z when yaw = 0
    Vec3f x = { cy, 0, -sy };
    Vec3f y = { sy * sp, cp, cy * sp };
    Vec3f z = { sy * cp, -sp, cy * cp };
    memset(m, 0, sizeof(*m));
    m->mf[0][0] = x.x; m->mf[1][0] = x.y; m->mf[2][0] = x.z;
    m->mf[0][1] = y.x; m->mf[1][1] = y.y; m->mf[2][1] = y.z;
    m->mf[0][2] = z.x; m->mf[1][2] = z.y; m->mf[2][2] = z.z;
    m->mf[3][0] = -(x.x * sEye.x + x.y * sEye.y + x.z * sEye.z);
    m->mf[3][1] = -(y.x * sEye.x + y.y * sEye.y + y.z * sEye.z);
    m->mf[3][2] = -(z.x * sEye.x + z.y * sEye.y + z.z * sEye.z);
    m->mf[3][3] = 1.0f;
}

void SceneViewer_Init(PlayState* play, u32 sceneVrom) {
    Matrix_Init(&play->state);
    // P2b: the game's own skybox setup (midday normal sky: no blend)
    gSaveContext.skyboxTime = CLOCK_TIME(12, 0);
    Skybox_Init(&play->state, &play->skyboxCtx, SKYBOX_NORMAL_SKY);
    Asset_SetSegment(4, Vrom_Load(0x00F03000 /* gameplay_keep */));
    DsObj* scene = Vrom_Load(sceneVrom);
    Asset_SetSegment(2, scene);
    SceneCmd* cmd = Asset_Resolve((void*)(uintptr_t)0x82000000u);
    for (; cmd != NULL && cmd->base.code != SCENE_CMD_ID_END; cmd++) {
        if (cmd->base.code == SCENE_CMD_ID_COLLISION_HEADER) {
            CollisionHeader* col = (CollisionHeader*)cmd->colHeader.data;
            log_printf("[scene] collision: %d verts %d polys bounds (%d,%d,%d)-(%d,%d,%d)\n", col->numVertices,
                       col->numPolygons, col->minBounds.x, col->minBounds.y, col->minBounds.z, col->maxBounds.x,
                       col->maxBounds.y, col->maxBounds.z);
            f32 cx = (col->minBounds.x + col->maxBounds.x) * 0.5f, cz = (col->minBounds.z + col->maxBounds.z) * 0.5f;
            f32 ext = (col->maxBounds.z - col->minBounds.z) * 0.55f;
            sEye = (Vec3f){ cx, col->maxBounds.y + ext * 0.9f, cz + ext };
            sYaw = 0;
            sPitch = -0x1C00;
        }
        if (cmd->base.code == SCENE_CMD_ID_LIGHT_SETTINGS_LIST) {
            sEnv = (EnvLightSettings*)cmd->lightSettingList.data;
            log_printf("[scene] %d light settings, amb %d,%d,%d\n", cmd->lightSettingList.length, sEnv->ambientColor[0],
                       sEnv->ambientColor[1], sEnv->ambientColor[2]);
        }
        if (cmd->base.code == SCENE_CMD_ID_ROOM_LIST) {
            RomFile* rf = (RomFile*)cmd->roomList.data;
            sNumRooms = cmd->roomList.length < MAX_ROOMS ? cmd->roomList.length : MAX_ROOMS;
            for (int i = 0; i < sNumRooms; i++) {
                sRooms[i].num = i;
                sRooms[i].segment = Vrom_Load(rf[i].vromStart);
                SceneCmd* rc = RoomNative_HeaderOf(&sRooms[i]);
                for (; rc->base.code != SCENE_CMD_ID_END; rc++) {
                    if (rc->base.code == SCENE_CMD_ID_ROOM_SHAPE) {
                        sRooms[i].roomShape = (RoomShape*)rc->mesh.data;
                    }
                }
                log_printf("[scene] room %d vrom %08lx shape type %d\n", i, (unsigned long)rf[i].vromStart,
                           sRooms[i].roomShape ? sRooms[i].roomShape->base.type : -1);
            }
        }
    }
}

void SceneViewer_Main(PlayState* play) {
    Input* in = &play->state.input[0];
    f32 sy = sinf(sYaw * (3.14159265f / 32768.0f)), cy = cosf(sYaw * (3.14159265f / 32768.0f));
    f32 fwd = in->cur.stick_y * 0.4f, side = in->cur.stick_x * 0.4f;
    sEye.x += -sy * fwd + cy * side;
    sEye.z += -cy * fwd - sy * side;
    if (in->cur.button & 0x0010) sEye.y += 15; // R
    if (in->cur.button & 0x2000) sEye.y -= 15; // L (Z)
    if (in->cur.button & 0x0002) sYaw += 0x200; // Y (C-left)
    if (in->cur.button & 0x0004) sYaw -= 0x200; // X (C-down)

    MtxF proj, view;
    perspective(&proj, 60.0f, 4.0f / 3.0f, 10.0f, 12800.0f);
    view_matrix(&view);
    Gfx3D_SetupFrame(20 << 3, 24 << 3, 28 << 3);
    Gfx3D_SetCamera(&proj, &view);
    if (sEnv != NULL) { // test lighting: first environment light setting (day)
        const s8 dirs[2][3] = { { sEnv->light1Dir[0], sEnv->light1Dir[1], sEnv->light1Dir[2] },
                                { sEnv->light2Dir[0], sEnv->light2Dir[1], sEnv->light2Dir[2] } };
        const u8 cols[2][3] = { { sEnv->light1Color[0], sEnv->light1Color[1], sEnv->light1Color[2] },
                                { sEnv->light2Color[0], sEnv->light2Color[1], sEnv->light2Color[2] } };
        Gfx3D_SetLights(sEnv->ambientColor, 2, dirs, cols);
    }
    Skybox_Draw(&play->skyboxCtx, play->state.gfxCtx, SKYBOX_NORMAL_SKY, play->envCtx.skyboxBlend, sEye.x, sEye.y,
                sEye.z);
    for (int i = 0; i < sNumRooms; i++) {
        Room_Draw(play, &sRooms[i], ROOM_DRAW_OPA | ROOM_DRAW_XLU);
    }
    struct GraphicsContext* g = play->state.gfxCtx;
    char buf[64];
    snprintf(buf, sizeof(buf), "SCENE TEST  rooms=%d", sNumRooms);
    Gfx2D_Text(8, 8, 255, 255, 255, 255, buf);
    snprintf(buf, sizeof(buf), "frame %lu", (unsigned long)g->frameCount);
    Gfx2D_Text(8, 20, 200, 255, 200, 255, buf);
}
