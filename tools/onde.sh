#!/usr/bin/env bash
# Onde esta definido? tools/onde.sh <nome> [...]  -> nome  arquivo:linha  (indice gerado por tools/mapa.py; prefixo com ^ ou texto parcial funciona)
cd "$(dirname "$0")/.."; { [ ! -f src/INDICE.tsv ] || [ -n "$(find src -newer src/INDICE.tsv \( -name "*.inc" -o -name "*.cpp" \) -print -quit)" ]; } && python3 tools/mapa.py >/dev/null
for n in "$@"; do grep -P "^${n}\t" src/INDICE.tsv || grep -i -P "^\w*${n}\w*\t" src/INDICE.tsv | head -15; done
