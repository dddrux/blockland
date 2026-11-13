#!/usr/bin/env bash
# patchJVM.10: carrega o C++ gerado de um mod no runtime AOT (sem o jogo) e roda preInit/registro/init. Uso: tests/jvm/modload.sh ironfurnaces-1.3.5   (pasta em mods/ com aot/)
set -e; R="$(cd "$(dirname "$0")/../.." && pwd)"; M="${1:?uso: tests/jvm/modload.sh <pasta-do-mod>}"; D="$R/mods/$M/aot"
[ -d "$D" ] || { echo "sem $D: rode tools/aot_mod.sh mods/$M.jar"; exit 1; }
printf '#include "jvmrt.h"\nint main(){ std::vector<std::string> lg; rt::loadMods(lg); for(auto&s:lg) printf("%%s\\n",s.c_str()); return 0; }\n' > /tmp/aot_ml.cpp
g++ -std=c++17 -O1 -w -I"$R/src/modules/jvm/aot" /tmp/aot_ml.cpp "$D"/*.cpp -o /tmp/aot_ml && /tmp/aot_ml
