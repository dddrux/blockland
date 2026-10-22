#!/usr/bin/env bash
# Android 64 bits (arm64-v8a) ou 32 bits (armeabi-v7a) -> executaveis/Blockland-android-64bits.apk | Blockland-android-32bits.apk
# Uso: build/android.sh 64|32 [release]      (BL_ANDROID=release tambem; padrao = APK debug, instalavel direto)
# Requer: JDK 17, Android SDK (ANDROID_HOME) com NDK 26 + CMake 3.22 (o script tenta instalar via sdkmanager), git e rede na 1a vez.
# OpenGL: por padrao OpenGL ES NATIVO (src/glesnative.inc). Caminho antigo (gl4es): BL_GL4ES=1.
# As alteracoes de manifesto/recursos do projeto ficam em data/android/ (aplicadas por data/android/apply.py); a pasta android/ e so o projeto gerado.
set -euo pipefail
source "$(dirname "$0")/lib.sh"
B="${1:-64}"; MODE="${BL_ANDROID:-debug}"; [ "${2:-}" = release ] && MODE=release
case "$B" in 64) ABI=arm64-v8a;; 32) ABI=armeabi-v7a;; *) die "uso: android.sh 64|32 [release]";; esac
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"; unset ANDROID_SDK_ROOT   # evita conflito do Gradle (ANDROID_HOME != ANDROID_SDK_ROOT)
[ -n "$SDK" ] && [ -d "$SDK" ] || die "defina ANDROID_HOME apontando para o Android SDK"
export ANDROID_HOME="$SDK"
SM="$SDK/cmdline-tools/latest/bin/sdkmanager"
[ -x "$SM" ] && yes | "$SM" --install "ndk;26.1.10909125" "cmake;3.22.1" "platforms;android-34" "build-tools;34.0.0" >/dev/null || true
[ -x ./android/setup.sh ] || [ -f ./android/setup.sh ] || die "android/setup.sh nao existe (a pasta android/ do projeto Gradle e necessaria so para buildar o APK)"
./android/setup.sh
python3 data/android/apply.py    # manifesto/recursos vindos de data/android (nao de android/)
# assets do jogo -> APK (+ manifesto para extrair no 1o uso; carimbo = hash do CONTEUDO dos assets)
A=android/app/src/main/assets; rm -rf "$A"; mkdir -p "$A"; cp -r assets "$A/assets"
STAMP="$(find assets -type f -exec md5sum {} + | sort -k2 | md5sum | cut -c1-12)"
{ echo "$STAMP"; (cd "$A" && find assets -type f | sort); } > "$A/bl_manifest.txt"
python3 android/make-icons.py 2>/dev/null || echo "aviso: sem Pillow, usando icones ja gerados (pip install pillow para atualizar)"
cd android
[ -x ./gradlew ] || { command -v gradle >/dev/null && gradle wrapper --gradle-version 8.7 || die "instale o gradle (ou coloque um gradlew em android/)"; }
T=$([ "$MODE" = release ] && echo assembleRelease || echo assembleDebug)
./gradlew "$T" -PblAbis="$ABI"
APK="$(ls -t app/build/outputs/apk/$MODE/*.apk | head -1)"; LIST="$(unzip -l "$APK" || true)"; MISS=0; NEED="libSDL2.so libmain.so"; [ "${BL_GL4ES:-0}" = 1 ] && NEED="libGL.so $NEED"
for L in $NEED; do case "$LIST" in *"/$L"*) ;; *) echo "FALTA no APK: $L"; MISS=1;; esac; done
[ $MISS = 0 ] || { echo "$LIST" | grep "lib/" || echo "(nenhuma lib)"; die "biblioteca fora do APK (veja a lista acima)"; }
cd "$ROOT"; cp -f "android/$APK" "$OUT/Blockland-android-${B}bits.apk"; echo "OK -> executaveis/Blockland-android-${B}bits.apk   (adb install -r)"
