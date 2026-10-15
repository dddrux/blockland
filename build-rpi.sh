#!/usr/bin/env bash
# Gera o executavel para Raspberry Pi 3B (Raspberry Pi OS 32/64 bits): ./blockland-rpi
# Uso: ./build-rpi.sh [--pack]      Requer: g++, libsdl2-dev, libgl1-mesa-dev (driver Mesa V3D/VC4 com GL ligado: raspi-config > Advanced > GL Driver)
# Modo BL_RPI: janela 640x420, 60 FPS travado (vsync + limitador), maximo 6 chunks, sem luz dinamica.
set -euo pipefail
cd "$(dirname "$0")"
PACK=0; for a in "$@"; do case "$a" in --pack) PACK=1;; *) echo "opcao desconhecida: $a"; exit 1;; esac; done
CXX="${CXX:-g++}"; command -v "$CXX" >/dev/null || { echo "ERRO: instale g++"; exit 1; }
if command -v sdl2-config >/dev/null; then SDL="$(sdl2-config --cflags --libs)"; else echo "ERRO: apt install libsdl2-dev libgl1-mesa-dev"; exit 1; fi
case "$(uname -m)" in aarch64) ARCH="-mcpu=cortex-a53";; armv7l|armv8l) ARCH="-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard";; *) ARCH="";; esac
echo "Compilando Raspberry Pi 3B ($CXX -O3 $ARCH)..."
$CXX -O3 -std=c++17 -fno-plt -fomit-frame-pointer $ARCH -DBL_RPI ${EXTRA:-} src/main.cpp $SDL -lGL -lpthread -o blockland-rpi
echo "OK -> ./blockland-rpi (execute na pasta do jogo)"
if [ $PACK = 1 ]; then
  rm -rf dist/blockland-rpi && mkdir -p dist/blockland-rpi && cp blockland-rpi dist/blockland-rpi/ && cp -r assets dist/blockland-rpi/
  tar -C dist -czf dist/blockland-rpi.tar.gz blockland-rpi && echo "Pacote -> dist/blockland-rpi.tar.gz"
fi
