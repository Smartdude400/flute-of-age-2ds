# Asset inspector

A separate build for hunting graphics mistakes. You can look at every object of the game part by part,
fly freely through the real scenes, and capture what looks wrong. Each capture records exactly where you
were and what you were looking at, so the view can be replayed on the PC.

## Build

```
python tools/build_inspect_catalog.py
./build.sh INSPECT=1
```

This produces `foa_3ds_inspect.3dsx`. Copy it to the SD card like the normal game (`/3ds/`), or open it in an
emulator. `BOOT=<entrance>` picks the start scene (default: the forest, `0xEE`).

## Modes

The game runs behind every mode. Switch with **START** or the **MODE** button.

| Mode | What you see |
|---|---|
| **OBJECTS** | One part of one object, alone on a grey background. Skeletons (animated models) come first and play the object's first animation; then every display list (single meshes) of the object. |
| **FLY** | The real game scene with a free camera. Link stands still and can't be hurt. |
| **PLAY** | The real game with the normal controls and camera (START is the pause menu again here; use the MODE button). |

## Controls

| | OBJECTS | FLY | PLAY |
|---|---|---|---|
| Circle Pad | fly forward / back, sideways | same | Link |
| D-Pad | turn left / right, look up / down | same | (game) |
| L / R | down / up | same | (game) |
| A / B | next / previous object | next / previous scene | (game) |
| X / Y | next / previous part | next / previous spawn point of the scene | (game) |
| SELECT | **capture** | **capture** | **capture** |
| START | next mode | next mode | pause menu |

Touch buttons on the bottom screen do the same: **< PART / PART >** (**< SPAWN / SPAWN >** in the scene modes),
**< PREV / NEXT >**, **<<10 / 10>>** (jump 10 objects or scenes), **MODE**, **CAPTURE**, **RESET** (back to the
default view), and **SLOW / FAST** (flying speed). The bottom screen also shows the object or scene, the part
(symbol name and address) and the camera position.

## Captures

Each capture adds one line to `sdmc:/3ds/foa/inspect_captures.txt` and saves the top screen as
`sdmc:/3ds/foa/inspect_<number>.bmp`. On a 2DS that is the SD card's `3ds/foa/` folder. In Azahar it is the
emulator's `sdmc` folder (its user directory; File > Open Azahar Folder).

To hand them over, copy `inspect_captures.txt` and the `inspect_*.bmp` files into `3dsport/captures/` (not
committed) and say so. Each capture is then replayed headlessly (`python tools/inspect_replay.py
captures/inspect_captures.txt <number>`), and you'll get a question about what is wrong with it.

## Survey of every object

`python tools/inspect_survey.py --start 0 --count 100` captures a run of objects (each object's first part) in
one headless run and lays them out as labelled contact sheets in `evidence/survey/` (not committed; regenerate
with the tool). All 382 objects take four runs of about five minutes.

## What is expected to look different here

- **OBJECTS mode:** a part is drawn without its actor. Anything the actor sets at draw time is missing:
  - eye and mouth textures (Link's are set), body colours chosen per enemy type (e.g. Tektite), colours the
    actor sets, and lists the actor assembles from several parts;
  - skin and curve skeletons (horses, treasure chest lid) are not drawn as skeletons; their display lists are.
  - skeletons whose animations live in another object (adult Zelda, the ending's characters, several
    townspeople) have no animation here and lie in their bind pose;
  - material-only lists (no geometry, e.g. the signpost's, the item colour lists) draw nothing, and models
    in several lists show one list at a time (the Mirror Shield's emblem is its second list).

  Check such models in FLY or PLAY mode before capturing them as mistakes.
- **FLY mode:**
  - rooms load around Link as in the game. Flying far away shows empty space; use the SPAWN buttons to go to
    other parts of a scene.
  - prerendered backgrounds (houses, shops, the market) only appear from the game's own camera, so use PLAY
    mode there.
