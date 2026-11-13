#!/usr/bin/env bash
# Empacota o patch: tools/zipar.sh <nome> <arquivo>...   (caminhos relativos a raiz; so PENDENCIAS.md entra sozinho; sem PATCH.md)
# Roda a sintaxe antes; aborta se falhar ou se faltar arquivo. Gera patch-<nome>.zip (e copia para /mnt/user-data/outputs se existir).
cd "$(dirname "$0")/.."; N="${1:?uso: tools/zipar.sh <nome> <arquivo>...}"; shift

for f in "$@"; do [ -e "$f" ] || { echo "nao existe: $f"; exit 1; }; done
tools/t.sh syntax | tail -3 | grep -q "syntax: OK" || { echo "sintaxe quebrada: nao zipou"; exit 1; }
rm -f "patch-$N.zip"; zip -q -r "patch-$N.zip" "$@" PENDENCIAS.md && unzip -l "patch-$N.zip" | tail -1
[ -d /mnt/user-data/outputs ] && cp "patch-$N.zip" /mnt/user-data/outputs/ && echo "copiado para outputs"
