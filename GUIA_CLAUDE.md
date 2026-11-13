# Guia rapido (Claude): trabalhar sem ler o projeto todo
1. `tools/pend.sh [filtro]` -> RETOMADA + abertas (nao abra PENDENCIAS.md inteiro; feitas em doc/HISTORICO_PENDENCIAS.md).
2. Achar: `tools/onde.sh <nome>` (arquivo:linha) | ler so a funcao: `tools/ler.sh <nome> [max]` | mapa de arquivos: `src/MAPA.md`. Indice se atualiza sozinho.
3. Backup + editar com str_replace (nao reescrever arquivo). Funcao nova de modulo: so via `api::*` (src/modules/api.inc).
4. Modulo novo: `tools/novo_modulo.sh <nome>` | teste novo: `tools/novo_teste.sh <nome>`.
5. Verificar (saida curta): `tools/t.sh syntax <teste>...` (ex.: `tools/t.sh syntax redstone`). Suite toda so quando pedido: `tests/run.sh`.
6. Entregar: escrever PATCH.md, atualizar PENDENCIAS.md, `tools/zipar.sh <nome> <arquivos alterados>` (roda a sintaxe antes).
Pastas de src/: core (log, png, noise, keymap), platform (android, vulkan, touch, gamepad), render (malhas, ceu, sombras), pack (atlas, resource pack, itens), world (mundo, blocos, redstone, estruturas), mobs, ui (menus, inventario, chat), net, game (sobrevivencia, crafting, fisica, audio), modules (plugins), loop (fases do main). Ache arquivos por tools/onde.sh / src/MAPA.md.
Laco principal: src/main_loop.inc = esqueleto; fases em src/loop/ (init, frame_begin, events, menus_timing, tick, render). Cada fragmento e texto de main() (mesmo escopo): nao declare funcao dentro dele.
Resposta final: Mudou / Testado / Conferir no app (VIS) / AINDA FALTA / Zip.
