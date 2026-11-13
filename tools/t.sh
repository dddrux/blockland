#!/usr/bin/env bash
# Verificacao com saida CURTA. tools/t.sh syntax | tools/t.sh <teste> [<teste>...]   (ex.: tools/t.sh redstone  -> tests/test_redstone.cpp)
# syntax: tipos/sintaxe do jogo todo (stubs). teste: compila, roda e mostra so FALHOU/erros/resumo.
cd "$(dirname "$0")/../tests" || exit 1
W="-Wall -Wno-unused -Wno-sign-compare -Wno-misleading-indentation -Wno-parentheses -Wno-enum-compare"
rc=0
for a in "$@"; do
  if [ "$a" = syntax ]; then
    out=$(g++ -std=c++17 -fsyntax-only $W -Istubs ../src/main.cpp 2>&1); e=$(echo "$out" | grep -c -E "error"); echo "$out" | grep -E "error" | head -20
    [ "$e" = 0 ] && echo "syntax: OK" || { echo "syntax: $e erro(s)"; rc=1; }
  else
    n="test_${a#test_}"; n="${n%.cpp}"; [ -f "$n.cpp" ] || { echo "sem $n.cpp"; rc=1; continue; }
    out=$(g++ -std=c++17 -O1 -Istubs -Wno-narrowing "$n.cpp" -o "/tmp/$n" -Wl,--unresolved-symbols=ignore-all -pthread 2>&1); [ -n "$(echo "$out" | grep -E 'error')" ] && { echo "$n: ERRO de compilacao"; echo "$out" | grep -E "error" | head -15; rc=1; continue; }
    r=$("/tmp/$n" 2>&1); s=$?; echo "$r" | grep -E "FALHOU|FAIL|Segmentation|verifica|passou|falhas" | tail -15; [ $s -ne 0 ] && { echo "$n: FALHA (exit $s)"; rc=1; } || echo "$n: OK"
  fi
done
exit $rc
