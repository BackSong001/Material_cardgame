#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
if (( $# > 1 )); then echo "Usage: ./build.sh [workbook.xlsx]" >&2; exit 2; fi
"${PYTHON:-python3}" tools/prepare.py "$tmp" "$@"
"${WINDRES:-x86_64-w64-mingw32-windres}" -I src src/game.rc -O coff -o "$tmp/ui.o"
"${CXX:-x86_64-w64-mingw32-g++}" -I"$tmp" -std=c++17 -Os -Wall -Wextra -municode -mwindows -static -s src/main.cpp "$tmp/ui.o" -lgdi32 -o "$tmp/game.exe"
mkdir -p dist
install -m755 "$tmp/game.exe" dist/MaterialClassifier.exe
