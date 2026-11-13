# Funcoes comuns dos scripts de build (use: source "$(dirname "$0")/lib.sh"). Tudo que ajuda a buildar mora em build/.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; cd "$ROOT"
OUT="$ROOT/executaveis"      # ONDE FICAM OS RESULTADOS (setup.exe, AppImage, apk)
WORK="$ROOT/build/work"      # temporarios (binarios intermediarios, AppDir, stage do instalador): pode apagar
CACHE="$ROOT/build/cache"    # downloads (appimagetool, runtimes, SDL2 mingw): baixados sozinhos 1 vez
mkdir -p "$OUT" "$WORK" "$CACHE" logs
# patchJVM.10: BL_AOT="ironfurnaces-1.3.5 ..." compila junto o C++ gerado (mods/<pasta>/aot/*.cpp, ver tools/aot_mod.sh) e o jogo roda o ciclo do mod ao iniciar (log "aot: ..."). Sem BL_AOT nada muda.
if [ -n "${BL_AOT:-}" ]; then for _m in $BL_AOT; do [ -d "mods/$_m/aot" ] || { echo "ERRO: BL_AOT: sem mods/$_m/aot (rode tools/aot_mod.sh mods/$_m.jar)" >&2; exit 1; }; EXTRA="${EXTRA:-} $(ls mods/$_m/aot/*.cpp | tr '\n' ' ')"; done; EXTRA="${EXTRA:-} -Isrc/modules/jvm/aot"; export EXTRA; fi
VER="$(sed -n 's/^#define BLOCKLAND_VERSION "v\{0,1\}\([0-9.]*\)[a-z]*".*/\1/p' src/main.cpp | head -1)"; VER="${VER:-1.0.0}"
SDLV_MINGW="${SDLV_MINGW:-2.30.9}"
die() { echo "ERRO: $*" >&2; exit 1; }
# patchForge.1: UM build por vez. A limpeza (trap abaixo) apaga build/work/*: um 2o build.sh terminando no meio da compilacao do 1o apagava a pasta de saida do link
# ("nao foi possivel abrir o arquivo de saida .../build/work/linux64/blockland"). Agora o 2o falha logo, sem apagar nada. Docker (IN_IMAGE) e filhos do mesmo build (BL_LOCKED) nao travam.
if [ -z "${IN_IMAGE:-}" ] && [ -z "${BL_LOCKED:-}" ] && command -v flock >/dev/null 2>&1; then
  exec 9>"$ROOT/build/.lock"; flock -n 9 || { echo "ERRO: ja existe outro build em andamento (feche-o ou espere): ele apagaria build/work no fim" >&2; exit 1; }; export BL_LOCKED=1
fi
# Ao terminar (ok ou erro) apaga TODO temporario de build; so sobra executaveis/ (+ logs/ e build/cache de downloads). BL_KEEP=1 mantem (build incremental).
limpar_build() {
  [ -n "${IN_IMAGE:-}" ] && return 0; [ "${BL_KEEP:-0}" = 1 ] && return 0
  local r="$ROOT" w
  for w in "$r"/build/work/*; do [ "$w" = "$r/build/work/rpi-sysroot" ] || rm -rf "$w"; done
  rm -rf "$r/build/android/app/build" "$r/build/android/build" "$r/build/android/.gradle" "$r/build/android/app/.cxx" "$r/build/android/app/src/main/assets" "$r/build/android/app/src/main/cpp/lib" "$r/build/android/deps/gl4es/lib" "$r/.gradle" "$r/dist" "$r/blockland" "$r/blockland.exe"
  find "$r/build/android/deps" -type d \( -name build -o -name CMakeFiles \) -prune -exec rm -rf {} + 2>/dev/null
  find "$r" -maxdepth 2 -name '*.o' -not -path "$r/executaveis/*" -delete 2>/dev/null
  return 0
}
trap limpar_build EXIT
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
# patchLinux.2: GNOME novo removeu a chave xsettings 'antialiasing' e o libdecor-gtk/GTK do sistema aborta (SIGABRT) no SDL_Init; X11 (XWayland) e sem plugin libdecor evita isso
if [ -n "\$WAYLAND_DISPLAY" ]; then : "\${SDL_VIDEODRIVER:=wayland}"; export SDL_VIDEODRIVER; elif [ -n "\$DISPLAY" ]; then : "\${SDL_VIDEODRIVER:=x11}"; export SDL_VIDEODRIVER; fi   # patchWayland.1
export SDL_VIDEO_WAYLAND_ALLOW_LIBDECOR="\${SDL_VIDEO_WAYLAND_ALLOW_LIBDECOR:-0}" LIBDECOR_PLUGIN_DIR="\${LIBDECOR_PLUGIN_DIR:-/nonexistent}"
# o AppImage e SOMENTE LEITURA: saves/, logs/, options.cfg... vao para uma pasta gravavel (BLOCKLAND_HOME ou ~/.local/share/Blockland); assets = link para dentro do AppImage
# patchLinux.4: se o AppImage/.run esta em <projeto>/executaveis, logs/saves/crash-reports ficam na RAIZ do projeto (como antes)
SRC="\${APPIMAGE:-\$BL_RUN}"; PROOT=""
if [ -n "\$SRC" ]; then SD="\$(dirname "\$(readlink -f "\$SRC")")"; [ "\$(basename "\$SD")" = executaveis ] && PROOT="\$(dirname "\$SD")"; fi
DATA="\${BLOCKLAND_HOME:-\${PROOT:-\${XDG_DATA_HOME:-\$HOME/.local/share}/Blockland}}"
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
  ARCH=$arch "$tool" --appimage-extract-and-run ${rt[@]+"${rt[@]}"} "$AD" "$dest" >> logs/build.log 2>&1 \
    || ARCH=$arch "$tool" ${rt[@]+"${rt[@]}"} "$AD" "$dest" >> logs/build.log 2>&1 || die "falha ao gerar o AppImage $arch (veja logs/build.log)"
  chmod +x "$dest"; echo "OK -> ${dest#$ROOT/}"
  # patchLinux.3: alem do AppImage, gera um .run autoextraivel (NAO precisa de FUSE: o AppImage nao monta em alguns Ubuntu novos). Extrai 1x em ~/.cache/Blockland e roda o AppRun
  local run="${dest%.AppImage}.run" id; id="$(date +%s)"
  { printf '%s\n' '#!/bin/sh' "ID=$id" 'D="${XDG_CACHE_HOME:-$HOME/.cache}/Blockland/$ID"' \
      '[ -x "$D/AppRun" ] || { rm -rf "$D.tmp"; mkdir -p "$D.tmp" || exit 1; echo "Extraindo Blockland (so na 1a vez)..."; tail -n +LINHA "$0" | tar xzf - -C "$D.tmp" && rm -rf "$D" && mv "$D.tmp" "$D" || exit 1; }' \
      'BL_RUN="$(readlink -f "$0")"; export BL_RUN' 'exec "$D/AppRun" "$@"' 'exit 0'; } > "$run.hdr"
  local n; n=$(( $(wc -l < "$run.hdr") + 1 )); sed -i "s/LINHA/$n/" "$run.hdr"
  { cat "$run.hdr"; tar czf - -C "$AD" .; } > "$run" && rm -f "$run.hdr" && chmod +x "$run" && echo "OK -> ${run#$ROOT/} (sem FUSE)"
}
