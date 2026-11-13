#!/bin/sh
# patchCtl.1: uso: tools/ctl.sh DIR "start meumundo" "wait 120" "cmd /tp 8 70 8" "shot a"   (jogo aberto com --control DIR; espera done.txt)
d=$1; shift; rm -f "$d/done.txt"; printf '%s\n' "$@" > "$d/cmd.tmp" && mv "$d/cmd.tmp" "$d/cmd.txt"
i=0; while [ ! -f "$d/done.txt" ] && [ $i -lt 600 ]; do sleep 0.2; i=$((i+1)); done; [ -f "$d/done.txt" ] && echo ok || { echo timeout; exit 1; }
