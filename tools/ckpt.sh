#!/usr/bin/env bash
# Checkpoint: tools/ckpt.sh <nome>  -> zip SO dos arquivos alterados desde o baseline (leve), copia p/ /mnt/user-data/outputs
cd "$(dirname "$0")/.." || exit 1
N="${1:?uso: tools/ckpt.sh <nome>}"; B=/home/claude/.baseline
L=$(find src tools tests conversor doc mods PENDENCIAS.md -type f -newer "$B" 2>/dev/null | grep -v -E '\.pyc$|__pycache__')
[ -z "$L" ] && { echo "nada alterado"; exit 0; }
rm -f "ckpt-$N.zip"; echo "$L" | zip -q "ckpt-$N.zip" -@ && unzip -l "ckpt-$N.zip" | tail -1
mkdir -p /mnt/user-data/outputs && cp "ckpt-$N.zip" /mnt/user-data/outputs/ && echo "ckpt-$N.zip -> outputs"
