#!/usr/bin/env bash
# Compila o Blockland e entrega TUDO PRONTO em executaveis/ (sempre empacotado):
#   Windows -> setup.exe (instalador) | Linux e Raspberry Pi -> AppImage | Android -> APK
# Alvos (pode combinar): --android64 --android32 --windows64 --windows32 --linux64 --linux32 --rpi
#   grupos: --android (64+32) --windows (64+32) --linux (64+32) --all (os 7)
# Opcoes: --pack (aceito por compatibilidade: sempre empacota) | --release (APK release) | --fast (-O1, compila mais rapido) | --plain (log simples) | -h
# Exemplos:  ./build.sh --all
#            RPI_DOCKER=1 RPI_ARCH="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard" ./build.sh --rpi --pack
# Cada alvo tem seu script em build/ (linux.sh windows.sh android.sh rpi.sh); log unico em logs/build.log; dependencias: build/instalar-dependencias.sh
set -uo pipefail
cd "$(dirname "$0")/.."
ALL=(android64 android32 windows64 windows32 linux64 linux32 rpi); SEL=(); PLAIN=0
add() { local t; for t in "$@"; do [[ " ${SEL[*]:-} " == *" $t "* ]] || SEL+=("$t"); done; }
for a in "$@"; do case "$a" in
  --android64|--android32|--windows64|--windows32|--linux64|--linux32) add "${a#--}";;
  --rpi|--raspberry) add rpi;; --android) add android64 android32;; --windows|--win) add windows64 windows32;; --linux) add linux64 linux32;;
  --all) add "${ALL[@]}";; --pack) ;; --release) export BL_ANDROID=release;; --fast) export BL_OPT=-O1;; --plain) PLAIN=1;;
  -h|--help) sed -n 2,10p "$0"; exit 0;; *) echo "opcao desconhecida: $a  (veja ./build.sh --help)"; exit 1;;
esac; done
if [ ${#SEL[@]} = 0 ]; then
  if [ -t 0 ] && [ -t 1 ]; then
    echo "Escolha os alvos (varios: ex. 1 3 7; 8 = todos):"; i=1; for t in "${ALL[@]}"; do printf '  %d) %s\n' $i "$t"; i=$((i+1)); done; echo "  8) todos"
    read -r -p "Alvos [8]: " R; R="${R:-8}"; for n in ${R//,/ }; do if [ "$n" = 8 ]; then add "${ALL[@]}"; elif [ "$n" -ge 1 ] 2>/dev/null && [ "$n" -le 7 ]; then add "${ALL[$((n-1))]}"; fi; done
  else add "${ALL[@]}"; fi
fi
[ ${#SEL[@]} -gt 0 ] || { echo "nenhum alvo"; exit 1; }
mkdir -p logs executaveis; : > logs/build.log; TIMES=logs/.build-times
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ] && [ $PLAIN = 0 ] && [ "${TERM:-dumb}" != dumb ]; then FANCY=1; G=$'\e[32m'; R=$'\e[31m'; D=$'\e[2m'; Z=$'\e[0m'; else FANCY=0; G=; R=; D=; Z=; fi
fmt() { printf '%dm%02ds' $(($1 / 60)) $(($1 % 60)); }
nome() { case "$1" in android64) echo "Android 64 bits (APK)";; android32) echo "Android 32 bits (APK)";; windows64) echo "Windows 64 bits (setup.exe)";; windows32) echo "Windows 32 bits (setup.exe)";; linux64) echo "Linux 64 bits (AppImage)";; linux32) echo "Linux 32 bits (AppImage)";; rpi) echo "Raspberry Pi 32 bits (AppImage)";; esac; }
cmd() { case "$1" in android*) CMD=(build/android.sh "${1#android}");; windows*) CMD=(build/windows.sh "${1#windows}");; linux*) CMD=(build/linux.sh "${1#linux}");; rpi) CMD=(build/rpi.sh);; esac; }
echo; printf ' Blockland %s - %d alvo(s) -> executaveis/\n\n' "$(sed -n 's/^#define BLOCKLAND_VERSION "\(.*\)".*/\1/p' src/main.cpp | head -1)" ${#SEL[@]}
declare -A RES DUR; FAIL=0; T0=$SECONDS; n=0
for t in "${SEL[@]}"; do
  n=$((n+1)); cmd "$t"; log="logs/build.log"; echo "== ${CMD[*]} ==" >> "$log"; t1=$SECONDS
  last="$(grep "^$t " "$TIMES" 2>/dev/null | tail -1 | cut -d' ' -f2)"
  if [ $FANCY = 1 ]; then
    "${CMD[@]}" >> "$log" 2>&1 & pid=$!; sp=('|' '/' '-' '\'); f=0
    while kill -0 $pid 2>/dev/null; do printf '\r\e[K [%d/%d] %s %s %s%s%s' $n ${#SEL[@]} "${sp[$((f%4))]}" "$(nome "$t")" "$D" "$(fmt $((SECONDS-t1)))${last:+ / ~$(fmt $last)}" "$Z"; f=$((f+1)); sleep 0.2; done
    wait $pid; rc=$?; printf '\r\e[K'
  else printf ' [%d/%d] %s ... ' $n ${#SEL[@]} "$(nome "$t")"; "${CMD[@]}" >> "$log" 2>&1; rc=$?; fi
  DUR[$t]=$((SECONDS-t1))
  if [ $rc = 0 ]; then RES[$t]=ok; { grep -v "^$t " "$TIMES" 2>/dev/null; echo "$t ${DUR[$t]}"; } > "$TIMES.tmp" && mv "$TIMES.tmp" "$TIMES"; [ $FANCY = 1 ] && printf ' %s✔%s %s %s(%s)%s\n' "$G" "$Z" "$(nome "$t")" "$D" "$(fmt ${DUR[$t]})" "$Z" || echo "ok ($(fmt ${DUR[$t]}))"
  else RES[$t]=fail; FAIL=$((FAIL+1)); [ $FANCY = 1 ] && printf ' %s✘%s %s %s(%s)%s\n' "$R" "$Z" "$(nome "$t")" "$D" "$(fmt ${DUR[$t]})" "$Z" || echo "FALHOU ($(fmt ${DUR[$t]}))"
        echo "     ultimas linhas de $log:"; tail -n 6 "$log" | cut -c1-160 | sed 's/^/       /'; fi
done
echo; echo " Arquivos em executaveis/:"; ls -1sh executaveis 2>/dev/null | sed '1d;s/^/   /'
if [ $FAIL = 0 ]; then printf '\n %sTudo pronto%s em %s\n\n' "$G" "$Z" "$(fmt $((SECONDS-T0)))"; else printf '\n %s%d de %d alvo(s) falharam%s (%s)\n\n' "$R" $FAIL ${#SEL[@]} "$Z" "$(fmt $((SECONDS-T0)))"; fi
[ $FAIL = 0 ]
