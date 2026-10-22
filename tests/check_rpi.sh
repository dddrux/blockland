#!/bin/sh
# patchRpi.1: o modo Raspberry (-DBL_RPI) tem de compilar e passar nos testes headless basicos
cd "$(dirname "$0")" || exit 1
g++ -std=c++17 -fsyntax-only -Wno-unused -Istubs -DBL_RPI ../src/main.cpp 2>&1 | grep -c error
for t in test_piston test_chest; do g++ -std=c++17 -O1 -Istubs -Wno-narrowing -DBL_RPI $t.cpp -o /tmp/rpi_$t -Wl,--unresolved-symbols=ignore-all -pthread 2>&1 | grep -c error; /tmp/rpi_$t 2>&1 | grep FALHOU; /tmp/rpi_$t 2>&1 | tail -1 | cut -c25-80; done
