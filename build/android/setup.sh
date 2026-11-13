#!/usr/bin/env bash
# Baixa SDL2 (+ classes Java) e gl4es para android/deps. Idempotente. Precisa de git e rede (so na 1a vez).
set -euo pipefail
cd "$(dirname "$0")"
SDL_TAG="${SDL_TAG:-release-2.30.10}"
mkdir -p deps
[ -d deps/SDL2 ]  || git clone --depth 1 --branch "$SDL_TAG" https://github.com/libsdl-org/SDL.git deps/SDL2
[ -d deps/gl4es ] || git clone --depth 1 https://github.com/ptitSeb/gl4es.git deps/gl4es
J=app/src/main/java/org/libsdl/app; mkdir -p "$J"
cp -f deps/SDL2/android-project/app/src/main/java/org/libsdl/app/*.java "$J/"
echo "deps OK"
