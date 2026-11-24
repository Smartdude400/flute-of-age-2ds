# 3dsport: native Nintendo 2DS port — architecture plan

Target: original 2DS (old-3DS hardware). Goal set by the human (intervention #7): run the game as
close to the original as possible, **no graphics cuts**. The nativeness rules of the brief still apply:
no emulation, no runtime interpretation of original graphics/audio formats, offline asset conversion,
game logic translated (no original-platform types or calls).

## What carries over from dsport/
| Part | Reuse |
|---|---|
| Game code (`arm9/source/game`, ~90 adapted engine files, headers) | as-is; the engine API it calls (`Gfx3D_*`, `Asset_*`, `RomBind_*`, …) is backend-neutral |
| Patch scripts (`tools/patches/*`, `strip_gbi.py`, rewriters) | as-is |
| ROM reader, xml specs, texture decode (`romfs.py`, `xmlspec.py`, `n64tex.py`) | as-is |
| Display-list front end (`dl2gx.py`: F3DEX2 vertex cache, tiles, TLUT, combiner, segment-0x0D) | front end kept, new PICA back end |
| Engine (`arm9/source/engine`) | API kept; DS backend replaced (runner, input, log, gfx3d, dsobj, file I/O) |

## Hardware mapping (no cuts)
| Original | 2DS native |
|---|---|
| CPU float (VR4300 FPU, FS=1 flush-to-zero) | VFPv2 hardware single precision, flush-to-zero; `-ffp-contract=off` keeps op order (no fused ops) |
| 4 MB RDRAM arenas | same arena sizes allocated from the 64 MB app heap (exact accounting, no budget pressure) |
| RSP F3DEX2 transform/lighting | PICA vertex shader: modelview/projection, directional + ambient lighting, texgen (env maps) |
| RDP 2-cycle colour combiner `(A-B)*C+D` | PICA texture combiner: 2 TEV stages per cycle (SUBTRACT then MULTIPLY_ADD), 6 stages available |
| RDP fog (shade alpha blend) | PICA fog LUT computed from the game's fog near/far |
| Texture formats I4/I8/IA4/IA8/IA16/RGBA16/RGBA32/CI4/CI8 | LA4/LA8 (intensity replicated to alpha) / RGBA5551 / RGBA8; CI expanded offline (no palettes on PICA) |
| 2 tiles (multi-texture) | 2 of the 3 PICA texture units; tile scroll via texture matrix |
| Render modes (blend, alpha compare, z-decal, cull) | PICA blend/alpha test/depth bias/cull state |
| 320x240 output | rendered 320x240 centred on the 400x240 top screen (original aspect & FOV; culling unchanged) |
| 20 fps game frame (R_UPDATE_RATE=3) | 3 vblanks at 60 Hz, same fixed timestep as dsport |
| Audio (sequence player + software synth) | game-side audio logic translated; synthesis to PCM on the ARM11, output via ndsp |
| Controller | circle pad = analog stick; A, B, L = Z, R, START; D-pad = C-buttons, X = C-up, Y = C-left (touch buttons later) |

## Phases
- **P0** toolchain + headless emulator harness — PASS (evidence/p0_boot_top.png)
- **P1** engine backend on 3DS: runner/fixed timestep, input, log, fault, RomFS asset loading; boot to
  the title/opening game states (game code unchanged) — PASS (evidence/p1_logo_f120.png); Play_Init runs
  and waits for room data, which P2's converter provides
- **P2a** `dl2pica` converter + citro3d renderer: player viewer and scene viewer — PASS
  (evidence/p2_player_f90.png, p2_scene.png)
- **P2b** graphics parity: per-call buffer + preset restoration, skybox, partial texture loads — PASS
  (evidence/p2b_sky.png); frame-buffer effects and depth reads move to the phases that need them
- **P3** Play state with the player in Sylvan Forest — PASS (evidence/p3_walk_strip.png; test boot `make BOOT=0xEE`)
- **P4** first-area actors, cutscene data, file select, native HUD — actors part done 2026-11-24: 19 forest
  actors run (Sylvan children, Sylva, Milo, fairies/companion, signs, grass, rocks, Gossip Stones, Skulltula,
  room transitions, ambience; evidence/p4_forest_npcs.png, p4_forest_walk_strip.png). Open: cutscene data
  (S13), file select (S12), native HUD
- **P5** 60 Hz presentation (frame interpolation, see below) — game logic stays at 20 Hz
- **P6** audio; **P7** equivalence traces + on-device performance

## Conventions found during bring-up
- Textures: stored top row first (tools/pica_tex.py); the GPU samples t = 1 at the first stored row, so
  draw code uses `t_gpu = 1 - t`.
- Screen: the top framebuffer is portrait (240x400); the game viewport is `C3D_SetViewport(0, 40, 240, 320)`
  with `Mtx_OrthoTilt` / tilted perspective matrices.
- Time base: the number of retrace input samples applied to the game (deterministic under replay).

## Renderer (P2a)
- Display lists are compiled offline (`tools/dl2pica.py`) into op programs that keep the original command
  order; textures convert to PICA formats sized to the wrap period (`.pobj`, `engine/pmesh.h`).
- Draw calls are recorded per buffer (opaque / translucent / overlay) and replayed in that order.
- `engine/tev.c` maps the two-cycle colour combiner onto the six PICA combiner stages (PRIM = stage
  constant, ENV = combiner buffer colour); `shaders/gfx3d.v.pica` does the per-vertex matrix palette,
  lighting, texgen and tile texture transforms.
- Render-state presets (`Gfx_SetupDL_*`) are compiled from the ROM table into `romfs/setupdl.pobj`.

## Actors (P4)
- `tools/import_actor.py Name...` brings an actor overlay's sources and headers in through the same source
  translation as the engine code, then rewrites the overlay-specific patterns: limb callbacks of the
  list-cursor variant (one native callback type), `gSegments[]` (CPU side: `Asset_SetCpuSegment`) vs.
  `gSPSegment` (renderer: `Gfx3D_SetSegmentObj`), fog and billboard-matrix commands, display lists kept
  in the overlay's own data (handles, below).
- Display lists embedded in overlays (`tools/build_overlays.py`): converted into `romfs/ovl/ovldata.pobj`,
  handles are the original VRAM addresses (segment 0, bound at start-up). The XML offsets are relative to
  where the block sits in the overlay's data, so the block base is recovered by self-consistency (vertex
  loads must point at the listed arrays, every list must end); overlays that fail the check are rejected
  rather than shipped as garbage (9 of 36, none in the forest).
- Skeleton limbs resolve their list when the limb is recorded, so actors built from several objects
  (Sylvan children: head, body, legs) can rebind segment 6 between limbs, as on the original.
- Runtime-built lists: a game list that only sets colours / render mode becomes a recorded segment list
  (`Gfx3D_SegDLBegin/End`, `Gfx3D_SetRenderMode` with the original RDP mode words, `engine/rdp_modes.h`);
  texels the game writes at runtime and binds to a segment (signpost shadow mask) are converted to a PICA
  texture at replay (`dyn_texture`).
- List preludes (`build_object.py PRELUDES`): a few lists draw with texture state another list left in
  texture memory (e.g. the light-glow circle); they are compiled with that list's state as their start.
- Limb index enums (`MILO_LIMB_HEAD`, ...) are generated into the object headers from the XML names and
  the skeleton's limb table in the ROM.
- Cost with the forest NPCs (Azahar): logic 3-5 ms, replay CPU ~11 ms for ~380 draw calls (was ~4 ms
  for ~150): per-draw submission cost dominates and is the first target of P7 on-device profiling; it
  also bounds P5 (60 Hz needs logic + 3 replays within 50 ms).

## 60 Hz on the 2DS (assessment, 2026-11-24)
The original runs its game logic at 20 Hz (one update every 3 retraces) and every gameplay constant is
tuned per update: speeds, accelerations, timers, animation steps, collision, RNG draws. Two ways to show
60 frames per second:

| Option | What changes | Verdict |
|---|---|---|
| Run the logic at 60 Hz | every per-frame constant, timer and animation rate in the game; physics and RNG sequences differ | rejected: changes how the game plays, breaks equivalence with the original |
| **Interpolate presentation** | logic stays 20 Hz and bit-identical; the renderer draws 2 extra frames between two logic states by blending their matrices | **chosen**: no game behaviour changes |

How it fits this renderer: every frame is already recorded as command lists (opaque / translucent /
overlay) before it is replayed, so the last two logic frames can be kept and replayed at t = 1/3, 2/3, 1,
blending camera, model, skeleton-limb and segment matrices between matching commands. Matching uses keys
(actor pointer, effect slot, room, skeleton limb index, order within the key) set by a few hooks in the
game's draw entry points (Actor_Draw, effects, rooms, skybox); unmatched commands and camera cuts (large
jumps, scene changes, camera mode switches) are drawn without blending. Cost: one logic frame (~50 ms) of
extra display latency, the usual price of interpolation; texture scrolls, flipbooks and the HUD keep their
20 Hz steps (as the original).

Measured budget (Azahar, Sylvan Forest with the player, ~150 draws / 1,900 triangles per frame): game logic
1-2 ms, replay CPU ~4 ms (state 0.7, submission 2.6, 2D 0.14), GPU ~4 ms, the rest of each 16.7 ms retrace
idle. A replay per retrace fits comfortably; real hardware must confirm (Azahar's CPU timing is
instruction-count based), and more actors will add logic and draws. If logic frames ever exceed one
retrace, the logic can move to the second CPU core (APT_SetAppCpuTimeLimit) while core 0 presents.

Effort: interpolation core + keys ~ one phase (P2b-sized); hooks in ~5 game files; a settings switch keeps
the original 20 Hz presentation available.
