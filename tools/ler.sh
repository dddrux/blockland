#!/usr/bin/env bash
# Mostra SO o corpo de uma funcao/struct pelo nome: tools/ler.sh <nome> [max_linhas=80]   (usa src/INDICE.tsv; se varias, lista e pega a 1a)
cd "$(dirname "$0")/.."; { [ ! -f src/INDICE.tsv ] || [ -n "$(find src -newer src/INDICE.tsv \( -name "*.inc" -o -name "*.cpp" \) -print -quit)" ]; } && python3 tools/mapa.py >/dev/null
N="${1:?uso: tools/ler.sh <nome> [max_linhas]}"; M="${2:-80}"
mapfile -t L < <(grep -P "^${N}\t" src/INDICE.tsv | cut -f2)
[ ${#L[@]} -eq 0 ] && { echo "nao achei '$N' (tente tools/onde.sh $N)"; exit 1; }
[ ${#L[@]} -gt 1 ] && echo "// ${#L[@]} definicoes: ${L[*]} (mostrando a 1a)"
F="src/${L[0]%%:*}"; S="${L[0]##*:}"
awk -v s="$S" -v m="$M" 'NR>=s { print NR": "$0; n++; o=gsub(/\{/,"{"); c=gsub(/\}/,"}"); d+=o-c; if (o>0) ab=1; if ((ab && d<=0) || (!ab && $0 ~ /;[ \t]*(\/\/.*)?$/) || n>=m) exit }' "$F" | cut -c1-240
