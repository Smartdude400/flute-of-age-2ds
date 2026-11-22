"""Headless test run on Azahar (portable copy in tools/bin/azahar, hidden desktop, RPC enabled).

  python tools/run_test.py foa_3ds.3dsx --frames 120 --shot evidence/x.png [--bottom-shot f.png]

Starts the emulator on a hidden desktop (tools/azahar_hidden.ps1), waits for the RPC server, polls
the game's test marker (gTestMarker: magic "FOA3DSMK", then the frame counter; address from the
ELF symbol table) until --frames game frames have run, takes screenshots, prints the SD-card log
written by the game, then kills the emulator."""
import argparse
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
# a portable Azahar folder (azahar.exe + user/ with the RPC server enabled and vsync off), see README
EMU = os.path.normpath(os.environ.get("FOA_AZAHAR_DIR") or os.path.join(ROOT, "..", "tools", "bin", "azahar"))
sys.path.insert(0, os.path.join(EMU, "scripting"))
from AzaharRPC import AzaharRPC  # noqa: E402

def find_nm():
    import shutil
    cands = [os.path.join(os.environ.get("DEVKITARM", ""), "bin", "arm-none-eabi-nm"),
             shutil.which("arm-none-eabi-nm") or "",
             os.path.join(os.environ.get("SystemDrive", "C:") + os.sep, "msys64", "opt", "devkitpro", "devkitARM", "bin",
                          "arm-none-eabi-nm.exe"),
             os.path.join(os.environ.get("SystemDrive", "C:") + os.sep, "devkitPro", "devkitARM", "bin", "arm-none-eabi-nm.exe")]
    for c in cands:
        if c and (os.path.exists(c) or os.path.exists(c + ".exe")):
            return c
    raise SystemExit("arm-none-eabi-nm not found: install devkitPro 3ds-dev or set DEVKITARM")


NM = find_nm()
SDLOG = os.path.join(EMU, "user", "sdmc", "foa_log.txt")


def symbol(elf, name):
    out = subprocess.run([NM, elf], capture_output=True, text=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[2] == name:
            return int(p[0], 16)
    raise SystemExit(f"symbol {name} not in {elf}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("--frames", type=int, default=60)
    ap.add_argument("--timeout", type=float, default=60)
    ap.add_argument("--shot")
    ap.add_argument("--bottom-shot")
    ap.add_argument("--script", help="input script copied to sdmc:/foa_input.txt (see engine/input.c)")
    ap.add_argument("--shots", help="comma list of game frames to screenshot: f1,f2,... (files <shot>_f<N>.png)")
    a = ap.parse_args()
    rom = os.path.abspath(a.rom)
    elf = os.path.splitext(rom)[0] + ".elf"
    marker = symbol(elf, "gTestMarker")
    SCRIPT = os.path.join(EMU, "user", "sdmc", "foa_input.txt")
    if os.path.exists(SCRIPT):
        os.remove(SCRIPT)
    if a.script:
        with open(SCRIPT, "w", newline="") as out:
            out.write(open(a.script, encoding="utf-8").read())
    for attempt in range(4):
        if os.path.exists(SDLOG):
            os.remove(SDLOG)
        pid = launch(rom)
        # Known Azahar issue on the hidden desktop: some launches freeze ~1 s into boot before the game runs.
        t_boot = time.time()
        while time.time() - t_boot < 10 and not os.path.exists(SDLOG):
            time.sleep(0.2)
        if os.path.exists(SDLOG):
            break
        print(f"[run_test] emulator stalled at boot, relaunching ({attempt + 1})")
        subprocess.run(["taskkill", "/F", "/PID", str(pid)], capture_output=True)
        time.sleep(1)
    run(a, pid, marker)


def launch(rom):
    return int(subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                              os.path.join(HERE, "azahar_hidden.ps1"), "-Rom", rom,
                              "-Emu", os.path.join(EMU, "azahar.exe")],
                             capture_output=True, text=True, check=True).stdout.strip().splitlines()[-1])


def run(a, pid, marker):
    # Azahar's RPC server runs synchronously with emulation: requests start only once the game logged.
    time.sleep(0.5)
    t0 = time.time()
    shots = sorted(int(x) for x in a.shots.split(",")) if a.shots else []
    rpc = AzaharRPC(timeout=2, retries=2)
    frames, ok = 0, False
    try:
        while time.time() - t0 < a.timeout:
            try:
                d = rpc.read_memory(marker, 16)
            except Exception:
                d = None
            if d and len(d) == 16 and d[:8] == b"3AOFKMSD":  # little-endian "FOA3" "DSMK"
                frames = int.from_bytes(d[8:12], "little")
                while shots and frames >= shots[0]:
                    base = os.path.splitext(a.shot or "shot.png")[0]
                    rpc.save_screenshot(f"{base}_f{shots[0]}.png")
                    print(f"[run_test] screenshot at game frame {frames}: {base}_f{shots[0]}.png")
                    shots.pop(0)
                if int.from_bytes(d[12:16], "little"):
                    print("[run_test] game reported a FAULT")
                    break
                if frames >= a.frames:
                    ok = True
                    break
            time.sleep(0.25)
        print(f"[run_test] frames={frames} after {time.time() - t0:.1f}s {'OK' if ok else 'TIMEOUT'}")
        if ok and a.shot:
            rpc.save_screenshot(a.shot)
            print("[run_test] screenshot", a.shot)
        if ok and a.bottom_shot:
            rpc.save_screenshot(a.bottom_shot, secondary_window=True)
        stats = rpc.perf_stats() if ok else None
        if stats:
            print("[run_test] perf", {k: round(v, 2) for k, v in stats.items() if isinstance(v, float)})
    finally:
        subprocess.run(["taskkill", "/F", "/PID", str(pid)], capture_output=True)
    if os.path.exists(SDLOG):
        print(open(SDLOG, encoding="utf-8", errors="replace").read(), end="")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
