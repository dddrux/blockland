# Módulos (trabalho em dupla)

Cada pessoa trabalha na **própria pasta** `src/modules/<nome>/` e só toca em um arquivo compartilhado: `modules_list.inc` (1 linha por módulo).

## Criar um módulo
1. Copie `src/modules/exemplo/` para `src/modules/<seu_nome>/` e renomeie o `.inc`.
2. Coloque tudo em `namespace mod_<seu_nome> { ... }` (o jogo é uma unidade de compilação só; nomes iguais colidem).
3. Registre no fim do arquivo: `GAME_MODULE_REGISTER(id, "nome", onInit, onTick, onDrawWorld, onDrawHud, onCommand, onShutdown);` (use `nullptr` no que não precisar).
4. Ligue em `modules_list.inc`: `#include "modules/<seu_nome>/<seu_nome>.inc"`.

## Ganchos
- `onInit()`: 1x, após pack/modelos/mods carregados.
- `onTick(Phys&)`: 20x/s (congela na pausa).
- `onDrawWorld(float pt)`: 3D, após mobs/itens.
- `onDrawHud(int w, int h)`: 2D, após HUD/chat.
- `onCommand(cmd, args, Phys&)`: `/cmd`; devolva `true` se tratou (cmd já vem minúsculo, sem acento).
- `onShutdown()`: ao fechar.

## Juntar os dois projetos
Copie a pasta do módulo do amigo para `src/modules/` e some a linha dele no `modules_list.inc`. Se os dois editarem outros arquivos de `src/`, combinem antes (idealmente só adicionando ganchos aqui).

## Regras para não colidir
- Prefixo próprio em comandos, nomes de arquivos de save e chaves (ex.: `meu_`).
- Não use IDs de bloco/item fixos: 0-31 base, 40-47 água, 120-255 mods, 900-969 modelos, 1000+ pack. Para blocos novos use `mods/<nome>/mod.txt` até existir reserva de IDs por módulo.
- Teste: `tests/run.sh` (inclui `test_modules.cpp`).
