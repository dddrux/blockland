# Funcoes comuns dos scripts de build (use: source "$(dirname "$0")/lib.sh"). Tudo que ajuda a buildar mora em build/.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; cd "$ROOT"
OUT="$ROOT/executaveis"      # ONDE FICAM OS RESULTADOS (setup.exe, AppImage, apk)
WORK="$ROOT/build/work"      # temporarios (binarios intermediarios, AppDir, stage do instalador): pode apagar
CACHE="$ROOT/build/cache"    # downloads (appimagetool, runtimes, SDL2 mingw): baixados sozinhos 1 vez
mkdir -p "$OUT" "$WORK" "$CACHE" logs
VER="$(sed -n 's/^#define BLOCKLAND_VERSION "v\{0,1\}\([0-9.]*\)[a-z]*".*/\1/p' src/main.cpp | head -1)"; VER="${VER:-1.0.0}"
SDLV_MINGW="${SDLV_MINGW:-2.30.9}"
die() { echo "ERRO: $*" >&2; exit 1; }
fetch() {   # fetch URL DESTINO
  [ -s "$2" ] && return 0
  { command -v curl >/dev/null && curl -fL -o "$2.part" "$1"; } || { command -v wget >/dev/null && wget -q -O "$2.part" "$1"; } || { rm -f "$2.part"; die "sem rede para baixar $1 (salve em $2)"; }
  mv "$2.part" "$2"
}
container_engine() { local e; for e in docker podman; do command -v $e >/dev/null && $e info >/dev/null 2>&1 && { echo $e; return; }; done; for e in docker podman; do command -v $e >/dev/null && { echo $e; return; }; done;  return 0; }
# run_in_image IMAGEM "dockerfile" comando args... -> roda o comando dentro de um container Debian (imagem criada 1 vez)
run_in_image() {
  local img=$1 df=$2; shift 2; local eng u; eng="$(container_engine)"
  [ -n "$eng" ] || die "sem toolchain para esta plataforma e sem docker/podman (Arch: sudo pacman -S docker && sudo systemctl enable --now docker)"
  if ! $eng image inspect "$img" >/dev/null 2>&1; then echo "Criando imagem $img (so na 1a vez)..."; printf '%s\n' "$df" | $eng build -q -t "$img" - >/dev/null || die "falha ao criar a imagem $img"; fi
  u="--user $(id -u):$(id -g)"; [ "$eng" = podman ] && u="--userns=keep-id"
  $eng run --rm $u -e HOME=/tmp -e IN_IMAGE=1 -e EXTRA="${EXTRA:-}" -e RPI_ARCH="${RPI_ARCH:-}" -e BL_OPT="${BL_OPT:-}" -v "$ROOT":/w -w /w "$img" "$@"
}
# make_appimage ARCH BINARIO NOMEBIN SOLIBDIR SAIDA   (ARCH: x86_64 | i686 | armhf) -> AppDir em build/work + AppImage
make_appimage() {
  local arch=$1 bin=$2 name=$3 solib=$4 dest=$5 AD="$WORK/AppDir-$1" tool rt=() l
  rm -rf "$AD"; mkdir -p "$AD/usr/bin" "$AD/usr/lib"
  cp "$bin" "$AD/usr/bin/$name"; cp -r assets "$AD/usr/bin/assets"
  for l in "$solib"/libSDL2-2.0.so.0*; do [ -e "$l" ] && cp -L "$l" "$AD/usr/lib/"; done   # embute a SDL2 (libGL fica com o sistema: depende do driver)
  ls "$AD"/usr/lib/libSDL2* >/dev/null 2>&1 || echo "aviso: libSDL2 nao embutida; o AppImage usara a SDL2 do sistema (libsdl2-2.0-0)"
  cat > "$AD/AppRun" <<EOT
#!/bin/sh
HERE="\$(dirname "\$(readlink -f "\$0")")"
export LD_LIBRARY_PATH="\$HERE/usr/lib:\$LD_LIBRARY_PATH"
# o AppImage e SOMENTE LEITURA: saves/, logs/, options.cfg... vao para uma pasta gravavel (BLOCKLAND_HOME ou ~/.local/share/Blockland); assets = link para dentro do AppImage
DATA="\${BLOCKLAND_HOME:-\${XDG_DATA_HOME:-\$HOME/.local/share}/Blockland}"
if mkdir -p "\$DATA/logs" "\$DATA/saves" 2>/dev/null; then
  { [ -d "\$DATA/assets" ] && [ ! -L "\$DATA/assets" ]; } || ln -sfn "\$HERE/usr/bin/assets" "\$DATA/assets"
  cd "\$DATA" || cd "\$HERE/usr/bin"
else cd "\$HERE/usr/bin"; fi
exec "\$HERE/usr/bin/$name" "\$@"
EOT
  chmod +x "$AD/AppRun"
  printf '[Desktop Entry]\nType=Application\nName=Blockland\nExec=%s\nIcon=blockland\nCategories=Game;\n' "$name" > "$AD/blockland.desktop"
  cp blockland.png "$AD/blockland.png"
  if command -v appimagetool >/dev/null; then tool=appimagetool; else
    fetch "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" "$CACHE/appimagetool"; chmod +x "$CACHE/appimagetool"; tool="$CACHE/appimagetool"; fi
  if [ "$arch" != x86_64 ]; then   # runtime da arquitetura de destino (o appimagetool roda no PC x86_64)
    fetch "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-$arch" "$CACHE/runtime-$arch"; rt=(--runtime-file "$CACHE/runtime-$arch"); fi
  rm -f "$dest"
  ARCH=$arch $tool --appimage-extract-and-run ${rt[@]+"${rt[@]}"} "$AD" "$dest" >> logs/appimage-$arch.log 2>&1 \
    || ARCH=$arch $tool ${rt[@]+"${rt[@]}"} "$AD" "$dest" >> logs/appimage-$arch.log 2>&1 || die "falha ao gerar o AppImage $arch (veja logs/appimage-$arch.log)"
  chmod +x "$dest"; echo "OK -> ${dest#$ROOT/}"
}
