# Interface

`gui.cfg` concentra os tamanhos e transparências principais da interface.

As imagens editáveis ficam em `assets/textures/gui/`:
- slot.png
- slot_selected.png
- slot_hover.png
- panel.png
- crosshair.png
- dim.png

O HUD/hotbar usa `textures/gui/`. O inventário usa `textures/inventory/`, para que os dois possam ter estilos diferentes.

O menu de pausa usa `textures/gui/widgets.png` (botões 200x20 no formato do Java: normal v=46, destacado v=66, desativado v=86). Edite o arquivo para trocar o visual dos botões.

## Rethoughted GUI
A interface usa as texturas do pack Rethoughted GUI (por CatFromNet) em `assets/textures/gui/rt/`. Para trocar o visual, substitua os PNGs dessa pasta mantendo nome e tamanho (botões 200x20, hotbar 182x22, fundos 256x256 com o painel no canto superior esquerdo, abas 26x32).
