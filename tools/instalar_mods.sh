#!/usr/bin/env bash
# Converte TODOS os .jar de mods 1.12.2 para o Blockland (blocos/itens aparecem no criativo e sao colocaveis).
# Uso: tools/instalar_mods.sh [pasta_com_jars]   (padrao: mods/ e mods_jar/, na raiz do projeto)
# Texturas de blocos do Minecraft (muitos mods usam): NAO precisa ser o vanilla. O conversor usa, nesta ordem: $VANILLA (zips separados por virgula),
#   os resource packs de assets/resourcepacks (os que trazem textures/blocks, ex.: Pixel Perfection), assets/*minecraft-assets*textures*.zip,
#   ~/.minecraft/versions/1.12.2/1.12.2.jar. A saida mostra "vanilla: ok (<zips usados>)". Reconverter tudo: FORCE=1 tools/instalar_mods.sh
# Depois abra o jogo: cada mod ganha uma aba no inventario criativo. Pula jar ja convertido (props.bin mais novo que o jar).
cd "$(dirname "$0")/.." || exit 1
command -v python3 >/dev/null || { echo "precisa de python3 (e: pip install pillow)"; exit 1; }
DIRS=("$@"); [ ${#DIRS[@]} -eq 0 ] && DIRS=(mods mods_jar)
VA=(); [ -n "$VANILLA" ] && VA=(--vanilla "$VANILLA")   # o conversor ja procura sozinho (ver linha "vanilla:" na saida)
ok=0; skip=0; bad=0
for d in "${DIRS[@]}"; do
  [ -d "$d" ] || continue
  for j in "$d"/*.jar; do
    [ -e "$j" ] || continue
    mk="mods/.conv/$(basename "$j").ok"   # patchConv.7: marcador por jar (o nome do jar nao e o modid: mcw-*, furniture=cfm, industrialcraft=ic2)
    if [ -z "$FORCE" ] && [ -f "$mk" ] && [ "$mk" -nt "$j" ]; then skip=$((skip+1)); continue; fi
    echo ">> convertendo $j"
    if python3 conversor/modconv/modconv.py "$j" --out mods "${VA[@]}"; then mkdir -p mods/.conv; touch "$mk"; ok=$((ok+1)); else echo "   FALHOU: $j"; bad=$((bad+1)); fi
  done
done
echo "convertidos: $ok  ja prontos: $skip  falhas: $bad"
echo "Confira mods/<mod>/RELATORIO.md (lacunas). Abra o jogo: aba do mod no criativo."
[ "$ok$skip" = "00" ] && echo "nenhum .jar encontrado em: ${DIRS[*]}"
exit $((bad>0))
