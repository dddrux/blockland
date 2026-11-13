#!/usr/bin/env bash
# patchJVM.1: bytecode -> C++ (jvmlift.py) -> g++ -> roda e compara com o java real. Uso: tests/jvm/aot.sh [Classe...]  (padrao: T)
set -e; R="$(cd "$(dirname "$0")/../.." && pwd)"; rm -rf /tmp/aot; mkdir -p /tmp/aot/src/demo /tmp/aot/cls /tmp/aot/gen
cp "$R/tests/jvm/T.java" "$R/tests/jvm/T2.java" "$R/tests/jvm/T3.java" /tmp/aot/src/demo/
java -m jdk.compiler/com.sun.tools.javac.Main -source 8 -target 8 -nowarn -d /tmp/aot/cls /tmp/aot/src/demo/*.java 2>&1 | grep -i error || true
python3 "$R/conversor/modconv/jvmlift.py" /tmp/aot/cls --out /tmp/aot/gen --calls /tmp/aot/calls.json
printf '#include "jvmrt.h"\nint main(int c,char**v){return rt::runMain(v[1]);}\n' > /tmp/aot/gen/main.cpp
g++ -std=c++17 -O1 -w -I"$R/src/modules/jvm/aot" /tmp/aot/gen/*.cpp -o /tmp/aot/run
for C in ${@:-T}; do java -cp /tmp/aot/cls demo.$C > /tmp/aot/exp_$C.txt; /tmp/aot/run demo/$C > /tmp/aot/got_$C.txt 2>/tmp/aot/err_$C.txt || true
  diff /tmp/aot/exp_$C.txt /tmp/aot/got_$C.txt > /tmp/aot/diff_$C.txt && echo "aot $C: IGUAL ao java real" || { echo "aot $C: DIFERE"; head -12 /tmp/aot/diff_$C.txt; head -5 /tmp/aot/err_$C.txt; exit 1; }; done
g++ -std=c++17 -O1 -w -I"$R/src/modules/jvm/aot" "$R/tests/jvm/shimtest.cpp" -o /tmp/aot/shim && /tmp/aot/shim
