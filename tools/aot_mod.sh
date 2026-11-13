#!/usr/bin/env bash
# patchJVM.4: traduz um mod (.jar) para C++ (mods/<mod>/aot/*.cpp) e gera mods/<mod>/aot_cobertura.txt (o que ainda falta no shim). Uso: tools/aot_mod.sh mods/<mod>.jar
cd "$(dirname "$0")/.." || exit 1; J="${1:?uso: tools/aot_mod.sh mods/<mod>.jar}"; M="$(basename "$J" .jar)"; D="mods/$M/aot"; rm -rf "$D"; mkdir -p "$D"
python3 conversor/modconv/jvmlift.py "$J" --out "$D" --calls "mods/$M/aot_chamadas.json" && python3 conversor/modconv/jvmcover.py "$J" --out "mods/$M/aot_cobertura.txt"
