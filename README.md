<div align="center">

# 🎶 Flute of Age — native 2DS port

**A from-source, native Nintendo 2DS / 3DS port of the *Flute of Age* decompilation —<br>no emulator, no N64 compatibility layer: the game's own code, translated and running on the 3DS hardware.**

<img src="docs/images/hero_forest.png" width="640" alt="The player standing in Sylvan Forest, rendered natively on the 2DS">

![platform](https://img.shields.io/badge/platform-2DS%20%7C%203DS-d12228?style=for-the-badge&logo=nintendo3ds&logoColor=white)
![language](https://img.shields.io/badge/language-C11-00599C?style=for-the-badge&logo=c&logoColor=white)
![gpu](https://img.shields.io/badge/GPU-PICA200%20via%20citro3d-6f42c1?style=for-the-badge)
![status](https://img.shields.io/badge/status-playable%20demo-f0a500?style=for-the-badge)

[What works](#-what-works) · [Screenshots](#-screenshots) · [Setup guide](#-setup-guide) · [How to play](#-how-to-play) · [How it works](#-how-it-works) · [Roadmap](#-roadmap)

</div>

> [!IMPORTANT]
> **This repository contains no game data.** You need your own legally obtained copy of the game ROM
> (**NTSC 1.0, US**). Every texture, model, scene, sound and text is converted on *your* machine from
> *your* ROM by the tools in this repository, and is never committed. Do not share built `.3dsx` files:
> they contain the converted game data.

---

## ✨ What works

| | Feature | Notes |
|:-:|---|---|
| ✅ | **Game logic runs natively** | The decompiled game code is translated to C for the ARM11 (hardware float, original math order) |
| ✅ | **The real `Play` state** | Scene + room loading, collision, camera, environment / time of day, actor system |
| ✅ | **The player** | Walk, run, turn, roll, collide — the original `z_player.c`, all 16 000 lines |
| ✅ | **Full-fidelity renderer** | Original draw order (opaque → translucent → overlay), the two-cycle colour combiner mapped onto the PICA200 TEV, fog, lighting, environment mapping, both texture layers, all 71 render presets |
| ✅ | **Skyboxes** | Day/night skies with the game's own blending logic |
| ✅ | **20 fps fixed timestep** | Same frame pacing as the original; input sampled per retrace (deterministic replays) |
| ✅ | **Saves** | Written to `sdmc:/3ds/foa/save.sra` |
| 🚧 | Other actors | NPCs, objects, enemies are skipped for now (each logged once) |
| 🚧 | Title screen demo, file select, pause menu | Not ported yet — the demo boots straight into the forest |
| 🚧 | Audio | Not ported yet (silent) |
| 🚧 | HUD | Placeholder text for hearts / rupees |

## 📸 Screenshots

<table>
<tr>
<td align="center"><img src="docs/images/walk.png" width="400"><br><sub>Running through Sylvan Forest</sub></td>
<td align="center"><img src="docs/images/roll.png" width="400"><br><sub>Rolling (A while running)</sub></td>
</tr>
<tr>
<td align="center"><img src="docs/images/sky_overview.png" width="400"><br><sub>Native skybox + scene renderer</sub></td>
<td align="center"><img src="docs/images/player_model.png" width="400"><br><sub>The player model on the PICA200</sub></td>
</tr>
</table>

<p align="center"><img src="docs/images/gameplay_strip.png" width="900"><br><sub>A scripted run, captured headlessly every ~2 s</sub></p>

<details>
<summary>📱 Both screens, as on the console</summary>
<p align="center"><img src="docs/images/dual_screen.png" width="400"><br>
<sub>Top: the game at its original 320×240, centred on the 400×240 screen. Bottom: the engine log console.</sub></p>
</details>

---

## 🛠 Setup guide

You will build the game yourself: install a toolchain, add your ROM, convert the data, compile.
Plan ~20 minutes for the first time.

### Step 1 — Install the tools

| Tool | Version | Why |
|---|---|---|
| **devkitPro** with the `3ds-dev` group | current | ARM compiler, libctru, citro3d, picasso (shader assembler) |
| **Python** | 3.10 or newer | the asset converters (standard library only — no packages to install) |
| **git** | any | to clone this repository |

<details open>
<summary><b>🪟 Windows</b></summary>

1. Download the **devkitPro installer** from <https://github.com/devkitPro/installer/releases> and run it.
2. When asked which components to install, tick **3DS Development**.
3. Install **Python** from <https://www.python.org/downloads/> (tick *“Add python.exe to PATH”*).
4. Open the **MSYS2 shell from the devkitPro start-menu folder** for the next steps (it has `make` and the toolchain on its path).

</details>

<details>
<summary><b>🐧 Linux / 🍎 macOS</b></summary>

1. Install devkitPro's pacman: follow <https://devkitpro.org/wiki/devkitPro_pacman>.
2. Install the 3DS toolchain:
   ```bash
   sudo dkp-pacman -S 3ds-dev
   ```
3. Make sure `DEVKITPRO` and `DEVKITARM` are set (log out and back in after installing, or `source /etc/profile.d/devkit-env.sh`).
4. Install Python 3.10+ from your package manager.

</details>

### Step 2 — Get the code

```bash
git clone https://github.com/Smartdude400/flute-of-age-2ds.git
cd flute-of-age-2ds
```

### Step 3 — Add your ROM

Copy your ROM into the repository root and name it **`baserom.z64`**.

| Requirement | Value |
|---|---|
| Version | NTSC 1.0 (US) |
| Format | `.z64` (big-endian) |
| MD5 | `5bd1fe107bf8106b2ab6650abecd54d6` |

> [!TIP]
> Keep the ROM elsewhere? Set the `FOA_BASEROM` environment variable to its path instead of copying it.
> `baserom.z64` is in `.gitignore`, so it can never be committed by accident.

### Step 4 — Convert the game data

```bash
python tools/setup_assets.py
```

This checks the ROM, then converts everything into native 2DS formats: every object, all 111 scenes,
textures, skies, text and render presets (about 50 MB in `romfs/`). It also generates the headers that
depend on the ROM. It takes a few minutes and only has to be run once.

### Step 5 — Build

```bash
make BOOT=0xEE
```

This produces **`foa_3ds_boot.3dsx`**: the playable demo, which starts in Sylvan Forest with a new file.

| Command | Output | What it is |
|---|---|---|
| `make BOOT=0xEE` | `foa_3ds_boot.3dsx` | **The demo** — new game, straight into Sylvan Forest |
| `make` | `foa_3ds.3dsx` | The normal boot flow (logo → title setup); the title demo isn't ported yet, so it stops there |
| `make HARNESS=1` | `foa_3ds_h1.3dsx` | Player-model viewer (renderer test) |
| `make HARNESS=2` | `foa_3ds_h2.3dsx` | Free-fly scene viewer (renderer test) |

> [!NOTE]
> On Windows without the devkitPro shell, `./build.sh BOOT=0xEE` runs the same `make` through MSYS2.

---

## 🎮 How to play

### On a real 2DS / 3DS

1. Your console needs **homebrew access** (the Homebrew Launcher, e.g. through Luma3DS).
2. Copy `foa_3ds_boot.3dsx` to the SD card, into **`/3ds/`**.
3. Start the **Homebrew Launcher** and pick **Flute of Age**.

### On an emulator (Azahar)

1. Install [Azahar](https://azahar-emu.org/).
2. **File → Load File…** and choose `foa_3ds_boot.3dsx`.

### Controls

| 2DS / 3DS | Original controller | In game |
|---|---|---|
| Circle Pad | Control stick | Move (analog) |
| **A** | A | Action / roll / talk |
| **B** | B | Sword |
| **L** | Z | Target / centre camera |
| **R** | R | Shield |
| **D-Pad** | C-buttons | Items / first-person (up) |
| **X** / **Y** | C-up / C-left | Alternatives for the D-Pad |
| **START** | START | Pause (not ported yet) |

---

## 🧠 How it works

The port follows one rule: **no emulation**. Nothing original is interpreted at run time: no display
lists, no N64 texture formats, no RSP microcode. The game *logic* is translated; the game *data* is
converted ahead of time into formats the 3DS GPU uses directly.

```mermaid
flowchart LR
    ROM[(Your ROM<br/>baserom.z64)] --> T[tools/setup_assets.py]
    XML[Asset XML specs<br/>tools/assets_xml] --> T
    T -->|display lists → ordered op programs<br/>textures → PICA formats| R[(romfs/<br/>.pobj objects & rooms)]
    T -->|tables, offsets, text| H[generated headers]
    SRC[Translated game code<br/>source/game] --> M[make]
    ENG[Native engine<br/>source/engine] --> M
    H --> M
    R --> M
    M --> X[foa_3ds_boot.3dsx]
```

| Layer | What it does |
|---|---|
| `source/game/` | The game's code translated to C: platform calls replaced, logic and math order kept exactly |
| `source/engine/` | The native platform layer: fixed-timestep runner, input, RomFS loading, saves, memory arenas |
| `source/engine/gfx3d.c` | Renderer: records draw calls per buffer (opaque / translucent / overlay) and replays them in the original order on the PICA200 |
| `source/engine/tev.c` | Maps the N64 two-cycle colour combiner `(A−B)·C+D` onto the 6 PICA texture-combiner stages |
| `source/shaders/gfx3d.v.pica` | Vertex shader: per-vertex matrix palette (skinned skeletons), lighting, texgen, tile transforms |
| `tools/dl2pica.py` | Offline display-list compiler: keeps every state change in original order, decodes all fields into native enums |

Read more in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md), the open stub list in
[`docs/STUBS.md`](docs/STUBS.md), and the full engineering diary in [`docs/RESEARCH_LOG.md`](docs/RESEARCH_LOG.md).

## 🗂 Repository layout

```
├── source/
│   ├── game/          translated game code (states, actors, player, scenes, environment, …)
│   ├── engine/        native platform layer + renderer
│   ├── shaders/       PICA200 vertex shaders (picasso)
│   └── test/          renderer test harnesses
├── tools/
│   ├── setup_assets.py   ← run this once (converts your ROM)
│   ├── dl2pica.py        display-list compiler
│   ├── build_*.py        object / scene / sky / text converters
│   ├── run_test.py       headless test runner (Azahar on a hidden desktop + RPC)
│   ├── assets_xml/       asset XML specs (names and offsets inside the ROM files)
│   ├── romdata/          ROM layout (file table)
│   └── patches/ …        porting scripts (need the original decompilation checkout)
├── trace/inputs/       scripted input for repeatable test runs
└── docs/               architecture, stubs, research log, images
```

## 🧪 Testing headlessly (optional)

`tools/run_test.py` runs a build in Azahar on a hidden Windows desktop and drives it through Azahar's
RPC server: it waits for N game frames, takes screenshots and prints the game's log.

```bash
python tools/run_test.py foa_3ds_boot.3dsx --frames 240 --script trace/inputs/p3_walk.txt --shot walk.png --shots 60,120
```

It needs a portable Azahar folder (`azahar.exe` plus a `user/` folder whose `config/qt-config.ini` has
`enable_rpc_server=true` and `use_vsync=false`); point `FOA_AZAHAR_DIR` at it.

## 🧭 Roadmap

- [x] Native engine layer, fixed-timestep runner, saves
- [x] Offline display-list compiler + full-fidelity PICA200 renderer
- [x] Skyboxes, render presets, original draw order
- [x] The real `Play` state with the player
- [ ] Actors of the first area (Sylvan children, the fairy companion, doors, grass, signs …)
- [ ] Cutscene data → title-screen demo
- [ ] File select, pause menu, the real HUD
- [ ] Audio (sequence player + synthesis on the ARM11, output through the DSP)
- [ ] Frame-perfect equivalence traces against the original

## ❓ Troubleshooting

<details>
<summary><b>“ROM check failed”</b></summary>

The MD5 must match NTSC 1.0 US exactly. If your dump is `.n64` (little-endian) or `.v64` (byte-swapped),
convert it to `.z64` first, e.g. for `.v64`:

```python
d = bytearray(open("rom.v64", "rb").read()); d[0::2], d[1::2] = d[1::2], d[0::2]; open("baserom.z64", "wb").write(d)
```
</details>

<details>
<summary><b><code>make: arm-none-eabi-gcc: not found</code></b></summary>

The devkitPro environment isn't loaded. Use the devkitPro MSYS2 shell on Windows; on Linux/macOS
`source /etc/profile.d/devkit-env.sh`.
</details>

<details>
<summary><b>Compile errors about missing <code>assets_agg/…</code> or <code>segment_symbols.h</code></b></summary>

Those headers are generated from your ROM: run `python tools/setup_assets.py` before `make`.
</details>

<details>
<summary><b>The bottom screen shows “[ACTOR] id … not ported”</b></summary>

Expected: only the player is ported so far. Everything else in the scene is skipped and logged once.
</details>

---

## ⚖️ Legal

This is a non-commercial fan project for research and preservation. It is **not affiliated with or
endorsed by Nintendo**. The game code here is a translation of a community decompilation project;
the original game, its code, assets and trademarks belong to their respective owners. **No game data
is included**: you must supply your own legally obtained ROM, and you must not distribute builds made
from it. Third-party library notices: [`THIRD_PARTY.md`](THIRD_PARTY.md).

<div align="center"><sub>Built with devkitARM · libctru · citro3d · picasso — tested on Azahar and designed for the original 2DS</sub></div>
