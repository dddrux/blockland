#!/usr/bin/env bash
# Cria um modulo novo: tools/novo_modulo.sh <nome>   (nome: minusculo, letras/numeros/_)
# Gera src/modules/<nome>/<nome>.inc a partir do molde (src/modules/exemplo) e liga em src/modules/modules_list.inc.
set -euo pipefail
cd "$(dirname "$0")/.."
N="${1:-}"; [[ "$N" =~ ^[a-z][a-z0-9_]*$ ]] || { echo "uso: tools/novo_modulo.sh <nome>  (minusculo, ex.: meu_mod)"; exit 1; }
D="src/modules/$N"; L="src/modules/modules_list.inc"
[ -e "$D" ] && { echo "ja existe: $D"; exit 1; }
mkdir -p "$D"
{ echo "// ---------- modulo ${N} (criado por tools/novo_modulo.sh; APIs em src/modules/api.inc) ----------"
  echo "// Namespace proprio mod_${N}. Ganchos opcionais (nullptr). Prefixo proprio em comandos/arquivos (${N}_...)."
  tail -n +5 src/modules/exemplo/exemplo.inc | sed "s/exemplo/${N}/g"; } > "$D/$N.inc"
echo "#include \"$N/$N.inc\"   // $N" >> "$L"
echo "ok: $D/$N.inc + linha em $L"
