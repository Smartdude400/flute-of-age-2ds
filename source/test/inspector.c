// test/inspector.c (native TOOL, not game code; built only with make INSPECT=1)
// Asset inspector: browse every object of the object table part by part (skeletons in their default pose,
// then each display list), or fly freely through the real game's scenes, and capture what is wrong.
//
// Modes (START or the MODE button):
//   OBJECTS  the selected object part drawn alone (the game state keeps running, unseen)
//   FLY      the real scene with a free camera (the game runs; Link does not move)
//   PLAY     the real game with the normal controls and camera
// Captures (SELECT or the CAPTURE button) append one line to sdmc:/3ds/foa/inspect_captures.txt and save the
// top screen to sdmc:/3ds/foa/inspect_<n>.bmp. sdmc:/3ds/foa/inspect_goto.txt, if present at start, holds
// one such line: the inspector starts there (used to look at a capture again).
#include "engine/types.h"
#include <3ds.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "play_state.h"
#include "actor.h"
#include "object.h"
#include "scene.h"
#include "sys_matrix.h"
#include "save.h"
#include "engine/asset.h"
#include "engine/dsobj.h"
#include "engine/gfx3d.h"
#include "engine/gfx2d.h"
#include "engine/input.h"
#include "engine/log.h"
#include "engine/screen.h"
#include "animation.h"
#include "test/inspector.h"

DsObj* Vrom_Load(u32 vromStart);

// ---------------------------------------------------------------- names (game tables)
#define DEFINE_SCENE(name, _1, _2, _3, _4, _5) #name,
static const char* const kSceneNames[] = {
#include "tables/scene_table.h"
};
#undef DEFINE_SCENE

#define DEFINE_ACTOR(name, _1, _2, _3) #name,
#define DEFINE_ACTOR_INTERNAL(name, _1, _2, _3) #name,
#define DEFINE_ACTOR_UNSET(_0) "unset",
static const char* const kActorNames[] = {
#include "tables/actor_table.h"
};
#undef DEFINE_ACTOR
#undef DEFINE_ACTOR_INTERNAL
#undef DEFINE_ACTOR_UNSET

// ---------------------------------------------------------------- catalog (tools/build_inspect_catalog.py)
#define MAX_OBJS 512
#define MAX_PARTS 768
typedef struct { u16 id; u8 seg; char name[40]; int line; } CatObj;
typedef struct { u32 addr; char kind; char type[8]; char sym[56]; } Part;
static char* sCat;
static CatObj sObjs[MAX_OBJS];
static int sNumObjs;

static void catalog_load(void) {
    FILE* f = fopen("romfs:/inspect/catalog.txt", "rb");
    if (f == NULL) {
        log_printf("[inspect] romfs:/inspect/catalog.txt missing (python tools/build_inspect_catalog.py)\n");
        return;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    sCat = malloc(n + 1);
    fread(sCat, 1, n, f);
    sCat[n] = 0;
    fclose(f);
    for (char* l = sCat; *l; l = strchr(l, '\n') ? strchr(l, '\n') + 1 : l + strlen(l)) {
        unsigned id, seg;
        char name[40];
        if (l[0] == 'O' && sNumObjs < MAX_OBJS && sscanf(l, "O %x %39s %u", &id, name, &seg) == 3) {
            CatObj* o = &sObjs[sNumObjs++];
            o->id = id;
            o->seg = seg;
            strcpy(o->name, name);
            o->line = (strchr(l, '\n') ? strchr(l, '\n') + 1 : l) - sCat;
        }
    }
    log_printf("[inspect] catalog: %d objects\n", sNumObjs);
}

// ---------------------------------------------------------------- state
enum { MODE_OBJECTS, MODE_FLY, MODE_PLAY, MODE_COUNT };
static const char* const kModeNames[] = { "OBJECTS", "FLY", "PLAY" };
static int sMode = MODE_OBJECTS;
static int sObjIdx;            // index into sObjs
static int sLoadedObj = -1;    // object whose data is loaded
static DsObj* sObj;
static Part sParts[MAX_PARTS];
static int sNumParts, sPart;
static u32 sFirstAnim;          // first animation of the object (played on its skeletons), 0 = none
static SkelAnime sSkel;          // the selected skeleton, animated
static Vec3s sJoints[128], sMorph[128];
static bool sSkelReady, sSkelIsPlayer;
static Vec3f sEye, sCenter;
static s16 sYaw, sPitch;
static f32 sRadius = 1000.0f, sSpeed = 1.0f;
static int sSceneId = -1;      // scene loaded by the game
static int sEntrance = -1;     // entrance index requested / in use
static int sPendingEntrance = -1;
static bool sCamFromGame;      // fly camera to be taken from the game's camera once the scene runs
static PlayState* sPlay;
static u32 sPlayFrames;
static int sCaptures;
static char sStatus[64];
static int sStatusTimer;
static bool sInit;
// goto (inspect_goto.txt)
static bool sGotoCam;
static Vec3f sGotoEye;
static s16 sGotoYaw, sGotoPitch;
static char sGotoSym[56];
static int sGotoPart = -1;

static void status(const char* s) {
    strncpy(sStatus, s, sizeof(sStatus) - 1);
    sStatusTimer = 60;
    log_printf("[inspect] %s\n", s);
}

// ---------------------------------------------------------------- object parts
static void mesh_bounds(const PMeshHdr* m, f32 lo[3], f32 hi[3]) {
    if (m->vtxSeg != 0) {
        return; // runtime vertices
    }
    const s16* v = (const s16*)((const u8*)sObj->data + m->vertsOff);
    for (u32 i = 0; i < m->numVerts; i++, v += 8) {
        for (int k = 0; k < 3; k++) {
            if (v[k] < lo[k]) lo[k] = v[k];
            if (v[k] > hi[k]) hi[k] = v[k];
        }
    }
}

static void reset_object_camera(void) {
    f32 lo[3] = { 1e9f, 1e9f, 1e9f }, hi[3] = { -1e9f, -1e9f, -1e9f };
    if (sObj != NULL && sNumParts > 0) {
        const Part* p = &sParts[sPart];
        if (p->kind == 'D') {
            const PMeshHdr* m = DsObj_FindMesh(sObj, p->addr & 0x0FFFFFFF);
            if (m != NULL) mesh_bounds(m, lo, hi);
        } else { // a skeleton: every list of the object (its limbs are among them)
            for (int i = 0; i < sNumParts; i++) {
                const PMeshHdr* m = sParts[i].kind == 'D' ? DsObj_FindMesh(sObj, sParts[i].addr & 0x0FFFFFFF) : NULL;
                if (m != NULL) mesh_bounds(m, lo, hi);
            }
        }
    }
    if (lo[0] > hi[0]) {
        lo[0] = lo[1] = lo[2] = -500.0f;
        hi[0] = hi[1] = hi[2] = 500.0f;
    }
    sCenter = (Vec3f){ (lo[0] + hi[0]) * 0.5f, (lo[1] + hi[1]) * 0.5f, (lo[2] + hi[2]) * 0.5f };
    f32 dx = hi[0] - lo[0], dy = hi[1] - lo[1], dz = hi[2] - lo[2];
    sRadius = sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f;
    if (sRadius < 10.0f) sRadius = 10.0f;
    sEye = (Vec3f){ sCenter.x, sCenter.y + sRadius * 0.4f, sCenter.z + sRadius * 2.2f };
    sYaw = 0;
    sPitch = -0x0A00;
}

static void load_object(int idx) {
    if (sNumObjs == 0) return;
    idx = (idx % sNumObjs + sNumObjs) % sNumObjs;
    sObjIdx = idx;
    if (sLoadedObj == idx) return;
    if (sObj != NULL) {
        DsObj_Free(sObj);
        sObj = NULL;
    }
    const CatObj* o = &sObjs[idx];
    sObj = Vrom_Load(gObjectTable[o->id].vromStart);
    sLoadedObj = idx;
    sNumParts = 0;
    sPart = 0;
    sFirstAnim = 0;
    sSkelReady = false;
    for (const char* l = sCat + o->line; *l && *l != 'O' && sNumParts < MAX_PARTS;
         l = strchr(l, '\n') ? strchr(l, '\n') + 1 : l + strlen(l)) {
        Part* p = &sParts[sNumParts];
        unsigned addr;
        if (l[0] == 'S' && sscanf(l, "S %x %7s %55s", &addr, p->type, p->sym) == 3) {
            p->addr = addr;
            p->kind = 'S';
            sNumParts++;
        } else if (l[0] == 'A' && sscanf(l, "A %x", &addr) == 1) {
            if (sFirstAnim == 0) sFirstAnim = addr;
        } else if (l[0] == 'D' && sscanf(l, "D %x %55s", &addr, p->sym) == 2) {
            p->addr = addr;
            p->kind = 'D';
            p->type[0] = 0;
            if (sObj != NULL && DsObj_FindMesh(sObj, addr & 0x0FFFFFFF) != NULL) {
                sNumParts++; // only lists that were converted
            }
        }
    }
    reset_object_camera();
}

static void select_part(int k) {
    if (sNumParts == 0) return;
    sPart = (k % sNumParts + sNumParts) % sNumParts;
    sSkelReady = false;
    reset_object_camera();
}

// longest chain of limb offsets from the root: a radius that holds for any pose
static f32 limb_reach(void** limbs, int i, int depth) {
    f32 best = 0.0f;
    while (i != 0xFF && depth < 64) {
        const StandardLimb* l = limbs[i];
        f32 len = sqrtf((f32)l->jointPos.x * l->jointPos.x + (f32)l->jointPos.y * l->jointPos.y +
                        (f32)l->jointPos.z * l->jointPos.z);
        f32 r = len + (l->child != 0xFF ? limb_reach(limbs, l->child, depth + 1) : 0.0f);
        if (r > best) best = r;
        i = l->sibling;
        depth++;
    }
    return best;
}

static void frame_skeleton(void** limbs, const Vec3s* root) {
    const StandardLimb* r = limbs[0];
    f32 reach = r->child != 0xFF ? limb_reach(limbs, r->child, 0) : 0.0f;
    sCenter = (Vec3f){ root->x, root->y, root->z };
    sRadius = reach * 1.15f + 50.0f;
    sEye = (Vec3f){ sCenter.x, sCenter.y + sRadius * 0.3f, sCenter.z + sRadius * 2.4f };
    sYaw = 0;
    sPitch = -0x0700;
}

// the selected skeleton with the object's first animation (Link: his idle animation), as actors set it up
static void setup_skeleton(PlayState* play, const Part* p) {
    void* header = (void*)(uintptr_t)(0x80000000u | p->addr);
    FlexSkeletonHeader* h = Asset_Resolve(header);
    if (h == NULL || h->sh.limbCount > 120) return;
    memset(&sSkel, 0, sizeof(sSkel));
    memset(sJoints, 0, sizeof(sJoints));
    memset(sMorph, 0, sizeof(sMorph));
    void* anim = sFirstAnim ? (void*)(uintptr_t)(0x80000000u | sFirstAnim) : NULL;
    sSkelIsPlayer = !strcmp(p->type, "flexlod") && !strncmp(p->sym, "gAren", 5);
    if (sSkelIsPlayer) {
        SkelAnime_InitAren(play, &sSkel, header, (void*)(uintptr_t)0x84003238u /* gPlayerAnim_aren_normal_wait */, 9,
                           sJoints, sMorph, h->sh.limbCount);
    } else if (!strcmp(p->type, "flex") || !strcmp(p->type, "flexlod")) {
        SkelAnime_InitFlex(play, &sSkel, header, anim, sJoints, sMorph, h->sh.limbCount);
    } else {
        SkelAnime_Init(play, &sSkel, header, anim, sJoints, sMorph, h->sh.limbCount);
    }
    if (sSkel.skeleton != NULL) { // first pose, then the framing around its root
        if (sSkelIsPlayer) {
            AnimTaskQueue_Reset(&play->animTaskQueue);
            ArenAnimation_Update(play, &sSkel);
            AnimTaskQueue_Update(play, &play->animTaskQueue);
        } else if (sSkel.animation != NULL) {
            SkelAnime_Update(&sSkel);
        }
        if (!sGotoCam) frame_skeleton(sSkel.skeleton, &sSkel.jointTable[0]);
    }
    sSkelReady = true;
}

// ---------------------------------------------------------------- scenes
static int entrance_of_scene(int scene, int nth) {
    int seen = 0;
    for (int e = 0; e < ENTR_MAX; e++) {
        if (gEntranceTable[e].sceneId == scene && (e == 0 || gEntranceTable[e - 1].sceneId != scene ||
                                                   gEntranceTable[e - 1].spawn != gEntranceTable[e].spawn)) {
            if (seen++ == nth) return e;
        }
    }
    return -1;
}

static int spawn_count(int scene) {
    int n = 0;
    while (entrance_of_scene(scene, n) >= 0 && n < 64) n++;
    return n;
}

static void request_scene(int scene, int nth) {
    for (int tries = 0; tries < SCENE_ID_MAX; tries++) {
        scene = (scene % SCENE_ID_MAX + SCENE_ID_MAX) % SCENE_ID_MAX;
        int e = entrance_of_scene(scene, nth);
        if (e >= 0) {
            sPendingEntrance = e;
            char b[64];
            snprintf(b, sizeof(b), "loading entrance 0x%03X", e);
            status(b);
            return;
        }
        scene += 1;
        nth = 0;
    }
}

static int current_spawn_index(void) {
    if (sSceneId < 0) return 0;
    for (int n = 0; n < 64; n++) {
        int e = entrance_of_scene(sSceneId, n);
        if (e < 0) break;
        if (gEntranceTable[e].spawn == gEntranceTable[sEntrance >= 0 ? sEntrance : e].spawn) return n;
    }
    return 0;
}

// ---------------------------------------------------------------- camera
static void perspective(MtxF* m, f32 fovyDeg, f32 aspect, f32 n, f32 f) {
    f32 c = 1.0f / tanf(fovyDeg * (M_PI / 360.0f));
    memset(m, 0, sizeof(*m));
    m->mf[0][0] = c / aspect;
    m->mf[1][1] = c;
    m->mf[2][2] = -(f + n) / (f - n);
    m->mf[2][3] = -1.0f;
    m->mf[3][2] = -2.0f * f * n / (f - n);
}

static void forward(Vec3f* f) {
    f32 cy = cosf(sYaw * (M_PI / 32768.0f)), sy = sinf(sYaw * (M_PI / 32768.0f));
    f32 cp = cosf(sPitch * (M_PI / 32768.0f)), sp = sinf(sPitch * (M_PI / 32768.0f));
    *f = (Vec3f){ -sy * cp, sp, -cy * cp };
}

static void view_matrix(MtxF* m) {
    f32 cy = cosf(sYaw * (M_PI / 32768.0f)), sy = sinf(sYaw * (M_PI / 32768.0f));
    f32 cp = cosf(sPitch * (M_PI / 32768.0f)), sp = sinf(sPitch * (M_PI / 32768.0f));
    Vec3f x = { cy, 0, -sy }, y = { sy * sp, cp, cy * sp }, z = { sy * cp, -sp, cy * cp };
    memset(m, 0, sizeof(*m));
    m->mf[0][0] = x.x; m->mf[1][0] = x.y; m->mf[2][0] = x.z;
    m->mf[0][1] = y.x; m->mf[1][1] = y.y; m->mf[2][1] = y.z;
    m->mf[0][2] = z.x; m->mf[1][2] = z.y; m->mf[2][2] = z.z;
    m->mf[3][0] = -(x.x * sEye.x + x.y * sEye.y + x.z * sEye.z);
    m->mf[3][1] = -(y.x * sEye.x + y.y * sEye.y + y.z * sEye.z);
    m->mf[3][2] = -(z.x * sEye.x + z.y * sEye.y + z.z * sEye.z);
    m->mf[3][3] = 1.0f;
}

static void camera_from(const Vec3f* eye, const Vec3f* at) {
    sEye = *eye;
    f32 dx = at->x - eye->x, dy = at->y - eye->y, dz = at->z - eye->z;
    sYaw = (s16)(atan2f(-dx, -dz) * (32768.0f / M_PI));
    sPitch = (s16)(atan2f(dy, sqrtf(dx * dx + dz * dz)) * (32768.0f / M_PI));
}

static void fly(const Input* in, u32 held, f32 step) {
    Vec3f f;
    forward(&f);
    f32 cy = cosf(sYaw * (M_PI / 32768.0f)), sy = sinf(sYaw * (M_PI / 32768.0f));
    f32 fwd = in->cur.stick_y / 80.0f * step, side = in->cur.stick_x / 80.0f * step;
    sEye.x += f.x * fwd + cy * side;
    sEye.y += f.y * fwd;
    sEye.z += f.z * fwd - sy * side;
    if (held & KEY_R) sEye.y += step * 0.6f;
    if (held & KEY_L) sEye.y -= step * 0.6f;
    if (held & KEY_DLEFT) sYaw += 0x300;
    if (held & KEY_DRIGHT) sYaw -= 0x300;
    if (held & KEY_DUP) sPitch += 0x200;
    if (held & KEY_DDOWN) sPitch -= 0x200;
    if (sPitch > 0x3F00) sPitch = 0x3F00;
    if (sPitch < -0x3F00) sPitch = -0x3F00;
}

// ---------------------------------------------------------------- touch buttons (bottom screen)
enum { BTN_MODE, BTN_CAPTURE, BTN_RESET, BTN_SLOWER, BTN_FASTER, BTN_PREV10, BTN_PREV, BTN_NEXT, BTN_NEXT10,
       BTN_PPREV, BTN_PNEXT, BTN_COUNT };
static const struct { s16 x, y, w, h; const char* label; } kButtons[BTN_COUNT] = {
    { 4, 204, 60, 32, "MODE" },    { 68, 204, 76, 32, "CAPTURE" }, { 148, 204, 56, 32, "RESET" },
    { 208, 204, 52, 32, "SLOW" },  { 264, 204, 52, 32, "FAST" },   { 4, 166, 72, 32, "<<10" },
    { 80, 166, 76, 32, "< PREV" }, { 160, 166, 76, 32, "NEXT >" }, { 240, 166, 76, 32, "10>>" },
    { 4, 128, 152, 32, "< PART" }, { 164, 128, 152, 32, "PART >" },
};

static int touched_button(void) {
    s16 x, y;
    if (!input_touch_tapped(&x, &y)) return -1;
    for (int b = 0; b < BTN_COUNT; b++) {
        if (x >= kButtons[b].x && x < kButtons[b].x + kButtons[b].w && y >= kButtons[b].y &&
            y < kButtons[b].y + kButtons[b].h) {
            return b;
        }
    }
    return -1;
}

// ---------------------------------------------------------------- captures
static void nearby_actors(char* out, size_t n) {
    out[0] = 0;
    if (sPlay == NULL) return;
    struct { f32 d; Actor* a; } best[3] = { { 1e30f, NULL }, { 1e30f, NULL }, { 1e30f, NULL } };
    Vec3f f;
    forward(&f);
    Vec3f look = { sEye.x + f.x * 300.0f, sEye.y + f.y * 300.0f, sEye.z + f.z * 300.0f };
    if (sMode == MODE_PLAY) look = sPlay->view.at;
    for (int c = 0; c < ACTORCAT_MAX; c++) {
        for (Actor* a = sPlay->actorCtx.actorLists[c].head; a != NULL; a = a->next) {
            f32 dx = a->world.pos.x - look.x, dy = a->world.pos.y - look.y, dz = a->world.pos.z - look.z;
            f32 d = dx * dx + dy * dy + dz * dz;
            for (int k = 0; k < 3; k++) {
                if (d < best[k].d) {
                    for (int j = 2; j > k; j--) best[j] = best[j - 1];
                    best[k].d = d;
                    best[k].a = a;
                    break;
                }
            }
        }
    }
    for (int k = 0; k < 3 && best[k].a != NULL; k++) {
        char b[64];
        Actor* a = best[k].a;
        snprintf(b, sizeof(b), "%s%s:0x%04X@%d,%d,%d", k ? ";" : "",
                 a->id < (s16)(sizeof(kActorNames) / sizeof(kActorNames[0])) ? kActorNames[a->id] : "?",
                 (u16)a->params, (int)a->world.pos.x, (int)a->world.pos.y, (int)a->world.pos.z);
        strncat(out, b, n - strlen(out) - 1);
    }
}

static void capture(void) {
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/foa", 0777);
    char line[512], actors[200];
    Vec3f eye = sEye;
    s16 yaw = sYaw, pitch = sPitch;
    if (sMode == MODE_PLAY && sPlay != NULL) {
        Vec3f e = sPlay->view.eye, a = sPlay->view.at;
        Vec3f se = sEye;
        s16 sy = sYaw, sp = sPitch;
        camera_from(&e, &a);
        eye = sEye;
        yaw = sYaw;
        pitch = sPitch;
        sEye = se;
        sYaw = sy;
        sPitch = sp;
    }
    if (sMode == MODE_OBJECTS) {
        const Part* p = sNumParts ? &sParts[sPart] : NULL;
        snprintf(line, sizeof(line),
                 "capture=%d mode=objects object=0x%04X name=%s part=%d kind=%s type=%s symbol=%s addr=0x%08lX "
                 "eye=%.1f,%.1f,%.1f yaw=0x%04X pitch=0x%04X",
                 sCaptures, sObjs[sObjIdx].id, sObjs[sObjIdx].name, sPart,
                 p ? (p->kind == 'S' ? "skeleton" : "dlist") : "-", p && p->type[0] ? p->type : "-",
                 p ? p->sym : "-", p ? (unsigned long)p->addr : 0ul, eye.x, eye.y, eye.z, (u16)yaw, (u16)pitch);
    } else {
        nearby_actors(actors, sizeof(actors));
        snprintf(line, sizeof(line),
                 "capture=%d mode=%s scene=0x%02X name=%s entrance=0x%03X room=%d time=0x%04X "
                 "eye=%.1f,%.1f,%.1f yaw=0x%04X pitch=0x%04X actors=%s",
                 sCaptures, sMode == MODE_FLY ? "fly" : "play", sSceneId,
                 sSceneId >= 0 ? kSceneNames[sSceneId] : "-", sEntrance & 0xFFFF,
                 sPlay ? sPlay->roomCtx.curRoom.num : -1, (u16)gSaveContext.save.dayTime, eye.x, eye.y, eye.z,
                 (u16)yaw, (u16)pitch, actors[0] ? actors : "-");
    }
    FILE* f = fopen("sdmc:/3ds/foa/inspect_captures.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s\n", line);
        fclose(f);
    }
    log_printf("[inspect] %s\n", line);
    char path[64];
    snprintf(path, sizeof(path), "sdmc:/3ds/foa/inspect_%03d.bmp", sCaptures);
    Screen_RequestSnapshot(path);
    char b[64];
    snprintf(b, sizeof(b), "captured #%d", sCaptures);
    status(b);
    sCaptures++;
}

static void count_captures(void) {
    FILE* f = fopen("sdmc:/3ds/foa/inspect_captures.txt", "r");
    if (f == NULL) return;
    char l[512];
    while (fgets(l, sizeof(l), f)) sCaptures++;
    fclose(f);
}

static const char* field(const char* line, const char* key) {
    char k[24];
    snprintf(k, sizeof(k), " %s=", key);
    const char* p = strstr(line, k);
    return p ? p + strlen(k) : NULL;
}

static void read_goto(void) {
    FILE* f = fopen("sdmc:/3ds/foa/inspect_goto.txt", "r");
    if (f == NULL) return;
    char line[512] = " ";
    fgets(line + 1, sizeof(line) - 1, f);
    fclose(f);
    const char* v;
    if ((v = field(line, "mode")) != NULL) {
        sMode = !strncmp(v, "fly", 3) ? MODE_FLY : !strncmp(v, "play", 4) ? MODE_PLAY : MODE_OBJECTS;
    }
    if ((v = field(line, "object")) != NULL) {
        unsigned id = strtoul(v, NULL, 16);
        for (int i = 0; i < sNumObjs; i++) {
            if (sObjs[i].id == id) sObjIdx = i;
        }
    }
    if ((v = field(line, "symbol")) != NULL) sscanf(v, "%55s", sGotoSym);
    if ((v = field(line, "part")) != NULL) sGotoPart = atoi(v);
    if ((v = field(line, "entrance")) != NULL) {
        int e = strtoul(v, NULL, 16);
        if (sMode != MODE_OBJECTS && e != gSaveContext.save.entranceIndex) sPendingEntrance = e;
    }
    if ((v = field(line, "eye")) != NULL &&
        sscanf(v, "%f,%f,%f", &sGotoEye.x, &sGotoEye.y, &sGotoEye.z) == 3) {
        sGotoCam = true;
    }
    if ((v = field(line, "yaw")) != NULL) sGotoYaw = (s16)strtoul(v, NULL, 16);
    if ((v = field(line, "pitch")) != NULL) sGotoPitch = (s16)strtoul(v, NULL, 16);
    log_printf("[inspect] goto: %s", line + 1);
}

static void apply_goto_object(void) {
    load_object(sObjIdx);
    for (int i = 0; i < sNumParts; i++) {
        if (sGotoSym[0] != 0 && strcmp(sParts[i].sym, sGotoSym) == 0) sGotoPart = i;
    }
    if (sGotoPart >= 0) select_part(sGotoPart);
    if (sGotoCam) {
        sEye = sGotoEye;
        sYaw = sGotoYaw;
        sPitch = sGotoPitch;
    }
}

// ---------------------------------------------------------------- hooks
static void init_once(void) {
    if (sInit) return;
    sInit = true;
    catalog_load();
    count_captures();
    // start on Link's object, a familiar model
    for (int i = 0; i < sNumObjs; i++) {
        if (!strcmp(sObjs[i].name, "object_aren_child")) sObjIdx = i;
    }
    read_goto();
    if (sMode == MODE_OBJECTS) {
        apply_goto_object();
    } else if (sGotoCam) {
        sCamFromGame = false;
    }
}

bool Inspector_Input(Input* input) {
    init_once();
    u32 held = input_keys_held(), pressed = input_keys_pressed();
    int b = touched_button();
    if (pressed & KEY_SELECT) b = BTN_CAPTURE;
    if ((pressed & KEY_START) && sMode != MODE_PLAY) b = BTN_MODE;
    if (b == BTN_MODE) {
        sMode = (sMode + 1) % MODE_COUNT;
        if (sMode == MODE_FLY && sPlay != NULL) {
            camera_from(&sPlay->view.eye, &sPlay->view.at);
        }
        if (sMode == MODE_OBJECTS) load_object(sObjIdx);
        char s[32];
        snprintf(s, sizeof(s), "mode %s", kModeNames[sMode]);
        status(s);
    } else if (b == BTN_CAPTURE) {
        capture();
    } else if (b == BTN_SLOWER) {
        sSpeed = sSpeed > 0.126f ? sSpeed * 0.5f : sSpeed;
    } else if (b == BTN_FASTER) {
        sSpeed = sSpeed < 16.0f ? sSpeed * 2.0f : sSpeed;
    }
    if (sMode == MODE_OBJECTS) {
        int d = 0;
        if (b == BTN_NEXT || (pressed & KEY_A)) d = 1;
        if (b == BTN_PREV || (pressed & KEY_B)) d = -1;
        if (b == BTN_NEXT10) d = 10;
        if (b == BTN_PREV10) d = -10;
        if (d != 0) {
            load_object(sObjIdx + d);
        }
        if (b == BTN_PNEXT || (pressed & KEY_X)) select_part(sPart + 1);
        if (b == BTN_PPREV || (pressed & KEY_Y)) select_part(sPart - 1);
        if (b == BTN_RESET) reset_object_camera();
        fly(input, held, sRadius * 0.06f * sSpeed);
        return true; // the game sees no input
    }
    if (sMode == MODE_FLY || sMode == MODE_PLAY) {
        int scene = sSceneId >= 0 ? sSceneId : 0;
        if (b == BTN_NEXT || (sMode == MODE_FLY && (pressed & KEY_A))) request_scene(scene + 1, 0);
        if (b == BTN_PREV || (sMode == MODE_FLY && (pressed & KEY_B))) request_scene(scene - 1, 0);
        if (b == BTN_NEXT10) request_scene(scene + 10, 0);
        if (b == BTN_PREV10) request_scene(scene - 10, 0);
        int n = spawn_count(scene);
        if (n > 0 && (b == BTN_PNEXT || (sMode == MODE_FLY && (pressed & KEY_X)))) {
            request_scene(scene, (current_spawn_index() + 1) % n);
        }
        if (n > 0 && (b == BTN_PPREV || (sMode == MODE_FLY && (pressed & KEY_Y)))) {
            request_scene(scene, (current_spawn_index() + n - 1) % n);
        }
        if (b == BTN_RESET && sPlay != NULL) camera_from(&sPlay->view.eye, &sPlay->view.at);
    }
    if (sMode == MODE_FLY) {
        fly(input, held, 20.0f * sSpeed);
        return true;
    }
    return false; // PLAY: the game gets the input
}

void Inspector_PlayFrame(PlayState* play) {
    init_once();
    if (play != sPlay || play->state.frames < sPlayFrames) { // a new scene was entered
        sPlay = play;
        sCamFromGame = !sGotoCam;
    }
    sPlayFrames = play->state.frames;
    sSceneId = play->sceneId;
    // Link does not move while you fly: the inspector keeps him alive
    gSaveContext.save.info.playerData.health = gSaveContext.save.info.playerData.healthCapacity;
    if (sEntrance < 0 || play->state.frames < 2) sEntrance = gSaveContext.save.entranceIndex;
    if (play->state.frames == 4) {
        if (sGotoCam && sMode != MODE_OBJECTS) {
            sEye = sGotoEye;
            sYaw = sGotoYaw;
            sPitch = sGotoPitch;
            sGotoCam = false;
        } else if (sCamFromGame && sMode != MODE_OBJECTS) {
            camera_from(&play->view.eye, &play->view.at);
        }
        sCamFromGame = false;
    }
    if (sPendingEntrance >= 0 && play->transitionTrigger == TRANS_TRIGGER_OFF && play->state.frames > 2) {
        play->nextEntranceIndex = sPendingEntrance;
        play->transitionTrigger = TRANS_TRIGGER_START;
        play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
        gSaveContext.save.cutsceneIndex = 0;
        sEntrance = sPendingEntrance;
        sPendingEntrance = -1;
    }
}

void Inspector_OverrideView(PlayState* play) {
    if (sMode != MODE_FLY || play->state.frames < 4) return;
    Vec3f f, at, up = { 0.0f, 1.0f, 0.0f };
    forward(&f);
    at = (Vec3f){ sEye.x + f.x * 100.0f, sEye.y + f.y * 100.0f, sEye.z + f.z * 100.0f };
    View_LookAt(&play->view, &sEye, &at, &up);
    View_SetPerspective(&play->view, 60.0f, play->view.zNear, play->view.zFar); // fixed: captures replay exactly
}

static void draw_ui(void) {
    char b[80];
    // background panels
    Gfx2D_Rect(0, 0, 320, 124, 16, 18, 26, 255);
    int y = 4;
    snprintf(b, sizeof(b), "INSPECTOR  [%s]  captures %d", kModeNames[sMode], sCaptures);
    Gfx2D_Text(4, y, 255, 230, 120, 255, b);
    y += 12;
    if (sMode == MODE_OBJECTS) {
        const CatObj* o = sNumObjs ? &sObjs[sObjIdx] : NULL;
        snprintf(b, sizeof(b), "object %d/%d  id 0x%04X", sObjIdx + 1, sNumObjs, o ? o->id : 0);
        Gfx2D_Text(4, y, 255, 255, 255, 255, b);
        y += 10;
        snprintf(b, sizeof(b), "%s", o ? o->name : "(catalog missing)");
        Gfx2D_Text(4, y, 160, 220, 255, 255, b);
        y += 12;
        if (sNumParts > 0) {
            const Part* p = &sParts[sPart];
            snprintf(b, sizeof(b), "part %d/%d  %s %s", sPart + 1, sNumParts, p->kind == 'S' ? "skeleton" : "dlist",
                     p->type);
            Gfx2D_Text(4, y, 255, 255, 255, 255, b);
            y += 10;
            snprintf(b, sizeof(b), "%s", p->sym);
            Gfx2D_Text(4, y, 160, 255, 160, 255, b);
            y += 10;
            snprintf(b, sizeof(b), "0x%08lX", (unsigned long)p->addr);
            Gfx2D_Text(4, y, 200, 200, 200, 255, b);
            y += 12;
        } else {
            Gfx2D_Text(4, y, 255, 160, 160, 255, sObj == NULL ? "not converted" : "no parts");
            y += 22;
        }
    } else {
        snprintf(b, sizeof(b), "scene 0x%02X  entrance 0x%03X  room %d", sSceneId & 0xFF, sEntrance & 0xFFFF,
                 sPlay ? sPlay->roomCtx.curRoom.num : -1);
        Gfx2D_Text(4, y, 255, 255, 255, 255, b);
        y += 10;
        Gfx2D_Text(4, y, 160, 220, 255, 255, sSceneId >= 0 ? kSceneNames[sSceneId] : "-");
        y += 32;
    }
    snprintf(b, sizeof(b), "eye %d %d %d  yaw %04X pitch %04X", (int)sEye.x, (int)sEye.y, (int)sEye.z, (u16)sYaw,
             (u16)sPitch);
    Gfx2D_Text(4, y, 200, 200, 200, 255, b);
    y += 10;
    snprintf(b, sizeof(b), "speed x%.2g", sSpeed);
    Gfx2D_Text(4, y, 200, 200, 200, 255, b);
    y += 12;
    Gfx2D_Text(4, 108, 150, 150, 170, 255,
               sMode == MODE_PLAY ? "PLAY: normal controls, SELECT capture"
                                  : "pad move  D-pad turn  L/R down/up");
    if (sStatusTimer > 0) {
        sStatusTimer--;
        Gfx2D_Text(4, y, 255, 255, 120, 255, sStatus);
    }
    for (int i = 0; i < BTN_COUNT; i++) {
        bool partRow = (i == BTN_PPREV || i == BTN_PNEXT);
        const char* label = kButtons[i].label;
        if (partRow && sMode != MODE_OBJECTS) label = (i == BTN_PPREV) ? "< SPAWN" : "SPAWN >";
        u8 r = i == BTN_CAPTURE ? 150 : 50, g = i == BTN_CAPTURE ? 50 : 60, bl = i == BTN_CAPTURE ? 50 : 90;
        Gfx2D_Rect(kButtons[i].x, kButtons[i].y, kButtons[i].w, kButtons[i].h, r, g, bl, 255);
        int tx = kButtons[i].x + (kButtons[i].w - (int)strlen(label) * 8) / 2;
        Gfx2D_Text(tx, kButtons[i].y + 12, 255, 255, 255, 255, label);
    }
}

void Inspector_DrawUI(PlayState* play) {
    (void)play;
    draw_ui();
}

static const u8 kAmb[3] = { 90, 90, 96 };
static const s8 kDirs[2][3] = { { 40, -80, -60 }, { -60, -20, 40 } };
static const u8 kCols[2][3] = { { 230, 230, 220 }, { 90, 90, 110 } };

bool Inspector_DrawObjects(PlayState* play) {
    if (sMode != MODE_OBJECTS) return false;
    init_once();
    if (sGotoCam && sNumParts > 0 && sParts[sPart].kind == 'D') sGotoCam = false; // (applied in load)
    if (sLoadedObj != sObjIdx) load_object(sObjIdx);
    MtxF proj, view;
    f32 n = sRadius * 0.02f;
    perspective(&proj, 60.0f, 4.0f / 3.0f, n < 1.0f ? 1.0f : n, sRadius * 40.0f + 2000.0f);
    view_matrix(&view);
    Gfx3D_SetupFrame(40, 44, 54);
    Gfx3D_SetCamera(&proj, &view);
    Gfx3D_SetLights(kAmb, 2, kDirs, kCols);
    if (sObj != NULL && sNumParts > 0) {
        const Part* p = &sParts[sPart];
        int seg = (p->addr >> 24) & 15;
        Asset_SetSegment(seg, sObj);
        Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
        Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
        if (p->kind == 'D') {
            GFX_TO(OPA, Gfx3D_LoadMtxF(&gIdentityMtxF));
            GFX_TO(OPA, Gfx3D_DrawDL(p->addr));
        } else if (!strcmp(p->type, "skin") || !strcmp(p->type, "curve")) {
            // not drawn here: these need the actor's own setup (skin vertex buffers / curve animation)
        } else {
            if (!sSkelReady) {
                setup_skeleton(play, p);
                if (sGotoCam && sMode == MODE_OBJECTS) {
                    sEye = sGotoEye;
                    sYaw = sGotoYaw;
                    sPitch = sGotoPitch;
                }
            }
            if (sSkelReady && sSkel.skeleton != NULL) {
                if (sSkelIsPlayer) {
                    AnimTaskQueue_Reset(&play->animTaskQueue);
                    ArenAnimation_Update(play, &sSkel);
                    AnimTaskQueue_Update(play, &play->animTaskQueue);
                } else if (sSkel.animation != NULL) {
                    SkelAnime_Update(&sSkel);
                }
                if (sSkelIsPlayer) { // what Player_DrawImpl sets: tunic colour, eye and mouth textures
                    extern Color_RGB8 sTunicColors[];
                    GFX_TO(OPA, Gfx3D_SetEnvColor(sTunicColors[0].r, sTunicColors[0].g, sTunicColors[0].b, 0));
                    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(uintptr_t)0x86000000u)); // eyes open
                    GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(uintptr_t)0x86004000u)); // mouth closed
                }
                Matrix_Translate(0.0f, 0.0f, 0.0f, MTXMODE_NEW);
                if (!strcmp(p->type, "flex")) {
                    GFX_TO(OPA, SkelAnime_DrawFlexOpa(play, sSkel.skeleton, sSkel.jointTable, sSkel.dListCount, NULL,
                                                      NULL, NULL));
                } else if (!strcmp(p->type, "flexlod")) {
                    GFX_TO(OPA, SkelAnime_DrawFlexLod(play, sSkel.skeleton, sSkel.jointTable, sSkel.dListCount, NULL,
                                                      NULL, NULL, 0));
                } else if (!strcmp(p->type, "lod")) {
                    GFX_TO(OPA, SkelAnime_DrawLod(play, sSkel.skeleton, sSkel.jointTable, NULL, NULL, NULL, 0));
                } else {
                    GFX_TO(OPA, SkelAnime_DrawOpa(play, sSkel.skeleton, sSkel.jointTable, NULL, NULL, NULL));
                }
            }
        }
    }
    Gfx3D_InterpSetContext(INTERP_CTX_NONE);
    Gfx3D_Screen(SCREEN_BOTTOM);
    draw_ui();
    if (sNumParts > 0 && (!strcmp(sParts[sPart].type, "skin") || !strcmp(sParts[sPart].type, "curve"))) {
        Gfx2D_Text(4, 96, 255, 160, 160, 255, "skin/curve skeleton: not drawn here");
    }
    Gfx3D_Screen(SCREEN_TOP);
    Gfx3D_InterpSetContext(0);
    return true;
}

bool Inspector_HideHud(void) {
    return true;
}
