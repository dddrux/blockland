#!/bin/sh
# Converte todo .ogg/.mp3 de assets/sounds (ou da pasta dada) em .wav ADPCM (o formato que o jogo le). Uso: tools/ogg2wav.sh [pasta]
D="${1:-assets/sounds}"
find "$D" -type f \( -name '*.ogg' -o -name '*.mp3' \) | while read -r f; do
  o="${f%.*}.wav"; [ -f "$o" ] && continue
  ffmpeg -nostdin -v error -y -i "$f" -ac 2 -ar 44100 -c:a adpcm_ima_wav "$o" && echo "ok: $o"
done
