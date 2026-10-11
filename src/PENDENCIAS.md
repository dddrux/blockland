# PENDENCIAS — Blockland (trabalho por partes)

Regra de trabalho: cada parte e pequena, compila e tem teste; ao fim de cada uma este arquivo e atualizado.
Legenda: [x] feito+testado headless | [~] parcial | [ ] nao iniciado | (VIS) falta conferir no jogo real.

## Ordem de prioridade (do usuario)
1. [ ] Multiplayer: dimensoes separadas, sync de mobs e animais, portal sem obsidiana, TP/puxar players, chunks em volta de TODOS os jogadores
2. [~] Nickname/skin — **Parte 2 feita** (regras + nick unico no host); falta o resto (ver abaixo)
3. [ ] Modo espectador (cavernas/lava), F3+F4 igual ao Java, F5 + escada (camera)
4. [~] Colisao do jogador com blocos — **Parte 1 feita** (ver abaixo); falta o resto
5. [ ] Pick block (botao do meio): estados de bloco, creative/survival, entidades, NBT, split, Android
6. [ ] Chat identico ao oficial (aparencia, T e /, Tab-complete, historico, cores, 256 chars, log, Android)
7. [ ] Vidracas/paineis/cercas/muros/grades: conexao, outline, modelo, colisao, luz
8. [ ] Laje e escada de arenito: topo, modelo, hitbox, culling, luz, orientacoes
9. [ ] Split screen ativavel pelo Esc, inventarios separados, PlayerStats do P2, lado a lado, ancora de chunks do P2
10. [ ] Mobs/animais: mirar em todos os jogadores, colisao/empurrao, limite por bloco, presos, esqueleto (animacao e atirar de costas)
11. [ ] Survival basico: spawn de animais, limite de corrida com fome <= 3 pernis, crafts faltando
12. [ ] Vilas: villagers na terra, casas, porta/cerca, ferreiro (stone no lugar da bigorna), portas que quebram
13. [ ] Modelos/UI: drops com modelo quebrado (facilitador), piston grudento no inventario
14. [ ] Comandos: /tp 300000 90 0, tp/puxar players, World Edit, /nick, /list
15. [ ] Resource packs oficiais 100% (animadas, HD, reload F3+T, Android)
16. [ ] API/documentacao de mods
17. [ ] Launcher e menu principal (visual, background parallax, nick, camera mais de cima)
18. [ ] Android: split, teclado do chat, acerto visual

## Item 4 — colisao: o que foi feito (Parte 1)
- [x] `physics.inc unstuckPlayer(P)`: se o hitbox do jogador sobrepoe bloco solido (tp, respawn, chunk que carregou depois, sair de veiculo/espectador, bloco colocado em cima), empurra para a posicao livre mais proxima (lados/cima ate 2 blocos; senao sobe a coluna). Ignora espectador e chunk nao carregado. Chamada todo tick antes de mover (`main_loop.inc` jogador 1, `splitscreen.inc` jogador 2).
- [x] `moveEntity` agora varre em sub-passos de no maximo 1 bloco quando o movimento passa de 1 bloco/tick (`moveEntityOnce` = o algoritmo original do 1.8.9, intacto). Movimento normal e identico ao anterior (testado).
- [x] Teste: `tests/test_collision.cpp` (86 verificacoes). Teste de mutacao: com `unstuckPlayer` desligado, 9 falham.
- Achado: o algoritmo original do 1.8.9 (recorta contra TODAS as caixas da regiao varrida) ja nao atravessa bloco em velocidade alta — o teste passa mesmo sem os sub-passos. A causa real de "entrar na parede" e o jogador JA nascer/ficar sobreposto (tp, respawn, chunk tardio, bloco colocado em cima); isso o `unstuckPlayer` resolve. Os sub-passos ficam como margem de seguranca.
- [ ] Falta: conferir no jogo real (VIS) escada, laje, cerca (1,5), painel, porta aberta/fechada, agua corrente, pistao empurrando; colisao com entidades separada da de blocos; hitbox agachado (1.8.9 nao muda; a lista pedia 0,6 de altura, que nao existe no Minecraft — confirmar com o usuario); /tp e entrada em dimensao ja passam pelo `unstuckPlayer`, mas conferir no jogo.

## Item 2 — nickname: o que foi feito (Parte 2)
- [x] `launcher.inc`: `nickValid/nickSanitize/nickEq/nickSuggest/nickIsAuto`. Regra do oficial: 1..16 de `[A-Za-z0-9_]`; na limpeza espaco vira `_` e o resto invalido (acento, simbolo) e descartado. Aplicada ao nick do launcher (`screens.inc launcherCommit`) e ao `profile.cfg` (`profileLoad`): perfil antigo "João Pedro" vira "Joo_Pedro".
- [x] `net.inc` (host, no HELLO): sanitiza o nick recebido; nick ja em uso (host ou convidado, sem diferenciar maiuscula) => recusa com KICK dizendo o nick e sugerindo variacao ("zeh" -> "zeh_1"); nome padrao ("Jogador", "Jogador 2") troca sozinho para "Jogador_N" sem recusar; convidado fantasma (>2 s sem pacote, voltou por outra porta) libera o nick. Protocolo NAO mudou (NET_VER igual).
- [x] Testes: `tests/test_nick.cpp` (27, handshake UDP real); mutacao (sem a checagem de unicidade) => 5 falhas. `test_lan.cpp` atualizado para a regra nova.
- Limitacao conhecida: o convidado com nome padrao nao e avisado do nome que o host escolheu (Jogador_N); ele so aparece certo para o host. Vai junto com a sincronizacao de nick (abaixo).
- [x] Parte 3A: `/list` (aliases `/lista`, `/players`) e `/nick <nome>` em `chat_debug.inc` (+ `cmdPlayerList/cmdNickChange` em `net.inc`). `/nick` so fora de partida em rede (valida com `nickValid`, grava `profile.cfg`); em rede recusa com mensagem, ate existir sync de nick. Convidado so lista os nomes que ja chegaram. Teste: `tests/test_cmd_nick.cpp` (11) + mutacao conferida.
- [ ] Parte 3B (proxima): `/tp`, `/give` por nick; `/tell` `/msg` `/w` `/r` (precisa pacote privado novo, NET_VER sobe).
- [ ] Falta no item 2: nametag acima da cabeca (billboard, fundo preto semitransparente, some agachado, atravessa parede so no mesmo time), nick no Tab/chat/HUD/mensagens, Tab com cabeca+ping e ordem por time, `/nick`, `/list`, `/tell /msg /tp /give` por nick, nick do jogador 2 no split, botao Tab no Android, campo de nick editavel no launcher com mensagem de erro, skin por conta + fallback Steve/Alex, sync de nick/skin entre clientes (incl. avisar o convidado do nome final).

## Fora do escopo (ja quebrados antes)
- `tests/test_joystick.cpp` (layout antigo) — marcar `DISABLED_`/TODO.
- `test_meta`: 1 falha de formato de save BL17 — abrir issue propria.
- Ja quebravam ANTES da Parte 1 (confirmado rodando o codigo original): `test_shapes` 1 falha (`gapBad == 0`, linha 45); `test_split` e `test_player_save` dao segfault no ambiente headless.
