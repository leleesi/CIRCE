#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p bin
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cxx=${CXX_WIN:-x86_64-w64-mingw32-g++}
rc=${WINDRES:-x86_64-w64-mingw32-windres}
"$rc" -I instalador --codepage=65001 -O coff instalador/idioma.rc "$tmp/language-dialog.o"
"$cxx" -std=c++20 -Os -s -static -shared -DUNICODE -D_UNICODE instalador/idioma.cpp "$tmp/language-dialog.o" \
    -o bin/CIRCEIdioma.dll -luser32
"$rc" -I src -O coff src/circe.rc "$tmp/resources.o"
build() {
    "$cxx" -std=c++20 -O2 -s -static -municode -mwindows -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 "$@"
}
build src/app.cpp src/language_win.cpp src/panel.cpp src/platform.cpp "$tmp/resources.o" -o bin/CIRCE.exe \
    -luser32 -lgdi32 -ldwmapi -luxtheme -lshell32 -lcomdlg32 -lcomctl32 -lole32 -luuid -ladvapi32
build native/window.cpp native/gravar.cpp -o bin/CIRCEJanela.exe -ld3d11 -ldxgi -lruntimeobject -lole32 -lgdi32 -luuid -ladvapi32
