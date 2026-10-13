# PENDENCIAS — Blockland (trabalho por partes)

Regra de trabalho: cada parte e pequena, compila e tem teste; ao fim de cada uma este arquivo e atualizado.
Legenda: [x] feito+testado headless | [~] parcial | [ ] nao iniciado | (VIS) falta conferir no jogo real.

## Ordem de prioridade (do usuario)
1. [~] Multiplayer (patch1.0: portal com obsidiana na rede + remoto em outra dimensao nao e desenhado/alvejado; patch2.0: mobs sincronizados host->convidados (src/mobsync.inc, NET_VER 16), hostis miram o jogador mais proximo incl. convidados, host gera chunks/spawn em volta dos convidados; falta drops/mobs por dimensao do convidado, sync de inventario de mula, reproducao): dimensoes separadas, sync de mobs e animais, portal sem obsidiana, TP/puxar players, chunks em volta de TODOS os jogadores
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

## Item 3 — F5 + escada
- [x] `main_loop.inc` (camera 3a pessoa): a celula da escada (XB_LADDER, placa fina) nao conta mais como parede no raio da camera; antes, subindo a escada, o pivo ficava dentro da celula 'solida' => distancia 0 => camera colada. Agora o raio pula a celula da escada (ate 6 vezes). (VIS) conferir F5 na escada, de costas e de frente (parede atras da escada ainda deve limitar a camera). Falta: espectador (cavernas/lava) e F3+F4.

## Item 17 — menu
- [x] `main_loop.inc drawPanorama`: camera do panorama inclinada 18 graus para baixo (antes 8). (VIS) ajustar o numero se ficar demais/de menos. Falta: background novo, parallax, launcher.

## Item 3 — F3+F4
- [x] Seletor de modo como no Java 1.16+: segurando F3, cada F4 avanca Criativo > Sobrevivencia > Aventura > Espectador; soltar F3 aplica (`playerlist.inc drawGmSwitcher`, `main_loop.inc`). O F3 sozinho agora alterna o debug ao SOLTAR (nao dispara mais junto com F3+F4). (VIS) conferir posicao e tamanho; icones sao letras C/S/A/E (sem arte). Falta: espectador com cavernas/lava.

## Item 10 — esqueleto atirando de costas
- [x] `hostile.inc`: removido o 'recuo' a < 5 blocos (o corpo virava para a direcao da fuga e a flecha saia de costas; o esqueleto do 1.8.9 para e mira) e o tiro so sai com o corpo a <= 50 graus do alvo. (VIS) conferir. Falta: animacoes/modelo mais fieis, mira em todos os jogadores, colisao/empurrar, limite por bloco, presos.

## Item 12 — aldeoes na terra
- [x] `mobs.inc vilSpawnPending`: espera o chunk carregar (antes descartava o pedido), procura o chao livre mais proximo (solido embaixo + 2 celulas livres) e nao nasce se nao houver. (VIS) conferir em vila nova. Falta: casas, porta/cerca, ferreiro/bigorna, portas que quebram.

## Item 12 — ferreiro
- [x] `village_build.inc` (casa do ferreiro, case 9): `STONE` em (8,1,1) trocado por `XB_ANVIL` (eixo longo em Z). So a casa do ferreiro; a linha 120 (outra casa) ainda usa pedra como no vanilla. (VIS) conferir bigorna e orientacao numa vila nova (so vilas geradas depois do patch).

## Item 12 — portas
- [~] `redstone.inc rsUse`: abrir/fechar porta agora troca as DUAS metades com `gNoBlockHooks` ligado (sem cascata de vizinhanca), para a checagem de suporte nao derrubar a porta no meio da troca. Causa NAO confirmada por execucao (so leitura do codigo): se ainda quebrar ao abrir, o suspeito e `rsSupported` case 15 (`solid(below)`/metade vizinha) em `redstone.inc:240`. (VIS) testar porta da vila e porta colocada. Falta: cerca e porta mal geradas.

## Item 6 — chat
- [x] `chat_debug.inc`: limite de 256 CARACTERES (antes 240 bytes; acentos contavam em dobro), o excedente e cortado, inclusive ao colar. Tab completa comando (`/gam` -> `/gamemode `) e nick (ultima palavra, usa `/list`); varios candidatos => completa o prefixo comum e lista as opcoes no chat. (VIS) conferir; a lista de comandos do Tab e fixa (`CMDS` em `chatTab`), atualizar ao criar comando novo. Falta: cores por tipo, `[Nome]` vs `<Nome>`, selecao de texto/cursor, aparencia identica, Android (teclado), chat por jogador no split.

## Item 18 — Android
- [x] `splitscreen.inc split2Enable`: no Android ligar o split mostra o aviso "Split screen indisponivel nesta plataforma" e nao liga (curto prazo). Medio prazo (scissor+viewport, toque por metade) segue pendente. Confirmar que `chatAdd` ja esta declarado antes de `splitscreen.inc` (usado em `split2Respawn` no mesmo arquivo, entao esta).
- Chat (item 6): cores por tipo ja existem (sistema amarelo, sussurro cinza, erro vermelho); nada a mudar. Falta so verde p/ sucesso e cor de time (sem times).

## Item 11 — spawn de animais
- [x] `hostile.inc animalSpawn` (chamado em `tickMobs`): a cada 5 s tenta 1 grupo de 2-4 (vaca/ovelha/porco/galinha) em GRASS a 24-48 blocos, so com ceu claro, teto de 12 animais num raio de 80; so overworld; convidado em rede nao gera (mobs ainda nao sincronizam). (VIS) conferir se grama do mundo e o id GRASS (blocos de grama do pack podem ter outro id) e ajustar teto/frequencia. Falta: spawn inicial na geracao do mundo, despawn, reproducao sincronizada.

## Item 5 — pick block
- [x] `main_loop.inc actMain`: botao do meio no mundo. Hotbar => seleciona; mochila => troca com o slot atual; criativo sem o item => cria 1 no slot atual; sobrevivencia sem o item => nada; espectador => nada (ja retornava antes). Blocos XB (laje, escada, tocha...) usam `xbDropItem(id, meta)` (item do modelo certo). patch3.0: pick em entidade no criativo (`mobPickType`: ovo de porco/vaca/ovelha; demais mobs nao tem ovo no jogo). Falta: stone->cobble ja nao se aplica (pick pega o bloco), NBT de bau/shulker, split screen, toque no Android. (VIS) conferir no jogo.

## Item 11 — survival
- [x] Limite de corrida: `main_loop.inc` `canSprintFood` (fome > 6 = mais de 3 pernis, ou criativo/voando, como no Java); corrida inicia so com isso e para sozinha ao cair para <= 6. (VIS) conferir no jogo. Sem teste headless (tests/ nao veio no zip).

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
- [x] Parte 3B: `/tell /msg /w <nick> <msg>`, `/r`, `/tp <nick>`, `/tp <alvo> <destino|x y z>`, `/puxar <nick>`, `/give <nick> <item> [qtd]`. Rede: NET_VER 12, pacotes `NP_PRIV/NP_TPTO/NP_GIVE` (1o byte = id do alvo; so o alvo age; TPTO/GIVE so valem vindos do host, id 0, pois nao ha sistema de op). Convidado so faz `/tp <nick>` (ele mesmo) e tell. Loop principal aplica `gNetTp` (main_loop.inc). Teste: `tests/test_cmd_player.cpp` (24) + mutacao conferida.
- [x] Sync de nick (NET_VER 13): WELCOME leva o nome final do convidado (ex.: `Jogador_2`); host manda `NP_ROSTER` (id->nick) a todos ao entrar alguem, ao trocar nick e a cada 5 s (perda de UDP); `/nick` em rede: host troca direto (checa unicidade, avisa todos), convidado manda `NP_NICKREQ` e o host aceita/recusa (`NP_SYS` = erro). Nick atribuido pelo host nao vai pro `profile.cfg`; so `/nick` aceito grava. Convidado ve nomes reais em `/list`, `/tell` e `netLabel`. Teste: `tests/test_nick_sync.cpp` (33) + mutacao conferida.
- [x] Sync de skin (NET_VER 14): `NP_SKIN` = pixels 64x64 RGBA em 16 pacotes (dono, indice, 1024 B). Convidado manda a propria skin ao entrar e a cada 20 s; host guarda (`gSkins`), repassa e manda as guardadas + a dele a quem entra. So o dono (ou o host repassando) pode mandar a skin de alguem; textura so e recriada se os pixels mudaram. `netDraw` desenha cada remoto com a textura dele (`NetSkinSwap`); sem skin recebida usa `assets/skins/default.png`, nunca a do jogador local. Teste: `tests/test_skin_sync.cpp` (16) + mutacao conferida. Limites: o modelo (bracos finos/slim) e global, entao skin slim de remoto usa o modelo do jogador local; parte de GL do `netDraw` nao e coberta pelos testes headless.
- [x] Nametag (`net.inc netDrawTags`, ligada em `main_loop.inc` apos `netDraw`): billboard (inverso da rotacao da camera), 1/60 x 1.6 bloco por pixel, 2.3 acima dos pes, fundo preto 25% (-1..8 px), texto branco com sombra, fonte do jogo; some agachado/morto/espectador alheio/alem de 64 blocos; sem escrever profundidade (paredes escondem). Teste da regra: `tests/test_nametag.cpp` (9). NAO verificado em tela: orientacao/tamanho do billboard e ordem de desenho precisam de conferencia no jogo; sem times ainda (sem 'atravessa parede' do mesmo time); nao desenha o proprio nick em 3a pessoa.
- [x] Parte 3E — Lista de jogadores (Tab) + ping (NET_VER 15): `playerlist.inc` (`tabBuild/tabLayout/tabPingLevel/drawTabList`): todos os conectados, ordem alfabetica sem diferenciar caixa, colunas de ate 20 linhas (regra do oficial, max 80), cabeca da skin (rosto + chapeu), nick e barras de ping (5 niveis, X cinza se desconhecido). So aparece segurando Tab em partida com mais de um jogador (como o Java). Ping: host manda `NP_PING` a cada 2 s, convidado devolve `NP_PONG` com o mesmo token, host suaviza (media movel 3:1); `NP_ROSTER` agora leva o ping (uint16) depois de cada nick, entao o convidado ve o ping de todos. Android: 7o botao no topo (Tab, vale enquanto o dedo esta nele). Teste: `tests/test_playerlist.cpp` (31) + mutacao conferida (7 falhas); `test_nick_sync` atualizado para o roster com ping (35). Nao verificado em tela: posicao/tamanho do Tab, UV da cabeca (rosto em (8,8), chapeu em (40,8)) e icone do botao de toque. Sem times: ordem so por nome.
- [x] Parte 3F — Nick no chat e no split: (1) o host so repassa/mostra `NP_CHAT` de convidado se o texto comecar com `<nick do remetente> ` e o id do pacote for o da conexao (`netChatOk`); forjar `<Outro> msg`, texto sem nick, quebra de linha ou id falso e descartado (antes o host repassava cru). Formato mantido `<Nome> msg` (o do Java; o documento citava `[Nome]`, que nao e o oficial). (2) Jogador 2 do split tem nick proprio: `gP2Name` (`nick2=` no `profile.cfg`), `p2Nick()` (padrao `Jogador_2`; se colidir com o do jogador 1 ou da rede ganha sufixo `_1`...), comando `/nick2 <nome>` (so com split ligado; valida e recusa duplicado), `/nick` do jogador 1 recusa o nick do 2. Aparece em `/list`, nas mensagens de morte/renascer do jogador 2, num rotulo no canto de cada viewport (`drawSplitNick`) e como nametag acima do corpo do OUTRO jogador local (`netDrawTagLocal`; some agachado). A nametag foi refatorada (`netTagOne`, `NetTagState`) sem mudar o desenho das remotas. Teste: `tests/test_nick_p2_chat.cpp` (29) + mutacao conferida (9 falhas). Nao verificado em tela: posicao do rotulo do viewport e da nametag do P2. Limites: o P2 ainda nao e alvo de `/tell /tp /give` por nick e nao fala no chat com o proprio nick (chat do P2 e item do split); `/nick2` nao troca em rede em tempo real para os outros (o P2 nao tem conexao propria).
- [ ] Falta no item 2 (apos a 3E): nick no chat/mensagens/HUD, nick do jogador 2 no split, campo de nick no launcher com mensagem de erro, skin por conta + fallback Steve/Alex, times (atravessar parede, ordem do Tab por time). Ja feito antes: nick unico, /nick, /list, /tell /msg /tp /give, sync de nick e skin, nametag, Tab com cabeca e ping.
- [ ] (lista antiga, mantida p/ referencia): atravessar parede so no mesmo time (precisa de times), nick no Tab/chat/HUD/mensagens, Tab com cabeca+ping e ordem por time, `/nick`, `/list`, `/tell /msg /tp /give` por nick, nick do jogador 2 no split, botao Tab no Android, campo de nick editavel no launcher com mensagem de erro, skin por conta + fallback Steve/Alex.

## Item 8 — laje e escada de arenito (patch4.0)
- [x] `redstone.inc`: cantos da escada como no 1.12 (`rsStairShape`: reta, externo/interno esquerdo/direito, calculado pelos vizinhos, mesma metade, nao salvo). Colisao/selecao (`rsStairBoxes`) e malha (`rsStairBake`/`rsStairQuadsAt`, usada em `models.inc emitXB`) seguem o canto. Escada reta fica identica a antiga.
- [x] Luz: `biome_light.inc lightOpacL` — escada e laje (inclusive meia) bloqueiam luz (opacidade 255 do 1.12); `lightOpac` (culling) continua tratando como nao-opaca. `world.inc`: flood/ceu usam `gLOpLut`; a celula de escada/laje usa a maior luz entre ela, cima e 4 lados (useNeighborBrightness). (VIS) conferir escuro sob lajes em mundo antigo.
- [x] Teste `tests/test_stairs.cpp` (324, pack real) + mutacao (4 mutantes pegos). `tests/test_conn.cpp` movido para tests/. `tests/stubs/SDL.h` completado (teclas/clipboard do chat).
- [ ] Falta: conferir no jogo (cantos, inventario/mao/chao); culling das faces laterais contra blocos inteiros.
- Achados: ja existem `/me /say /kill`, `logs/latest.log`, rolagem/cursor/selecao do chat e `/team`. `/weather` nao tem sistema de clima. `/give @p2` depende de inventario separado do P2 (`inv[]` e global). Crafts: so ha receitas em `crafting.inc` (sem escada, laje, cerca, porta, bau, fornalha, tocha, escada de mao, cama, trilho); `gRsDrop` so e preenchido apos `loadCreativeCfg()`.
- Testes: test_shapes continua com 1 falha previa (`gapBad`, linha 45); test_redstone da SKIP (pack, cwd).

## Fora do escopo (ja quebrados antes)
- `tests/test_joystick.cpp` (layout antigo) — marcar `DISABLED_`/TODO.
- `test_meta`: 1 falha de formato de save BL17 — abrir issue propria.
- Ja quebravam ANTES da Parte 1 (confirmado rodando o codigo original): `test_shapes` 1 falha (`gapBad == 0`, linha 45); `test_split` e `test_player_save` dao segfault no ambiente headless.

## patch3.0
- `SDL_SetWindowMinimumSize(win, 854, 480)` (main_loop.inc): janela nao encolhe abaixo do minimo (launcher/split). Ainda falta layout relativo testado em 1280x720..2560x1440 (VIS).
- Mensagem de sucesso do `/tp` em verde (0x55FF55).

## patch5.0 (crafts + animais)
- [x] `crafting.inc` + `chest.inc packRecipes()`: crafts com id dinamico do pack (recriados se o pack muda): escada (oak/pedra/arenito/tijolo de pedra), laje (idem + stone), cerca, portao, porta (carvalho/ferro), alcapao, bau, placa de pressao, botao, tocha, escada de mao, trilho, fornalha, cama branca. (VIS) conferir no jogo; sem teste headless novo.
- [x] `hostile.inc animalSpawn`: despawn de animal a >= 128 blocos do jogador; com < 4 animais por perto tenta spawn a cada 1 s (spawn inicial rapido). Falta: reproducao/crescimento sincronizados.
- [x] patch5.1: crafts de painel de vidro, barras, muro, carpete, placa; /say em azul (anuncio).
- [x] patch5.2: pick block em entidade agora pega ovo de galinha, zumbi, esqueleto, creeper, aldeao e porco-zumbi (mobEggItem em mobs.inc). Mobs sem ovo (bote, minecart, quadro, armor stand) continuam pendentes.
- [x] patch5.3: split screen ativavel pelo Esc (botao 'Tela dividida' no menu de pausa, pause_menu.inc/main_loop.inc/splitscreen.inc). Falta: escolha lado a lado vs cima/baixo e de dispositivo. (VIS) conferir o botao e o menu mais alto.
- [x] patch5.4: Esc > botao de layout do split (lado a lado / cima e baixo), grava splitmode. Falta so a escolha de dispositivo.

## Conferido no codigo (patch5.5): a lista antiga estava desatualizada
- [x] Jogador 2 do split JA tem inventario proprio (`split2SwapInv`), vida/fome (`SurvCtx`/`split2SwapSurv`, arco e comida no contexto) e ancora de chunks (`main_loop.inc` ~932: `stream` em volta do P2 quando ele esta a >2 chunks do P1; `gTrimX2` na descarga). O comentario do topo de `splitscreen.inc` esta velho.
- [ ] Reproducao de animais: NAO existe base (sem item trigo, sem campo de filhote em `MobEnt`, sem modelo/escala de filhote, sem sync). Precisa de: item trigo + cenoura/semente, `inLove`/`age` em `MobEnt`, filhote (escala .5, cabeca maior), sync em `mobsync.inc` (NET_VER sobe), save. Trabalho de varias partes.
- [x] patch5.6: tests/test_craft_pack.cpp (15 verificacoes, pack real, 0 falhas) cobre os crafts novos; test_chest/chest2/nether continuam 0 falhas.
- [~] patch5.7: tests/test_village_door.cpp: portas geradas pela vila (seeds 1-6, 10 portas): as 2 metades existem, meta igual, suporte ok; abrir (rsUse) + avisar os 6 vizinhos + 40 ticks NAO quebra nenhuma. Logica de dados OK, entao a causa em jogo nao esta em rsUse/rsSupported/gen. Suspeitas restantes (VIS): modelo do pack (porta com janela so na metade de cima, 'oak_door_top' tem recorte) ou algo so de render/rede. Precisa de video/descricao do que aparece.

## patch5.8 (item 6, etapa 1: flechas)
- [x] `bow.inc`: flecha tem `cre` (atirada em criativo). Regra 1.12.2: survival atira = recolhivel (ALLOWED, vira item se couber, senao fica); criativo atira = so criativo recolhe (CREATIVE_ONLY, some sem dar item; antes nunca era recolhida). Ja existia: crava, tremor 7 ticks, 1200 ticks (1 min), cai no vazio (y<-64), solta se o bloco some. Teste: `tests/test_arrow_pick.cpp` (7) + mutacao conferida (4 falhas).
- [ ] Decisao: flecha de esqueleto NAO e recolhivel no 1.12.2 (so as de jogador); mantido. "Flechas somem ao entrar em creative" NAO e regra do oficial; nao feito.
- [ ] Falta (etapa 2): flecha de outro jogador recolhivel por quem passa perto (precisa de pacote de pickup sincronizado: hoje so o dono recolhe; `arrowNetRecv` cria com pick=false); estado "stuck" via rede (os outros so veem voar ate sumir); flecha ainda nao vira entity item; som/animacao de coleta; cacto/lava/fogo; funil; P2 do split (arrowsTick so recebe o P1); Android. (VIS) conferir no jogo.

## patch5.9 (item 2: cerca/painel das vilas) — ACUMULATIVO (inclui o 5.8)
- [x] `village_build.inc VB::finish()`: antes conectava com qualquer bloco `< NB` (tocha, flor, lava...) e ignorava escada/portao/painel-colorido; agora usa a mesma regra do `rsConnMask` (so face SOLID: `rsFullCube`, escada pelas costas, portao, cerca da mesma familia, painel com painel/`XB_CPANE`). Vizinho fora do chunk nao era considerado (cerca/painel na borda ficava meio-conectado): as posicoes de borda vao pra `gVilEdge` e `vilEdgeTick()` (blockupdate.inc, a cada 4 ticks do host) chama `rsConnFix` quando os 4 chunks vizinhos ja existem (reenvia o bloco pela rede se mudou).
- Teste: `tests/test_village_conn.cpp` (342 cercas/paineis em 6 seeds, 42 na borda): codigo antigo = 42 erradas, novo = 0. Sem o pack Ashen na maquina de teste (so blocos do core); (VIS) conferir vidros/cercas no jogo com o pack.
- [ ] Item 1 (porta da vila) continua esperando video/descricao do usuario.

## patch5.10 (item 3: transparencia sobre entidades) — ACUMULATIVO (inclui 5.8 e 5.9)
- [x] `main_loop.inc`: agua, vidro colorido e portal (que esta na parte de vidro do wmesh) sao desenhados com blend e SEM escrever profundidade, mas drops/itens/jogadores remotos (`netDraw`)/mobs/flechas/corpo do split/personagem em 3a pessoa eram desenhados DEPOIS deles: passavam no teste de profundidade contra o terreno de tras e cobriam a transparencia com cor cheia. Agora esse bloco vem antes de nuvens+agua+vidro (boats/carts ja eram assim). Particulas, nametags, mao em 1a pessoa e `modulesDrawWorld` continuam depois. So moveu codigo (diff conferido linha a linha), sem teste headless possivel (GL).
- [ ] (VIS) conferir: jogador/mob atras de agua e do portal do Nether (tinge e some com a distancia), mob dentro da agua, 3a pessoa na agua, split screen (2 passadas), nametag atras da agua, godrays, e se algum modulo (`modulesDrawWorld`) desenhava entidade opaca e agora fica por cima da agua.

## patch5.11 (item 4, etapa 1: atalhos do inventario) — ACUMULATIVO (inclui 5.8, 5.9, 5.10)
- Ja existia (conferido no codigo, nao refeito): abas, busca, rolagem/paginas, clique esq/dir, shift-clique (inventario<->hotbar<->bau, armadura, elitro), clique do meio (criativo), arrastar esq/dir para distribuir, duplo clique (juntar), soltar fora do painel, lixeira, 1-9 sobre slot.
- [x] `inventory_ui.inc`: Q sobre o slot solta 1 item, Ctrl+Q a pilha (resultado do craft so com Ctrl = 1 fabricacao); F sobre o slot troca com a mao secundaria (armadura valida, resultado do craft so se a offhand estiver vazia); 1-9 sobre o slot agora so age com o cursor vazio (GuiContainer.checkHotbarKeys). `main_loop.inc` liga as teclas Q/F com o inventario aberto (sem a busca do criativo digitando). Vale para survival, criativo e bau.
- Teste: `tests/test_inv_keys.cpp` (9). `test_chest`/`test_chest2` sem regressao.
- [ ] Falta (item 4): palette do criativo com Q (nao soltar), 1-9 mudando `sel` com inventario aberto e o mouse fora de slot (o Java ignora), shift+duplo clique, clique-direito no slot vazio com cursor cheio ja ok; Android/touch nao tem Q/F. (VIS) conferir no jogo.

## patch5.12 (item 4 da lista: ovo arremessavel, etapa 1 local) — ACUMULATIVO (inclui 5.8 a 5.11)
- [x] `egg.inc` (novo, incluido apos bow.inc): botao direito com ovo no ar, agachado ou em bloco nao interativo (bau/mesa/alavanca/rs) arremessa. Fisica EntityThrowable (v 1.5, imprecisao 1.0, gravidade .03, arrasto .99 / .8 na agua), cooldown de 10 ticks, consome 1 (criativo nao), pilha de 16 (`maxStack`), funciona em aventura, nao no espectador. Colide com bloco (raySolidDistance) e mob (dano 0: so empurra e pisca, ja que `mobHurt` ignora dmg<=0). Quebra com 8 particulas do item; chance 1/8 de chocar (1/32 delas = 4). Desenho: sprite do item virado para a camera (`eggsDraw`).
- Teste: `tests/test_egg.cpp` (12) + mutacao conferida (5 falhas). chest/chest2/nether/craft_pack/arrow_pick sem regressao.
- [ ] Falta: SOM de quebra (o projeto nao tem sistema de audio); filhote (item 5: hoje nasce galinha ADULTA); jogador remoto (pacote de rede e ver o ovo dos outros); mob em convidado (dano/empurrao e do host); P2 do split; Android/touch (botao de usar ja deve cair no mesmo clique, nao testado); jogador remoto atingido (vanilla: empurra, sem dano); dispenser lancando ovo. (VIS) conferir sprite, tamanho e trajetoria no jogo.

## patch5.13 (ovo: rede visual) — ACUMULATIVO (inclui 5.12)
- [x] `NP_EGG` (net.inc, NET_VER 17 -> 18: **protocolo mudou**, host e convidados precisam do mesmo patch): quem arremessa transmite posicao e velocidade; os outros simulam o ovo so para ver (quebra com particulas ao bater em bloco, sem empurrar mob, sem chocar). Quem arremessa decide o choco localmente.
- Teste: `tests/test_egg.cpp` agora com 14 verificacoes (inclui `eggNetRecv`). chest e nether sem regressao.
- [ ] Falta: o choco de convidado cria galinha so no cliente dele (mobs em rede sao do host: mandar pedido ao host); ovo que acerta jogador remoto; som; filhote; P2 do split; Android. Nao testei o reencaminhamento do host para os outros convidados (usei o mesmo `netBroadcast` da flecha). (VIS) conferir em LAN com 2 instancias.

## patch5.14 (ovo: choco de convidado + P2 do split) — incremental sobre o a.zip (que ja tem 5.12/5.13)
- [x] `NP_EGGHATCH` (net.inc, NET_VER 18 -> 19: **protocolo mudou**): ovo de convidado que choca manda o pedido ao host, que cria a galinha (mobs sao do host e o sync replica). Host valida n 1..4, posicao no mundo e cooldown (anti-spam). Convidado nunca cria galinha local.
- [x] P2 do split (`splitscreen.inc`): gatilho de uso com ovo arremessa (mesma regra do P1: no ar, agachado ou em bloco nao interativo), cooldown proprio `S2.eggCd` (.5 s), consome do inventario do P2. `eggThrow` ganhou parametro `int& cd`.
- [x] Android/touch: toque ja cai em `actAt(SDL_BUTTON_RIGHT)` e ovo nao esta em `gTouchUseItem`, entao usa o mesmo caminho do clique (conferido por leitura, nao testado).
- Teste: `tests/test_egg.cpp` 21 verificacoes (+ mutacao: sem o pedido ao host = 5 falhas). chest/chest2/nether/arrow_pick sem regressao; craft_pack SKIP (sem o pack na maquina de teste).
- [ ] Falta (ovo): SOM (projeto sem audio); filhote (depende do item 8, hoje nasce galinha adulta, TODO em `eggHatchSpawn`); ovo acertando jogador remoto (vanilla: empurra sem dano); dispenser lancando ovo; convidado nao empurra/pisca mob no host (so visual local). (VIS) conferir em LAN com 2 instancias e no split com 2 controles.

## patch5.15 (pequenos 2, 4, 5, 6, 8) — incremental sobre patch5.14
- [x] Item 4: `invDropKey` — Q sobre a paleta do criativo solta 1, Ctrl+Q uma pilha (so no criativo).
- [x] Item 5: `main_loop.inc` — 1-9 com inventario aberto e mouse fora de slot nao muda mais `sel` (Java ignora).
- [x] Item 6: `invMouseDown` — shift+duplo clique (250 ms, mesmo slot) move todos os iguais do mesmo inventario (mochila/bau/craft/armadura), como `shiftClickedSlot` do GuiContainer.
- [x] Item 2: `redstone.inc rsMapItems` — pistao grudento no inventario era o sprite do topo do pistao normal; agora cubo com `piston_top_sticky` em cima.
- [x] Item 8: `bow.inc` — flecha recolhida empurra um `PickFx` (voa ate o jogador em 3 ticks, sprite do item flecha). Falta: modelo da flecha no voo, som. (VIS)
- Teste: `tests/test_inv_keys2.cpp` (9). inv_keys/chest/chest2/nether/egg/arrow_pick/craft_pack sem regressao.
- [ ] Item 1 (lava x tocha): ja existem tintas separadas (`lightTint`: tocha 0, lava 1); nao ficou claro o que esta errado — descrever (cor da luz? do bloco? do item?).
- [ ] Item 3 (drops): nao achei defeito por leitura (`drawDrops` usa `meshOf`, igual ao inventario); precisa saber qual item/bloco aparece quebrado.
- [ ] Item 7 (Q/F no touch): precisa de botoes novos no HUD touch; nao feito.

## patch5.16 (item 7: pick block com slot ocupado) — incremental sobre patch5.15
- [x] `main_loop.inc` lambda `pickPut`: (1) item ja no hotbar => seleciona; (2) hotbar com slot vazio => move o item atual para o vazio e traz o novo (da mochila ou criado no criativo); (3) sem vazio => substitui o slot atual. Vale para bloco e para ovo de mob.
- Teste: so sintaxe (a logica esta dentro do laco principal, sem teste headless). (VIS) conferir no jogo.
- [ ] Falta: item 8 (NBT de bau/shulker, entidades sem ovo, split, Android), item 3 (Q/F touch), VIS bau duplo/barras, 9, 11, 12, 14, 15 e os grandes.

## patch5.17 (item 8 parcial) — incremental sobre patch5.16
- [x] `main_loop.inc`: pick block no criativo em bote (`boatPick`/`boatItemId`) e carrinho (`cartPick`/`cartItemId`), pelo mesmo `pickPut` (hotbar/vazio/substitui).
- [x] Item 3 (Q/F touch): ja existia (`TA_DROP`/`TA_SWAP` em touch.inc + main_loop.inc linhas ~605); conferido por leitura, sem mudanca. (VIS) conferir no aparelho.
- Teste: so sintaxe + regressao headless sem falha. (VIS) mirar bote/carrinho no criativo e apertar o botao do meio.
- [ ] Falta (item 8): armor stand e quadro (nao ha entidade no projeto?), NBT de bau/shulker, split (P2) e Android (botao de pick no touch).

## patch5.18 (bau duplo, UV das laterais) — incremental sobre patch5.17
- [x] `entmodels.inc entChest`: nas metades de bau duplo (half 0/1) a parede de fora usava a regiao de textura trocada (o modelo vanilla e girado 180 em Y em relacao a este: a parede de fora da esquerda = regiao x-max, da direita = x-min). Agora oeste/leste das metades usam `wA/wB`/`eA/eB`; bau simples identico ao de antes (conferido: mesmos numeros).
- Teste: sintaxe + test_chest/test_chest2 sem regressao. Corrigido por raciocinio sobre o modelo vanilla; sem a textura normal_left/right aqui para conferir. (VIS)
- [ ] Falta (bau duplo, VIS): se ainda houver defeito, mandar print (qual face, esquerda/direita, aberto ou fechado). Culling entre as metades e tampa aberta nao foram revistos.

## patch5.19 (item 1: barras/paineis, parte 1) — incremental sobre patch5.18
- [x] Conferido `rsConnMask` (redstone.inc ~322): barras/paineis ligam em painel/painel colorido, cubo SOLID e costas de escada, como o 1.12.2; outline/colisao (~243) usam a mesma mascara; sem defeito por leitura.
- [x] `blockupdate.inc vilEdgeTick`: reconexao na borda de chunk so cobria cerca e painel; agora tambem muro e painel colorido.
- Teste: so sintaxe. (VIS) barras/paineis ao lado de blocos na borda de chunk e perto de vidro/gelo.
- [ ] Falta: (VIS) 1.12 exclui vidro/gelo/glowstone/lanterna do mar/pistao da conexao (isExcludedBlock); `rsFullCube` so exclui folha, abobora, melancia, beacon, cacto, shulker. Nao confirmado, nao alterado.
- [ ] Falta (lista): item 11a-f (flecha remota/rede), 8 (NBT bau/shulker, quadro, P2, pick touch), 9, 12, 14/15 e os grandes (16-24).

## patch5.20 (item 11a: flecha de jogador remoto recolhivel) — incremental sobre patch5.19
- [x] `bow.inc`: flecha remota nasce recolhivel (flag no pacote); qualquer jogador perto dela a recolhe (mesma regra do dono: addItem, ou so-criativo) e manda `NP_ARROW` tipo 2; os outros clientes removem a copia (`arrowNetTake`). O dono tambem avisa ao recolher, senao a copia remota ficaria duplicada.
- [x] `net.inc`: NET_VER 20. HANDOFF.md atualizado.
- Teste: sintaxe + test_arrow_pick (7) e test_egg (21) sem falha. Sem teste headless da troca de pacote. (VIS/LAN) 2 jogadores: atirar, outro passar na flecha cravada, ver sumir nos dois.
- [ ] Falta: 11b-f (estado "travada" na rede, flecha como item com som/animacao, cacto/lava/fogo, funil, P2 e Android). Flecha de esqueleto segue sem recolher; perguntar se prefere o comportamento novo.

## patch5.21 (item 11b: estado "travada" na rede) — incremental sobre patch5.20
- [x] `bow.inc`: ao cravar, a flecha do dono manda `NP_ARROW` tipo 3 (ponto + direcao); a copia remota mais proxima do mesmo dono cola ali (posicao, direcao, bloco, tremida). Antes cada cliente cravava por conta propria e podia divergir (ponto de recolha e tipo 2 desencontrados). Flecha de mob nao manda.
- [x] `net.inc`: NET_VER 21; HANDOFF.md atualizado.
- Teste: sintaxe + test_arrow_pick (7) e test_egg (21) sem falha; sem teste headless da troca de pacote. (VIS/LAN) 2 jogadores: flecha cravada igual nos dois.
- [ ] Falta: 11c-f (flecha como item com som/animacao, cacto/lava/fogo, funil, P2 e Android).

## patch5.22 (item 11f: P2 do split recolhe flecha; 11c-e analisados) — incremental sobre patch5.21
- [x] `bow.inc`: recolha extraida para `arrowTryPick` (mesma regra; o P1 continua no `arrowStep`) + `arrowsPickTick(P)`; `splitscreen.inc` chama no contexto do P2, logo o P2 recolhe flechas cravadas (as dele e as do P1) no inventario dele.
- [x] Decisao do usuario: flecha de esqueleto fica sem recolher, como no 1.12.2 (nada a fazer).
- 11c (flecha como item + som): flecha nao e entidade item (a cravada vira item so ao recolher, com animacao PICKS); nao ha audio no projeto. 11d/11e (cacto/lava/fogo, funil): so valem p/ item de flecha solto no chao, que ja usa o sistema geral de drops; nao e algo especifico da flecha. Nada alterado.
- Teste: sintaxe + test_arrow_pick (7), test_egg (21), test_chest sem falha; sem teste headless do P2. (VIS) split: P2 passar na flecha cravada.
- [ ] Falta (11): Android (botao/toque de arco ja usa o mesmo caminho; so conferir no aparelho). Depois: item 8, 9, 12, 14/15, 16-24.

## patch5.23 (item 8: botao de pick no touch) — incremental sobre patch5.22
- [x] `touch.inc`: novo `TA_PICK` + 4o botao na coluna lateral (`side[4]`, icone alvo `pPick`); `main_loop.inc` chama `actMain(SDL_BUTTON_MIDDLE)` (mesmo caminho do clique do meio).
- Teste: sintaxe + test_chest, test_egg, test_inv_keys2 sem falha. (VIS) icone e posicao do botao (coluna lateral agora com 4); conferir no aparelho que cabe na tela.
- [ ] Falta (8): NBT de bau/shulker no pick (precisa de dados por pilha: `Slot` so tem id/n/d e shulker ainda nao guarda conteudo; mudanca grande em save/rede/inventario), quadro/armor stand, P2 do split (pick via S2). Depois: 9, 12, 14/15, 16-24.

## patch5.24 (item 8: pick por controle, P1 e P2 do split) — incremental sobre patch5.23
- [x] `gamepad.inc`: nova acao `GA_PICK` ("Pegar Bloco", padrao GB_RS, mapeavel); `main_loop.inc`: P1 chama `actMain(SDL_BUTTON_MIDDLE)`; `splitscreen.inc`: `split2Pick` (hotbar > vazio > substitui; so bloco, sem mob/bote) chamado no P2.
- [x] Quadro / armor stand: nao existem no jogo (nada a fazer no pick).
- Teste: sintaxe + test_chest (42), test_egg (21), test_inv_keys2 (9), test_arrow_pick (7), 0 falhas; sem teste headless do P2. (VIS) tela de mapeamento de controle agora com 14 acoes (conferir se cabe); P2 pick em jogo.
- [ ] Falta (8): NBT de bau/shulker no pick (precisa de dados por pilha; mudanca grande), pick de mob/bote no P2. Depois: 9, 12, 14/15, 16-24.

## patch5.25 (item 9: revisao de nick e skin) — so documentacao, sem codigo
- [x] Conferido no codigo: nick (validacao, erro no launcher `screens.inc` ~142, unico, `/nick`, sync via NP_ROSTER, nametag, Tab, chat, nick do J2 `gP2Name`), skin (launcher, skin do J2, sync por pixels, remoto sem skin = `default.png` e nunca a do local) e times (`/team`, cor, ordem do Tab, nametag) ja existem. Item 9 considerado FEITO.
- Nao feito de proposito: "atravessar parede no mesmo time" (nao ha colisao jogador x jogador; nao existe no vanilla), fallback Alex (nao ha asset de skin slim no projeto; so `default.png`), "skin por conta" (nao ha contas; a skin fica no profile.cfg). Se quiser algum, diga qual e como.
- [ ] Falta: 12 (colisao), 14/15, 16-24, Android (conferir), item 8 (NBT de bau/shulker; pick de mob/bote no P2).

## patch5.26 (item 12, parte 1: teste das caixas de colisao) — incremental sobre patch5.25
- [x] Novo `tests/test_collision2.cpp` (14 verificacoes): cerca (caixa 1,5 de altura, poste 6/16..10/16, braco na ligacao, nao passa andando, pousa em cima a 1,5), painel (1 de altura, 2/16 de espessura), muro (poste, 1,5), fio de redstone nao bloqueia, degrau de 1 bloco bloqueia. Todas passam, sem mudar codigo do jogo: leitura de `redstone.inc case 18/19/21/22` bate com o 1.12.2.
- Teste: test_collision2 (14) e test_collision (86), 0 falhas.
- [ ] Falta (12): testes/conferencia de escada, laje, porta aberta/fechada, alcapao, agua corrente, pistao empurrando (precisam do encoding de meta de cada um); colisao com entidades separada da de blocos; (VIS) tudo no jogo real.

## patch5.27 (item 12, parte 2: laje e escada) — incremental sobre patch5.26
- [x] `tests/test_collision2.cpp` ampliado (agora cobre laje baixo/cima/dupla, andar sobre laje sem pular, escada de baixo nas 4 direcoes e de cima). Sem mudar codigo do jogo: bate com o 1.12.2.
- Achado: porta e alcapao (`redstone.inc case 13/15`) usam a caixa do MODELO do pack como colisao; sem o pack instalado (como no teste headless) ficam sem colisao. Nao testavel aqui; conferir no jogo com pack (VIS). Portao de cerca (case 14) fechado = 1,5 de altura, aberto = sem colisao, como no 1.12.2 (por leitura).
- Teste: test_collision2 e test_collision, 0 falhas.
- [ ] Falta (12): porta/alcapao com pack (VIS), agua corrente, pistao empurrando; colisao com entidades separada da de blocos.

## patch5.28 (item 12, parte 3: agua corrente) — incremental sobre patch5.27
- [x] `physics.inc`: `waterPush` agora segue `World.handleMaterialAcceleration`/`BlockLiquid.getFlow` do 1.12.2: vetor de cada celula normalizado antes de somar (antes celulas de gradiente maior pesavam mais), agua caindo ao lado de bloco cheio ganha corrente para baixo (normalizado + (0,-6,0)) e a soma tambem empurra em Y. Novo `flowFull`; `flowVector` (usado pelo bote) intacto.
- Aproximacao: "causesDownwardCurrent" considera so cubo cheio (gelo nao excluido: nao ha id de gelo; escada ja fica fora por nao ser cubo).
- Teste: `test_collision2` (41: correnteza .014 normalizada, queda junto a parede empurra para baixo, fora da agua nao empurra), test_collision, test_chest, test_nether, test_egg, test_arrow_pick, test_inv_keys2 sem falha. (VIS) nadar em correnteza e em cachoeira junto a parede.
- [ ] Falta (12): pistao empurrando jogador/entidades; colisao com entidades separada da de blocos; porta/alcapao com pack (VIS).

## patch5.29 (launcher, parte 1) — incremental sobre patch5.28
- [x] `screens.inc`: launcher redesenhado (tema escuro, fundo de terra e logo do projeto): lista de skins com miniatura do rosto (estilo 7), previa grande frente + costas (Braco fino/grosso), bloco de nome organizado (rotulo, contador n/16, Aa, status "Nome valido"/erro), botoes "Escolher arquivo..." e "Jogar". Duplo clique numa skin = Jogar.
- [x] Split screen e divisao removidos do launcher (so Esc em jogo / F12). Skin do J2 so aparece quando o launcher e aberto de dentro (Opcoes > Perfil e Skin).
- [x] `launcher.inc`: miniaturas (gSkinHead), `skinSelect`, `skinPickFile` (zenity/kdialog no Linux, janela do Windows via PowerShell; celular: avisa), skin arrastada com nome repetido e conteudo diferente vira `nome_2.png`, aviso `lmsg`.
- Teste: sintaxe + test_chest (42), test_egg (21), test_inv_keys2 (9), 0 falhas. (VIS) tudo do layout; (VIS) seletor de arquivo nao testado (sem GUI aqui).
- [ ] Falta (launcher parte 2): navegador de arquivos proprio para Android (sem zenity), escolher skin do J2 e ativar split pelo menu Esc com preview, conferir encaixe em telas pequenas. Depois: 12 (pistao/entidades), 8, 14/15, 16-24.

## patch5.30 (launcher, parte 2) — incremental sobre patch5.29
- [x] `screens.inc`: navegador de arquivos proprio (pagina "skinfile", lista estilo 8: pastas + PNGs, Subir pasta / Downloads / Inicio / Cancelar). Abre quando nao ha seletor do sistema (Android, Linux sem zenity/kdialog, falha do PowerShell). Comeca em Download (Android) ou ~/Downloads.
- [x] `launcher.inc`: `skinPickFile` devolve 1 ok / 0 cancelou / -1 sem seletor (usa `command -v` para distinguir cancelar de ausente).
- [x] Esc em jogo: com split ligado aparece "Skin do Jogador 2..." (`pause_menu.inc` PB_SKIN2, `main_loop.inc`, `splitscreen.inc` splitOn). Abre o launcher com flag `gLFirst=false`: previa mostra J1 e J2 lado a lado e a linha de skin do J2.
- Teste: sintaxe + test_chest (42), test_egg (21), test_inv_keys2 (9), 0 falhas. (VIS) navegador, botao novo do Esc, previa J1/J2. Android: leitura de /sdcard pode exigir permissao de armazenamento (pasta aparece vazia sem ela).
- [ ] Falta: nick do J2 no launcher; conferir telas pequenas; item 12 (pistao/entidades), 8, 14/15, 16-24.

## patch5.31 (launcher, layout) — incremental sobre patch5.30
- [x] `screens.inc`: corrigido o print: bonecos saiam do painel de previa e o nome cobria as pernas. Agora layout em `launcherLayout(sw, sh, first)`: largura 400 (310 em tela estreita), lista 48% + previa, bonecos centralizados acima do nome (barra do nome no rodape do painel), "chao" sob os pes, tipo do braco no cabecalho da previa, painel ate 128 de altura (boneco escala 3 em telas de 256 de altura), barra de rolagem na lista de skins, setas cima/baixo trocam a skin.
- Teste: sintaxe + test_chest (42), test_egg (21), test_inv_keys2 (9), 0 falhas. (VIS) conferir o novo print.
- [ ] Falta: nick do J2 no launcher; telas pequenas; item 12, 8, 14/15, 16-24.

## patch5.32 (launcher, resto) — incremental sobre patch5.31
- [x] `screens.inc`: nick do J2 no launcher (so quando aberto de dentro do jogo: linha "J2" campo de nome + skin < >). Valida igual ao J1 (`nickSanitize`), recusa nome igual ao do J1 (aviso vermelho, mantem o anterior), grava em profile.cfg (`nick2=`).
- [x] Teclado virtual aberto (`oskActive`): layout compacta (sem logo, lista baixa) para o campo de nome nao ficar escondido atras dele; com painel < 40 de altura os bonecos nao sao desenhados.
- Achado: item 12 "pistao empurrando": o pistao do jogo e so um cubo (redstone.inc case 1..5, SHAPE_CUBE), nao existe mecanica de extensao/empurrar. Fazer isso e um item novo e grande (estado estendido, cabeca, empurrar ate 12 blocos, grudento, entidades), nao um ajuste.
- Teste: sintaxe + test_chest (42), test_egg (21), test_inv_keys2 (9), 0 falhas. (VIS) linha do J2, teclado virtual.
- [ ] Falta: item 12 (colisao de entidades separada da de blocos; pistao precisa ser criado antes), 8 (NBT de bau/shulker), 14/15, 16-24.

## patch5.33 (item 15: /give para o jogador 2) — incremental sobre patch5.32
- [x] `chat_debug.inc` + `splitscreen.inc`: `/give @p2 <item> [qtd]` (ou o nick do J2) entrega no inventario do jogador 2 (`split2Give`: troca o inventario com `split2SwapInv`, `addItem`, troca de volta). So com split ligado; avisa "(inventario cheio)". /help atualizado.
- Teste: sintaxe + test_chest (42), test_egg (21), test_inv_keys2 (9), 0 falhas; sem teste headless do P2. (VIS) no jogo: ligar split, /give @p2 diamond 3, abrir inventario do J2.
- [ ] Falta: 14 (chat por jogador no split, teclado virtual no Android), 15 (dispositivo do P2, arco/bote/baú do P2), 8 (NBT de bau/shulker; pick de mob/bote no P2), pistao (mecanica nova) + colisao de entidades, 16-24.

## patch5.34 (item 15: dispositivo do J2) — incremental sobre patch5.33
- [x] `screens.inc`: launcher aberto de dentro do jogo com split ligado mostra qual dispositivo e de quem (0 controles: aviso vermelho "J2 sem controle"; 1: J1 teclado/mouse + J2 controle 1; 2+: J1 controle 1 + J2 controle 2), refletindo `gpPadFor`. Nao ha seletor manual de dispositivo (a regra automatica de gamepad.inc continua).
- Achado: o comentario do cabecalho de `splitscreen.inc` (linha 7, "sem arco/comida/bote/chest para o jogador 2") parece desatualizado: o codigo tem gBoat2, bowOn em SurvCtx e abertura de bau do J2 (~linha 147). Nao confirmei no jogo; conferir (VIS) arco/bote/bau do J2 antes de marcar como feito.
- Teste: sintaxe + test_chest (42), test_egg (21), test_inv_keys2 (9), 0 falhas. (VIS) texto do dispositivo.
- [ ] Falta: 14 (chat por jogador no split + teclado virtual Android; o J2 so tem controle, entao precisa do teclado virtual do gamepad), 15 (conferir arco/bote/bau do J2 no jogo), 8, pistao (mecanica nova) + colisao de entidades, 16-24.

## patch5.35 (item 14: chat do jogador 2) — incremental sobre patch5.34
- [x] Botao de chat (GA_CHAT) do controle do J2 abre o chat como J2: `splitscreen.inc` (`gP2ChatReq`, `chatNick`, `chatSend`), `main_loop.inc` (abre com `setChat`, `gChatAs2` zera ao fechar), `gamepad.inc` (`gChatAs2`; o teclado virtual do chat segue o controle do J2), `chat_debug.inc` (fala, /me e /say saem com o nick do J2).
- Limite: como convidado de LAN o host so aceita o nick do proprio convidado (`netChatOk`), entao a fala do J2 fica so local; como host/offline vai para todos. Comandos digitados pelo J2 rodam como no J1 (ex.: /tp usa o corpo do J1).
- Teste: sintaxe + test_chest (42), test_egg (21), test_inv_keys2 (9), 0 falhas; sem teste headless. (VIS) abrir chat pelo controle do J2, digitar no teclado virtual, ver "<nick2> msg".
- [ ] Falta: 14 (teclado virtual no Android: so conferir, usa o teclado do sistema), 15 (conferir arco/bote/bau do J2), 8, pistao (mecanica nova) + colisao de entidades, 16-24.

## patch5.36 (item 15: comentario do split) — incremental sobre patch5.35
- [x] `splitscreen.inc`: cabecalho corrigido (o codigo tem vida/fome, comida, arco, bote, bau e mobs mirando o J2). So texto.
- Teste: sintaxe. (VIS) conferir no jogo arco/bote/bau/comida do J2 antes de dar o item 15 por feito.
- [ ] Falta: 15 (conferir no jogo), 14 (Android), 8, pistao (decidir) + colisao de entidades, 16-24.

## patch5.37 (pistao, parte 1/5: modelo e forma) — incremental sobre patch5.36
- Plano: 1 modelo/forma | 2 energia (alavanca, bloco de redstone, botao, tocha) | 3 estender/recolher + empurrar ate 12 blocos (+grudento) | 4 entidades (jogador, mobs, itens) | 5 rede/save/animacao.
- Estado no meta de XB_PIST (975) / grudento (976): bits0-2 dir6, bit3 = CABECA (placa 4px + haste, bloco a frente), bit4 = CORPO estendido (12px). Nao ha id novo (1016+ e o pack).
- [x] `redstone.inc`: modelo (rsBuild) e colisao/selecao (rsShape, antes cubo fixo) do corpo estendido e da cabeca; cabeca nao dropa. Pistao normal continua cubo.
- Teste: sintaxe + test_chest 42, test_egg 21, test_inv_keys2 9, test_collision2 41, 0 falhas. (VIS) ainda nao da para estender: so o modelo existe (testar com /setblock se houver).
- [ ] Falta: partes 2 a 5 do pistao; item 14/15 (conferir), 8, 16-24.

## patch5.38 (pistao, partes 2+3: energia, estender/recolher, empurrar, jogador) — incremental sobre patch5.37
- [x] `redstone.inc` (fim): pistonClass (movel / quebra / imovel), pistonPowered (alavanca ligada, tocha de redstone, bloco de redstone so com pack; tambem via fonte encostada em UM bloco solido vizinho; a frente nao conta), pistonExtend (ate 12 blocos, 13 falha; imovel = bedrock, obsidiana, portal, bau, funil, dispenser/dropper, cama, pistao estendido; planta/tocha/fio/liquido/fogo somem), pistonRetract (grudento puxa 1 bloco), pistonCheck, pistonScan (reavalia pistoes a ate 2 blocos). Instantaneo, sem animacao nem tick.
- Ganchos: `blockupdate.inc` xbNeighbor chama pistonCheck; `models.inc` xbToggleLever chama pistonScan; rsPlace checa o pistao recem colocado. Cabeca sem corpo some; corpo cuja cabeca quebrou volta ao normal. Jogador local (gPvpP) e empurrado para fora da celula ocupada.
- Teste novo: tests/test_piston.cpp (25 verificacoes) + test_chest 42, test_egg 21, test_inv_keys2 9, test_collision2 41, 0 falhas.
- Limites: sem botao/placa/poeira (nao ha propagacao de energia no jogo); alavanca vizinha de bloco nao confere a face de fixacao; sem animacao; mobs, itens soltos e J2 nao sao empurrados; sem pacote de rede novo (blocos vao por netSendBlock).
- [ ] Falta: pistao parte 4 (mobs/itens/J2 empurrados), parte 5 (animacao, som nao existe), botao/placa/poeira; itens 14/15 (conferir), 8, 16-24. (VIS) tudo do pistao no jogo.

## patch5.39 (pistao, parte 4: entidades) — incremental sobre patch5.38
- [x] `redstone.inc`: pistonPushPt/pistonPushPlayer empurram para fora da celula ocupada: jogador 1, jogador 2 (`pistonP2()` em splitscreen.inc), mobs vivos (MOBS) e itens soltos (DROPS), na direcao do movimento. Botes/carrinhos nao.
- Teste: tests/test_piston.cpp (27 verificacoes) + test_chest 42, test_egg 21, 0 falhas. (VIS) mob/item/J2 sendo empurrados.
- [ ] Falta: pistao animacao; botao/placa/poeira (sem propagacao de energia no jogo); itens 14/15 (conferir), 8, 12 (colisao de entidades), 16-24.

## patch5.40 (pistao, botao) — incremental sobre patch5.39
- [x] `redstone.inc`: botao aperta (rsUse, bit4 do meta) e solta sozinho via scheduleTick (pedra 20, madeira 30 ticks; `rsTick`, ligado em `blockupdate.inc` com h.tick); botao apertado e fonte de energia do pistao (pistonSrc agora le o meta). Rede: netSendBlock.
- Teste: test_piston 30 verificacoes, test_chest 42, test_egg 21, 0 falhas. (VIS) clicar no botao ao lado do pistao; modelo apertado.
- [ ] Falta: placa de pressao (precisa detectar entidades por tick), poeira (propagacao), animacao do pistao; 14/15 (conferir), 8, 12, 16-24.

## patch5.41 (pistao, placa de pressao) — incremental sobre patch5.40
- [x] `redstone.inc`: placa de pressao (bit4 do meta = apertada) aperta com jogador 1/2 e mobs (madeira tambem itens soltos; pedra ignora itens) via `rsPlatesTick` (chamado em runWorldTick, `blockupdate.inc`, so host/offline) e solta por scheduleTick de 10 em 10 ticks quando ninguem esta em cima (`rsTick`). Placa apertada e fonte do pistao.
- Teste: test_piston 34 verificacoes, test_chest 42, test_egg 21, 0 falhas. (VIS) andar sobre a placa ao lado do pistao; modelo apertado. Detecta pelo centro da entidade (nao pela caixa toda).
- [ ] Falta: poeira (propagacao), animacao do pistao; 14/15 (conferir), 8, 12, 16-24.

## patch5.42 (poeira de redstone) — incremental sobre patch5.41
- [x] `redstone.inc`: `rsDustRecalc` propaga energia pela rede de poeira (fontes: alavanca ligada, tocha, bloco de redstone (pack), botao/placa apertados; nivel 15 ao lado da fonte, -1 por poeira, sobe/desce degraus como rsDustConn); nivel nos bits 4-7 do meta (cor da poeira); rede de ate 600 celulas. Dispara em: rsDustFix (vizinho mudou), colocar poeira, pistonScan (fonte mudou). Poeira com nivel > 0 liga o pistao encostado nela.
- Teste: test_piston 39 verificacoes (alavanca + 5 fios + pistao: niveis 15..11, estende, desliga e recolhe), test_chest 42, test_egg 21, test_inv_keys2 9, test_collision2 41, 0 falhas. (VIS) cor da poeira por nivel; rede grande.
- Limites: poeira so liga pistao encostado (nao "aponta"); sem repetidor/comparador/tocha invertendo/observador; sem energia por bloco solido (poeira->bloco->pistao so via vizinho do bloco).
- [ ] Falta: repetidor/tocha de redstone invertida (precisam de tick), animacao do pistao; 14/15 (conferir), 8, 12, 16-24.

## patch5.43 (repetidor de redstone) — incremental sobre patch5.42
- [x] `redstone.inc`: repetidor com atraso (meta: dir de saida 0-3 N L S O | atraso<<2 | ligado<<4; clique direito muda o atraso, ja existia): entrada = celula de tras (fonte, poeira com nivel > 0, ou outro repetidor ligado apontando para ele); liga/desliga apos (1+atraso)*2 ticks (scheduleTick, rsTick); saida = nivel 15 na poeira da frente, liga pistao da frente, encadeia com repetidor da frente. Dispara por vizinho (xbNeighbor em `blockupdate.inc`), poeira mudando, fonte mudando (pistonScan) e ao colocar.
- Teste: test_piston 46 verificacoes (alavanca -> repetidor -> poeira -> pistao, liga e desliga), test_chest 42, test_egg 21, 0 falhas. (VIS) direcao da saida: assumi saida = direcao para onde o jogador olha ao colocar (meta = pf) e o modelo com tocha fixa na saida; conferir no jogo, inclusive o modelo ligado/desligado.
- Sem: travamento (lock), comparador, tocha de redstone invertida (so existem 5 ids de tocha, sem modelo apagado), observador.
- [ ] Falta: tocha invertida (precisa de ids/modelo apagado), animacao do pistao; 14/15 (conferir), 8, 12, 16-24.

## patch5.44 (tocha de redstone invertida) — incremental sobre patch5.43
- [x] `redstone.inc`: tocha de redstone (ids XB_RTORCH..+4) acende a menos que o bloco onde esta presa (`rsTorchAttach`) receba energia (`rsBlockPowered`: alavanca/botao/placa/bloco de redstone/poeira com nivel/saida de repetidor vizinhos ao bloco, ou tocha de pe logo abaixo dele); atraso de 2 ticks (scheduleTick, rsTick); estado no meta bit0 = APAGADA; apagada nao e fonte (rsStrongSrc, pistonSrc). Ao mudar reavalia tochas a ate 2 blocos, poeira, repetidores e pistoes (`rsTorchNotify`). Gatilhos: vizinho (`xbNeighbor`), colocar (hook added `rsAddedXb`), poeira mudando, fonte mudando.
- [x] `models.inc`: modelo apagado (`gRtOff`, mesmos modelos do pack com textura redstone_torch_off) escolhido em emitXB pelo meta bit0.
- Teste: test_piston 51 verificacoes (tocha inverte, apaga e acende o pistao), test_chest 42, test_egg 21, 0 falhas. (VIS) textura apagada da tocha de pe e de parede; atualizar a malha do chunk quando o meta muda (usei pistonPutB/setBM como o resto do redstone; conferir se a tocha redesenha).
- Limites: energia bloco->poeira (poeira ao lado de bloco energizado) e bloco->repetidor nao existem; luz da tocha apagada ainda emite como acesa (lightEmit so olha o id); minecart.inc ainda trata toda tocha como ligada.
- [ ] Falta: energia por bloco solido (bloco energizado liga poeira/repetidor/pistao), luz da tocha apagada, animacao do pistao; 14/15 (conferir), 8, 12, 16-24.

## patch5.45 (energia por bloco solido) — incremental sobre patch5.44
- [x] `redstone.inc`: `rsStrongBlock` (bloco solido fortemente energizado: tocha acesa de pe logo abaixo ou repetidor ligado apontando para ele) semeia a poeira vizinha com nivel 15; repetidor aceita entrada de bloco solido energizado atras dele (`rsBlockPowered`); pistao tambem liga por bloco energizado por repetidor; `pistonScan` reavalia poeira/repetidor ao lado de blocos vizinhos da fonte; repetidor que muda chama `pistonScan` na celula da frente quando e bloco solido.
- Teste: test_piston 62 verificacoes (tocha->bloco->poeira->pistao, repetidor->bloco->poeira->pistao, poeira->bloco->repetidor), test_chest 42, test_egg 21, test_inv_keys2 9, test_collision2 41, 0 falhas. (VIS) tudo no jogo.
- Limites: alavanca/botao presos ao bloco ainda nao o energizam como "forte" para a poeira; bloco energizado so por poeira (fraco) nao liga poeira (correto no 1.12); sem comparador/observador/travamento.
- [ ] Falta: luz da tocha apagada (lightEmit so olha o id), minecart.inc tratando toda tocha como ligada, animacao do pistao, rede, save; 14/15 (conferir), 8, 12, 16-24.

## patch5.46 (luz da tocha de redstone apagada) — incremental sobre patch5.45
- [x] `world.inc`: `computeLight` ignora tocha de redstone apagada (meta bit0) como emissor (`rUnlit`), nas duas passadas (luz geral e cores misturadas); `setBM` refaz a luz dos 8 chunks vizinhos quando so o bit acesa/apagada muda.
- Teste: test_piston 65 verificacoes (tocha acesa = 7 e propaga 6, apagada = 0, acende de novo), test_chest 42, test_egg 21, test_nether, 0 falhas. (VIS) quarto escurecendo ao apagar a tocha; item da tocha na mao nao muda.
- [ ] Falta: minecart.inc tratando toda tocha como ligada, animacao do pistao, rede, save; 14/15 (conferir), 8, 12, 16-24.

## patch5.47 (trilho x tocha de redstone apagada) — incremental sobre patch5.46
- [x] `minecart.inc railSrcNear`: tocha de redstone so energiza o trilho se acesa (meta bit0 = apagada); antes qualquer tocha contava.
- Teste: test_minecart 24 verificacoes (acesa energiza, apagada nao), test_piston 65, test_chest 42, 0 falhas. (VIS) trilho energizado por tocha invertida.
- Limites: trilho ainda ignora poeira, botao e placa como fonte (so alavanca e tocha).
- [ ] Falta: animacao do pistao, rede, save; trilho com poeira/botao/placa; 14/15 (conferir), 8, 12, 16-24.

## patch5.48 (trilho energizado por poeira, botao e placa) — incremental sobre patch5.47
- [x] `minecart.inc railSrcNear`: alem de alavanca e tocha acesa, poeira com nivel > 0 e botao/placa apertados energizam o trilho (a vizinhanca ja chama `railNeighbor` quando a fonte encostada muda de meta).
- Teste: test_minecart 31 verificacoes (poeira, botao, placa; trilho liga ao apertar o botao), test_chest 42, test_nether 310, test_piston 65, 0 falhas. (VIS) botao/placa/poeira ao lado do trilho energizado.
- Limites: fonte na diagonal abaixo do trilho (y-1) so atualiza o trilho quando algo encostado muda; repetidor e bloco energizado ainda nao energizam trilho.
- [ ] Falta: animacao do pistao (precisa de entidade de bloco em movimento, nao existe), rede e save (ticks agendados nao sao salvos); 14/15 (conferir), 8, 12, 16-24.

## patch5.49 (save: botao/placa apertados) — incremental sobre patch5.48
- [x] `world.inc genImpl` (chunk vindo do save) chama `rsChunkLoaded` (`blockupdate.inc`): botao/placa com bit4 (apertado) reagendam a soltura (botao 20, placa 10 ticks), porque ticks nao sao salvos e o botao voltava preso para sempre.
- Teste: test_piston 71 verificacoes (chunk salvo com botao e placa apertados + botao solto: so os apertados reagendam; soltam depois de 40 ticks), test_chest 42, test_egg 21, 0 falhas. test_player_save (da lista de ignorados) segue dando segfault, nao investigado. (VIS) apertar botao, salvar e sair, recarregar: solta sozinho.
- Limites: repetidor e tocha de redstone no meio de um atraso ao salvar nao reagendam (estado so se corrige quando um vizinho muda); pistao nao tem estado pendente. Convidado em rede nao roda ticks.
- [ ] Falta: animacao do pistao (entidade de bloco em movimento), rede (pacote de pistao nao ha: usa netSendBlock); 14/15 (conferir), 8, 12, 16-24.

## patch5.50 (alavanca em bloco, travamento do repetidor, trilho por bloco energizado) — incremental sobre patch5.49
- [x] `redstone.inc rsStrongBlock`: alavanca ligada presa ao bloco (parede, chao ou teto) o energiza forte: liga poeira vizinha ao bloco (e pistao/repetidor via as regras existentes).
- [x] `redstone.inc rsRepLocked`: repetidor ligado ao lado apontando para outro repetidor o trava (nao muda de estado enquanto durar); `rsRepCheck` e `rsTick` respeitam; ao destravar o repetidor reavalia a entrada.
- [x] `minecart.inc railSrcNear`: bloco vizinho fortemente energizado (tocha sob ele, repetidor, alavanca presa) energiza o trilho.
- Teste: test_piston 78 (alavanca->bloco->poeira->pistao; travamento ligado/desligado), test_minecart 33, test_chest 42, test_egg, test_collision2, test_nether, test_arrow_pick, 0 falhas. (VIS) sem textura/modelo de repetidor travado (barra lateral do 1.12); alavanca em bloco ao lado de poeira.
- Limites: botao preso ao bloco ainda nao o energiza forte (nao confirmei a convencao do meta do botao); sem comparador/observador.
- [ ] Falta: animacao do pistao, visual do repetidor travado; NBT de bau no pick; 14/15 (conferir), 8, 12, 16-24.

## patch5.51 (botao e placa energizam o bloco) — incremental sobre patch5.50
- [x] `redstone.inc rsStrongBlock`: botao apertado preso ao bloco (meta: 0 chao, 1 teto, 2..5 parede = dir+2, igual ao `rsSupported`) e placa apertada em cima do bloco o energizam forte: poeira ao lado do bloco, pistao e trilho (via `railSrcNear`) passam a ligar.
- Teste: test_piston 88 (botao de parede -> bloco -> poeira -> pistao, solta no tick, botao em outra face nao energiza, placa em cima do bloco liga e desliga), test_minecart 33, test_chest 42, test_egg 21, test_collision2 41, test_nether 310, 0 falhas. (VIS) botao na parede de um bloco ao lado de poeira.
- Limites: dedução da face do botao vem de `rsSupported` (nao conferi com o modelo do pack); comparador e observador nao existem.
- [ ] Falta: animacao do pistao, visual do repetidor travado; NBT de bau no pick; 14/15 (conferir), 8, 12, 16-24.

## patch5.52 (observador de redstone) — incremental sobre patch5.51
- [x] `redstone.inc`: observador (XB_OBS, meta: dir 0-5 igual ao pistao = FRENTE/rosto, bit4 = pulso): detecta mudanca de id/meta na celula da frente (`rsObsNeighbor`, assinatura em `gObsSig`, registrada ao colocar; nao e salva, o 1o evento so registra), liga 2 ticks depois e fica 2 ticks ligado, empurrando a celula de TRAS (`rsObsOut`, via `rsRepOutInto`: poeira, pistao direto ou por bloco, repetidor atras, `rsStrongBlock`). `blockupdate.inc xbNeighbor/rsAddedXb` chamam o observador antes do corte de blocos solidos; `rsTick` despacha `rsObsTick`.
- Teste: test_piston 101 (muda frente/remove pulsa, 2+2 ticks, lateral nao conta, sem repulso, alimenta bloco atras -> poeira -> pistao), test_minecart 33, test_chest 42, test_egg 21, test_collision2 41, test_nether 310, 0 falhas.
- Assuncao (VIS): frente = lado do rosto (`observer_front`) na direcao da meta de colocacao do projeto ("frente voltada para o jogador"); se no jogo o rosto ficar para o lado errado, inverter `PS_*[d]` em `rsObsSeen/rsObsOut`. Mudanca de meta so visual (ex.: poeira mudando nivel) tambem pulsa, como no 1.12.
- Limites: sem save da assinatura; trilho nao le o observador direto (so por bloco); sem comparador.
- [ ] Falta: animacao do pistao, visual do repetidor travado e do observador ligado (textura on); NBT de bau no pick; 14/15 (conferir), 8, 12, 16-24.

## patch5.53 (trilho le repetidor/observador, repetidor travado, observador ligado) — incremental sobre patch5.52
- [x] `minecart.inc railSrcNear`: repetidor ligado apontando para o trilho e observador ligado com o trilho atras (via `rsRepOutInto`) energizam o trilho direto, sem bloco no meio.
- [x] `redstone.inc rsRepLockSync`: bit 5 do meta do repetidor = travado (so visual); sincronizado em `rsRepCheck` e no `rsTick` (setBM refaz a malha). Modelo: barra de bedrock (2,2,7)-(14,4,9) quando travado.
- [x] `redstone.inc rsBuild` observador: com pulso (bit4) a face de tras usa `block/observer_back_on`.
- Teste: test_piston 103 (bit5 liga/desliga com o travamento), test_minecart 39 (repetidor certo/errado/desligado, observador com costas no trilho e pulso acabando), test_chest 42, test_egg 21, test_collision2 41, test_nether 310, test_arrow_pick 7, test_inv_keys2 9, 0 falhas.
- Limites: modelo da barra e textura `observer_back_on` sem teste headless (sem pack); se o pack nao tiver `observer_back_on` a face fica sem textura (VIS). Repetidor travado salvo: bit5 vem do save; so corrige quando um vizinho muda.
- [ ] Falta: animacao do pistao (entidade de bloco em movimento), comparador (logica), reagendar repetidor/tocha no meio de atraso ao carregar chunk; NBT de bau no pick; 14/15 (conferir), 8, 12, 16-24.

## patch5.54 (comparador, reagendar atrasos ao carregar chunk) — incremental sobre patch5.53
- [x] `redstone.inc` comparador (XB_COMP; meta: dir 0-3 | modo<<2 (1 = subtrair, ja alternava com o clique) | ligado<<3 | forca 0-15 nos bits 4-7): `rsSigInto` (forca entregue por poeira/repetidor/observador/comparador/fonte/bloco energizado), `rsCompOutVal` (comparar: traseira >= lateral; subtrair: traseira - lateral; laterais so poeira/repetidor/comparador/observador), `rsCompCheck`/`rsCompTick` (atraso 2 ticks), `rsRepOutInto` aceita comparador (alimenta poeira, pistao, repetidor, bloco, trilho). `rsRepCheck` despacha para o comparador; hooks em `blockupdate.inc` e no recalculo de poeira.
- [x] `blockupdate.inc rsChunkLoaded`: repetidor, tocha de redstone e comparador de chunk salvo reagendam a reavaliacao; `rsTick` ignora se algum dos 4 chunks vizinhos nao esta carregado (nao apaga por entrada desconhecida).
- Teste: test_piston 119 (comparador: sem entrada, liga, subtrair com e sem lateral, comparar com lateral, apaga, liga pistao; chunk com repetidor/tocha velhos reavaliam; borda de chunk nao carregado nao decide), test_minecart 39, test_chest 42, test_egg 21, test_nether 310, test_collision2 41, test_arrow_pick 7, test_inv_keys2 9, 0 falhas.
- Limites: comparador nao le conteudo de bau/fornalha (so sinal de redstone); saida para a poeira vai com forca 15 (a forca fica so no meta do comparador e na conta de subtrair); sem modo de medir bloco/caixa; poeira so entrega ao comparador pela forca do nivel; repetidor de borda de chunk com vizinho que carrega depois so corrige quando um vizinho muda. (VIS) modelo do comparador nao revisado no jogo.
- [ ] Falta: animacao do pistao (entidade de bloco em movimento); comparador lendo bau (precisa de contagem de itens por bloco); rede: conferir sync de observador/poeira/comparador no convidado; NBT de bau no pick; 14/15 (conferir), 8, 12, 16-24.

## patch5.55 (comparador le bau, conferencia de rede) — incremental sobre patch5.54
- [x] `chest.inc chestSignal`: saida do comparador = 0 vazio, senao floor(1 + enchimento*14) (soma de n/maxStack por slot; bau duplo soma as duas metades; -1 se nao e bau que guarda itens). `redstone.inc rsSigInto` usa quando a celula de TRAS do comparador e um bau.
- [x] `chest.inc chestSignalChanged`: comparadores vizinhos reavaliam quando o conteudo muda (a cada quadro com a tela aberta, ao fechar; so host/offline).
- [x] Conferido por leitura do codigo: toda escrita de bloco de redstone (poeira, repetidor, tocha, observador, comparador, botao, placa, pistao) passa por `pistonPutB`/`netSendBlock`; nao ha pacote novo (NET_VER continua 21).
- Teste: test_piston 128 (bau vazio 0, 1 item 1, metade 8, cheio 15, comparador copia o nivel para a poeira, esvaziar apaga, nao-bau -1), test_chest 42, test_chest2 43, test_minecart 39, test_egg 21, test_nether 310, test_collision2 41, test_arrow_pick 7, test_inv_keys2 9, 0 falhas.
- Limites: so bau (nao ha fornalha/funil/dispensador com conteudo); bau de ender e shulker nao contam; comparador nao le atraves de bloco solido; a saida para poeira continua com forca 15; rede nao testada com convidado real (VIS: observador/poeira/comparador no convidado).
- [ ] Falta: animacao do pistao (entidade de bloco em movimento); rede: conferir no jogo com convidado; NBT de bau no pick; 14/15 (conferir), 8, 12, 16-24.

## patch5.56 (animacao do pistao: blocos deslizando) — incremental sobre patch5.55
- [x] `world.inc`: `PAnim`/`gPAnim` (destino, direcao, id, meta, deslocamento inicial/final, `hide`); `pAnimHides`; `build()` pula a celula escondida (o bloco real ja esta no mundo, so fica fora da malha).
- [x] `redstone.inc pAnimAdd`: `pistonExtend` anima cada bloco empurrado e a cabeca (-1 -> 0); `pistonRetract` anima a cabeca voltando (0 -> -1, sem esconder) e o bloco puxado do grudento (+1 -> 0). So desenha cubo comum e modelos de redstone (`isRs`); outros modelos (plantas etc.) ficam sem animacao.
- [x] `blockupdate.inc pAnimExpire` (em `runWorldTick`): dura 2 ticks; ao acabar refaz a malha dos chunks em volta (`dirtyC`).
- [x] `redstone.inc drawPistonAnims` + `main_loop.inc`: bloco desenhado sem culling, com luz de entidade (`entLightAt`), deslizando em ~100 ms (relogio comeca no 1o quadro desenhado: `SDL_GetTicks` nao existe nos testes headless e derruba o teste).
- Teste: test_piston 142 (estender empurrando 2 blocos = 3 deslizes que escondem o destino e expiram em 2 ticks; recolher = cabeca sem esconder; grudento = bloco puxado + cabeca), test_minecart 39, test_chest 42, test_chest2 43, test_egg 21, test_nether 310, test_collision2 41, test_arrow_pick 7, test_inv_keys2 9, 0 falhas. Sintaxe do projeto completa OK.
- Limites (VIS, nada disto foi visto no jogo): tinta de bioma (grama/folhas) nao aparece no bloco deslizando; faces vizinhas da celula escondida ficam sem desenhar por ~100 ms (pode aparecer um buraco); a cabeca do pistao recolhida e o corpo trocam de modelo na hora (o corpo nao anima); o convidado em rede nao anima (ticks so rodam no host); mapeamento de faces do cubo (topo/lado/base) de bloco do pack nao conferido.
- [ ] Falta: conferir a animacao no jogo (tempo, buraco nas vizinhas, orientacao); NBT de bau no pick; 14/15 (conferir), 8, 12, 16-24.

## patch5.57a (NBT por pilha + pick de bau com conteudo) — incremental sobre patch5.56
- [x] `inventory_ui.inc`: `Slot` ganhou `std::string nbt` (vazio = nenhum); `sameStack()` (item + NBT iguais). Empilhar, dividir (clique direito), arrastar, shift-click, duplo clique, `addItem(id, n, nbt)`, `returnCursor` e Q respeitam/levam o NBT. Novo `invPickPut` (era a lambda `pickPut` do `main_loop.inc`): hotbar/mochila so casam com o MESMO NBT (bau cheio != bau vazio).
- [x] `chest.inc`: `chestNbtPack`/`chestNbtUnpack` (texto compacto `BET1|slot,n,tam:chave tam:nbt...`, recursivo: bau dentro de bau; equivale a `BlockEntityTag:{Items:[...]}`), `chestPickNbt` (so bau e bau armadilhado; Ender e shulker nao), `chestApplyNbt` (recria o conteudo ao colocar). `chestAdd`/`chestEq` consideram o NBT; quebrar o bau solta as pilhas com NBT.
- [x] Pick: Ctrl + botao do meio no bau copia o conteudo (criativo cria 1 bau com o conteudo; sobrevivencia so acha pilha ja existente com o mesmo NBT). Sem Ctrl, touch e controle = bau vazio. Colocar o item (`rsPlace` + `gPlaceNbt`) recria o conteudo no bau novo; cada metade do bau duplo copia so a propria.
- [x] Save: `player.dat` e `chests.dat`/`ender.dat` ganharam linha opcional `x <nbt>` logo depois da pilha (saves antigos carregam igual; buffers de leitura subiram de 600/1024 para 128 KB; marcador `?hash` nunca e gravado).
- [x] Rede (NET_VER 22): novo `NP_NBT` (texto do NBT em fragmentos de 1000 bytes: hash, tamanho, deslocamento, tamanho do pedaco); `NP_CHEST`/`NP_ENDER` ganharam cauda opcional no FIM (u8 qtd + u8 slot + u32 hash por pilha com NBT) e `NP_DROP` um u32 de hash no fim. Cache hash -> texto nos dois lados; hash desconhecido (pacote perdido) vira o marcador `?hash`, que volta pelo mesmo hash e o dono do texto reconhece (nao apaga o conteudo). Texto adulterado (hash nao confere) e descartado.
- Teste novo `tests/test_pick_nbt.cpp` (103 verificacoes, 0 falhas): slot_serialize_nbt (texto, player.dat com 27 baus cheios dentro de um bau, save antigo, chests.dat), slot_packet_nbt (fragmentos fora de ordem/duplicados/adulterados, cauda do NP_CHEST, NP_DROP, spawnDrop), slot_stack_nbt (empilhar/clicar/dividir/shift/cursor/chestAdd/quebrar bau), pick_chest (copiar, hotbar com mesmo NBT, criativo x sobrevivencia, colocar via `rsPlace`, armadilhado, Ender/shulker fora, duplo), pick_chest_empty.
- Regressao (0 falhas): test_chest 44, test_chest2 49, test_inv_keys 9, test_inv_keys2 9, test_piston 142, test_minecart 39, test_egg 21, test_nether 310, test_collision2 41, test_arrow_pick 7, test_mobsync 25, test_boat 36. Sintaxe do projeto completa OK. `test_player_save` e `test_split` dao segfault em `randomSeed()` (localtime) TAMBEM no codigo do patch5.56 (problema do ambiente de teste, nao deste patch); nao deu para rodar.
- Limites: o pick por Ctrl so existe no teclado/mouse (touch e controle nao copiam); convidado em rede nao coloca bau com conteudo (nao guarda conteudo; a colocacao e recusada para nao perder os itens); sem tooltip com a lista de itens do bau (VIS: conferir); rede com convidado real nao testada (so os pacotes, por teste); item de bau com conteudo empilha como o bau comum (64, so com NBT igual).
- (VIS) Ctrl + botao do meio num bau cheio no criativo: o item vem com o conteudo, colocar recria o conteudo; sem Ctrl vem vazio; quebrar o bau cheio solta os itens; guardar o bau cheio dentro de outro bau e reabrir o jogo.
- [ ] AINDA FALTA: **5.57b** shulker com inventario (tela, save, rede, quebrar vira item com conteudo, empilha em 1) + pick de shulker; **5.58** forca real do comparador na poeira (hoje sai sempre 15); **5.59** repetidor de borda de chunk com vizinho que carrega depois; **5.60** itens 12 e 14/15 (conferir no jogo); **5.61** itens 16-24; **5.62** conferir a animacao do pistao no jogo (5.56, VIS). Depois: comparador lendo shulker/funil/fornalha quando existirem.
