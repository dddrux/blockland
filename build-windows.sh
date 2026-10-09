#!/usr/bin/env bash
# Gera o executavel do Windows: ./blockland.exe     Uso: ./build-windows.sh [--pack] [--console] [--debug]
#  --pack     tambem cria dist/blockland-windows.zip (exe + SDL2.dll + libwinpthread-1.dll + assets/)
#  --console  mantem a janela de console (ver os printf/erros); sem isso o exe abre sem console (-mwindows)
# Dois modos, detectados sozinhos:
#  * MSYS2 (shell MinGW64): g++ + sdl2-config nativos        (pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2)
#  * Linux -> Windows (cross): x86_64-w64-mingw32-g++ + SDL2 do mingw (Arch: mingw-w64-gcc e mingw-w64-sdl2 do AUR)
# Variaveis: MINGW_PREFIX=/usr/x86_64-w64-mingw32 (raiz do SDL2 do mingw)   EXTRA="..." (flags extras)
set -euo pipefail
cd "$(dirname "$0")"
PACK=0; OPT="-O2"; GUI="-mwindows"
for a in "$@"; do case "$a" in --pack) PACK=1;; --console) GUI="-mconsole";; --debug) OPT="-O0 -g";; -h|--help) sed -n 2,9p "$0"; exit 0;; *) echo "opcao desconhecida: $a"; exit 1;; esac; done
LIBS="-lopengl32 -lws2_32 -static-libgcc -static-libstdc++"
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    command -v g++ >/dev/null && command -v sdl2-config >/dev/null || { echo "ERRO: no MSYS2 MinGW64 instale: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2"; exit 1; }
    SDLF="$(sdl2-config --cflags --libs)"; CXX=g++; DLLDIR="$(dirname "$(command -v g++)")" ;;
  *)
    CXX="${CXX:-x86_64-w64-mingw32-g++}"
    command -v "$CXX" >/dev/null || { echo "ERRO: $CXX nao encontrado. Arch: pacman -S mingw-w64-gcc (+ mingw-w64-sdl2 do AUR) | Debian/Ubuntu: apt install g++-mingw-w64-x86-64"; exit 1; }
    M="${MINGW_PREFIX:-/usr/x86_64-w64-mingw32}"; INC=""
    for d in "$M/include/SDL2" "$M/include"; do [ -f "$d/SDL.h" ] && INC="$d" && break; done
    [ -n "$INC" ] || { echo "ERRO: SDL.h do mingw nao encontrado em $M/include[/SDL2]. Instale mingw-w64-sdl2 ou defina MINGW_PREFIX."; exit 1; }
    SDLF="-I$INC -L$M/lib -lmingw32 -lSDL2main -lSDL2"; DLLDIR="$M/bin" ;;
esac
echo "Compilando Windows ($CXX $OPT $GUI)..."
$CXX $OPT -std=c++17 ${EXTRA:-} src/main.cpp $SDLF $LIBS $GUI -o blockland.exe
for d in SDL2.dll libwinpthread-1.dll; do   # DLLs ao lado do exe
  if [ -f "$DLLDIR/$d" ]; then cp -f "$DLLDIR/$d" . ; elif [ ! -f "$d" ]; then echo "aviso: $d nao encontrada; copie para junto do .exe"; fi
done
echo "OK Windows -> ./blockland.exe (com SDL2.dll e a pasta assets/ ao lado)"
if [ $PACK = 1 ]; then
  rm -rf dist/blockland-windows && mkdir -p dist/blockland-windows && cp blockland.exe dist/blockland-windows/ && cp -r assets dist/blockland-windows/
  for d in SDL2.dll libwinpthread-1.dll; do [ -f "$d" ] && cp "$d" dist/blockland-windows/; done
  rm -f dist/blockland-windows.zip
  if command -v zip >/dev/null; then (cd dist && zip -qr blockland-windows.zip blockland-windows); else python3 -c "import shutil;shutil.make_archive('dist/blockland-windows','zip','dist','blockland-windows')"; fi
  echo "Pacote -> dist/blockland-windows.zip"
fi
