#!/bin/sh
# Build wrapper for Windows without a devkitPro shell: runs `make` inside MSYS2 with the devkitPro
# 3DS toolchain on the path (C:\msys64 + /opt/devkitpro), and appends a line to build_log.csv.
# On Linux/macOS or inside a devkitPro shell, just run `make` directly.
cd "$(dirname "$0")"
if command -v make >/dev/null 2>&1 && [ -n "$DEVKITARM" ]; then
  make "$@"; exit $?
fi
DIR=$(pwd -W 2>/dev/null || pwd)
OUT=$(mktemp)
/c/msys64/usr/bin/env MSYSTEM=MSYS CHERE_INVOKING=1 /c/msys64/usr/bin/bash -lc \
  'export DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM; export PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitARM/bin:$PATH; cd "$(cygpath "$0")" && make "$@"' "$DIR" "$@" > "$OUT" 2>&1
RC=$?
cat "$OUT"
CE=$(grep -c ': error:\|fatal error:' "$OUT")
LE=$(grep -c 'undefined reference\|ld returned' "$OUT")
WN=$(grep -c ': warning:' "$OUT")
[ -f build_log.csv ] || echo "timestamp,exit,compile_errors,link_errors,warnings,args" > build_log.csv
echo "$(date +%Y-%m-%dT%H:%M:%S),$RC,$CE,$LE,$WN,$*" >> build_log.csv
rm -f "$OUT"
exit $RC
