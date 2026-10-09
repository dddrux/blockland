#!/usr/bin/env bash
# Verificacao SEM SDL2-dev: (1) sintaxe/tipos do jogo inteiro com headers de stub, (2) testes de logica headless.
# Os stubs (tests/stubs) so DECLARAM a API do SDL/GL; nao servem para rodar o jogo, so para checar e testar a logica.
set -euo pipefail
cd "$(dirname "$0")"
W="-Wall -Wno-unused -Wno-sign-compare -Wno-misleading-indentation -Wno-parentheses -Wno-enum-compare"
echo "== sintaxe =="; g++ -std=c++17 -fsyntax-only $W -Istubs ../src/main.cpp && echo OK
python3 -m http.server 18080 --directory site >/dev/null 2>&1 & SRV=$!; trap 'kill $SRV 2>/dev/null' EXIT; sleep 1   # servidor local para test_web
for t in test_*.cpp; do
  n="${t%.cpp}"; echo "== $n =="
  g++ -std=c++17 -O1 -Istubs -Wno-narrowing "$t" -o "/tmp/$n" -Wl,--unresolved-symbols=ignore-all -pthread
  "/tmp/$n"
done
