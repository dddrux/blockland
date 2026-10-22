#!/usr/bin/env bash
# Gera o executavel do Linux: ./blockland     Uso: scripts/build-linux.sh [--pack] [--debug]
#  --release  -O2 (mais lento de compilar, jogo mais rapido)   --pack  tambem cria dist/blockland-linux.tar.gz (executavel + assets/)      --debug  -O0 -g (sem otimizar)
# Requer: g++ (C++17), SDL2 (pacote -dev / "sdl2" no Arch) e OpenGL (Mesa, libGL).   Variaveis: CXX=g++  EXTRA="-march=native"
set -euo pipefail
cd "$(dirname "$0")/.."
PACK=0; OPT="-O1"   # padrao -O1: compila bem mais rapido; use --release (ou --pack) para -O2
for a in "$@"; do case "$a" in --pack) PACK=1; OPT="-O2";; --release) OPT="-O2";; --debug) OPT="-O0 -g";; -h|--help) sed -n 2,5p "$0"; exit 0;; *) echo "opcao desconhecida: $a"; exit 1;; esac; done
CXX="${CXX:-g++}"
command -v "$CXX" >/dev/null || { echo "ERRO: $CXX nao encontrado (instale g++)"; exit 1; }
if command -v sdl2-config >/dev/null; then SDL="$(sdl2-config --cflags --libs)"
elif command -v pkg-config >/dev/null && pkg-config --exists sdl2; then SDL="$(pkg-config --cflags --libs sdl2)"
else echo "ERRO: SDL2 (desenvolvimento) nao encontrado. Arch: pacman -S sdl2 | Debian/Ubuntu: apt install libsdl2-dev"; exit 1; fi
source scripts/fastcc.sh
echo "Compilando Linux ($CCACHE $CXX $OPT${CCACHE:+, cache})..."
$CCACHE $CXX $OPT $FASTFLAGS -std=c++17 ${EXTRA:-} src/main.cpp $SDL -lGL -o blockland
echo "OK Linux -> ./blockland"
if [ $PACK = 1 ]; then
  rm -rf dist/blockland-linux && mkdir -p dist/blockland-linux && cp blockland dist/blockland-linux/ && cp -r assets dist/blockland-linux/
  tar -C dist -czf dist/blockland-linux.tar.gz blockland-linux && echo "Pacote -> dist/blockland-linux.tar.gz"
fi
