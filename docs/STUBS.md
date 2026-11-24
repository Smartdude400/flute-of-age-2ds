# STUBS (3dsport)

Every temporary stub is listed here and must be closed before the final build. Logic stubs log
`[LSTUB] name` and presentation stubs `[PSTUB] name` on first use (engine/stub.h).
IDs continue the dsport numbering; history of S1-S9 is in dsport/STUBS.md.

## Logic / system stubs
| ID | Stub | Opened | Replacement | Status |
|---|---|---|---|---|
| S1 | `Matrix_Init` | DS | adapted `sys_matrix.c` | closed (DS) |
| S2 | `View_Init` | DS | adapted `z_view.c` | closed (DS) |
| S3 | `Play_Init` / `Play_Main` | DS | real `z_play.c` linked | **closed 2026-11-21 (P1)** |
| S4-S6 | `Sram_InitSram` / `Sram_Alloc` / `Sram_InitDebugSave` | DS | real `z_sram.c`; save memory backed by `sdmc:/3ds/foa/save.sra` (write-through) | **closed 2026-11-21 (P1)** |
| S7 | `Room_DrawImage` (prerendered background rooms) | DS | PICA200 background renderer (P2) | open |
| S8 | Audio request API + flute (ocarina) state (`engine/audio_api.c`) | DS | translated audio logic + ARM11 synthesis to ndsp (P5); flute staffs report "no song" (0xFF) | open |
| S9 | Message tables / text | DS | `tools/build_text.py`: tables from the ROM `code` file, symbols = original segment addresses, text bytes in RomFS | **closed 2026-11-21 (P1)** |
| S10 | `Player_Init/Update/Draw/Destroy` (ovl_player_actor) | 2026-11-21 | `z_player.c` translated | **closed 2026-11-22 (P3)** |
| S11 | `KaleidoScope_Update/Draw` (pause menu) | 2026-11-21 | port ovl_kaleido_scope | open |
| S12 | `FileSelect_Init` (ovl_file_choose): faults with a message if reached | 2026-11-21 | port ovl_file_choose | open |
| S13 | Cutscene data (`Cutscene_ProcessScript` with an unconverted script): the cutscene ends at once. First seen: Sylva's greeting when the forest is entered on a new file | 2026-11-24 | convert cutscene command streams per command layout (P4) | open |

## Presentation stubs (P-series; all close with P2 "no graphics cuts")
| Stub | Notes |
|---|---|
| ~~`Gfx3D_DrawMesh`, `Gfx3D_SetCamera`, lights, fog, flex skeletons, tex scroll~~ | **closed P2a**: PICA200 renderer (engine/gfx3d.c) |
| ~~Buffer target + preset index of each game draw call~~ | **closed P2b**: tools/restore_buffers.py (633 exact, 53 inferred, 41 inherit the caller's buffer by design) |
| ~~`DsObj_Texture`~~ | **closed P2a**: PICA textures loaded on first use |
| ~~`Skybox_Draw`, `Skybox_UpdateMatrix`, `Skybox_Setup` + `Skybox_Calculate*`~~ | **closed P2b**: game logic from the original, native face draws |
| ~~`ConsoleLogo_Draw`~~ | withdrawn: the original (renamed decomp) also draws GfxPrint text + fade |
| `PreRender_ApplyFilters/SaveFrame/DrawSaved`, `VisCvg/VisMono/VisZBuffer_Draw` | framebuffer effects: GPU render-to-texture on the PICA200 |
| `DebugDisplay_DrawObjects` | debug-only objects (retail never adds any) |
| Lens-flare z-buffer test (`Environment_GetPixelDepth`), light glow depth test | PICA200 depth buffer can be read back (P2) |
| `func_8002E830` (per-object texgen look-at + specular highlight tiles) | renderer uses the camera axes for texgen; highlight tiles pending |

## Divergences kept on purpose (D-series)
Places where the original read memory it did not own; the port cannot reproduce the bytes, so it uses a
defined value and logs the event. Each one is invisible in play (the value is overwritten before use).
| ID | Where | Original | Port |
|---|---|---|---|
| D-ANIMSEG | `Animation_GetLength/GetLastFrame` (z_skelanime.c) | En_Ko asks for animation lengths while segment 6 still holds gameplay_keep (before its own objects load) and reads unrelated bytes; the value is replaced when the objects load | frame count 0; `Asset_Resolve: unresolved ...` logged |

## Known visual issues (open)
| Issue | Notes |
|---|---|
| Forest dust motes (Object_Kankyo) draw as small squares | texture `gSun1Tex` converted and bound through segment 8 (the model list now inherits its material list's tile state); sampling still wrong, cause not found yet |
| Sword / stick swing trail | `EffectBlure_Draw` presentation stub (listed above) |
