#!/bin/sh
# usa os headers SDL2/GL do proprio projeto (build/android/deps) so para compilar sem SDL2-dev
cd "$(dirname "$0")/.." || exit 1
S=${SDL_INC:-build/android/deps/SDL2/include}; G=${GL_INC:-build/android/deps/gl4es/include}
g++ -std=c++17 -O1 -w -I$S -I$G -Isrc tests/test_endgate.cpp -Wl,--unresolved-symbols=ignore-all -o /tmp/test_endgate && /tmp/test_endgate
