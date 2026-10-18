#!/usr/bin/env bash
# Gera o APK do Blockland para Android.   Uso: scripts/build-android.sh [debug|release] [abis]   ex.: scripts/build-android.sh release arm64-v8a,armeabi-v7a
# Requer: JDK 17, Android SDK (ANDROID_HOME) com NDK 26 + CMake 3.22 (o script tenta instalar via sdkmanager), git e rede na 1a vez.
# OpenGL: por padrao o Android usa OpenGL ES NATIVO (src/glesnative.inc, sem gl4es). Para o caminho antigo (gl4es) exporte BL_GL4ES=1.
# Controles touch ja vem LIGADOS (src/android.inc).
set -euo pipefail
cd "$(dirname "$0")/.."
MODE=debug; ABIS=arm64-v8a; POS=0
for a in "$@"; do case "$a" in --debug|debug) MODE=debug;; --release|release) MODE=release;; --abis=*) ABIS="${a#--abis=}";; -h|--help) sed -n 2,2p "$0"; exit 0;; --*) echo "opcao desconhecida: $a"; exit 1;; *) POS=$((POS+1)); [ $POS = 1 ] && ABIS="$a";; esac; done   # aceita "release arm64-v8a" (antigo) e "--release --abis=arm64-v8a"
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
# evita conflito do Gradle (ANDROID_HOME != ANDROID_SDK_ROOT): usa so ANDROID_HOME
unset ANDROID_SDK_ROOT
[ -n "$SDK" ] && [ -d "$SDK" ] || { echo "ERRO: defina ANDROID_HOME apontando para o Android SDK"; exit 1; }
export ANDROID_HOME="$SDK"
SM="$SDK/cmdline-tools/latest/bin/sdkmanager"
[ -x "$SM" ] && yes | "$SM" --install "ndk;26.1.10909125" "cmake;3.22.1" "platforms;android-34" "build-tools;34.0.0" >/dev/null || true
./android/setup.sh
# assets do jogo -> APK (+ manifesto para extrair no 1o uso; carimbo muda quando os assets ou o codigo mudam)
A=android/app/src/main/assets; rm -rf "$A"; mkdir -p "$A"; cp -r assets "$A/assets"
STAMP="$(find assets -type f -printf '%p %s\n' | sort | md5sum | cut -c1-12)"   # so muda quando os assets mudam (nao a cada recompilacao)
{ echo "$STAMP"; (cd "$A" && find assets -type f | sort); } > "$A/bl_manifest.txt"
python3 android/make-icons.py 2>/dev/null || echo "aviso: sem Pillow, usando icones ja gerados (pip install pillow para atualizar)"
cd android
[ -x ./gradlew ] || { command -v gradle >/dev/null && gradle wrapper --gradle-version 8.7 || { echo "ERRO: instale o gradle (ou coloque um gradlew em android/)"; exit 1; }; }
T=$([ "$MODE" = release ] && echo assembleRelease || echo assembleDebug)
./gradlew "$T" -PblAbis="$ABIS"
APK="$(ls -t app/build/outputs/apk/$MODE/*.apk | head -1)"
LIST="$(unzip -l "$APK" || true)"
MISS=0; NEED="libSDL2.so libmain.so"; [ "${BL_GL4ES:-0}" = 1 ] && NEED="libGL.so $NEED"
for L in $NEED; do case "$LIST" in *"/$L"*) ;; *) echo "FALTA no APK: $L"; MISS=1;; esac; done
if [ $MISS = 1 ]; then
  echo "--- libs dentro do APK:"; echo "$LIST" | grep "lib/" || echo "(nenhuma)"
  echo "--- libGL* gerados no disco:"; find . -name "libGL*" -not -path "*/deps/gl4es/src/*" 2>/dev/null
  echo "--- *.so/.a do gl4es:"; find deps/gl4es -name "*.so*" -o -name "*.a" 2>/dev/null
  echo "ERRO: biblioteca fora do APK (veja a lista acima e me mande)"; exit 1
fi
cp -f "$APK" ../blockland-android-$MODE.apk
echo "OK Android -> ./blockland-android-$MODE.apk   (adb install -r blockland-android-$MODE.apk)"
