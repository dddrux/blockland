#!/usr/bin/env bash
# Resumo CURTO das pendencias (nao leia PENDENCIAS.md inteiro): tools/pend.sh [filtro]   -> RETOMADA + abertas (110 col)
cd "$(dirname "$0")/.."; sed -n '/^# RETOMADA/,/^$/p' PENDENCIAS.md | cut -c1-200
grep -n -E '^- \[[ ~]\]' PENDENCIAS.md | grep -i -- "${1:-.}" | cut -c1-110
