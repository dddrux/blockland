#!/usr/bin/env bash
# Um comando no Linux (Debian/Ubuntu): instala as dependencias e gera TUDO:
#   ./blockland (Linux)   ./blockland.exe (Windows, cross)   dist/setup.exe (instalador Windows, NSIS)
# Uso: ./build-tudo-linux.sh [--sem-apt] [--so-linux]      (precisa de sudo so para o apt)
set -euo pipefail
cd "$(dirname "$0")"
APT=1; WIN=1; SDLV=2.30.9
for a in "$@"; do case "$a" in --sem-apt) APT=0;; --so-linux) WIN=0;; -h|--help) sed -n 2,5p "$0"; exit 0;; *) echo "opcao desconhecida: $a"; exit 1;; esac; done
SUDO=""; [ "$(id -u)" = 0 ] || SUDO="sudo"
if [ $APT = 1 ] && command -v apt-get >/dev/null; then
  $SUDO apt-get update -q
  PK="g++ zip libsdl2-dev libgl1-mesa-dev"; [ $WIN = 1 ] && PK="$PK g++-mingw-w64-x86-64 nsis curl"
  $SUDO apt-get install -y $PK
fi
./build-linux.sh --pack
[ $WIN = 1 ] || exit 0
M="${MINGW_PREFIX:-}"
if [ -z "$M" ]; then   # SDL2 para mingw nao vem no apt: baixa o pacote oficial uma vez
  D="$HOME/.cache/blockland-sdl2-mingw"
  if [ ! -d "$D/SDL2-$SDLV/x86_64-w64-mingw32" ]; then mkdir -p "$D"; curl -fL "https://github.com/libsdl-org/SDL/releases/download/release-$SDLV/SDL2-devel-$SDLV-mingw.tar.gz" | tar -xz -C "$D"; fi
  M="$D/SDL2-$SDLV/x86_64-w64-mingw32"
fi
export MINGW_PREFIX="$M"
if command -v x86_64-w64-mingw32-g++-posix >/dev/null; then export CXX=x86_64-w64-mingw32-g++-posix; fi
for d in /usr/x86_64-w64-mingw32/lib /usr/lib/gcc/x86_64-w64-mingw32/*-posix; do [ -f "$d/libwinpthread-1.dll" ] && cp -f "$d/libwinpthread-1.dll" . && break; done
INSTALLER=nsis ./build-windows.sh --installer
echo "PRONTO: ./blockland  ./blockland.exe  dist/setup.exe"
