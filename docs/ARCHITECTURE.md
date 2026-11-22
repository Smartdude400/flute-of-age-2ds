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
- **P4** first-area actors, cutscene data, file select, native HUD; P5 audio; P6 equivalence traces + perf

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
