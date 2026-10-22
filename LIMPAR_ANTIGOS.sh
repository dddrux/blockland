#!/usr/bin/env bash
# patchBuild.1: apaga o que a reorganizacao substituiu (rode uma vez na raiz do projeto, depois de extrair o zip).
cd "$(dirname "$0")"
rm -rf scripts installer tools/android-galeria tools/android-voice AppDir-rpi appimagetool runtime-armhf build-rpi.sh rpi-simulation.sh SDL2.dll libwinpthread-1.dll crash-reports SONS_FALTANDO.md
echo "ok: projeto limpo (apps antigos removidos; o que vale agora esta em build/, data/ e executaveis/)"
