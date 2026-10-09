#!/usr/bin/env bash
# Compila no Linux e no Windows (shell MSYS2 "MinGW64"). Requer g++ e SDL2 (pacote de desenvolvimento).
set -euo pipefail
cd "$(dirname "$0")"
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    # -mwindows: sem janela de console; troque por -mconsole para ver os printf
    g++ -O2 -std=c++17 src/main.cpp $(sdl2-config --cflags --libs) -lopengl32 -lws2_32 \
        -static-libgcc -static-libstdc++ -o blockland.exe
    echo "OK -> ./blockland.exe (copie SDL2.dll e a pasta assets/ junto, se for distribuir)" ;;
  *)
    g++ -O2 -std=c++17 src/main.cpp $(sdl2-config --cflags --libs) -lGL -o blockland
    echo "OK -> ./blockland" ;;
esac
