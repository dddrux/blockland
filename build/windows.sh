#!/usr/bin/env bash
# Windows 64 ou 32 bits -> executaveis/Blockland-setup-windows-64bits.exe | ...-32bits.exe  (instalador: instala/atualiza, sem admin)
# Uso: build/windows.sh 64|32
# Cross no Linux: x86_64-w64-mingw32-g++ (64) / i686-w64-mingw32-g++ (32) + NSIS (makensis).   Debian/Ubuntu: apt install g++-mingw-w64-x86-64 g++-mingw-w64-i686 nsis
# Arch: pacman -S mingw-w64-gcc nsis.  A SDL2 do mingw (64 e 32) e baixada sozinha 1 vez para build/cache. No MSYS2 usa o g++/SDL2 do proprio MinGW (so a arquitetura do shell).
# Variaveis: MINGW_PREFIX=raiz da SDL2 mingw, CXX=..., EXTRA="..." (flags), CONSOLE=1 (mantem janela de console)
set -euo pipefail
source "$(dirname "$0")/lib.sh"
B="${1:-64}"; OPT="${BL_OPT:--O2}"; GUI="-mwindows"; [ "${CONSOLE:-0}" = 1 ] && GUI="-mconsole"
case "$B" in 64) T=x86_64-w64-mingw32;; 32) T=i686-w64-mingw32;; *) die "uso: windows.sh 64|32";; esac
W="$WORK/windows$B"; S="$W/stage"; rm -rf "$S"; mkdir -p "$S"
source build/fastcc.sh
LIBS="-lopengl32 -lws2_32 -static-libgcc -static-libstdc++"
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    case "${MSYSTEM:-}" in MINGW64|UCRT64|CLANG64) [ $B = 64 ] || die "este shell MSYS2 e 64 bits; abra o 'MSYS2 MinGW 32-bit' para o 32 bits";; MINGW32) [ $B = 32 ] || die "este shell MSYS2 e 32 bits";; esac
    command -v g++ >/dev/null && command -v sdl2-config >/dev/null || die "no MSYS2 instale: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 (ou mingw-w64-i686-... no 32 bits)"
    SDLF="$(sdl2-config --cflags --libs)"; CXX=g++; DLLDIR="$(dirname "$(command -v g++)")"; RC=windres ;;
  *)
    if [ -z "${CXX:-}" ]; then CXX="$T-g++"; command -v "$T-g++-posix" >/dev/null && CXX="$T-g++-posix"; fi   # variante posix = std::thread; a win32 nao tem
    command -v "$CXX" >/dev/null || die "$CXX nao encontrado (Debian/Ubuntu: apt install g++-mingw-w64-x86-64 g++-mingw-w64-i686 | Arch: pacman -S mingw-w64-gcc)"
    M="${MINGW_PREFIX:-}"; [ "$B" = 64 ] || M=""   # MINGW_PREFIX so vale para o 64 bits
    inc_ok() { [ -f "$1/include/SDL2/SDL.h" ] || [ -f "$1/include/SDL.h" ]; }
    if [ -z "$M" ]; then for c in "/usr/$T" "$CACHE/SDL2-$SDLV_MINGW/$T"; do inc_ok "$c" && { M="$c"; break; }; done; fi
    if [ -z "$M" ]; then   # SDL2 do mingw nao vem no apt: baixa o pacote oficial (traz 32 e 64 bits)
      echo "Baixando SDL2 $SDLV_MINGW para mingw (1a vez)..."; TB="$CACHE/SDL2-devel-$SDLV_MINGW-mingw.tar.gz"
      fetch "https://github.com/libsdl-org/SDL/releases/download/release-$SDLV_MINGW/SDL2-devel-$SDLV_MINGW-mingw.tar.gz" "$TB"; tar -xzf "$TB" -C "$CACHE"; M="$CACHE/SDL2-$SDLV_MINGW/$T"
    fi
    inc_ok "$M" || die "SDL.h do mingw nao encontrado em $M/include[/SDL2]"
    INC="$M/include/SDL2"; [ -f "$INC/SDL.h" ] || INC="$M/include"
    SDLF="-I$INC -L$M/lib -lmingw32 -lSDL2main -lSDL2"; DLLDIR="$M/bin"; RC="${CXX%g++*}windres"; command -v "$RC" >/dev/null || RC="$T-windres" ;;
esac
echo "Compilando Windows ${B} bits ($CXX $OPT $GUI)..."
RES=""; if command -v "$RC" >/dev/null && (cd build/installer && "$RC" blockland.rc -O coff -o "$W/res.o") 2>/dev/null; then RES="$W/res.o"; else echo "aviso: windres nao encontrado; exe sem icone/manifesto"; fi
$CCACHE $CXX $OPT -pipe -std=c++17 ${EXTRA:-} src/main.cpp $RES $SDLF $LIBS $GUI -o "$S/blockland.exe"
# DLLs ao lado do exe
cp -f "$DLLDIR/SDL2.dll" "$S/" 2>/dev/null || { [ $B = 64 ] && cp -f build/libs/win64/SDL2.dll "$S/"; } || echo "aviso: SDL2.dll nao encontrada"
PW="$($CXX -print-file-name=libwinpthread-1.dll 2>/dev/null || true)"
if [ -f "$PW" ]; then cp -f "$PW" "$S/"; elif [ -f "$DLLDIR/libwinpthread-1.dll" ]; then cp -f "$DLLDIR/libwinpthread-1.dll" "$S/"; elif [ $B = 64 ]; then cp -f build/libs/win64/libwinpthread-1.dll "$S/"; else echo "aviso: libwinpthread-1.dll (32 bits) nao encontrada; copie para $S"; fi
cp -r assets "$S/assets"
build/installer/build-installer.sh "$S" "$OUT/Blockland-setup-windows-${B}bits.exe" "$VER" "$B"
echo "OK -> executaveis/Blockland-setup-windows-${B}bits.exe"
