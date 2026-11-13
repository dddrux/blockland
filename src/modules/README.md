# Módulos (trabalho em dupla)

Cada pessoa trabalha na **própria pasta** `src/modules/<nome>/` e só toca em um arquivo compartilhado: `modules_list.inc` (1 linha por módulo).

## Criar um módulo
0. Atalho: `tools/novo_modulo.sh <nome>` faz os passos 1, 2 (molde) e 4 sozinho.
1. Copie `src/modules/exemplo/` para `src/modules/<seu_nome>/` e renomeie o `.inc`.
2. Coloque tudo em `namespace mod_<seu_nome> { ... }` (o jogo é uma unidade de compilação só; nomes iguais colidem).
3. Registre no fim do arquivo: `GAME_MODULE_REGISTER(id, "nome", onInit, onTick, onDrawWorld, onDrawHud, onCommand, onShutdown, onEvent);` (use `nullptr` no que não precisar).
4. Ligue em `modules_list.inc`: `#include "modules/<seu_nome>/<seu_nome>.inc"`.

## Ganchos
- `onInit()`: 1x, após pack/modelos/mods carregados.
- `onTick(Phys&)`: 20x/s (congela na pausa).
- `onDrawWorld(float pt)`: 3D, após mobs/itens.
- `onDrawHud(int w, int h)`: 2D, após HUD/chat.
- `onCommand(cmd, args, Phys&)`: `/cmd`; devolva `true` se tratou (cmd já vem minúsculo, sem acento).
- `onShutdown()`: ao fechar.
- `onWorldLoad(dir)` / `onWorldSave(dir)`: depois de carregar / ao salvar `saves/<dir>/` (grave seus arquivos como `saves/<dir>/<prefixo>_x.dat`). Save só no host/solo.
- `onWorldReset()`: ao sair do mundo; zere o estado do módulo.
- `onBlockBreak(x,y,z,id)` (antes de virar ar) / `onBlockPlace(x,y,z,id,meta)`: só o jogador local.
- Para usar os ganchos novos monte o módulo por campo: `static GameModule mk(){ GameModule m; m.name="x"; m.onWorldSave=...; return m; }` + `GAME_MODULE_REGISTER_FN(x, mk);`
- `onEvent(const SDL_Event&)`: evento SDL bruto (teclas/mouse); `true` consome. Nunca recebe SDL_QUIT.

## Mini-APIs (`src/modules/api.inc`, sempre disponíveis)
- `api::blk::get/meta/set/setLocal/isAir` (set = local + rede) | `api::chat::say/ok/err`
- `api::hud::begin(w,h)/end()/rect(x,y,w,h,rgb,a)/text(x,y,sc,str,rgb)/width` (use dentro de onDrawHud)
- `api::item::name/give` | `api::mob::spawn` | `api::arg::str/num/flt/has` (args de /comando)
- `api::Cmd` + `api::dispatch(tabela, n, cmd, args, P)`: vários /comandos numa tabela; uso errado mostra "Uso: ..."
- `api::Cfg c("nome")`: `modulo_<nome>.cfg` (get/getInt/set/setInt/save)
- Achar qualquer outra função do jogo: `tools/onde.sh <nome>` (índice em src/INDICE.tsv, mapa em src/MAPA.md; regenerar: `python3 tools/mapa.py`).

## Juntar os dois projetos
Copie a pasta do módulo do amigo para `src/modules/` e some a linha dele no `modules_list.inc`. Se os dois editarem outros arquivos de `src/`, combinem antes (idealmente só adicionando ganchos aqui).

## Regras para não colidir
- Prefixo próprio em comandos, nomes de arquivos de save e chaves (ex.: `meu_`).
- Não use IDs de bloco/item fixos: 0-31 base, 40-47 água, 120-255 mods, 900-969 modelos, 1000+ pack. Para blocos novos use `mods/<nome>/mod.txt` até existir reserva de IDs por módulo.
- Teste: `tests/run.sh` (inclui `test_modules.cpp`).
