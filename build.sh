#!/usr/bin/env bash
# Compila o Blockland. Escolha as plataformas com flags (pode combinar varias):
#   ./build.sh --linux | --windows | --android | --rpi | --all
# Sem plataforma: compila para o sistema atual (Linux ou MSYS2/MinGW64 no Windows).
# Opcoes:
#   --pack            tambem gera o pacote em dist/ (linux/windows/rpi: exe + assets; android: copia o APK)
#   --debug           sem otimizacao (-O0 -g); no Android = APK debug (padrao)
#   --release         so Android: APK release
#   --abis=A,B        so Android: ABIs (padrao arm64-v8a; ex.: --abis=arm64-v8a,armeabi-v7a)
#   --console         so Windows: mantem a janela de console
#   --installer       so Windows: gera dist/setup.exe (Inno Setup/NSIS; veja installer/README.md)
#   --plain           sem cores/animacao (log simples; automatico se nao for terminal ou NO_COLOR)
#   -h, --help        esta ajuda
# Atalhos: TARGET=windows ./build.sh   |   scripts/build-tudo-linux.sh (instala dependencias e gera tudo)
# Cada plataforma tem o seu script em scripts/ (build-linux.sh, build-windows.sh, build-rpi.sh, build-android.sh).
set -uo pipefail
cd "$(dirname "$0")"
T_LINUX=0; T_WIN=0; T_AND=0; T_RPI=0; PACK=0; DEBUG=0; RELEASE=0; CONSOLE=0; INST=0; ABIS="arm64-v8a"; PLAIN=0
case "${TARGET:-}" in windows) T_WIN=1; INST=1;; linux) T_LINUX=1;; android) T_AND=1;; rpi) T_RPI=1;; esac   # compatibilidade com TARGET=...
for a in "$@"; do case "$a" in
  --linux) T_LINUX=1;; --windows|--win) T_WIN=1;; --android) T_AND=1;; --rpi|--raspberry) T_RPI=1;;
  --all) T_LINUX=1; T_WIN=1; T_AND=1; T_RPI=1;;
  --pack) PACK=1;; --debug) DEBUG=1;; --release) RELEASE=1;; --console) CONSOLE=1;; --installer) INST=1; T_WIN=1;; --plain) PLAIN=1;;
  --abis=*) ABIS="${a#--abis=}";;
  -h|--help) sed -n 2,18p "$0"; exit 0;;
  *) echo "opcao desconhecida: $a  (veja ./build.sh --help)"; exit 1;;
esac; done
if [ $DEBUG = 1 ] && [ $RELEASE = 1 ]; then echo "ERRO: --debug e --release juntos"; exit 1; fi
if [ $((T_LINUX + T_WIN + T_AND + T_RPI)) = 0 ]; then   # nenhuma plataforma: a do sistema atual
  case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) T_WIN=1;; *) T_LINUX=1;; esac
fi

# ---------------- interface: barra de progresso geral ----------------
# Compilar e um passo unico e longo (g++ em main.cpp), entao a % e ESTIMADA: o tempo decorrido contra a duracao da
# ultima compilacao daquela plataforma (guardada em logs/.build-times; na 1a vez usa um chute). Chega a 100% so quando termina.
mkdir -p logs; TIMES=logs/.build-times
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ] && [ $PLAIN = 0 ] && [ "${TERM:-dumb}" != dumb ]; then FANCY=1; else FANCY=0; fi
case "${LC_ALL:-${LC_CTYPE:-${LANG:-}}}" in *UTF-8*|*utf8*|*UTF8*) FULL="█"; EMPTY="░"; SPIN=(⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏); OKM="✔"; BADM="✘";; *) FULL="#"; EMPTY="-"; SPIN=('|' '/' '-' '\'); OKM="OK"; BADM="XX";; esac
if [ $FANCY = 1 ]; then C0=$'\e[0m'; CB=$'\e[1m'; CD=$'\e[2m'; CG=$'\e[32m'; CR=$'\e[31m'; CC=$'\e[36m'; CY=$'\e[33m'; else C0=; CB=; CD=; CG=; CR=; CC=; CY=; fi
bar() { local pct=$1 w=${2:-28} i f out=""; f=$((pct * w / 100)); for ((i = 0; i < w; i++)); do if [ $i -lt $f ]; then out+="$FULL"; else out+="$EMPTY"; fi; done; printf '%s' "$out"; }
guess() { case "$1" in linux) echo 45;; windows) echo 60;; rpi) echo 90;; android) echo 240;; *) echo 60;; esac; }
lastdur() { local d; d=$(grep "^$1 " "$TIMES" 2>/dev/null | tail -1 | cut -d' ' -f2); echo "${d:-$(guess "$1")}"; }
savedur() { { grep -v "^$1 " "$TIMES" 2>/dev/null; echo "$1 $2"; } > "$TIMES.tmp" && mv "$TIMES.tmp" "$TIMES"; }
fmt() { printf '%dm%02ds' $(($1 / 60)) $(($1 % 60)); }
human() { [ -e "$1" ] && du -h "$1" 2>/dev/null | cut -f1; }

TARGETS=(); [ $T_LINUX = 1 ] && TARGETS+=(linux); [ $T_WIN = 1 ] && TARGETS+=(windows); [ $T_RPI = 1 ] && TARGETS+=(rpi); [ $T_AND = 1 ] && TARGETS+=(android)
declare -A NOME=([linux]="Linux" [windows]="Windows" [rpi]="Raspberry Pi" [android]="Android")
declare -A EXPD; TOTAL=0; for t in "${TARGETS[@]}"; do EXPD[$t]=$(lastdur "$t"); TOTAL=$((TOTAL + EXPD[$t])); done
DONE_T=0; FAIL=(); OK=(); declare -A RES DUR
cmd_for() {   # monta o comando de cada plataforma
  local A=()
  case "$1" in
    linux)   [ $PACK = 1 ] && A+=(--pack); [ $DEBUG = 1 ] && A+=(--debug); CMD=(scripts/build-linux.sh ${A[@]+"${A[@]}"});;
    windows) [ $PACK = 1 ] && A+=(--pack); [ $DEBUG = 1 ] && A+=(--debug); [ $CONSOLE = 1 ] && A+=(--console); [ $INST = 1 ] && A+=(--installer); CMD=(scripts/build-windows.sh ${A[@]+"${A[@]}"});;
    rpi)     [ $PACK = 1 ] && A+=(--pack); CMD=(scripts/build-rpi.sh ${A[@]+"${A[@]}"});;
    android) local M=debug; [ $RELEASE = 1 ] && M=release; CMD=(scripts/build-android.sh "$M" "$ABIS");;
  esac
}
draw() {   # $1 = % geral, $2 = rotulo, $3 = tempo
  printf '\r\e[K %s%s%s %s%3d%%%s  %s%s%s  %s  %s%s%s' "$CC" "$(bar "$1")" "$C0" "$CB" "$1" "$C0" "$CY" "${SPIN[$((FR % ${#SPIN[@]}))]}" "$C0" "$2" "$CD" "$3" "$C0"
}
run_target() {
  local t=$1 log="logs/build-$1.log" exp=${EXPD[$1]} t0 el pct tp base; cmd_for "$t"; t0=$SECONDS; FR=0
  echo "== ${CMD[*]} ==" > "$log"
  if [ $FANCY = 0 ]; then
    printf '[%3d%%] %s ... ' $((DONE_T * 100 / TOTAL)) "${NOME[$t]}"
    if "${CMD[@]}" >> "$log" 2>&1; then rc=0; else rc=$?; fi; el=$((SECONDS - t0))
    [ $rc = 0 ] && echo "ok ($(fmt $el))" || echo "FALHOU ($(fmt $el))"
    return $rc
  fi
  "${CMD[@]}" >> "$log" 2>&1 & local pid=$!
  while kill -0 $pid 2>/dev/null; do
    el=$((SECONDS - t0)); tp=$((el * 100 / exp)); [ $tp -gt 95 ] && tp=95
    pct=$(((DONE_T * 100 + exp * tp) / TOTAL)); [ $pct -gt 99 ] && pct=99
    draw $pct "Compilando ${NOME[$t]}" "$(fmt $el)  (estimado)"; FR=$((FR + 1)); sleep 0.12
  done
  if wait $pid; then rc=0; else rc=$?; fi
  return $rc
}
echo; printf ' %sBlockland%s  %s·%s  %d plataforma(s): %s\n\n' "$CB" "$C0" "$CD" "$C0" ${#TARGETS[@]} "${TARGETS[*]}"
T_ALL=$SECONDS
for t in "${TARGETS[@]}"; do
  t1=$SECONDS
  if run_target "$t"; then RES[$t]=ok; OK+=("$t"); savedur "$t" $((SECONDS - t1 > 3 ? SECONDS - t1 : 3)); else RES[$t]=fail; FAIL+=("$t"); fi
  DUR[$t]=$((SECONDS - t1)); DONE_T=$((DONE_T + EXPD[$t]))
  if [ $FANCY = 1 ]; then
    pct=$((DONE_T * 100 / TOTAL)); [ ${#FAIL[@]} -gt 0 ] && [ "${RES[$t]}" = fail ] && pct=$(((DONE_T - EXPD[$t]) * 100 / TOTAL))
    printf '\r\e[K'; if [ "${RES[$t]}" = ok ]; then printf ' %s%s%s %s%3d%%%s  %s%s%s %s  %s(%s)%s\n' "$CG" "$(bar $pct)" "$C0" "$CB" $pct "$C0" "$CG" "$OKM" "$C0" "${NOME[$t]}" "$CD" "$(fmt ${DUR[$t]})" "$C0"
    else printf ' %s%s%s %s%3d%%%s  %s%s%s %s  %s(%s)%s\n' "$CR" "$(bar $pct)" "$C0" "$CB" $pct "$C0" "$CR" "$BADM" "$C0" "${NOME[$t]}" "$CD" "$(fmt ${DUR[$t]})" "$C0"; fi
  fi
  if [ "${RES[$t]}" = fail ]; then echo "${CR}   ultimas linhas de logs/build-$t.log:${C0}"; tail -n 6 "logs/build-$t.log" | cut -c1-160 | sed 's/^/     /'; fi
done
if [ $T_AND = 1 ] && [ "${RES[android]:-}" = ok ] && [ $PACK = 1 ]; then mkdir -p dist; cp -f blockland-android-*.apk dist/ 2>/dev/null && echo " APK copiado para dist/"; fi
# ---------------- resumo ----------------
echo; printf ' %s%s%s\n' "$CD" "────────────────────────────────────────────────────" "$C0"
for t in "${TARGETS[@]}"; do
  case "$t" in linux) F=blockland;; windows) F=blockland.exe;; rpi) F=blockland-rpi;; android) F=$(ls -t blockland-android-*.apk 2>/dev/null | head -1);; esac
  if [ "${RES[$t]}" = ok ]; then printf ' %s%s%s %-14s %s%-8s%s %s\n' "$CG" "$OKM" "$C0" "${NOME[$t]}" "$CD" "$(fmt ${DUR[$t]})" "$C0" "${F:+-> $F $(human "$F")}"
  else printf ' %s%s%s %-14s %s%-8s%s %s\n' "$CR" "$BADM" "$C0" "${NOME[$t]}" "$CD" "$(fmt ${DUR[$t]})" "$C0" "(veja logs/build-$t.log)"; fi
done
printf ' %s%s%s\n' "$CD" "────────────────────────────────────────────────────" "$C0"
if [ ${#FAIL[@]} = 0 ]; then printf ' %s%s100%%  Tudo pronto%s em %s\n\n' "$CG" "$CB" "$C0" "$(fmt $((SECONDS - T_ALL)))"; else printf ' %s%d de %d plataforma(s) falharam%s  (%s)\n\n' "$CR" ${#FAIL[@]} ${#TARGETS[@]} "$C0" "$(fmt $((SECONDS - T_ALL)))"; fi
[ ${#FAIL[@]} = 0 ]
