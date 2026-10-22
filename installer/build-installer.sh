#!/usr/bin/env bash
# Monta dist/stage e gera dist/setup.exe com Inno Setup 6. Pre-requisito: ./blockland.exe (build-windows.sh).
# Windows (MSYS2/Git Bash): ISCC no PATH ou ISCC_EXE="/c/Program Files (x86)/Inno Setup 6/ISCC.exe"
# Linux: wine + Inno Setup 6 (instale uma vez: wine innosetup-6.x.exe /VERYSILENT); ISCC_EXE=<caminho do ISCC.exe> (padrao do wine abaixo)
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f blockland.exe ] || { echo "ERRO: blockland.exe nao existe; rode scripts/build-windows.sh primeiro"; exit 1; }
VER="$(sed -n 's/^#define BLOCKLAND_VERSION "v\{0,1\}\([0-9.]*\)[a-z]*".*/\1/p' src/main.cpp | head -1)"; VER="${VER:-1.1.0.19}"
S=dist/stage; rm -rf "$S"; mkdir -p "$S"
cp blockland.exe "$S/"; for d in SDL2.dll libwinpthread-1.dll; do [ -f "$d" ] && cp "$d" "$S/"; done
cp -r assets "$S/assets"
# patchBuild.1: no Linux sem wine, usa NSIS (apt install nsis) -> blockland.nsi; no Windows/wine continua o Inno Setup
if [ "${INSTALLER:-}" = nsis ] || { [ -z "${ISCC_EXE:-}" ] && ! command -v iscc >/dev/null && ! command -v wine >/dev/null && command -v makensis >/dev/null; }; then
  command -v makensis >/dev/null || { echo "ERRO: instale NSIS (Debian/Ubuntu: apt install nsis | Arch: pacman -S nsis)"; exit 1; }
  mkdir -p dist; (cd installer && makensis -V2 "-DAppVersion=$VER" "-DStage=../dist/stage" blockland.nsi); echo "OK -> dist/setup.exe (v$VER, NSIS)"; exit 0
fi
ISCC="${ISCC_EXE:-}"
if [ -z "$ISCC" ]; then
  if command -v iscc >/dev/null; then ISCC=iscc
  elif command -v ISCC.exe >/dev/null; then ISCC=ISCC.exe
  else ISCC="$HOME/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe"; fi
fi
cd installer
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) "$ISCC" "/DAppVersion=$VER" "/DStage=..\\dist\\stage" blockland.iss ;;
  *) if command -v iscc >/dev/null && [ "$ISCC" = iscc ]; then iscc "/DAppVersion=$VER" "/DStage=../dist/stage" blockland.iss
     else command -v wine >/dev/null || { echo "ERRO: instale wine + Inno Setup 6 (veja installer/README.md)"; exit 1; }
          wine "$ISCC" "/DAppVersion=$VER" "/DStage=..\\dist\\stage" blockland.iss; fi ;;
esac
echo "OK -> dist/setup.exe (v$VER)"
