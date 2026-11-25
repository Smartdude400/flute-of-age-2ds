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
| S7 | `Room_DrawImage` (prerendered background rooms) | DS | original Room_DrawImage* restored; backgrounds (JPEG / RGBA16 / CI) converted offline into room textures, drawn as the original's copy (no depth test or write) after the room's depth-only lists | **closed 2026-11-24 (P4)** |
| S8 | Audio request API + flute (ocarina) state (`engine/audio_api.c`) | DS | translated audio logic + ARM11 synthesis to ndsp (P5); flute staffs report "no song" (0xFF) | open |
| S9 | Message tables / text | DS | `tools/build_text.py`: tables from the ROM `code` file, symbols = original segment addresses, text bytes in RomFS | **closed 2026-11-21 (P1)** |
| S10 | `Player_Init/Update/Draw/Destroy` (ovl_player_actor) | 2026-11-21 | `z_player.c` translated | **closed 2026-11-22 (P3)** |
| S11 | `KaleidoScope_Update/Draw` (pause menu) | 2026-11-21 | ovl_kaleido_scope imported with its RDP commands kept; the player preview drawn into an offscreen image (`Player_DrawPause`, `PreRender_SaveFramebuffer`); shown on the bottom screen (layout L1-L4 below) | **closed 2026-11-24 (P4)** |
| S12 | `FileSelect_Init` (ovl_file_choose): faults with a message if reached | 2026-11-21 | ovl_file_choose imported with its RDP commands kept (engine/rdp_disps.h) | **closed 2026-11-24 (P4)** |
| S13 | Cutscene data (`Cutscene_ProcessScript` with an unconverted script): the cutscene ends at once. First seen: Sylva's greeting when the forest is entered on a new file | 2026-11-24 | `tools/cutscene.py`: every script converted per field width into the scene's 'CSDT' table (161 scripts, 36 scenes); the guard stays as a safety net | **closed 2026-11-24 (P4)** |

## Presentation stubs (P-series; all close with P2 "no graphics cuts")
| Stub | Notes |
|---|---|
| ~~`Gfx3D_DrawMesh`, `Gfx3D_SetCamera`, lights, fog, flex skeletons, tex scroll~~ | **closed P2a**: PICA200 renderer (engine/gfx3d.c) |
| ~~Buffer target + preset index of each game draw call~~ | **closed P2b**: tools/restore_buffers.py (633 exact, 53 inferred, 41 inherit the caller's buffer by design) |
| ~~`DsObj_Texture`~~ | **closed P2a**: PICA textures loaded on first use |
| ~~`Skybox_Draw`, `Skybox_UpdateMatrix`, `Skybox_Setup` + `Skybox_Calculate*`~~ | **closed P2b**: game logic from the original, native face draws |
| ~~`ConsoleLogo_Draw`~~ | withdrawn: the original (renamed decomp) also draws GfxPrint text + fade |
| ~~`PreRender_SaveFrame/DrawSaved/SaveFramebuffer`~~ | **closed P4**: frame capture (display transfer) and offscreen render textures (engine/screen.c, engine/gfx3d.c) |
| `PreRender_ApplyFilters`, `PreRender_DrawCoverage` | kept as no-ops on purpose: they ran the VI coverage anti-aliasing / divot filters over the saved N64 image, which repair N64 coverage artifacts the PICA200 image does not have |
| `VisCvg/VisMono/VisZBuffer_Draw` | framebuffer effects: GPU render-to-texture on the PICA200 |
| `DebugDisplay_DrawObjects` | debug-only objects (retail never adds any) |
| Lens-flare z-buffer test (`Environment_GetPixelDepth`), light glow depth test | PICA200 depth buffer can be read back (P2) |
| `func_8002E830` (per-object texgen look-at + specular highlight tiles) | renderer uses the camera axes for texgen; highlight tiles pending |

## 2DS layout (L-series; the human's decision of 2026-11-24, presentation only)
The pause menu and HUD live on the bottom screen; game logic is unchanged (the menu still pauses the game).
| ID | Where | Original | Port |
|---|---|---|---|
| L1 | `Play_DrawOverlayElements` (z_play.c) | pause menu and HUD drawn over the frame | drawn on the bottom screen (320x240, the original frame size) at their original coordinates; text boxes stay on top |
| L2 | `Interface_Draw` (z_parameter.c) | one HUD layer | the A button with its action label, and the full-screen fade, are drawn on the top screen (the fade on both) |
| L3 | `KaleidoScope_Draw/DrawPages`, page switch (z_kaleido_scope.c) | four pages on a turning cube, 16-step scroll between pages | the active page drawn flat facing the view (inactive pages' draw code still runs, output discarded); the 16 switch steps run in one frame |
| L4 | `engine/input.c`, `engine/touch.h` | no touch screen | tapping the B / C / START icons or the page arrows acts as that button; tapping an item or equipment moves the cursor there (a second tap on an equipment equips it) |

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
| 2D vertex quads (HUD action button / heart, file select windows) | transformed on the CPU and drawn with affine texture mapping (no perspective correction inside a quad); overlay viewports are not scissored |
| Sword / stick swing trail | `EffectBlure_Draw` presentation stub (listed above) |
| object_zf (Lizalfos) not converted | a limb list branches into a runtime segment (07); the converter has no path for that yet |
