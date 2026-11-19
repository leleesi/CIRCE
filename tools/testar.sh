#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
case "$#" in
    0) ;;
    1) [ "$1" = "--portable" ] || { echo "Usage: sh tools/testar.sh [--portable]" >&2; exit 2; } ;;
    *) echo "Usage: sh tools/testar.sh [--portable]" >&2; exit 2 ;;
esac
python3 tools/prepare_presentmon.py
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for t in csv metrics game_list reader pipeline layout localization; do
 g++ -std=c++20 -O2 -fshort-wchar -ffunction-sections -fdata-sections -fno-builtin "tests/$t.cpp" -Wl,--gc-sections -o "$tmp/$t"
 "$tmp/$t" tests/data/presentmon-test_case_1.csv
 echo "OK: $t"
done
g++ -std=c++20 -O2 -I tests/installer_dialog tests/installer_dialog/test.cpp -o "$tmp/installer-dialog"
"$tmp/installer-dialog"
echo "OK: escolha de idioma, foco e botão predefinido, cancelamento e falha de criação do diálogo."
g++ -std=c++20 -O2 -fshort-wchar -ffunction-sections -fdata-sections tests/session_video.cpp -Wl,--gc-sections -o "$tmp/session_video"
g++ -std=c++20 -O2 tests/video_labels.cpp -o "$tmp/video_labels"
sh tests/video-test.sh "$tmp"
if command -v pwsh >/dev/null 2>&1; then
    pwsh -NoLogo -NoProfile -NonInteractive -File tests/powershell.ps1 "$PWD" "$tmp"
    pwsh -NoLogo -NoProfile -NonInteractive -File tests/video-component.ps1 "$PWD" "$tmp"
fi
if [ "${1:-}" = "--portable" ]; then
    exit 0
fi
sh tools/package.sh
for f in bin/CIRCE.exe bin/CIRCEJanela.exe ./CIRCE-Setup.exe; do
 objdump -f "$f" | grep -q pei-x86-64
 echo "PE x64: $f"
done
setup=./CIRCE-Setup.exe
sevenzip=${SEVENZIP:-7z}
"$sevenzip" t "$setup" > "$tmp/archive-test.txt"
objdump -p "$setup" | grep -q 'Windows GUI'
objcopy --dump-section .rsrc="$tmp/app.resources" bin/CIRCE.exe "$tmp/app-pe.exe"
grep -aq 'requireAdministrator' "$tmp/app.resources"
grep -aq 'PerMonitorV2' "$tmp/app.resources"
objdump -p bin/CIRCEJanela.exe | grep -iq 'DLL Name: d3d11.dll'
objdump -p bin/CIRCE.exe > "$tmp/imports.txt"
for api in CoCreateInstance SHCreateItemFromParsingName SHGetKnownFolderPath; do
    grep -qw "$api" "$tmp/imports.txt"
done
for app in bin/CIRCE.exe bin/CIRCEJanela.exe; do
 if objdump -p "$app" | grep -Ei 'DLL Name: (VCRUNTIME|MSVCP[0-9]|libstdc|libgcc|libwinpthread)' >/dev/null; then
  echo "Runtime externo inesperado em $app" >&2
  exit 1
 fi
done
echo "OK: manifesto de administrador e DPI, importação de Direct3D 11 e executáveis sem runtime C++ externo."
"$sevenzip" x -y "-o$tmp/extracted" "$setup" > "$tmp/extract-log.txt"
for file in bin/CIRCE.exe bin/CIRCEJanela.exe tools/diagnostico.ps1 tools/deps.ps1 tools/video.ps1 assets/marca.ico vendor/PresentMon/PresentMon.exe; do
    actual=$(find "$tmp/extracted" -type f -name "$(basename "$file")")
    cmp "$file" "$actual"
done
test -f "$tmp/extracted/\$PLUGINSDIR/Idioma.ps1"
main=$(find "$tmp/extracted" -type f -name CIRCE.exe)
cmp LICENSE "$(dirname "$main")/LICENSE"
cmp vendor/PresentMon/LICENSE.txt "$(find "$tmp/extracted" -name 'PresentMon-LICENSE.txt')"
cmp vendor/PresentMon/THIRD_PARTY.txt "$(find "$tmp/extracted" -name 'PresentMon-THIRD_PARTY.txt')"
cmp tools/Idioma.ps1 "$tmp/extracted/\$PLUGINSDIR/Idioma.ps1"
cmp tools/Idioma.ps1 "$(dirname "$main")/tools/Idioma.ps1"
cmp assets/installer.bmp "$tmp/extracted/\$PLUGINSDIR/modern-wizard.bmp"
cmp assets/installer-header.bmp "$tmp/extracted/\$PLUGINSDIR/modern-header.bmp"
cmp bin/CIRCEIdioma.dll "$tmp/extracted/\$PLUGINSDIR/CIRCEIdioma.dll"
python3 -B tests/installer.py "$tmp" "${WINDRES:-x86_64-w64-mingw32-windres}" "$sevenzip"
echo "OK: assistente NSIS x64 com subsistema gráfico, integridade do pacote e conteúdo dos binários e scripts de medição."

for binary in bin/CIRCE.exe "$setup"; do
    "${WINDRES:-x86_64-w64-mingw32-windres}" -J coff -O rc "$binary" "$tmp/version.rc"
    grep -q 'VALUE "ProductName", "CIRCE"' "$tmp/version.rc"
    grep -q 'VALUE "ProductVersion", "0.0.1"' "$tmp/version.rc"
    grep -q 'VALUE "FileVersion", "0.0.1.4"' "$tmp/version.rc"
done
echo "OK: o recurso de versão do instalador e o da aplicação identificam CIRCE 0.0.1."
