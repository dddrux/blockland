#!/usr/bin/env bash
# Linux 64 ou 32 bits -> executaveis/Blockland-linux-64bits.AppImage | Blockland-linux-32bits.AppImage
# Uso: build/linux.sh 64|32      (BL_OPT="-O1" p/ compilar mais rapido; LINUX_DOCKER=1 forca o container Debian = AppImage que roda em distros mais antigas)
# Sem g++/SDL2 (32 bits: sem g++-multilib/libsdl2-dev:i386) usa docker/podman sozinho.
set -euo pipefail
source "$(dirname "$0")/lib.sh"
B="${1:-64}"; OPT="${BL_OPT:--O2}"
case "$B" in
  64) ARCH=x86_64; IMG=blockland-linux64; M32=""; PKGS="g++ libsdl2-dev libgl1-mesa-dev"; PRE="";;
  32) ARCH=i686;   IMG=blockland-linux32; M32="-m32"; PKGS="g++-multilib libsdl2-dev:i386 libgl1-mesa-dev:i386"; PRE="dpkg --add-architecture i386 &&";;
  *) die "uso: linux.sh 64|32";;
esac
W="$WORK/linux$B"; mkdir -p "$W/lib"
[ $B = 32 ] && export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig:/usr/lib32/pkgconfig:/usr/share/pkgconfig
sdlflags() { pkg-config --cflags --libs sdl2 2>/dev/null || sdl2-config --cflags --libs; }
native_ok() {
  command -v g++ >/dev/null && { command -v pkg-config >/dev/null || command -v sdl2-config >/dev/null; } && sdlflags >/dev/null 2>&1 || return 1
  echo 'int main(){}' | g++ $M32 -x c++ - -lGL -o /dev/null 2>/dev/null
}
compile() {
  source build/fastcc.sh
  echo "Compilando Linux ${B} bits (g++ $M32 $OPT)..."
  $CCACHE g++ $M32 $OPT $FASTFLAGS -std=c++17 ${EXTRA:-} src/main.cpp $(sdlflags) -lGL -o "$W/blockland"
  local d; d="$(pkg-config --variable=libdir sdl2 2>/dev/null || echo /usr/lib)"; cp -L "$d"/libSDL2-2.0.so.0* "$W/lib/" 2>/dev/null || true
}
if [ -n "${IN_IMAGE:-}" ]; then compile; exit 0; fi
if [ "${LINUX_DOCKER:-0}" != 1 ] && native_ok; then compile
else
  DF="FROM debian:bookworm
RUN $PRE apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq $PKGS pkg-config ca-certificates file && rm -rf /var/lib/apt/lists/*"
  run_in_image "$IMG" "$DF" build/linux.sh "$B"
fi
[ -s "$W/blockland" ] || die "o binario nao foi gerado"
make_appimage "$ARCH" "$W/blockland" blockland "$W/lib" "$OUT/Blockland-linux-${B}bits.AppImage"
