#!/usr/bin/env bash
# Raspberry Pi 32 bits (armhf) -> executaveis/Blockland-raspberrypi-32bits.AppImage
# Uso: build/rpi.sh          (pelo build.sh: RPI_DOCKER=1 RPI_ARCH="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard" ./build.sh --rpi --pack)
# RPI_DOCKER=1 forca docker/podman mesmo com toolchain do yay/AUR (ignora o sysroot local). Sem toolchain armhf no PC usa docker/podman sozinho (imagem Debian em cache).
# No PC (x86_64) faz CROSS-COMPILACAO; no proprio Pi compila nativo (g++, libsdl2-dev, libgl1-mesa-dev). Variaveis: RPI_ARCH="-mcpu=..." RPI_NATIVE=1 EXTRA="..."
# Modo BL_RPI: janela 640x420, 60 FPS travado (vsync + limitador), maximo 6 chunks, sem luz dinamica.
set -euo pipefail
source "$(dirname "$0")/lib.sh"
W="$WORK/rpi"; mkdir -p "$W/lib"; SYS="${RPI_SYSROOT:-$WORK/rpi-sysroot}"; [ -n "${IN_IMAGE:-}" ] && SYS=/nonexistent
HOST="$(uname -m)"; CROSS=0; LOCALSYS=0; ML=/usr/lib/arm-linux-gnueabihf
if [ "${RPI_NATIVE:-0}" != 1 ] && [ "$HOST" != armv7l ] && [ "$HOST" != armv8l ] && [ "$HOST" != aarch64 ]; then CROSS=1; fi
compile() {
  if [ $CROSS = 1 ]; then
    CXX="${CXX:-arm-linux-gnueabihf-g++}"; command -v "$CXX" >/dev/null || die "$CXX nao encontrado (apt install g++-arm-linux-gnueabihf)"; command -v pkg-config >/dev/null || die "apt install pkg-config"
    if [ ! -f "$SYS/lib/pkgconfig/sdl2.pc" ] && ! PKG_CONFIG_LIBDIR="$ML/pkgconfig" pkg-config --exists sdl2 2>/dev/null; then
      echo "Sem SDL2/GL armhf: montando sysroot local (build/rpi-sysroot.sh)..."; bash build/rpi-sysroot.sh || exit 1; fi
    if [ -f "$SYS/lib/pkgconfig/sdl2.pc" ]; then
      LOCALSYS=1; export PKG_CONFIG_LIBDIR="$SYS/lib/pkgconfig"   # libstdc++/libgcc estaticas: a g++ do PC e mais nova que a da Pi OS
      SDL="$(pkg-config --cflags --libs sdl2) -I$SYS/include -L$SYS/lib -Wl,-rpath-link,$SYS/lib -static-libstdc++ -static-libgcc"; SOLIB="$SYS/lib"
    else export PKG_CONFIG_LIBDIR="$ML/pkgconfig:/usr/share/pkgconfig"; SDL="$(pkg-config --cflags --libs sdl2)"; SOLIB="$ML"; fi
    ARCH="${RPI_ARCH:-}"; ARCH="${ARCH:--mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard}"
  else
    CXX="${CXX:-g++}"; command -v "$CXX" >/dev/null || die "instale g++"; command -v sdl2-config >/dev/null || die "apt install libsdl2-dev libgl1-mesa-dev"
    SDL="$(sdl2-config --cflags --libs)"; SOLIB="$(dirname "$(ldconfig -p | awk '/libSDL2-2.0.so.0/{print $NF; exit}')")"
    case "$HOST" in aarch64) ARCH="-mcpu=cortex-a53";; armv7l|armv8l) ARCH="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard";; *) ARCH="";; esac; ARCH="${RPI_ARCH:-$ARCH}"
  fi
  echo "Compilando Raspberry Pi 32 bits ($CXX -O3 $ARCH)$([ $CROSS = 1 ] && echo ' [cross]')..."
  $CXX -O3 -std=c++17 -fno-plt -fomit-frame-pointer -fno-stack-protector -fno-stack-clash-protection -U_FORTIFY_SOURCE -funroll-loops -mtune=cortex-a53 -fno-semantic-interposition -fvisibility=hidden -fno-math-errno -fno-trapping-math -ffunction-sections -fdata-sections -Wl,--gc-sections $ARCH -DBL_RPI ${EXTRA:-} src/main.cpp $SDL -lGL -o "$W/blockland-rpi"
  cp -L "$SOLIB"/libSDL2-2.0.so.0* "$W/lib/" 2>/dev/null || true
  if [ $CROSS = 1 ] && command -v arm-linux-gnueabihf-objdump >/dev/null; then   # glibc da Pi OS Bookworm = 2.36
    GV="$(arm-linux-gnueabihf-objdump -T "$W/blockland-rpi" | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -Vu | tail -1)"
    [ -n "$GV" ] && [ "$(printf '%s\n2.36\n' "$GV" | sort -V | tail -1)" != 2.36 ] && echo "AVISO: o binario exige glibc $GV (> 2.36 da Pi OS Bookworm); use RPI_DOCKER=1 se nao abrir no Pi."
  fi
  return 0   # patchRpiBuild.1: sem isto o ultimo teste (&&) devolvia 1 e o set -e abortava o build com o binario ja pronto
}
if [ -n "${IN_IMAGE:-}" ]; then compile; exit 0; fi
if [ $CROSS = 1 ] && { [ "${RPI_DOCKER:-0}" = 1 ] || ! command -v "${CXX:-arm-linux-gnueabihf-g++}" >/dev/null; }; then
  DF="FROM debian:bookworm
RUN dpkg --add-architecture armhf && apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq g++-arm-linux-gnueabihf libsdl2-dev:armhf libgl1-mesa-dev:armhf pkg-config wget ca-certificates file && rm -rf /var/lib/apt/lists/*"
  run_in_image blockland-rpi-cross "$DF" build/rpi.sh
else compile; fi
[ -s "$W/blockland-rpi" ] || die "o binario nao foi gerado"
make_appimage armhf "$W/blockland-rpi" blockland-rpi "$W/lib" "$OUT/Blockland-raspberrypi-32bits.AppImage"
