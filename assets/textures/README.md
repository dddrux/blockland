# Texturas do Blockland

**Blocos e itens vem SEMPRE do resource pack** (`assets/resourcepacks/`, padrao `Ashen_16x.zip`, escolhido em `pack.cfg`).
Nao existem mais texturas placeholder (`blocks/`, `items/`, `atlas.png` e o gerador `gen_items.py` foram removidos).
Se o pack nao tiver um arquivo, o jogo mostra o xadrez magenta/preto (como o Minecraft).

- Todo PNG de `textures/block/` e `textures/item/` do pack vira um item de inventario (ids 1000+), na aba **Resource pack** do criativo
  e na busca (nomes em ingles, a partir do nome do arquivo). Sao itens so de inventario: nao podem ser colocados no mundo.
- `gui/` — interface (Rethoughted GUI em `gui/rt/`); `inventory/` — painel e slots do inventario; `colormap/` — cores de bioma da grama/folhas.
