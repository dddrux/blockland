#!/usr/bin/env bash
# Gera Blockland-x86_64.AppImage. Requer rede só para baixar o appimagetool (se não estiver no PATH).
set -e
cd "$(dirname "$0")"
./build.sh
rm -rf AppDir && mkdir -p AppDir/usr/bin AppDir/usr/lib
cp blockland AppDir/usr/bin/
cp -r assets AppDir/usr/bin/
# embute a SDL2 (libGL fica a cargo do sistema, pois depende do driver da placa de vídeo)
for l in $(ldd blockland | awk '/libSDL2/{print $3}'); do cp -L "$l" AppDir/usr/lib/; done
cat > AppDir/AppRun <<'EOT'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
cd "$HERE/usr/bin"
exec "$HERE/usr/bin/blockland" "$@"
EOT
chmod +x AppDir/AppRun
cat > AppDir/blockland.desktop <<'EOT'
[Desktop Entry]
Type=Application
Name=Blockland
Exec=blockland
Icon=blockland
Categories=Game;
EOT
cp blockland.png AppDir/blockland.png
if ! command -v appimagetool >/dev/null; then
  [ -f appimagetool ] || wget -q -O appimagetool https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
  chmod +x appimagetool
  TOOL=./appimagetool
else TOOL=appimagetool; fi
ARCH=x86_64 $TOOL --appimage-extract-and-run AppDir Blockland-x86_64.AppImage 2>/dev/null || ARCH=x86_64 $TOOL AppDir Blockland-x86_64.AppImage
echo "Pronto: Blockland-x86_64.AppImage"
