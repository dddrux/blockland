# Resource packs do Minecraft (retrocompatibilidade)

Coloque aqui qualquer resource pack **Java** (.zip direto, sem precisar extrair, ou pasta).
O pack ativo é escolhido em `pack.cfg`:

    pack=Ashen_16x.zip      # nome do .zip/pasta   |   sem pack.cfg = primeiro pack encontrado

O que é lido de `assets/minecraft/textures/`:
- `block/` e `item/` (nomes modernos e antigos `blocks/`, `items/`), qualquer resolução (reduzida para 16x16; animações usam o 1º quadro)
- armaduras vestidas: `entity/equipment/humanoid*/` (1.21.2+) ou `models/armor/*_layer_1/2.png` (antigo)

Não é lido de propósito: `gui/` (a interface do jogo continua exatamente a mesma). O que o pack não tiver aparece como textura "ausente" (xadrez); não há mais texturas placeholder.

Todos os PNGs de `block/` e `item/` que não são usados pelos blocos/itens nativos viram itens extras (aba **Resource pack**, ids 1000+).
