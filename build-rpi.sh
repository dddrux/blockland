#!/usr/bin/env bash
# Gera o executavel 32 bits (armhf) para Raspberry Pi: ./blockland-rpi  e, com --pack, dist/Blockland-armhf.AppImage
# Uso: scripts/build-rpi.sh [--pack] [--appimage]      (--pack = tar.gz + AppImage; --appimage = so o AppImage)
# RPI_DOCKER=1 forca o docker/podman mesmo com toolchain do yay/AUR instalada (ignora o .rpi-sysroot): RPI_DOCKER=1 ./build.sh --rpi --pack
# Sem toolchain no PC, usa docker/podman automaticamente (imagem Debian em cache). No PC (x86_64) faz CROSS-COMPILACAO 32 bits automaticamente. Requer (Debian/Ubuntu):
#   sudo dpkg --add-architecture armhf && sudo apt update        (Ubuntu: fontes armhf em ports.ubuntu.com)
#   sudo apt install g++-arm-linux-gnueabihf libsdl2-dev:armhf libgl1-mesa-dev:armhf pkg-config
# No proprio Pi compila nativo (g++, libsdl2-dev, libgl1-mesa-dev). Variaveis: RPI_ARCH="-mcpu=..." RPI_NATIVE=1 EXTRA="..."
# Modo BL_RPI: janela 640x420, 60 FPS travado (vsync + limitador), maximo 6 chunks, sem luz dinamica.
set -euo pipefail
cd "$(dirname "$0")/.."
PACK=0; APPI=0
for a in "$@"; do case "$a" in --pack) PACK=1; APPI=1;; --appimage) APPI=1;; *) echo "opcao desconhecida: $a"; exit 1;; esac; done
HOST="$(uname -m)"; CROSS=0; LOCALSYS=0
if [ "${RPI_NATIVE:-0}" != 1 ] && [ "$HOST" != armv7l ] && [ "$HOST" != armv8l ] && [ "$HOST" != aarch64 ]; then CROSS=1; fi
ML=/usr/lib/arm-linux-gnueabihf
if [ $CROSS = 1 ] && [ -z "${RPI_IN_DOCKER:-}" ] && { [ "${RPI_DOCKER:-0}" = 1 ] || ! command -v "${CXX:-arm-linux-gnueabihf-g++}" >/dev/null; }; then
  # sem toolchain armhf no PC (Arch/CachyOS/Fedora...): roda tudo num container Debian (docker ou podman), com imagem em cache
  ENG=""; for e in docker podman; do command -v $e >/dev/null && { ENG=$e; break; }; done
  [ -n "$ENG" ] || { echo "ERRO: sem g++ armhf e sem docker/podman. Arch/CachyOS: sudo pacman -S docker && sudo systemctl enable --now docker (ou: podman)"; exit 1; }
  IMG=blockland-rpi-cross
  if ! $ENG image inspect $IMG >/dev/null 2>&1; then
    echo "Criando imagem $IMG (so na 1a vez, baixa ~500 MB)..."
    $ENG build -q -t $IMG - <<'DF' || { echo "ERRO ao criar a imagem (rede/permissao do $ENG?)"; exit 1; }
FROM debian:bookworm
RUN dpkg --add-architecture armhf && apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq g++-arm-linux-gnueabihf libsdl2-dev:armhf libgl1-mesa-dev:armhf pkg-config wget ca-certificates file && rm -rf /var/lib/apt/lists/*
DF
  fi
  UARG="--user $(id -u):$(id -g)"; [ $ENG = podman ] && UARG="--userns=keep-id"
  echo "Compilando dentro do container ($ENG)..."
  exec $ENG run --rm $UARG -e HOME=/tmp -e RPI_IN_DOCKER=1 -e RPI_SYSROOT=/nonexistent -e EXTRA="${EXTRA:-}" -e RPI_ARCH="${RPI_ARCH:-}" -v "$PWD":/w -w /w $IMG scripts/build-rpi.sh "$@"
fi
if [ $CROSS = 1 ]; then
  CXX="${CXX:-arm-linux-gnueabihf-g++}"
  command -v "$CXX" >/dev/null || { echo "ERRO: $CXX nao encontrado (apt install g++-arm-linux-gnueabihf)"; exit 1; }
  command -v pkg-config >/dev/null || { echo "ERRO: apt install pkg-config"; exit 1; }
  SYS="${RPI_SYSROOT:-$PWD/.rpi-sysroot}"; LOCALSYS=0
  if [ ! -f "$SYS/lib/pkgconfig/sdl2.pc" ] && ! PKG_CONFIG_LIBDIR="$ML/pkgconfig" pkg-config --exists sdl2 2>/dev/null; then
    echo "Sem SDL2/GL armhf: montando sysroot local (scripts/rpi-sysroot.sh)..."; bash scripts/rpi-sysroot.sh || exit 1
  fi
  if [ -f "$SYS/lib/pkgconfig/sdl2.pc" ]; then
    LOCALSYS=1; export PKG_CONFIG_LIBDIR="$SYS/lib/pkgconfig"
    # libstdc++/libgcc estaticas: a g++ do PC e mais nova que a da Pi OS
    SDL="$(pkg-config --cflags --libs sdl2) -I$SYS/include -L$SYS/lib -Wl,-rpath-link,$SYS/lib -static-libstdc++ -static-libgcc"
  else
    export PKG_CONFIG_LIBDIR="$ML/pkgconfig:/usr/share/pkgconfig"; SDL="$(pkg-config --cflags --libs sdl2)"
  fi
  ARCH="${RPI_ARCH:-}"; ARCH="${ARCH:--mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard}"
else
  CXX="${CXX:-g++}"; command -v "$CXX" >/dev/null || { echo "ERRO: instale g++"; exit 1; }
  if command -v sdl2-config >/dev/null; then SDL="$(sdl2-config --cflags --libs)"; else echo "ERRO: apt install libsdl2-dev libgl1-mesa-dev"; exit 1; fi
  case "$HOST" in aarch64) ARCH="-mcpu=cortex-a53";; armv7l|armv8l) ARCH="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard";; *) ARCH="";; esac
  ARCH="${RPI_ARCH:-$ARCH}"
fi
echo "Compilando Raspberry Pi 32 bits ($CXX -O3 $ARCH)$([ $CROSS = 1 ] && echo ' [cross]')..."
$CXX -O3 -std=c++17 -fno-plt -fomit-frame-pointer $ARCH -DBL_RPI ${EXTRA:-} src/main.cpp $SDL -lGL -lpthread -o blockland-rpi
echo "OK -> ./blockland-rpi (execute na pasta do jogo)"
if [ $CROSS = 1 ] && command -v arm-linux-gnueabihf-objdump >/dev/null; then   # glibc da Pi OS Bookworm = 2.36
  GV="$(arm-linux-gnueabihf-objdump -T blockland-rpi | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -Vu | tail -1)"
  [ -n "$GV" ] && [ "$(printf '%s\n2.36\n' "$GV" | sort -V | tail -1)" != 2.36 ] && echo "AVISO: o binario exige glibc $GV (> 2.36 da Pi OS Bookworm); pode nao abrir no Pi. Use o docker (apague .rpi-sysroot e instale docker) se acontecer."
fi
if [ $PACK = 1 ]; then
  rm -rf dist/blockland-rpi && mkdir -p dist/blockland-rpi && cp blockland-rpi dist/blockland-rpi/ && cp -r assets dist/blockland-rpi/
  tar -C dist -czf dist/blockland-rpi.tar.gz blockland-rpi && echo "Pacote -> dist/blockland-rpi.tar.gz"
fi
if [ $APPI = 1 ]; then
  rm -rf AppDir-rpi && mkdir -p AppDir-rpi/usr/bin AppDir-rpi/usr/lib dist
  cp blockland-rpi AppDir-rpi/usr/bin/ && cp -r assets AppDir-rpi/usr/bin/
  # embute a SDL2 armhf (libGL fica com o sistema: depende do driver Mesa do Pi)
  SOLIB="$ML"; [ $LOCALSYS = 1 ] && SOLIB=/nonexistent; [ $CROSS = 0 ] && SOLIB="$(dirname "$(ldconfig -p | awk '/libSDL2-2.0.so.0/{print $NF; exit}')")"
  for l in "$SOLIB"/libSDL2-2.0.so.0*; do [ -e "$l" ] && cp -L "$l" AppDir-rpi/usr/lib/; done
  ls AppDir-rpi/usr/lib/libSDL2* >/dev/null 2>&1 || echo "AVISO: libSDL2 nao embutida (o Pi usa a SDL2 do sistema: libsdl2-2.0-0, ja vem na Pi OS desktop)"
  cat > AppDir-rpi/AppRun <<'EOT'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
cd "$HERE/usr/bin"
exec "$HERE/usr/bin/blockland-rpi" "$@"
EOT
  chmod +x AppDir-rpi/AppRun
  printf '[Desktop Entry]\nType=Application\nName=Blockland\nExec=blockland-rpi\nIcon=blockland\nCategories=Game;\n' > AppDir-rpi/blockland.desktop
  cp blockland.png AppDir-rpi/blockland.png
  # appimagetool roda no PC (x86_64) e usa o runtime armhf no AppImage final
  if command -v appimagetool >/dev/null; then TOOL=appimagetool; else
    [ -f appimagetool ] || wget -q -O appimagetool https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage || { echo "ERRO: sem rede p/ baixar appimagetool (ponha 'appimagetool' no PATH)"; exit 1; }
    chmod +x appimagetool; TOOL=./appimagetool
  fi
  [ -f runtime-armhf ] || wget -q -O runtime-armhf https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-armhf || { echo "ERRO: sem rede p/ baixar runtime-armhf (salve-o na raiz do projeto)"; exit 1; }
  ARCH=armhf $TOOL --appimage-extract-and-run --runtime-file runtime-armhf AppDir-rpi dist/Blockland-armhf.AppImage >> logs/appimage-rpi.log 2>&1 \
    || ARCH=armhf $TOOL --runtime-file runtime-armhf AppDir-rpi dist/Blockland-armhf.AppImage >> logs/appimage-rpi.log 2>&1 \
    || { echo "ERRO ao gerar AppImage (veja logs/appimage-rpi.log)"; exit 1; }
  echo "AppImage -> dist/Blockland-armhf.AppImage"
fi
