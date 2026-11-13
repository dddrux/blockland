#!/usr/bin/env bash
# Instala as dependencias para buildar TUDO no Debian/Ubuntu (precisa de sudo so para o apt).   Uso: build/instalar-dependencias.sh
# Android: instale o JDK 17 + Android SDK e defina ANDROID_HOME (o build instala NDK/CMake sozinho). Raspberry Pi e Linux 32 bits usam docker/podman (imagens criadas sozinhas).
set -euo pipefail
SUDO=""; [ "$(id -u)" = 0 ] || SUDO="sudo"
command -v apt-get >/dev/null || { echo "Sem apt: instale g++, SDL2 (dev), mesa/libGL (dev), vulkan (dev), mingw-w64 (g++), nsis, docker, curl. Arch: pacman -S gcc sdl2 mesa vulkan-headers vulkan-icd-loader mingw-w64-gcc nsis docker curl"; exit 0; }
$SUDO apt-get update -q
$SUDO apt-get install -y g++ pkg-config libsdl2-dev libgl1-mesa-dev libvulkan-dev g++-mingw-w64-x86-64 g++-mingw-w64-i686 nsis curl zip python3 docker.io
echo "Pronto. Agora: ./build.sh --all"
