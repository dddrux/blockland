#!/usr/bin/env bash
# Atalho que chama os geradores separados.   Uso: ./build-all.sh [linux|windows|android|all] [--pack] [debug|release (so android)]
#   ./build-linux.sh    -> ./blockland          ./build-windows.sh -> ./blockland.exe          ./build-android.sh [debug|release] [abis] -> APK
set -euo pipefail
cd "$(dirname "$0")"
what="${1:-all}"; shift || true
PACK=(); MODE=debug
for a in "$@"; do case "$a" in --pack) PACK=(--pack);; debug|release) MODE="$a";; esac; done
android() { ./build-android.sh "$MODE"; if [ ${#PACK[@]} -gt 0 ]; then mkdir -p dist; cp -f blockland-android-*.apk dist/ 2>/dev/null && echo "APK copiado para dist/"; fi; }
case "$what" in
  linux)   ./build-linux.sh ${PACK[@]+"${PACK[@]}"} ;;
  windows) ./build-windows.sh ${PACK[@]+"${PACK[@]}"} ;;
  android) android ;;
  all)     ./build-linux.sh ${PACK[@]+"${PACK[@]}"}; ./build-windows.sh ${PACK[@]+"${PACK[@]}"}; android ;;
  *) echo "uso: $0 [linux|windows|android|all] [--pack] [debug|release]"; exit 1 ;;
esac
