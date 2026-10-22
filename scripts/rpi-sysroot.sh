#!/usr/bin/env bash
# Prepara o "sysroot" armhf LOCAL (.rpi-sysroot/) para cross-compilar o Blockland sem docker, usando o
# arm-linux-gnueabihf-gcc do yay/AUR. Cria: SDL2 2.26.5 (a mesma da Pi OS Bookworm) + headers GL + libGL "stub" (so p/ linkar;
# no Pi a libGL/SDL2 reais do sistema sao usadas). Rode uma vez: scripts/rpi-sysroot.sh   (precisa de rede p/ baixar a SDL2)
set -euo pipefail
cd "$(dirname "$0")/.."
T=arm-linux-gnueabihf; SYS="$PWD/.rpi-sysroot"; SDLV="${SDLV:-2.26.5}"
for c in $T-gcc $T-g++ make; do command -v $c >/dev/null || { echo "ERRO: $c nao encontrado (yay -S arm-linux-gnueabihf-gcc)"; exit 1; }; done
mkdir -p "$SYS/include" "$SYS/lib" "$SYS/build"
echo "[1/3] headers OpenGL (do seu PC)"
for d in GL KHR EGL GLES2 GLES3; do [ -d /usr/include/$d ] && cp -r /usr/include/$d "$SYS/include/"; done
[ -f "$SYS/include/GL/gl.h" ] || { echo "ERRO: /usr/include/GL/gl.h ausente. Rode: sudo pacman -S mesa libglvnd"; exit 1; }
echo "[2/3] libGL stub (simbolos gl* usados em src/)"
grep -rhoE '\bgl[A-Z][A-Za-z0-9_]*' src --include='*.cpp' --include='*.h' --include='*.hpp' --include='*.c' --include='*.inl' --include='*.inc' 2>/dev/null | sort -u > "$SYS/build/glsyms.txt" || true
{ echo 'void glStubDummy(void){}'; while read -r s; do [ -n "$s" ] && echo "void $s(void){}"; done < "$SYS/build/glsyms.txt"; } > "$SYS/build/glstub.c"
$T-gcc -shared -fPIC -nostdlib -Wl,-soname,libGL.so.1 -o "$SYS/lib/libGL.so.1" "$SYS/build/glstub.c"
ln -sf libGL.so.1 "$SYS/lib/libGL.so"; echo "  $(wc -l < "$SYS/build/glsyms.txt") simbolos"
echo "[3/3] SDL2 $SDLV armhf"
if [ ! -f "$SYS/lib/pkgconfig/sdl2.pc" ]; then
  TB="$SYS/build/SDL2-$SDLV.tar.gz"
  if [ ! -f "$TB" ]; then
    URL="https://github.com/libsdl-org/SDL/releases/download/release-$SDLV/SDL2-$SDLV.tar.gz"
    if command -v curl >/dev/null; then curl -fL -o "$TB" "$URL"; else wget -O "$TB" "$URL"; fi || { rm -f "$TB"; echo "ERRO: sem rede p/ baixar $URL (baixe e salve em $TB)"; exit 1; }
  fi
  tar -xzf "$TB" -C "$SYS/build"; cd "$SYS/build/SDL2-$SDLV"
  export PKG_CONFIG_LIBDIR=/nonexistent PKG_CONFIG_PATH=
  CC=$T-gcc ./configure --host=$T --prefix="$SYS" --enable-shared --disable-static \
    --disable-video-x11 --disable-video-wayland --disable-video-kmsdrm --disable-pulseaudio --disable-alsa --disable-jack \
    --disable-sndio --disable-pipewire --disable-libudev --disable-dbus --disable-ime --disable-ibus > "$SYS/build/configure.log" 2>&1 \
    || { tail -n 15 "$SYS/build/configure.log"; echo "ERRO no configure (log: $SYS/build/configure.log)"; exit 1; }
  make -j"$(nproc)" > "$SYS/build/make.log" 2>&1 && make install >> "$SYS/build/make.log" 2>&1 \
    || { tail -n 15 "$SYS/build/make.log"; echo "ERRO no make (log: $SYS/build/make.log)"; exit 1; }
fi
echo "OK: sysroot em $SYS  -> agora: ./build.sh --rpi --pack"
