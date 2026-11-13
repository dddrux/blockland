- patchPerf.5: world.inc computeLight (early-exit do ceu, emissores), main_loop.inc frustum por chunk com maxY. Sintaxe OK
- patchLOD.4: lod.inc lodLight; world.inc (LOD sem computeLight/conn/laco de voxels/espera de vizinhos). Build LOD 0,62->0,14 ms. Sintaxe OK
- patchLOD.3: lod.inc saia de 2 blocos nas bordas do chunk LOD; tests/test_lod.cpp 364/0. Sintaxe OK
- patchLOD.2: lod.inc lodWater (agua distante fundida, UV constante), world.inc (LOD pula o laco de voxels, chama lodWater); tests/test_lod.cpp 322/0. Sintaxe OK
- patchLOD.1: lod.inc (malha greedy por campo de alturas, passo 2/4), world.inc (Chunk::lodL, gMeshLod, stream/build), assets_atlas.inc (tile branco), main_loop.inc (raios LOD); tests/test_lod.cpp 117/0. Sintaxe OK; test_bfc 22/22, test_cave 14/0; test_shapes 337/4 e test_meta segfault iguais ao original
- patchWolf.1 (v1.1.0.106rc): mobs.inc M_WOLF + MD (ai 3) + ModelWolf + mordida dano 2 + alerta so de mesma especie + ovo; audio.inc T[19]; hostile.inc spawn floresta/taiga; chat_debug.inc /lobo /wolf; tests/test_wolf.cpp 6/0. Sintaxe OK
- patchOcelot.1 (v1.1.0.105rc): mobs.inc M_OCELOT + MD + modelo + fuga (AvoidEntity 7 blocos) + ovo; audio.inc T[18]; hostile.inc spawn em B_JUNGLE; chat_debug.inc /jaguatirica /ocelot; tests/test_ocelot.cpp 4/0. Sintaxe OK
- patchRabbit.1 (v1.1.0.104rc): mobs.inc enum M_RABBIT + MD + modelo + salto no tickMob + drops + ovo; audio.inc T[17]; hostile.inc spawn natural; chat_debug.inc /coelho /rabbit; tests/test_rabbit.cpp 7/0; mooshroom/breed/audio ok. Sintaxe OK
- patchFix.28 (v1.1.0.103rc): mining_sky.inc Part ganha cr/cg/cb (cor base) e tickParts recalcula r/g/b = base x lightNear(posicao atual) com partFloor (piso Nether/End); main.cpp versao; tests/test_cdp.cpp stub. Sintaxe OK
- patchFix.27 (v1.1.0.102rc): dragon.inc dragonTick: orbita com alvo a frente, aceleracao <= .05, giro <= 4 graus/tick, freio nas fases 3/5, piso y 66; tests/test_dragonmove.cpp 2/0. Sintaxe OK
- patchFix.26 (v1.1.0.101rc): dragon.inc: dragonTop/dragonEggSpawn(first,active) (fonte inativa desde o inicio, portal+ovo so na morte), morte (sobe .1, gira 20/tick, explosoes, coluna roxa), dragonBar (visual + 96 blocos); tests/test_dragonfx.cpp 8/0. Sintaxe OK
- patchFix.25 (v1.1.0.100rc): sky_cycle.inc entLightAt: End (gDim==2) = piso neutro .92 + gEntSk 15; main_loop.inc: overlay escuro cheio com o olho em bloco opaco (antes do HUD); tests/test_endlight.cpp 2/0; test_unstuck 7/0, test_collision 86/0. Sintaxe OK
- patchFix.24 (v1.1.0.99rc): physics.inc unstuckPlayer: so age com cubo cheio na caixa encolhida .01 e passo <= .1 por chamada (deep continua instantaneo); tests/test_unstuck.cpp +2 casos; tests/test_collision.cpp itera ate livre. test_unstuck 7/0, test_collision 86/0, test_collision2 41/0. Sintaxe OK
- patchBugs.1: nether.inc fireTick/fireSync (fogo envelhece e apaga, idade nos bits 4-7 do meta); item_data.inc tesoura stack 1; crops.inc saplingGrow + hook em cropRandomTick (muda vira arvore); assets/creative/mc-1.12.2.cfg vidros coloridos -> Construcao; tests/test_sapling_fire.cpp 19/0. Sintaxe OK
- patchVk.3: src/render/renderer.inc (rd* matrizes) incluido em main.cpp apos vkgl.inc; sed em todos os .inc: glMatrixMode/LoadIdentity/PushMatrix/PopMatrix/Translatef/Rotatef/Scalef/Ortho/Frustum/LoadMatrixf -> rd* (612 usos; vkgl.inc e glesnative.inc intactos). Sintaxe OK; test_slimeblock 22/22, test_chest 42/0, test_boat 36/36
- patchVk.1: screens.inc rendererSwitchAtStart (le renderer= do options.cfg; se o exe atual nao e o escolhido, execv do irmao _vulkan / sem sufixo; BL_NO_SWITCH evita vai-e-volta), kApiIsVulkan, BL_GFX_DEF por exe no PC, Sobre mostra o renderizador REAL; main_loop.inc chama no inicio do main; build_pc_renderers.sh (raiz). So sintaxe do build OpenGL (sem headers Vulkan aqui)
- patchBeacon.1: redstone.inc rsMapItems: PkItem copia "block/beacon_cage" (mods.inc creativeResolve l.184) agora gXbItem=XB_DECOR var 2, cube=false (antes ficava cubo da textura). Sintaxe OK; test_deco nao cobre o item do criativo
- patchUI.2: main_loop.inc l.1866: drawHud pulado quando pauseOpen e gPage nao vazia (pagina de Configuracoes). Sintaxe OK
- patchTouch.11: touch.inc pF1/pF3/pF5 (glifos de tecla) -> icones 7x5 (moldura = esconder HUD, lupa = debug, olho = camera). Sintaxe OK
- patchNether.1: main_loop.inc ~l.1474: Nether (gDim==1) fim da nevoa = max(24, min(dist,192)/2), inicio = 5% do fim (antes fim 110, inicio do overworld). Sintaxe OK
- patchIcon.1: asset_manager.inc pkIsCube: slime_block comparado por n12 (pack 1.12 chama "slime", textura com alpha => nao era cubo => icone chato). Sintaxe OK
- patchText.1: menu.inc dica F12->START; main_loop.inc msg touch sem F8; screens.inc botao "HUD touch"; comentario splitscreen.inc. Sintaxe OK
- patchCapa.1/.2/.3: gif.inc (decodificador GIF LZW/interlace/transparencia/disposal), capes.inc (CapeAnim 128x64 por quadro, 8 capas prontas, imagem->capa, RGB332+PackBits), capes_ui.inc (capeSyncSel/capeDropped), player_skin.inc pCape, player_render.inc (Pose.capeTex/capeVy + LayerCape em drawPlayer, preview do boneco), net.inc NP_CAPE (NET_VER 50; fila paginada, host guarda/repassa/reenvia 30 s), netlan.inc (limpa ao sair), screens.inc (botao Capa), main_loop.inc (capeSyncSel + soltar .gif). test_cape 73/0; sintaxe OK
- patchLight.4: mobs.inc draw(): camada eyes do enderman (w==1) zera gEntDim, blend ONE/ONE, depthMask off, uEntSky=15 (restaura ao fim); mining_sky.inc addPart: br >= .30 (Nether) / .45 (End); sky_cycle.inc entLightAt: bf >= .60 no End. Sintaxe OK; test_endfix 17/0; test_biomes ja nao compilava (pre-existente)
- patchEgg.1: main_loop.inc actMain: ovo de spawn aceita rpt (repeticao do botao direito segurado, linha 1056) com jitter +-.45 em x/z; sintaxe OK; test_egg 21/0; test_mooshroom 5/1 igual ao original (pre-existente)
- patchTouch.10: touch.inc: moved so cancela antes de brk e a 30 px; ring 220 ms / brk 480 ms / prog 450 ms; anel R 30*gf, T 3*gf + leque translucido proporcional a touchBrkProg. Sintaxe OK; test_joystick 13/6 igual ao original (falhas pre-existentes)
- patchUI.1: menu.inc: titulo do loading com escala = min(2, (sw-bx-6)/uWidth) (antes glScalef 2 fixo, cortava); splash do menu py = ly + lh*.68 (antes .9). Sintaxe OK; regressoes test_boat 36/36, test_chest 42/0, test_breed 12/0, test_player_save 14/14
- patchLoad.4: splash.inc: removido o texto "Carregando . . ." + ultimo arquivo da tela de loading (fica barra + %)
- patchChest.3: chestApplyNbt (chest.inc): convidado que coloca bau com conteudo (pick) manda NP_CHEST ao host (dono do conteudo) em vez de perder; sem mudar NET_VER (formato do pacote igual). Ordenhar como convidado: nao exige codigo (leite so mexe no inventario de quem ordenha; a vaca existe no convidado via mobsync)
- patchColl.1: AVALIADO, sem reescrita: moveEntityOnce/AABB (physics.inc, blockshape.inc) ja sao port fiel do Entity.moveEntity/AxisAlignedBB (Y,X,Z; degrau .6 com 2 variantes; borda ao agachar; onGround = collV && my<0; sub-passos para > 1 bloco), iguais no 1.8.9 e 1.12.2. Segfault de test_player_save era do teste (faltava SDL_GetTicks, usado por randomSeed); corrigido
- patchBoat4.2: boatRowPose(.., driver) so pose de remo para o condutor (net.inc remoto: rider==id; main_loop: !gBoatPaxSelf na sombra F7, reflexo, corpo e rowHandFp); boatEyeDelta (boat.inc) + eyeX/Y/Z em main_loop.inc: olho 1a pessoa segue inclinacao/balanco do casco; tests/test_boatrow.cpp (3 checks)
- patchSlime.3: bloco de slime. physics.inc: slimeBlkId/slimeUnder; moveMob (mobs.inc): quica (my=-my pre-colisao, so se > .1) e zera `fall`, sem dano de queda sobre slime; tickDrops (drops.inc): item quica em slime (x0,8); redstone.inc: estrutura movel pistonStructure/pistonAdd (ate 12 blocos, linha empurrada + 4 vizinhos perpendiculares de cada slime, imoveis grudados nao vem e nao travam, puxar exige destino livre), pistonMoveAll move tudo 1 celula, grudento recolhe a estrutura; gPsAct evita reentrada no proprio pistao; tests/test_slimeblock.cpp (22 checks)
- patchSign.3: signDrawUI sem os 2 textos de ajuda
- patchSlime.2: M_SLIME (enum, MD, modelo 2 camadas, ssz/sjump/squish em MobEnt), IA de pulo em tickMob, contato em hostileTick, slimeSpawn/isSlimeChunk, slimeFlush (divisao fora do laco), save 13o campo, rede bits 5-6 do flag, audio T[16] (tambem corrige enderman mudo: type<14 -> <16), /slime; tests/test_slime.cpp (10101 checks)
- patchBoat4.1: bote de 4 removido (boatBig/boat4Register/boat4Creative/chat '4'); boatCap=2, BOAT_PAXN=1; tipo>=6 normalizado em boatNetApply e no load; NET_VER 48; test_boat/test_boat_pax adaptados
- patchSign.2: editor da placa: grab=false + relMouse(false) + SDL_StartTextInput ao abrir; restaura ao fechar (main_loop.inc, transicao de gSignEd)
> A lista de pendencias ATUAL fica na raiz do jogo: `PENDENCIAS.md`. Este arquivo e so o historico por patch.

# PENDENCIAS — Blockland (trabalho por partes)

Regra de trabalho: cada parte e pequena, compila e tem teste; ao fim de cada uma este arquivo e atualizado.
Legenda: [x] feito+testado headless | [~] parcial | [ ] nao iniciado | (VIS) falta conferir no jogo real.

## patchBoat2.2 (v1.1.0.80rc, NET_VER 46)
- [x] `boat.inc`: BOAT_MAXOCC=4; BoatEnt.pax[3]/paxT[3] (boatPaxIdx/Add/Del); boatSeat/boatSeatL: 1 = centro, 2 = frente/tras, 3-4 = grade 2x2; boatSeatAll usa vagas livres (rede ocupa a vaga fixa: condutor 0, passageiro k = k+1); boatRiderBegin(sf, sl) com deslocamento lateral; passageiro mais antigo assume se o condutor sai. `net.inc`: NP_BOAT leva 3 passageiros, NP_BOATPAX por jogador. Testes: test_boat_pax 23/23, test_boat ajustado (3 animais cabem), test_boat_net 20/20.
- Nota: fabric-api-0_83_0_1_20.jar so tem codigo/tags da Fabric API (sem modelo de bote); o modelo usado continua o ModelBoat 1.12.2 de boat.inc. (VIS) 4 jogadores reais, tamanho dos assentos 2x2 no casco.

## patchBoat2.1 (v1.1.0.79rc, NET_VER 45)
- [x] `boat.inc`: BoatEnt.pax/paxT, gBoatPaxSelf, boatOcc (max 2 ocupantes), boatSeatNet, boatOfPax; boatInteract: lotado = recusa, bote com condutor de rede = entra como passageiro (nao conduz; se o condutor sai, assume); boatPick(skipId) deixa o extra do split mirar o bote do J1; boatRiderBegin(sf) desloca o assento. `net.inc`: NP_BOATPAX + pax no NP_BOAT; remoto desenhado no assento certo.
- [x] `main_loop.inc`: 1a pessoa remando (rowHandFp: mao sem item segue fase do remo); sombra F7 e reflexo da agua usam boatRowPose. Testes: test_boat_pax (13/13, novo), test_boat_net ajustado (host entra como passageiro).
- (VIS) tudo acima: 2 clientes reais, formato/amplitude da mao em 1a pessoa, sombra do remador.

## patchBreed.3 (v1.1.0.78rc)
- [x] `boat.inc boatsSave/mobsLoadFile`: mobs.dat ganha 5 campos (baby, babyAge, love, breedCd, sheared); linha antiga de 7 campos ainda carrega. `main_loop.inc breedFood`: M_MOOSH come trigo como a vaca. Teste novo `tests/test_mobsave.cpp` (5/5). (VIS) sair e entrar com filhote: continua pequeno e cresce.

## patchMenu.6 + patchHeart.1 (v1.1.0.77rc)
- [x] `assets/ui/titulo_bg.png` trocado pela foto enviada (1366x768, RGB); parallax/zoom 1,08 de `titleImgDraw` ja existentes. (VIS) conferir leitura do logo/botoes (a imagem e clara no centro; escurecimento fica em splash.inc, glColor .7).
- [x] `mobs.inc spawnHearts`: puff kind 3 (coracao rosa, sobe), 1 a cada ~12 ticks por animal em modo amor + 7 por pai no nascimento. (VIS) tamanho/cor.

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
- [x] Item 7 (Q/F no touch): JA FEITO — botoes TA_SWAP (F) e TA_DROP (Q) em `touch.inc` (L.side[0..1]) ligados em `main_loop.inc` ~883. (VIS) conferir no aparelho.

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

## patch5.58 (FOV, /kill, operadores, seletores) — incremental sobre patch5.57a + sync de chunks (NET_VER 23)
- [x] FOV: o slider "Campo de Visao" agora vale (30..110, padrao 70 = Normal; `main_loop.inc`: `fov = (30 + 80*gsf("fov")) * fovHand`); `/fov <30-110>` (`screens.inc cmdSetFov`).
- [x] `/kill` funciona no criativo (`cmdKillSelf`) e em outros jogadores (`/kill <nick>` -> `NP_KILL`).
- [x] Operadores (NET_VER 24): host sempre op; `/op` e `/deop <nick>` so no host; lista por nick em `saves/ops.txt`; `NP_OPS` leva os ids online a todos (reenviado a cada `netRosterSend`). Convidado op pode `/tp` e `/give` e `/kill` outros (o host repassa ao alvo); receptor so aceita TPTO/GIVE/KILL do host ou de op. Offline = sempre op. Sem op no MP: `/give` e `/gamemode` negados.
- [x] Seletores `@s @a @e @r @p` em `/kill`, `/tp`, `/give` (executa o comando por alvo; `@e` no /kill tambem mata mobs; `@p` = outro jogador mais proximo, sem outro = voce).
- Teste novo `tests/test_ops_select.cpp` (23 verificacoes, 0 falhas); regressao 0 falhas: test_conn 33, test_chunk_sync 6, test_chest 44, test_egg 21, test_inv_keys2 9.
- Limites: `/gamemode` de outros e `/tp @a <destino>` com seletor no destino nao existem; op por nick (nick trocado = sem op); `/give @a` exige op; matar no criativo usa a tela de morte normal (VIS).
- (VIS) slider de FOV na tela e em jogo; `/kill` no criativo (tela de morte/respawn); `/op` com 2 clientes; `/kill @a` no MP.
- [ ] AINDA FALTA (fila): fogo (voando, de lado), areia com gravidade, alavanca x slab, itens queimaveis, lava queimando inflamaveis, animacao de fogo na lava; depois perola do End, mobs sumindo, esponja, vitoria-regia, vinha; bloco embaixo de jogador/mob nao empurra; TNT; luz e time set no MP; TP otimizado/por dimensao; pistao bugado; splitscreen+MP; Nether >19 chunks; conversor/escadas; conversor de mods.

## patch5.59 (colocar bloco nao empurra jogador/mob) — SOMA o patch5.58 (FOV, /kill, operadores, seletores; NET_VER 24) e o sync de chunks (NET_VER 23)
- [x] `placement.inc entBlocksBox`: como o Minecraft (checkNoEntityCollision), `placeBlock` recusa bloco com colisao onde ha jogador local (J1/J2), jogador remoto (nao espectador/morto), mob vivo, bote ou carrinho. Antes so checava quem colocava; os outros eram empurrados para fora. Itens no chao e flechas nao impedem.
- Teste novo `tests/test_place_entity.cpp` (13 verificacoes, 0 falhas): vazio coloca; remoto/mob/carrinho/bote bloqueiam; espectador, morto e entidade acima da celula nao; a entidade nao se move. Regressao 0 falhas: test_piston 142, test_minecart 39, test_chest 44, test_egg 21, test_collision2 41, test_ops_select 23.
- Limites: caixas aproximadas (jogador .6x1.8, agachado 1.65; bote 1.5x.6; carrinho .98x.7); convidado usa a posicao sincronizada das entidades; blocos que caem/pistao/redstone continuam podendo ocupar a celula (pistao empurra de proposito).
- (VIS) colocar bloco onde outro jogador/mob esta (recusado, sem empurrao) e embaixo dos proprios pes pulando (permitido).
- [ ] AINDA FALTA (fila): fogo (voando, de lado), areia com gravidade, alavanca x slab, itens queimaveis, lava queimando inflamaveis, animacao de fogo na lava; perola do End, mobs sumindo, esponja, vitoria-regia, vinha; TNT; luz e time set no MP; TP otimizado/por dimensao; pistao bugado; splitscreen+MP; Nether >19 chunks; conversor/escadas; conversor de mods.

## patch5.61 (lava acende madeira, item queima no fogo, fogo na tela) — SOMA 5.58 + 5.59 + 5.60
- [x] 5.60 (ja estava na pasta de trabalho, sem entrada neste arquivo; conferido agora): `falling.inc` areia/cascalho caem (entidade de bloco, so host); `nether.inc` fogo sem apoio some e cola na parede (`fireSideMask`/`fireBaseOk`); alavanca/tocha nao prendem em lateral de laje/escada. `tests/test_fall_fire.cpp` 29 verificacoes, 0 falhas.
- [x] `nether.inc lavaIgniteTick` (chamado de `runWorldTick` a cada 20 ticks, so host/offline): por chunk carregado, 4 colunas ao acaso; cada celula de lava tenta 3 vezes acender o ar ao lado/acima que encosta em bloco inflamavel (`fireFlam`). Sem inflamavel por perto nada acende.
- [x] `drops.inc`: item na celula de fogo (XB_FIRE) e destruido, como na lava.
- [x] `nether.inc drawFireOverlay` + `main_loop.inc`: com `gFire > 0` (lava = 15 s, fogo = 8 s; nao no criativo/espectador) desenha 2 camadas do fogo na frente da camera, 90% opaco (renderFireInFirstPerson).
- Teste novo `tests/test_lava_fire.cpp` (6 verificacoes, 0 falhas): sem inflamavel nao acende; com tabua perto a lava acende; item no fogo some; sem fogo fica. Regressao 0 falhas: test_nether 310, test_fall_fire 29, test_egg 21, test_piston 142, test_ops_select 23.
- Limites: o fogo na tela nao existe no P2 do split nem na 3a pessoa/jogador remoto (so mobs tem `drawMobFire`); a lava nao acende plantas/lenha que nao estejam em `fireFlam` (tabua, tronco, folha, la, estante); itens so queimam, sem lista de resistentes ao fogo (Nether).
- (VIS) fogo na tela ao entrar na lava (posicao, animacao, nao tampar a mao); lava acendendo madeira de verdade num lago de lava com casa de madeira ao lado.
- [ ] AINDA FALTA (fila): chamas no modelo do jogador (3a pessoa/remoto) e no P2; perola do End, mobs sumindo/nao spawnando, esponja, vitoria-regia, vinha; TNT; luz e time set no MP; TP otimizado/por dimensao; pistao bugado; splitscreen+MP; Nether >19 chunks; conversor/escadas; conversor de mods.

## patch5.62 (perola do Ender, esponja) — SOMA 5.58 a 5.61 (NET_VER 25)
- [x] Perola do Ender (`egg.inc`, reaproveita o sistema do ovo: fisica de EntityThrowable, cooldown, rede, choque com mob/bloco): `EggEnt.kind` (0 ovo, 1 perola); clicar com o item `item/ender_pearl` do pack arremessa; no impacto o dono e teleportado ao ponto (`pearlApply`, chamado no laco logo depois de `eggsTick`), com 5 de dano (nada no criativo), velocidade e queda zeradas; procura vaga livre subindo ate 4 blocos; mob atingido so e empurrado. NP_EGG leva +1 byte (kind): NET_VER 25. Pearl de outro jogador so aparece (sem teleportar ninguem).
- [x] Esponja (`falling.inc spongeAbsorb`/`spongeHooksInit`): ao ser colocada ou quando agua encosta, absorve agua num raio de 6 passos (ar propaga), ate 65 blocos, e vira `block/wet_sponge` (se o pack nao tiver, continua esponja). Hooks registrados junto com `fallHooksInit` (a cada troca de pack).
- Teste novo `tests/test_pearl_sponge.cpp` (17 verificacoes, 0 falhas): pearl arremessa/teleporta/dano/criativo/ovo continua ovo; esponja seca sem agua, piscina pequena, limite de 65, agua depois aciona, agua longe nao. Regressao 0 falhas: test_egg 21, test_fall_fire 29, test_lava_fire 6, test_ops_select 23.
- Limites: P2 do split nao arremessa perola (so o P1); esponja molhada nao seca no forno/Nether; sem som; a perola nao tem a regra de chance de spawn de endermite; sem pack nao ha item/bloco (sponge e pearl dependem do resource pack).
- (VIS) perola voando (sprite) e o ponto de chegada na parede/chao; esponja absorvendo num lago e trocando para molhada.
- [ ] AINDA FALTA (fila): vitoria-regia, vinha; mobs sumindo/nao spawnando; chamas no modelo do jogador e no P2; TNT; luz e time set no MP; TP otimizado/por dimensao; pistao bugado; splitscreen+MP; Nether >19 chunks; conversor/escadas; conversor de mods.

## patch5.63 (vitoria-regia, vinha) — SOMA 5.58 a 5.62
- [x] Vitoria-regia = XB 1004 (`XB_LILY`), vinha = XB 1005 (`XB_VINE`), dentro do framework rs (`isRs` = DUST..FIRE): modelo (`rsBuild` case 30/31, textura `block/lily_pad` e `block/vine`), forma/selecao (`rsShape`), item do pack mapeado na tabela `R` (`block/lily_pad`, `block/vine`), drop pelo `gRsDrop`.
- [x] Vitoria-regia: `lilyUse` (falling.inc, chamado no clique direito depois do balde): o raio atravessa liquidos e so poe sobre FONTE de agua com ar por cima; colide (pisa-se nela); `lilyNeighbor` solta o item se a agua de baixo sumir (esponja, balde).
- [x] Vinha: `rsPlace` case 31: grudada no lado de bloco solido (bit k como a escada de mao: 0 +Z, 1 -X, 2 -Z, 3 +X) ou embaixo de bloco (bit 4); clicar de novo em outro lado soma na mesma celula; `vineNeighbor` tira o lado sem apoio (a vinha de cima com o mesmo lado segura a de baixo) e some sem nenhum; sem colisao; escala como a escada (`climbBlk` em main_loop e splitscreen).
- Teste novo `tests/test_lily_vine.cpp` (19 verificacoes, 0 falhas). Regressao 0 falhas: test_piston 142, test_chest 44, test_minecart 39, test_pearl_sponge 17, test_fall_fire 29.
- Limites: a vinha NAO cresce nem se espalha (so a colocada pelo jogador); sem tinta de folhagem (a textura do pack sai como esta); vinha solta o item pelo `gRsDrop` (no Minecraft so com tesoura); vitoria-regia nao nasce no mundo; P2 do split nao usa `lilyUse` (so P1); a vinha nao e gerada em selva.
- (VIS) vinha na parede/teto e escalar; vitoria-regia sobre lago (altura, selecao, pisar); itens no inventario criativo com icone plano.
- [ ] AINDA FALTA (fila): crescimento/geracao de vinha e vitoria-regia (opcional); mobs sumindo/nao spawnando; chamas no modelo do jogador e no P2; TNT; luz e time set no MP; TP otimizado/por dimensao; pistao bugado; splitscreen+MP; Nether >19 chunks; conversor/escadas; conversor de mods.

## patch5.64 (mobs sumindo no multiplayer, TNT) — SOMA 5.58 a 5.63
- [x] Mobs sumindo/nao nascendo (`hostile.inc activePlayers`/`playerNear2`): o despawn (hostil: 96 blocos ou 40 com 1/900; animal: 128) e a contagem de animais olhavam so o jogador LOCAL, entao no multiplayer os mobs perto de um convidado sumiam (longe do host). Agora vale o jogador mais proximo entre local, P2 e remotos vivos/nao espectadores; o spawn (animal e hostil) sorteia um jogador como centro e os limites (12 animais / 14 hostis) valem por jogador.
- [x] TNT (`falling.inc`): bloco do pack `block/tnt`; acende com isqueiro (`flintUse`), fogo ao lado ou redstone ligada (ganchos added/neighbor); vira entidade `TNTS` com pavio de 80 ticks (pula, cai, pisca), explode com forca 4 (`explosionAt`, raios como o creeper) e acende em cadeia as TNT atingidas (10-29 ticks). So host/offline simula; convidado ve os blocos sumirem (NP_BLOCKS).
- [x] `hostile.inc`: `creeperBoom` agora chama `explosionAt(cx,cy,cz,forca,P,causa,self)` (forca 3, comportamento igual); dano = (i^2+i)/2*8*(2*forca)+1 com i = 1 - dist/(2*forca); atinge jogador, P2 e mobs.
- Testes novos `tests/test_mob_near.cpp` (8) e `tests/test_tnt.cpp` (13), 0 falhas. Regressao 0 falhas: test_mobsync 25, test_nether 310, test_fall_fire 29, test_lava_fire 6, test_pearl_sponge 17, test_lily_vine 19, test_piston 142.
- Limites: o convidado nao ve a TNT acesa (so o bloco sumir e a explosao); convidado nao acende TNT com o isqueiro (so o host); a explosao nao fere jogadores remotos (so host/P2/mobs); sem som; sem TNT em carrinho acendendo por fogo; TNT nao e inflamavel pelo fogo espalhando (so acende se o fogo encostar).
- (VIS) TNT acesa (tamanho, pisca, queda); cratera; mobs perto de um convidado nao sumirem mais.
- [ ] AINDA FALTA (fila): mobs nao spawnando em mundo convertido (investigar com o mapa); TNT visivel e dano em convidados; chamas no modelo do jogador e no P2; luz e time set no MP; TP otimizado/por dimensao/pra todos; pistao bugado; splitscreen+MP; Nether >19 chunks; conversor/escadas; conversor de mods; crescimento de vinha/vitoria-regia.

## patch5.65 (hora do mundo e TP entre dimensoes no multiplayer) — SOMA 5.58 a 5.64 (NET_VER 26)
- [x] `time set` nao sincronizava (net.inc `NP_TIME`): antes so o WELCOME levava a hora; cada cliente andava no proprio relogio (pausar o menu atrasava um so) e `/time` nao chegava aos outros. Agora o host manda hora + ciclo (dia/noite ligado ou parado) a cada 20 ticks e ao mudar; o convidado aplica. `/time set|add|pause|resume` so para operador no MP (host sempre; convidado op manda o comando ao host, que aplica e repassa); `/time query` livre.
- [x] TP entre dimensoes: `NP_TPTO` leva a dimensao de destino (+1 byte; sem o byte = nao troca); `/tp <nick>` vai para a dimensao do alvo (`cmdPlayerDim`, usa `RemotePl.dim` que ja era sincronizado), `/tp <alvo> <dest>` e `/puxar` levam o alvo para a dimensao certa (`dimSwitch` antes de mover); `/tp x y z` fica na dimensao atual. Ja valia para todos os jogadores o `/tp` em si mesmo; mover OUTROS exige operador (patch5.58).
- Teste novo `tests/test_mp_time_tp.cpp` (14 verificacoes, 0 falhas). Regressao 0 falhas: test_conn 33, test_chunk_sync 6, test_ops_select 23, test_nether 310.
- Limites: sem retry/ack no NP_TIME (a cada 1 s ele se corrige); a iluminacao "de alguns blocos" nao foi tocada (sem como reproduzir sem o jogo — preciso de qual bloco e em qual situacao); TP "otimizado" so no sentido de nao travar na troca de dimensao (carregamento gradual ja existia); o P2 do split nao entra em dimensao diferente do P1.
- (VIS) dois clientes: `/time set night` pelo host/op aparece nos outros; `/tp <nick>` com o alvo no Nether leva voce para la.
- [ ] AINDA FALTA (fila): iluminacao nao sincronizada de alguns blocos (preciso do exemplo); mobs nao spawnando em mundo convertido; TNT visivel e dano em convidados; chamas no modelo do jogador e no P2; pistao bugado; splitscreen+MP; Nether >19 chunks; conversor/escadas; conversor de mods; crescimento de vinha/vitoria-regia.

## patch5.66 (TNT visivel e dano em convidados; escadas do conversor conferidas) — SOMA 5.58 a 5.65 (NET_VER 27)
- [x] `NP_TNT` (host -> todos): a TNT acesa aparece para o convidado (entidade so visual em `TNTS`, anima e some; a explosao chega pelos blocos). `tntTick` agora anima no convidado sem explodir.
- [x] `NP_BOOM` (host -> vitima): jogadores remotos na mesma dimensao (nao espectadores) levam dano e recuo de explosao de TNT e de creeper (`explosionAt` -> `boomReceive` em pvp.inc, mesma formula de dano, sem usar o NP_PVP para a mensagem/recuo nao saiam errados).
- [x] Escadas do conversor de mapa (`conversor/mapping.py`): conferido por sonda no jogo real (`rsStairBackDir`): dir 0/1/2/3 = lado alto N/L/S/O, igual ao "ascendente" do Minecraft (dados 0 L, 1 O, 2 S, 3 N) -> mapeamento `{0:1, 1:3, 2:2, 3:0}` esta certo; bit de cima (invertida) = bit 2 do meta. Nao mexi.
- Testes: `tests/test_tnt.cpp` 17 verificacoes (convidado: TNT so visual, nao acende, boomReceive) 0 falhas. Regressao 0 falhas: test_conn 33, test_mobsync 25, test_mp_time_tp 14, test_mob_near 8, test_lava_fire 6, test_ops_select 23.
- Limites: dano a convidados sem raios de exposicao (so distancia, igual ao do host); a mensagem de morte do convidado diz "explodido por TNT/creeper" sem o nome de quem acendeu.
- [ ] AINDA FALTA (fila): iluminacao nao sincronizada de alguns blocos (preciso do exemplo); mobs nao spawnando em mundo convertido; chamas no modelo do jogador e no P2; pistao bugado; splitscreen+MP; Nether >19 chunks; conversor de mods; crescimento de vinha/vitoria-regia.

## patch5.67 (blocos fantasma no multiplayer, chamas no jogador) — SOMA 5.58 a 5.66 (NET_VER 28)
- Iluminacao "de alguns blocos" no MP: tirada da fila (era o `time set` que nao sincronizava; resolvido no 5.65).
- [x] Pistao que "ativa e nao desativa" / bloco inquebravel invisivel no multiplayer — causa provavel: `NP_BLOCKS` e UDP sem confirmacao, um pacote perdido deixava o convidado com o bloco/estado errado para sempre (o pistao manda varias edicoes seguidas). `net.inc netEditRepeatTick`: cada edicao e reenviada mais 2x (150 ms entre elas) com o valor ATUAL da celula (`netEdits`), em lotes de ate 80 por rodada; `setBM` igual e no-op no receptor; para guest->host tambem. NAO reproduzi o bug no jogo: marque (VIS) e me diga se ainda acontece.
- [x] Chamas no modelo do jogador (`nether.inc drawFireWH`, mobs usam o mesmo): jogador remoto em chamas (bit 2 do byte `ely` do `NP_STATE`; `gFire > 0`, nao no criativo/morto) e o proprio jogador em 3a pessoa/espelho. Fogo na tela (1a pessoa) ja existia (5.61).
- Teste novo `tests/test_edit_repeat.cpp` (9 verificacoes, 0 falhas). Regressao 0 falhas: test_conn 33, test_chunk_sync 6, test_piston 142, test_mp_time_tp 14, test_tnt 17.
- Limites: ate 3 envios por edicao (explosoes grandes levam alguns segundos para completar a 2a e 3a rodada); o P2 do split continua sem fogo na tela nem no modelo remoto; Nether com mais de 19 chunks NAO foi investigado (preciso de um print).
- (VIS) dois clientes: jogador em chamas visto por outro (3a pessoa) e em F5; pistao em MP com rede ruim.
- [ ] AINDA FALTA (fila): mobs nao spawnando em mundo convertido; fogo no P2 (tela e modelo); Nether >19 chunks (print); splitscreen+MP; conversor de mods; crescimento de vinha/vitoria-regia; adaptador de blocos de textura.

## patch5.68 (vinha/vitoria-regia com cor, grama alta quebra junto)
- [x] Vinha com cor de bioma (`redstone.inc` case 31: quads `tint+foli`; `models.inc` XQuad `foli` usa a cor de FOLHAGEM do `biomeTints`, nao a da grama). Vitoria-regia com cor fixa 0x208030 como o MC (`tc`).
- [x] Grama alta/samambaia grande/girassol/lilas/roseira: quebrar a metade de CIMA remove a de baixo (`blockupdate.inc xbNeighbor`, so reage a mudanca vinda de cima, entao colocar baixo->cima e a geracao nao apagam nada). Quebrar a de baixo ja removia a de cima.
- Teste novo `tests/test_tall_vine.cpp` (5 verificacoes + 4 de malha que so rodam com pack instalado), 0 falhas. Regressao 0 falhas: test_piston 142, test_pearl_sponge 17.
- Limites: sem pack no ambiente, a cor da vinha/vitoria-regia nao foi verificada na malha; variacao por bioma de agua e outros blocos nao feita; item de vinha no inventario continua sem tint.
- (VIS) vinha verde em floresta/pantano/taiga com cores diferentes; vitoria-regia verde; grama alta: bater na de cima faz sumir as duas.
- [ ] AINDA FALTA (fila): cor de bioma na agua (e demais blocos); outline do fogo; reflexo da agua de lado e de flores; fade in de chunk; controles sem gatilhos de tras; host kitar remove jogadores; Windows host/join; pausar com LAN nao congela; pick em todos os blocos; lajes/escadas 1.12.2.

## patch5.69 (outline do fogo, cor de bioma na agua)
- [x] Outline do fogo (`nether.inc`): selecao agora e uma placa fina onde estao as chamas (chao: 1/16 embaixo; parede: 1/16 colado em cada lado marcado em `meta`), nao mais o cubo. Continua sem colisao e socavel.
- [x] Cor da agua por bioma (`terrain112.inc biomeWaterTint`, usado em `world.inc` no build da agua, so overworld): pantano esverdeado, taiga/tundra azul mais frio/escuro, selva levemente mais verde; media 3x3 como a grama. Valores aproximados do 1.12, ajustaveis na tabela `WT`.
- Teste novo `tests/test_fire_outline.cpp` (11 verificacoes), 0 falhas. Regressao 0 falhas: test_fall_fire 29, test_lava_fire 6, test_nether 310.
- Limites: o raio que acerta o fogo agora e a placa fina (mirar no ar acima do fogo no chao nao o apaga mais; mire na base); sem agua colorida no Nether/End; tons de agua por bioma sao aproximacao, sem a transicao suave por vertice.
- (VIS) fogo no chao/parede com contorno fino; agua do pantano esverdeada e da tundra mais fria; sem emenda feia na borda de bioma.
- [ ] AINDA FALTA (fila): reflexo da agua de lado e de flores; fade in de chunk; controles sem gatilhos de tras; host kitar remove jogadores; Windows host/join; pausar com LAN nao congela; pick em todos os blocos; lajes/escadas 1.12.2; cor de bioma nos demais blocos.

## patch5.70 (host sai remove jogadores, pausa com LAN, controle sem LB/RB)
- [x] Host que sai/cai: o convidado agora limpa TODOS os jogadores remotos (`net.inc netDropAllRemotes`, antes so tirava o host e deixava os outros convidados parados na tela), tanto no `NP_BYE` do host quanto no timeout de 8 s. `netStop` manda o BYE 3x (UDP sem ack).
- [x] Pausar com a LAN ligada nao congela mais (`main_loop.inc`: congela so se nao for host nem convidado; antes so quando havia peer conectado, entao um host com LAN aberta e sem ninguem congelava e o convidado entrava num mundo parado).
- [x] Controle: hotbar anterior/proximo agora vem no direcional esquerda/direita por padrao (`gamepad.inc`); LB/RB ficam livres (so rolagem opcional em menus; o stick direito rola igual). Quebrar/colocar continuam em RT/LT. Configs antigas salvas com LB/RB continuam valendo ate resetar o mapa.
- Teste novo `tests/test_host_leave.cpp` (6 verificacoes), 0 falhas. Regressao 0 falhas: test_conn 33, test_chunk_sync 6.
- Limites: "Windows host/join" nao tocado (precisa de teste em Windows); o BYE ainda e UDP (3 copias, sem confirmacao); pausa com LAN aberta deixa mobs/fome rodando enquanto o menu esta aberto (e o pedido).
- (VIS) dois clientes + host: host sai -> convidados ficam sem os avatares; pausar no host com LAN aberta e ninguem conectado nao congela; controle jogando so com RT/LT/direcional.
- [ ] AINDA FALTA (fila): Windows host/join; reflexo da agua de lado e de flores; fade in de chunk; pick em todos os blocos; lajes/escadas 1.12.2; cor de bioma nos demais blocos; esponja/TNT resto/perola do End.

## patch5.71 (facilitador do pick block)
- [x] `pickItemAt(x,y,z)` (`redstone.inc`): ponto unico do pick block, usado pelo P1 (`main_loop.inc`) e P2 (`splitscreen.inc`). Resolve o que falhava: metade de CIMA da porta, cabeca do pistao e cabeca da cama davam 0 (a regra de drop os exclui; o pick agora devolve o item inteiro). Fogo/portal/ar/liquido -> 0. Novo `rsDropItemM(b, meta)` (rsDropItem passa a chamar ele).
- Para um bloco novo funcionar no pick basta ter item em `gRsDrop`/`gXbDrop`/`gXbDrop` (nada de lista separada); caso especial entra em `pickItemAt`.
- Teste novo `tests/test_pick_all.cpp` (10 verificacoes), 0 falhas. Regressao 0 falhas: test_piston 142, test_chest 42.
- Limites: sem o pack no ambiente de teste, os itens reais (gRsDrop) foram simulados; plantacao (`XB_CROP`) ainda segue a regra de drop (cenoura/nada); pick de mob em sobrevivencia continua desligado (so criativo, so mobs com ovo); pick de mobs sem ovo nao feito.
- (VIS) botao do meio na metade de cima de uma porta, na cabeca do pistao estendido e na cabeca da cama; trilho, laje, escada, tocha.
- [ ] AINDA FALTA (fila): pick de mob sem ovo e de plantacao; Windows host/join; reflexo da agua de lado e de flores; fade in de chunk; lajes/escadas 1.12.2; cor de bioma nos demais blocos; esponja/TNT resto/perola do End.

## patch5.72 (TNT: limite por tick e recuo entre TNTs)
- [x] Desempenho com muita TNT (`falling.inc tntTick`): no maximo `TNT_MAX_BOOM`=6 explosoes por tick; as demais ficam com pavio 0 e explodem no tick seguinte (antes dezenas de explosoes de 1352 raios + reconstrucao de malha no mesmo quadro travavam). Nada some sem explodir.
- [x] Recuo entre TNTs (`hostile.inc explosionAt`): TNT acesa dentro do raio ganha velocidade para longe do centro (motion += direcao * impacto, como o Java); a cadeia agora espalha as TNTs.
- `TntEnt`/`TNTS` foram movidos de falling.inc para hostile.inc (a explosao precisa deles).
- Teste novo `tests/test_tnt_perf.cpp` (6 verificacoes), 0 falhas. Regressao 0 falhas: test_tnt 17, test_mob_near 8.
- Limites: nao medi FPS (sem GPU/jogo); o custo por explosao em si (1352 raios, setB + netSendBlock por bloco, malha) nao foi reduzido; itens soltos e jogadores remotos nao empurram TNT; sem agua protegendo blocos nem som.
- (VIS) cratera grande com varias TNTs (canhao de TNT) sem travar; TNTs se espalhando na cadeia.
- [ ] AINDA FALTA (fila): custo por explosao (lotes de netSendBlock/malha); Windows host/join; reflexo da agua de lado e de flores; fade in de chunk; lajes/escadas 1.12.2; cor de bioma nos demais blocos; pick de mob sem ovo.

## patch5.73 (lajes e escadas 1.12.2 + facilitador)
- [x] Facilitador `RS_SM` (`redstone.inc`): uma linha {lado, topo, base} por material; a linha = variante (max 16, 4 bits do meta). `rsMat` le a tabela (antes: array + `if`s soltos). Receita no comentario: 1 linha na tabela + 2 linhas em `R` (rsRegister) + nome em ptbr.
- [x] Materiais novos (variantes 11..14), escada e laje: tijolos do Nether, quartzo (lado/topo/base), arenito vermelho (lado/topo/base), purpur. Agora cobrem todas as lajes e escadas do 1.12.2 (carvalho, abeto, bétula, selva, acacia, carvalho escuro, pedregulho, arenito, arenito vermelho, tijolos, tijolos de pedra, tijolos do Nether, quartzo, purpur; laje de pedra lisa so como laje). Entram no jogo pelos itens do pack com esses nomes (como as demais).
- Teste novo `tests/test_slab_mats.cpp` (10 verificacoes), 0 falhas. Regressao 0 falhas: test_stairs 202; test_redstone pulado (sem pack).
- Limites: depende do pack ter os itens "Nether Brick Stairs", "Quartz Slab", "Red Sandstone Stairs", "Purpur Slab" etc. na aba criativa (sem pack nao verifiquei); laje de madeira "petrified" e laje de pedra/quartzo "lisa" (smooth) nao existem; o 5o bit livre do meta nao foi usado (16 materiais no maximo).
- Culling do topo da laje de arenito: NAO mexi. A logica (`models.inc` slabLike/oBelow/oAbove) parece correta no modelo (topo cortado so se ha bloco opaco em cima). Preciso de um print do defeito (qual laje, em cima de que bloco, qual face some).
- (VIS) as 4 lajes/escadas novas em posicoes cima/baixo/duplas, texturas de topo/base do quartzo e do arenito vermelho.
- [ ] AINDA FALTA (fila): culling da laje de arenito (print); custo por explosao de TNT; Windows host/join; reflexo da agua de lado e de flores; fade in de chunk; cor de bioma nos demais blocos; pick de mob sem ovo.

## patch5.74 (lote de blocos da explosao + pick de plantacao)
- [x] Lote de rede (`net.inc`): `netBatchBegin()/netBatchEnd()`; durante o lote `netSendBlock` so registra e o fim envia 1 pacote NP_BLOCKS por 80 blocos (antes 1 pacote por bloco) e agenda o reenvio UDP. `explosionAt` (`hostile.inc`) usa o lote. A malha ja era marcada `dirty` e refeita uma vez por chunk, entao nao mudou.
- [x] Pick de plantacao (`redstone.inc pickItemAt`): XB_CROP devolve o item do tipo em qualquer idade (cenoura confirmada; trigo/batata/beterraba dependem de existir em `gRsDrop` do pack).
- Teste novo `tests/test_net_batch.cpp` (9 verificacoes), 0 falhas. Regressao 0 falhas: test_tnt_perf 6, test_pick_all 10, test_tnt 17, test_piston 142.
- Limites: nao medi FPS/banda; o loop de 1352 raios e o hook de vizinhos por bloco (`blockChangedHook`) continuam; sem pack so a cenoura foi testada; pick de mob sem ovo NAO feito.
- (VIS) canhao de TNT no MP com convidado: crateras sincronizadas sem blocos fantasma; botao do meio numa plantacao.
- [ ] AINDA FALTA (fila): pick de mob sem ovo; custo do hook por bloco; Windows host/join; reflexo da agua de lado e de flores; fade in de chunk; culling da laje de arenito (print); cor de bioma nos demais blocos; esponja/TNT resto; fisica nova; terreno; comandos.

## patch5.75 (cor da vinha no inventario; fila de biomas conferida)
- [x] `iconTintRGB` (`render_items.inc`): agora tambem usa `gXbItem` (item -> bloco): vinha (XB_VINE) e vitoria-regia (XB_LILY) recebem a cor de inventario mesmo se o arquivo do item nao for `block/vine`/`block/lily_pad`. Antes dependia so do nome do arquivo.
- Conferido sem mexer: cor da agua por bioma JA existe (`biomeWaterTint`, terrain112.inc, usada em world.inc, so overworld; patch5.68). Pick de mob sem ovo: todos os 9 mobs ja tem ovo (`mobEggItem`) e o Java nao faz pick de mob em sobrevivencia; item encerrado.
- Teste novo `tests/test_icon_tint.cpp` (4 verificacoes), 0 falhas. Regressao: test_pick_all 10, 0 falhas.
- Limites: se a vinha continuar sem cor no inventario, a causa e outra (ex. item desenhado por outro caminho) e preciso de um print; sem pack nao vi o nome real do arquivo do item. Cor de bioma nos DEMAIS blocos (cana, vitoria-regia no mundo e outros) NAO feita.
- (VIS) vinha e vitoria-regia verdes no inventario criativo e na hotbar; agua em pantano/biomas frios no mundo.
- [ ] AINDA FALTA (fila): cor de bioma nos demais blocos (cana-de-acucar, etc.); outline do fogo; reflexo da agua de lado e de flores; fade in de chunk; Windows host/join; culling da laje de arenito (print); custo do hook por bloco; fisica nova; terreno; comandos.

## patch5.76 (afogamento e sufocamento)
- [x] Afogamento (`survival.inc survTick`, `gAir`): cabeca na agua gasta 1 de ar/tick (300 = 15 s); ao chegar a -20 leva 2 de dano (ignora armadura) e o ar volta a 0 (novo dano a cada 20 ticks); fora da agua o ar enche na hora (1.8.9); criativo e respawn restauram 300.
- [x] Sufocamento: olhos (y+1,62) dentro de bloco solido e opaco (`lightOpacL >= 15`) = 1 de dano, limitado pela invulnerabilidade (~1 a cada 0,5 s). Vidro, folha e liquido nao sufocam.
- Conferido sem mexer (itens ja prontos, fila desatualizada): fade in de chunk (1,2 s, `main_loop.inc` born/fade); cor da agua por bioma (5.68); outline do fogo (`nether.inc:454`, placa fina no chao/parede); pick de mob sem ovo (todos os mobs tem ovo); cor de bioma nas plantas em cruz (XB_PLANT) e folhas. Cana-de-acucar NAO existe como bloco no jogo (so o nome no ptbr), entao nao ha o que tingir.
- Teste novo `tests/test_drown.cpp` (8 verificacoes), 0 falhas. Regressao 0 falhas: test_fall_fire 29, test_tnt 17.
- Limites: SEM bolhas de ar no HUD (so o dano); jogador 2 (split) e convidados remotos NAO se afogam/sufocam; mobs nao se afogam; sufocamento usa so o ponto dos olhos (nao a caixa inteira da cabeca como o Java); nao toquei em "bloco embaixo do jogador nao empurra".
- (VIS) ficar ~15 s com a cabeca na agua leva dano; enterrar a cabeca na areia/pedra (por ex. com bloco caindo) causa dano.
- [ ] AINDA FALTA (fila): bolhas de ar no HUD; afogar P2/convidados/mobs; outline do fogo: se estiver errado, preciso de print; reflexo da agua de lado e de flores; Windows host/join; culling da laje de arenito (print); custo do hook por bloco; cana-de-acucar (bloco novo); elytra, nadar 1.13, rastejar, trapdoor; terreno; comandos.

## patch5.77 (bolhas de ar no HUD + ar do jogador 2)
- [x] Bolhas de ar (`survival.inc survDrawHud`, `BUBBLE_M`, `survBubbles`): aparecem acima da barra de fome so com ar < 300; cheias = ceil((ar-2)*10/300), estouradas (vazias) = ceil(ar*10/300) - cheias, como o GuiIngame do Java.
- [x] Jogador 2 (split): `gAir` entra na troca de contexto (`SurvCtx.air`, `split2SwapSurv`); como o P2 ja roda `survTick` via troca, ele agora se afoga e sufoca com o proprio ar (antes `gAir` seria compartilhado).
- Teste: `tests/test_drown.cpp` ampliado para 15 verificacoes, 0 falhas. Regressao: test_fall_fire 29, 0 falhas.
- Limites: sem animacao de "estouro" nem tremor das bolhas; convidados remotos e mobs NAO se afogam; split nao tem teste headless do desenho (so da troca de contexto); posicao das bolhas (-21) nao conferida visualmente.
- (VIS) bolhas azuis acima da fome ao mergulhar, sumindo da direita para a esquerda; o mesmo no P2 no split.
- [ ] AINDA FALTA (fila): afogar mobs/convidados remotos; reflexo da agua de lado e de flores; Windows host/join; culling da laje de arenito (print); cana-de-acucar (bloco novo); elytra 1x1, nadar 1.13, rastejar, trapdoor; terreno; comandos.

## patch5.78 (afogamento de mobs)
- [x] `mobAirTick` (`mobs.inc`, chamado no inicio de `mobFireTick`, ou seja em todos os caminhos: normal, jogador 2 e jogador remoto): mob com a cabeca (y + altura*0,85) na agua gasta 1 de ar por tick (`MobEnt.air`, 300); a -20 leva 2 de dano (`mobHurt`, respeita a invulnerabilidade) e o ar volta a 0; fora da agua enche na hora. Mob morto nao leva dano.
- Teste novo `tests/test_mob_drown.cpp` (6 verificacoes), 0 falhas. Regressao 0 falhas: test_mob_near 8, test_fall_fire 29, test_tnt_perf 6.
- Limites: TODOS os mobs atuais se afogam (nao ha mob aquatico no jogo); sem sufocamento de mob em bloco (de proposito: evitaria matar mobs gerados dentro do terreno em mundo convertido); convidado remoto na rede AINDA nao se afoga (o dano dele e decidido no cliente dele); altura dos olhos do mob e aproximada (85%).
- (VIS) porco/zumbi jogado em lago fundo: apos ~15 s leva dano a cada segundo; animais de superficie boiando so com as costas na agua nao se afogam.
- [ ] AINDA FALTA (fila): afogar convidado remoto; reflexo da agua de lado e de flores; Windows host/join; culling da laje de arenito (print); cana-de-acucar (bloco novo); elytra 1x1, nadar 1.13, rastejar, trapdoor; bloco embaixo de jogador/mob nao empurra; terreno; comandos.

## patchRPI.1 (modo Raspberry Pi 3B, parte 1/2)
- [x] `build-rpi.sh` (-DBL_RPI, -mcpu=cortex-a53, gera `blockland-rpi`, --pack); constantes em `main.cpp` (BL_WIN_W/H 640x420, BL_RPI_MAXRD 6)
- [x] janela 640x420 (minimo igual), vsync ligado + limitador de 60 FPS (`main_loop.inc`), `rdChunks()` max 6 e padrao 6 (`screens.inc`), `dynLightUpdate` desligado (`player_render.inc`)
- Teste: sintaxe ok com e sem -DBL_RPI. Limites: sem teste em Pi real (VIS); modo normal intacto.
- [ ] Falta (parte 2): grafico reduzido (shaders/sombras/god rays/reflexo off), folhas opacas + culling, padrao de controle (gamepad), osk/menus para 640x420

## patchRPI.2 (modo Raspberry Pi 3B, parte 2/2; tudo so com -DBL_RPI)
- [x] `BL_NO_FANCY 1` (`screens.inc`): sem pacote de shader, god rays, reflexo/agua realista; `shadowToggle` vira no-op (`shadows.inc`)
- [x] folhas: textura OPACA (transparentes -> media escurecida, `pkblocks.inc`) e culling folha-folha (`world.inc`); so faces voltadas ao ar
- [x] controle: teclado virtual sempre ligado (`gamepad.inc`); gamepad ja era o padrao de UI quando conectado
- Teste: sintaxe ok com e sem -DBL_RPI. Limites: sem teste em Pi real; GUI usa escala "Auto" (escala 1 em 640x420) (VIS)
- [ ] Falta: conferir no Pi (FPS, folhas, menus); se o menu ficar pequeno, forcar escala 2 em guiScaleFor (inventory_ui.inc)

## patchRPI.3 (Raspberry, itens pequenos da fila)
- [x] split no modo RPI: 4 chunks (`gRpiSplit`, `rdChunks`) e limitador em 30 FPS; panorama do titulo reduzido a raio 2 (`PANO_RAD`, nao removido por completo)
- Teste: sintaxe ok com e sem -DBL_RPI. (VIS) no Pi.
- [ ] Falta: panorama totalmente removido; "remover gatilhos de trás" (sem botao de gatilho traseiro no mapa; LT/RT sao os frontais - confirmar o que quer dizer); MP no Pi

## patchRPI.4 (Raspberry: luz estilo Alpha, menos atraso, -O3)
- [x] luz PLANA por face so com -DBL_RPI (`world.inc`): cada face usa a luz da celula da frente, sem media de 4 vertices e sem AO (como a Alpha); modo normal intacto
- [x] menos atraso de entrada: `glFinish()` apos o swap (`main_loop.inc`) impede a GPU de enfileirar quadros; vsync + limitador mantidos
- [x] `build-rpi.sh`: -O3, -fno-plt, -fomit-frame-pointer
- Teste: sintaxe ok com e sem -DBL_RPI. Limites: sem teste em Pi real; culling extra (alem de folha-folha e face enterrada) nao feito (VIS)
- [ ] Falta: medir FPS/atraso no Pi; culling por frustum/chunk oculto; panorama removido; MP no Pi

## patchRPI.5 (Raspberry: nuvens 2D + mais culling; so com -DBL_RPI)
- [x] nuvens 2D (`mining_sky.inc`): um plano unico (sem espessura/laterais/pre-passe), celulas vizinhas em x fundidas em faixas, raio 14 celulas (168 blocos) em vez de 40
- [x] culling: malha "longe" a partir de 3 chunks (`sFar = 3` em `main_loop.inc`): sem plantas/modelos pequenos e sem faces enterradas sob teto solido; Nether continua sem (regra existente)
- Teste: sintaxe ok com e sem -DBL_RPI. Limites: sem teste em Pi real; chunks a 3+ chunks nao mostram plantas (VIS)
- [ ] Falta: culling de oclusao por chunk (caverna); medir vertices/FPS no Pi; panorama removido; MP no Pi

## patchRPI.6 (Raspberry: sem panorama)
- [x] `main_loop.inc`: com -DBL_RPI o titulo nao gera mundo (`panoInit` pula o stream) e `drawPanorama` so limpa a tela em cinza escuro (tema escuro); economiza memoria/CPU na abertura
- Teste: sintaxe ok com e sem -DBL_RPI. Limites: sem teste no Pi (VIS: titulo com fundo liso e botoes legiveis)
- [ ] Falta: culling de oclusao por chunk; medir FPS no Pi; MP no Pi; "remover gatilhos de tras" (confirmar significado)

## patchRPI.7 (Raspberry: culling de visao mais justo)
- [x] `main_loop.inc`: `cullT` sem a folga de 8% (1.0 em vez de 1.08) so com -DBL_RPI; o frustum por chunk/secao ja existia
- Teste: sintaxe ok com e sem -DBL_RPI. (VIS) no Pi: se chunks "piscarem" nas bordas da tela ao girar, voltar a 1.08
- [ ] Falta: culling de oclusao por chunk; medir FPS no Pi; MP no Pi

## patchRPI.8 (inventario criativo: setas das abas de mods na tela)
- [x] `inventory_ui.inc`: com mais de 1 pagina de abas (mods), o painel desce ate ter 48 unidades de GUI no topo (`invLayout`); botoes "<" ">" e o "1 / N" seguem `min(50, oy/gs)` (`pageBtnRect`, `drawPager`); clique usa o mesmo retangulo
- Teste: sintaxe ok com e sem -DBL_RPI. Limites: sem teste em Pi real; em 640x420 (escala 2) a aba de baixo pode perder ~2 unidades no rodape (VIS)
- [ ] Falta: conferir no Pi (setas visiveis e clicaveis); se a aba de baixo cortar demais, usar escala 1 so com paginador

## patchRPI.9 (titulo em tela pequena)
- [x] `menu.inc`: splash "Faça de tudo!" limitado a borda direita (`spX`); se versao + copyright nao cabem lado a lado, a versao sobe uma linha (`twoLn`); com sh<240 botoes mais juntos (22/44/70 em vez de 24/48/84) em `drawTitle` e `menuEvent` (mesma geometria)
- Teste: sintaxe ok com e sem -DBL_RPI. Sem teste headless de UI (VIS)
- [ ] Falta: conferir no Pi (splash inteiro, rodape sem sobrepor, cliques nos botoes); demais telas (opcoes/mods) em 640x420 nao revisadas

## patchRPI.10 (V1: modelo do pistao)
- [x] `redstone.inc` rsBuild case 1/2: modelo com a frente em CIMA (+Y) girado por (rx, ry) conforme a direcao (antes frente=norte e a faixa de piston_side ficava de lado/errada => "andaime"); fechado = cubo com cabeca em cima; estendido: corpo 12px com piston_inner na frente + haste de 4px, cabeca = placa de 4px + haste de 12px; grudento usa piston_top_sticky
- Teste: sintaxe ok (com e sem -DBL_RPI); test_piston 142/0 falhas, test_chest 42/0 (logica; malha so com pack). Limites: sentido da rotacao deduzido de rsDirRot, nao visto no jogo (VIS)
- [ ] Falta: conferir no jogo pistao/grudento nas 6 direcoes, fechado e estendido; se algum lado sair invertido, trocar rx/ry em rsBuild; V2 portas, V3 arenito; icone do pistao grudento no inventario (cubo, nao revisado)

## patchRPI.11 (sons basicos 1.12.2)
- [x] `audio.inc` (novo): mixer proprio sobre SDL_OpenAudioDevice (44,1 kHz estereo, 24 vozes, sem SDL_mixer), le WAV PCM16/IMA-ADPCM; sons em `assets/sounds/**.wav` (os .ogg do zip foram convertidos com ffmpeg: ADPCM ~11 MB; musica 22,05 kHz estereo). BLOCKLAND_MUTE=1 desliga.
- [x] ligado: quebrar bloco (`random/break`, tom variado) e colocar (`step/stone1-3` a 0,8) em `main_loop.inc`; passos a cada ~1,67 blocos (`step/stone`, so no chao e sem agachar); clique de menu (`random/click`, titulo/telas); acerto de flecha em mob (`random/successful_hit`, `bow.inc`); mobs (`mobs.inc`): fala ociosa, dano e morte de porco/galinha/vaca/zumbi/esqueleto/creeper + `plop` da galinha ao botar ovo; musica `calm1/calm2` (1a faixa ~5 s apos entrar, depois 10-20 min; para ao voltar ao titulo, `menu.inc`)
- Teste: `tests/test_audio.cpp` (decodificador contra PCM do ffmpeg nos 45 arquivos, diferenca <= 2; mixer: pan, pitch, soma, fim da voz) 0 falhas; test_piston 142/0, test_egg 21/0; sintaxe ok com e sem -DBL_RPI. SEM teste em dispositivo de audio real (VIS/auditivo)
- Limites: passos sempre "pedra" (so ha esse som); sem sons de agua/lava/porta/andar em outros materiais, nem de convidado remoto (mobs sao simulados no host); sem controle de volume (menu), sem musica de menu; mob sem som no zip (ovelha, aldeao, pigman) fica mudo; clique so no menu do titulo/telas (nao no pause/inventario)
- [ ] Falta: conferir no jogo (volume, estalos no Pi); controle de volume nas opcoes (`gSndMaster`, `gSndMusicVol` ja existem); clique no pause/inventario; sons de dano do jogador, queda, explosao, porta, bau; sons para convidado no MP

## patchRPI.12 (sistema de musica + volumes)
- [x] musica calm1/calm2 (`audio.inc`): `sndMusicStart` alterna as faixas (nunca repete a ultima), volume ao vivo pelo slider, slider Musica = 0 nao toca; 1a faixa ~5 s apos entrar, depois 10-20 min (como o MusicTicker do Java); para ao voltar ao titulo
- [x] Opcoes > Musica & Sons agora FUNCIONAM: m0 Volume Principal, m1 Musica, m4 Blocos, m5 Criaturas Hostis, m6 Criaturas Amigaveis, m7 Jogadores (passos) -> `sndSyncVol` (`screens.inc`); salvos em options.cfg (`soundvol=`) ao soltar o slider
- [x] `/music play [1|2] | stop | status` (`chat_debug.inc`, tambem na /ajuda)
- Teste: sintaxe ok com e sem -DBL_RPI; test_audio 0 falhas (decodificador + mixer). Sem teste em dispositivo de audio real (auditivo)
- Limites: sliders m2 Blocos Musicais, m3 Clima, m8 Ambiente ainda nao tem som associado; sem musica de menu; passos (m7) tocam so no proprio jogador
- [ ] Falta: conferir no jogo; botoes "Sound Device"/"All Sounds"/"Refresh" da tela ainda sao visuais; clique no pause/inventario; sons de dano do jogador/porta/bau/explosao; V2 portas, V3 arenito

## patchRPI.13 (sons: grama, ovo, itens, UI, events.cfg, saida rapida)
- [x] bug: quebrar grama tocava `random/break` (som de bigorna); agora grama/terra/folha/plantas usam `dig/grass1-4`, o resto `step/stone` (`sndBlockEvt`, `drops.inc`); colocar bloco idem
- [x] sons novos: ovo arremessado (`random/bow`) e impacto (`random/pop`), pegar item (`random/pop`, tom variado), dropar item (Q/Ctrl+Q/split/gamepad via `throwItem`)
- [x] sistema facil: `assets/sounds/events.cfg` (`evento = arquivos ; vol pitch var cat`, `nome#` = nome1..9); recriado se apagado; `sndEvtAt/sndEvtUi` em `audio.inc`; `tools/ogg2wav.sh` converte .ogg/.mp3 em .wav
- [x] clique de UI tambem no pause, tela de morte e inventario (alem do menu principal)
- [x] salvar e sair: fade 350->120 ms e titulo entra com fade curto (0,12 s; botoes sem atraso)
- Teste: `test_audio` (eventos, # expande, arquivo ausente, 7 wavs novos) 0 falhas; test_egg 21/0; test_piston 142/0; sintaxe ok. Sem teste em audio real nem de tempo (VIS/auditivo)
- Limites: o jogo so le .wav (sem decodificador ogg); baus (`block/chest/*`), pérola e sons de convidado remoto ainda sem gancho; o tempo de `saveCurWorld` nao foi medido
- [ ] Falta: conferir no jogo; sons de bau abrir/fechar; clique em chat/gamepad nos menus; se a saida ainda demorar, medir `saveCurWorld`/`resetWorldData`

## patchChisel.1 (Chisel 1.12.2, parte 1: blocos)
- [x] `tools/chisel_import.py`: converte o zip de assets do Chisel em `assets/chisel/` (2040 blocos cubicos, 1996 tiles 16x16 deduplicados, `chisel.cfg`); topo/lado/base por modelo (coluna, bottom_top, 2 camadas compostas, lava/agua)
- [x] `asset_manager.inc` `loadChisel()`: registra como itens do pack (file `chisel/<material>/<variante>`, salvos/rede por nome), luz (glowstone, lavastone, magma), `pkTileEnd()` (main.cpp) substitui `PKTILE0+PK.size()` em entmodels/models/nether; aba "Chisel" no criativo (`mods.inc`), busca por nome; funciona sem resource pack
- Teste: `tests/test_chisel.cpp` 8/0 (registro, faces, luz, hashes unicos); test_piston 142/0, test_chest 42/0, test_egg 21/0; sintaxe ok
- Limites: nomes em ingles; 339 variantes ignoradas (carpete, auto_chisel, modelos vanilla); CTM e animacao viram o 1o quadro/textura base; atlas 16px compartilhado com o pack (sobra ~400 tiles; excedente e cortado, ver log "Chisel:"); aba unica rolavel (VIS)
- [ ] Falta (parte 2): Cinzel (item que troca variante ao clicar) + receitas; lajes/pilares/vidros/barras (`01_Slabs`, `ironpane`); nomes em portugues; icones dos itens (`03_Itens`); conferir no jogo com pack grande se cabem todos os tiles

## patchChisel.2 (Chisel parte 2: cinzel)
- [x] itens Iron Chisel / Diamond Chisel (`assets/chisel/items/`, pilha 1) registrados em `loadChisel()`; receita graveto + lingote de ferro / diamante (`crafting.inc`)
- [x] `chisel_tool.inc`: botao direito num bloco do Chisel com o cinzel troca para a proxima variante do mesmo material; agachado = anterior; desgaste (ferro 250, diamante 5000; criativo nao gasta); sincroniza por `netSendBlock`; gancho em `main_loop.inc` antes do isqueiro
- Teste: `test_chisel` 15/0 (troca, volta, desgaste, quebra, bloco vanilla ignorado, criativo); test_piston 142/0, test_egg 21/0; sintaxe ok
- Limites: sem GUI de selecao do Chisel (so cicla); grupo = pasta do material (nao mistura com pedra vanilla); sem clique esquerdo/area; nomes em ingles (VIS no jogo)
- [ ] Falta (parte 3): lajes/pilares de `01_Slabs`, vidros e barras (`ironpane`); GUI do cinzel; nomes em portugues; vanilla na mesma lista (pedra, tijolos...)

## patchChisel.3 (Chisel parte 3: nomes em portugues)
- [x] `tools/chisel_import.py`: dicionario PT (~230 palavras), substantivo na frente + concordancia de genero/numero ("Marmore: Tijolos Desgastados", "Pedra Antiga: Tijolos Enfileirados"); cinzeis "Cinzel de Ferro/Diamante"; busca do criativo acha pelo nome em portugues; `chisel.cfg` regenerado (tiles iguais)
- Teste: `test_chisel` 15/0; sintaxe ok
- Limites: traducao por palavras (alguns nomes podem ficar estranhos; palavras fora do dicionario ficam em ingles); sem acento na busca (foldAscii)
- [ ] Falta: lajes/escadas do Chisel (o motor guarda material de laje em 6 bits em `redstone.inc`; precisa de tabela maior, ver comentario da linha 61); vidros e barras; GUI do cinzel; vanilla na mesma lista de variantes; clique esquerdo/area

## patchTex.1 (pack Default-Java-1.12.2 ativo)
- [x] `assets/resourcepacks/Default-Java-1.12.2.zip` (reempacotado: sem a pasta extra, com `pack.mcmeta`) + `pack.cfg` apontando para ele
- [x] `mcpack.inc`: pastas 1.12 `textures/blocks/` e `textures/items/` aceitas como `block/` e `item/` (get e listPng); +724 blocos/itens extras do pack listados
- Teste: abre o pack, `blocks/stone.png` e `planks_oak` resolvem; test_chisel 15/0, test_piston 142/0, test_chest 42/0, test_egg 21/0
- Limites: itens extras ficam com nome 1.12 ("Planks Oak", "Bucket Water"); recursos que procuram nome 1.13 (balde de agua/lava, isqueiro: `item/water_bucket`, `item/flint_and_steel`) podem nao achar; sem teste visual (VIS)
- [ ] Falta: tabela de renomeio 1.12->1.13 dos itens (balde, isqueiro, barcos...) ; conferir no jogo blocos orientados (fornalha, abobora), vidro, folhas, agua

## patchChisel.4 (vidros/translucidos + vanilla no grupo)
- [x] `loadChisel()`: tiles com alpha viram `gPkTrans` (1 = semi-translucido com blend, 2 = alpha-test); variantes de `chisel/glass` ficam transparentes para a luz (opac 0)
- [x] `chisel_tool.inc`: pedra, pedregulho, terra, tabuas de carvalho, tijolos, arenito, obsidiana, glowstone e estante entram como 1o item do grupo do cinzel (ciclo vanilla -> variantes -> vanilla)
- Teste: `test_chisel` 39/0 (translucidez, vidro, ciclo com vanilla, sem grupo = nada); test_piston 142/0; sintaxe ok
- Limites: 311 variantes `cube_all` continuam fora porque a textura (ex. `ancient_stone/emboss`) nao existe no zip enviado; so 19 vidros (sem paineis `glasspane`); translucido do Chisel sem teste visual (VIS)
- [ ] Falta: lajes/escadas (tabela de material em `redstone.inc`); paineis de vidro/barras; GUI do cinzel; clique esquerdo/area

## patchTex.2 (GUI do pack 1.12.2 + mapa de blocos faltando)
- [x] `asset_manager.inc`: `loadPackGUI()` recorta do pack ativo botoes (widgets y=66/86/46), hotbar, selecao, inventario, bancada, 3 abas do criativo e scroller; sem o arquivo no pack mantem o GUI do jogo; `gui_pack=0` em `assets/ui/gui.cfg` desliga
- [x] `pk112.inc`: renomeio 1.12.2 -> nomes canonicos (planks_acacia -> acacia_planks, wool_colored_red -> red_wool, hardened_clay_stained_*, concrete_*, portas, flores, trilhos...); extras e blocos X_top/X_side passam a usar o nome canonico (casa com `ptbr.inc`)
- [x] `doc/BLOCOS_FALTANDO.md` (gerado por `tests/test_pkaudit.cpp`): 437 texturas de bloco extras (236 cubos, 201 nao-cubos: portas 6, plantas/estagios 70, trilhos 8, comandos 9, outros 108), 178 itens extras, 304 sem nome PT
- [x] `tests/stubs/SDL.h`: +SDL_WaitThread (a sintaxe falhava)
- Teste: `test_packgui` 10/0 (recortes reais do zip), `test_pk112` 10/0, test_chisel 39/0, test_piston 142/0, test_egg 21/0; sintaxe ok
- Limites: so compila e passa headless. (VIS) o GUI do pack 1.12 e CLARO (cinza), conflita com a preferencia de tema escuro: `gui_pack=0` volta ao escuro. Abas do criativo continuam as do jogo (26px x 28px do pack). Slot, tooltip e mira nao trocados. Nao-cubos do relatorio ainda nao sao colocaveis
- [ ] Falta: nomes PT dos 304 extras; modelos dos nao-cubos (portas, plantas, trilhos, comandos); abas/slot/tooltip/mira do pack; icones; conferir no jogo

## patchTex.3 (laje de arenito + icones do pack 1.12) (V3)
- [x] `models.inc`: `pk112Rev()` no `xbTexTile`: textura canonica (acacia_planks, nether_bricks, red_sandstone...) que so existe com nome 1.12 no pack passa a resolver; antes 7 dos 15 materiais de laje/escada tinham face -1 (quad descartado = topo/lado sumindo)
- [x] `mcpack.inc`: `packLegacyAlias` (~70 nomes 1.13 -> 1.12: redstone_dust, book_normal, beef_raw, bow_standby, spawn_egg, bucket_*, minecart_*...) em `packTile16`; fim dos icones magenta (aba Redstone/Pesquisar, carne, arco, ovos); `gPackMiss` lista o que nenhum nome resolveu
- Teste: `test_slabtex` 15/0 (todo material de RS_SM resolve lado/topo/base; nenhum MISS), test_pk112 10/0, test_packgui 10/0, test_chisel 39/0, test_piston 142/0, test_chest 42/0, test_egg 21/0; sintaxe ok
- Limites: causa da laje deduzida pelo teste de textura (VIS: conferir a laje de arenito no jogo); ovos de spawn ficam sem a tinta por mob; abas/slot do GUI ainda do jogo
- [ ] Falta: blocos/itens 1.12 sem modelo (`doc/BLOCOS_FALTANDO.md`); nomes PT dos extras; conferir no jogo

## patchTex.3b (laje de arenito invisivel) (V3)
- [x] `mcpack.inc` `gCoreTile` + `asset_manager.inc`: lado/topo/base do arenito (e demais texturas de bloco do jogo) registrados; `models.inc` `xbTexTile` usa esse tile antes de alocar outro. Causa provavel: tile novo alocado depois do envio do atlas ficava em branco
- Teste: `test_slabtex` 17/0 (os 15 materiais usam tile ja existente; arenito = TILES[SANDSTONE]); test_pk112, test_piston, test_chest ok; sintaxe ok
- Limites: nao testado no jogo (VIS)
- [ ] Falta: botoes/abas/slots ainda escuros no GUI (`inventory_ui.inc`, `asset_manager.inc`); mais blocos 1.12 (`doc/BLOCOS_FALTANDO.md`)

## patchTex.4 (abas claras, texto legivel, cacto na mao)
- [x] `asset_manager.inc`: abas do criativo vem de `creative_inventory/tabs.png` (celulas 28x32, 4 linhas x 7 colunas); `gGuiLight` liga quando o inventario usa o pack; `gTabW`
- [x] `inventory_ui.inc`: `UI_TXT` = 0x404040 com GUI claro (era 0xE0E0E0, branco sobre cinza = sem contraste); campo de busca idem; aba 28px sem recuo
- [x] `render_items.inc`: cacto na mao/chao/3a pessoa com as 4 laterais 1/16 para dentro (como o modelo 1.12 e como ja era no mundo)
- Teste: `test_cactus_hand` 4/0, `test_packgui` 38/0 (+28 recortes de abas), test_slabtex 17/0, test_piston 142/0, test_chest 42/0, test_chisel 39/0; sintaxe ok
- Limites: (VIS) nao conferido no jogo. Nao mexi: hotbar do HUD (o do pack 1.12 e preto translucido de verdade), botoes `<` `>` desativados (textura escura do pack), slot de lixeira/mao secundaria e botao do capacete do canto
- [ ] Falta: slot/tooltip/mira do pack; botao do capacete e slot escuro do canto (prints ajudam); mais blocos 1.12 (`doc/BLOCOS_FALTANDO.md`)

## patchTex.5 (slots claros no inventario)
- [x] `asset_manager.inc`: `rtSlot` = moldura 18x18 da `inventory.png` do pack; troca o slot ESCURO do Rethoughted na lixeira do criativo, nos slots do bau e onde mais `rtSlot` e usado (era a caixa preta no canto do inventario)
- Teste: `test_packgui` 40/0 (slot miolo 139), test_chest 42/0, test_piston 142/0; sintaxe ok
- Limites: (VIS) nao conferido no jogo; botoes `<` `>` desativados, hotbar do HUD e botao do capacete do canto seguem como estao
- [ ] Falta: botao do capacete (print), tooltip/mira do pack, mais blocos 1.12 (`doc/BLOCOS_FALTANDO.md`)

## patchInv.1 (slots de armadura padrao + ovos de todos os mobs)
- [x] `asset_manager.inc`: slots vazios de capacete/peitoral/calca/bota/escudo usam a textura PADRAO do pack (`items/empty_armor_slot_*`, ou `item/...` 1.13+); sem pack, mantem `gui/empty_*.png`
- [x] `inventory_ui.inc` aba Diversos: ovos de aldeao, porco-zumbi, zumbi, esqueleto e creeper (os 9 mobs do jogo agora tem ovo)
- Teste: sintaxe ok; test_packgui/test_egg abaixo
- Limites: (VIS) nao conferido no jogo; ovos sem a tinta por mob (textura do item)
- [ ] Falta: slot/tooltip/mira do pack; botao do capacete (print); V1-V6

## patchInv.2 (abas azul/vermelha do criativo)
- [x] `inventory_ui.inc` drawTabs: a coluna 7 do `tabs.png` do pack 1.12 e um placeholder de debug ("UN SEL" vermelho / "SEL" azul); com o GUI do pack (gTabW==28) as abas das colunas 6 e 7 usam a coluna 6 (cinza)
- Teste: sintaxe ok (sem teste headless de UI)
- Limites: (VIS) nao conferido no jogo
- [ ] Falta: mais blocos 1.12 (ids XB 900..1015 estao cheios; precisa ampliar XB_END em main.cpp:151 e hooks em blockupdate.inc:133-136): comecar por plantas (deadbush, estagios de cultura), depois command blocks, bolo, caldeirao, bigorna danificada

## patchBlk.1 (lajes e escadas 1.12.2 completas)
- [x] Auditoria: as 14 escadas e as lajes da 1.12.2 ja existiam como bloco (RS_SM 0..14); faltavam receitas e 2 correcoes
- [x] `chest.inc`: receitas de escada (6 blocos -> 4) e laje (3 -> 6) de abeto, betula, selva, acacia, carvalho escuro, tijolos, tijolos do Nether, quartzo, arenito vermelho e purpur (antes so carvalho, pedregulho, arenito, tijolo de pedra)
- [x] `redstone.inc`: laje de pedra (var 10) usa `stone_slab_side`/`stone_slab_top` como no 1.12; nova var 15 = laje de madeira petrificada (sem receita, como no jogo); `assets/creative/mc-1.12.2.cfg` + `ptbr.inc` com o item
- Teste: test_slab_mats 11/0 (atualizado p/ 16 variantes), test_slabtex 18/0 (com pack), test_piston 142/0, test_chest 42/0; sintaxe ok. test_craft_pack nao rodou (precisa do Ashen_16x)
- Limites: (VIS) nao conferido no jogo; receitas nao testadas headless; RS_SM chegou ao maximo de 16 (4 bits): materiais novos exigem ampliar o meta
- [ ] Falta: laje dupla soltar 2 lajes (conferir); mais blocos 1.12 (plantas, command blocks, bolo, caldeirao)

## patchBlk.2 (caminho de terra 15/16 + caldeirao e bolo)
- [x] `redstone.inc`: caminho de terra (grass_path/dirt_path) vira XB_FARM meta 1: 15/16 de altura em modelo, colisao e selecao; item do pack mapeado, drop = terra, colocavel em qualquer lugar; `village.inc`/`village_build.inc` geram o caminho novo
- [x] Novo `XB_DECOR` (id 1006, `main.cpp`): var 0 caldeirao (modelo com pernas, paredes e fundo; colisao 5 caixas, selecao cubo), var 1 bolo (modelo 1.12, 7 pedacos, precisa de chao, clique direito come: +2 fome/+0,4 saturacao, so com fome fora do criativo, nao dropa)
- Teste: `test_decor` 44/0 (novo; forma, colocar, suporte, comer, texturas do pack), test_slab_mats 11/0, test_slabtex 18/0, test_piston 142/0, test_chest 42/0, test_egg 21/0; sintaxe ok
- Limites: (VIS) nao conferido no jogo; caminho ja existente em saves antigos continua cubo cheio (bloco antigo do pack); caldeirao sem agua/enchimento; itens ficam como sprite plano (como no 1.12)
- [ ] Falta: converter caminho antigo (PK) ao carregar chunk; agua no caldeirao; command blocks, bancada de pocoes, plantas (arbusto seco, estagios); `assets/creative/mc-1.12.2.cfg` do bolo/caldeirao conferir aba

## patchBlk.3 (criativo: blocos/itens do pack 1.12 aparecem nas abas + folhas pretas + madeiras)
- [x] `mods.inc` loadCreativeCfg: o cfg usa nomes 1.13 (white_concrete) e o pack 1.12 usa os antigos (concrete_white): alias pk112/pk112Item + tabela CA (furnace_front, anvil, piston, quartz_ore...) -> 485 itens deixavam de aparecer; agora 234 -> 660 itens nas abas (BUILD 65->195, DECO 29->170, BREW 8->51, MATERIALS 33->55). `BL_CREATIVE_DEBUG=1` imprime CFGMISS
- [x] `pk112.inc`: pk112Item (door_wood, record_13, dye_powder_*, minecart_*, bucket_*, fish_*, wood_/gold_ ferramentas) e pkCanon; `redstone.inc` rsMapItems acha o item do pack 1.12 por nome canonico (portas viram porta colocavel, nome PT "Porta de abeto"...)
- [x] Aba "Resource pack" nao repete o que ja entrou nas abas (gCreativeBase)
- [x] `inventory_ui.inc` drawIcon: folha (isLeafB) no inventario/hotbar tem silhueta PRETA atras: pixels transparentes ficam pretos
- [x] Madeiras (abeto, betula, selva, acacia, carvalho escuro): tabuas, tronco, folhas, muda, laje, escada, cerca, portao, alcapao, porta, barco, placa de pressao nas abas (conferido por teste)
- Teste: `test_creative` 0 falhas (novo, usa o pack), test_piston 142/0, test_chest 42/0, test_egg 21/0, test_decor 44/0, test_slab_mats 11/0; sintaxe ok
- Limites: (VIS) nao conferido no jogo; sem bloco "madeira com casca nos 6 lados" (nao existe no 1.12 criativo); placas de outras madeiras e ovos/pocoes por nome nao tem textura no pack 1.12 (59 do cfg); ovos de mobs sem item ficam fora
- [ ] Falta: bloco de casca (oak wood) 1.13; ovos dos demais mobs; V1-V6; fila normal

## patchInv.3 (folha clara, bigorna)
- [x] `inventory_ui.inc` drawIcon: removida a silhueta preta atras da folha (inventario/hotbar mais claros)
- [x] `pk112.inc`: alias anvil_base -> anvil (o modelo da bigorna pedia block/anvil, que no pack 1.12 e anvil_base)
- Teste: sintaxe ok (sem teste headless de UI/pack)
- Limites: (VIS) nao conferido no jogo; tochas nao alteradas (sem o pack nao achei a causa); placas nao tocadas
- [ ] Falta: tochas faltando (qual aba/qual tocha? print), placas (poste curto/contorno, `redstone.inc`); V1-V6; fila normal

## patchInv.4 (tochas, cacto no inventario)
- [x] `pkblocks.inc`: alias gPkByFile block/torch, block/redstone_torch, block/anvil -> torch_on/redstone_torch_on/anvil_base do pack 1.12 (textura, drop e item das tochas e da bigorna)
- [x] `inventory_ui.inc` drawIcon: cacto com laterais recuadas 1/16 (igual ao bloco no mundo)
- Teste: sintaxe ok (sem pack/UI headless)
- Limites: (VIS) nao conferido no jogo; causa das tochas deduzida do codigo
- [ ] Falta: placas (poste/contorno, redstone.inc); V1-V6; fila normal

## patchInv.5 (placa no inventario igual 1.12.2)
- [x] `redstone.inc` rsIconQuads: placa com item do pack (item/...) usa o sprite plano (inventario, mao, chao), como no 1.12.2; o bloco colocado segue com o modelo 3D (entSign)
- Teste: test_piston/test_chest/test_egg (regressao); sintaxe ok
- Limites: (VIS) nao conferido no jogo; modelo da placa colocada (poste/contorno) NAO alterado: entSign ja segue as medidas do 1.12 (poste 9,3px + tabua 8px) e a selecao e .25..75 x 0..1; sem print mostrando o erro nao mexi
- [ ] Falta: poste curto/contorno da placa colocada (print de perto + angulo); V1-V6; fila normal

## patchV.1 (uma placa so; V1 auditado; V2 porta sem modelo)
- [x] `assets/creative/mc-1.12.2.cfg`: so a placa de carvalho no criativo (1.12.2 tem uma so)
- [x] V1 pistao: auditado, modelo fechado/estendido (corpo 12px, cabeca 4px+haste), colisao e selecao ja seguem o 1.12 (RPI.10); nada alterado
- [x] V2 `redstone.inc` case 15: se o pack nao traz models/ da porta (1.12), usa lamina vanilla de 3px embutida (mesma rotacao/abertura)
- Teste: test_piston 142/0, test_chest 42/0, test_egg 21/0; sintaxe ok
- Limites: (VIS) nao conferido no jogo; sem o pack nao reproduzi a porta "moldura"; dobradica espelhada (UV) nao tratada no fallback
- [ ] Falta: V2 se persistir (print da porta + pack), V3 arenito, V4-V6; placa colocada (poste/contorno)

## patchV.2 (bigorna 1.12: topo; placa: textura do pack 1.12)
- [x] Bigorna: topo usava anvil_base (bacia escura); agora anvil_top -> anvil_top_damaged_0 (`mods.inc` CA, `pkblocks.inc`) e topo rachado/quebrado -> anvil_top_damaged_1/2 (`models.inc` xbTex)
- [x] Placa: `entmodels.inc` entLoadOne carrega `entity/sign.png` (1.12) como signs/oak; sem isso caia no modelo de reserva (tabuas)
- Teste: test_piston 142/0, test_chest 42/0, test_egg 21/0; sintaxe ok
- Limites: (VIS) sem o pack nao conferi; faixa cinza no topo da tabua da placa pode ser a textura do pack (UV segue o 1.12); poste/contorno nao mexidos
- [ ] Falta: conferir bigorna/placa no jogo (print); V2 porta; V3-V6

## patchV.3 (portas com pack 1.12)
- [x] `redstone.inc` case 15: lamina de reserva (pack sem models/) estava girada 90 graus (z 0..3); agora e a do vanilla (x 0..3, 16x16x3) com UV da dobradiça espelhada, mesmas rotacoes de aberta/fechada; textura resolvida pelo alias pk112 (door_wood_lower etc.)
- [x] Novo `tests/test_door_pack.cpp` (precisa de assets/resourcepacks/Default-Java-1.12.2.zip): 7 tipos x metade x aberta x dobradica com malha e tile >= 0; caixas finas (3px) nos 4 lados, fechada e aberta
- Teste: test_door_pack 0 falhas; test_village_door, test_piston 142/0, test_chest 42/0, test_egg 21/0; sintaxe ok
- Limites: (VIS) nao conferido no jogo; com pack que traz models/ (1.13+) vale o modelo do pack; sentido da abertura (dobradica esq/dir) conferido so pela caixa
- [ ] Falta: V3 arenito; V4-V6; placa colocada (poste/contorno)

## patchV.4 (plantas colocaveis, abobora, fornalha com o pack 1.12)
- [x] `pkblocks.inc`: flower_*/double_plant_*/sapling_*/tallgrass do pack 1.12 ganham alias canonico em gPkByFile (poppy, sunflower_top...): itens ligados ao bloco, entao flores, girassol e mudas voltam a ser colocaveis
- [x] `pkblocks.inc` FD: pumpkin_face_off/on (topo/base = pumpkin_top, rosto so na frente) e furnace_front_off como frente da fornalha (antes: rosto/frente nas 6 faces)
- [x] Bigorna: auditado o modelo do icone (gAnvQ) com o pack: topo = tile proprio (769) ≠ corpo (768) (corrigido no patchV.2)
- [x] Novo `tests/test_icons_pack.cpp` (precisa do pack): plantas -> bloco, faces da abobora/fornalha, tiles da bigorna
- Teste: test_icons_pack 0 falhas; test_piston 142/0, test_chest 42/0, test_egg 21/0, test_creative 0 falhas; sintaxe ok
- Limites: (VIS) nao conferido no jogo; nao tratado ainda: shulker, cama, painel de vidro, bau/mesa de encantamento/fornalha transparentes no icone, zumbi (braco/perna), alavanca, pistao normal
- [ ] Falta: shulker (modelo posto + icone), cama, painel de vidro 1.12, icones transparentes (bau, mesa de encantamento), zumbi sem braco/perna, alavanca, pistao normal x grudento

## patchV.5 (zumbi, pistao normal no icone)
- [x] `mobs.inc` biped64: braco/perna esquerdos do zumbi espelham os direitos (zombie.png 64x32 do 1.12 nao tem UV esquerdo)
- [x] `redstone.inc`: icone do pistao normal = cubo com piston_top_normal em cima e piston_side nos lados (como o grudento)
- Teste: test_piston 142/0, test_chest 42/0, test_egg 21/0; sintaxe ok
- Limites: (VIS) nao conferido no jogo
- [ ] Falta: shulker (posto + icone), cama, painel de vidro 1.12, icones transparentes (bau, mesa de encantamento), alavanca

## patchV.6 (mesa de encantamento no icone - palpite)
- [x] `inventory_ui.inc` drawIcon: silhueta opaca atras do icone da mesa de encantamento (reaproveita o bloco da silhueta que estava desligado para folhas)
- Teste: sintaxe ok
- Limites: (VIS) PALPITE: sem print nao confirmei a causa da transparencia; bau nao tratado
- [ ] Falta: bau transparente no icone, shulker, cama, painel de vidro 1.12, alavanca, placa colocada, V3-V6

## patchInstaller.1 (setup.exe Windows)
- [x] `installer/` (blockland.iss, build-installer.sh, blockland.ico/.rc/.manifest, README.md); `build-windows.sh --installer` e `TARGET=windows ./build.sh`; exe com icone+manifesto asInvoker (windres)
- Teste: bash -n nos scripts, XML do manifesto, icone .ico gerado
- Limites: (VIS) nao compilei no Windows nem rodei Inno/Wine; falta testar instalar, atualizar (saves/profile/resourcepacks preservados), atalhos e desinstalar
- [ ] Falta: rodar no Windows, assinatura digital (README), depois V8 a V12

## patchWin.1 (avisos do mingw)
- [x] `modules/monitor/web/jpeg.inc`: `1L << 60` virava 0/UB no Windows (long = 32 bits) => tabela de Huffman do JPEG do monitor nao era montada; agora `long long`/`1LL`
- [x] `mining_sky.inc:282`: `#endif` com texto solto virou comentario (aviso -Wendif-labels)
- Teste: sintaxe sem erros e sem esses avisos; test_monitor 10/0 (test_web precisa de servidor de rede; nao roda aqui)
- Limites: (VIS) monitor com JPEG no Windows nao testado
- [ ] Falta: instalador no Windows, V8 a V12

## patchMap.1 (conversor: portas, alcapoes, camas, botoes, teia)
- [x] `conversor/mapping.py`: portas (7 tipos; dir/aberta/cima), alcapoes (oak/ferro; dir/topo/aberto), cama (dir, pes/cabeca, branca), botoes (face + pedra/madeira), teia -> `block/cobweb`; saem da lista de "ar"
- [x] `conversor/convert.py`: metade de cima da porta herda o d da de baixo (como a planta alta)
- Teste: mapping.resolve conferido por script (meta calculada a mao); sintaxe ok. O mapa original NAO esta no projeto: precisa reconverter (python3 convert.py saida/) com o mundo em /tmp/m e o pack em /tmp/a
- Limites: (VIS) direcao de porta/alcapao/cama/botao e palpite pelo modelo (rotacao em redstone.inc); dobradica da porta perdida (sem bit no LUT); cor da cama sempre branca (tile entity); botao/placa apertados ignorados
- [ ] Falta: reconverter o mapa; baus/placas/mobs/posicao do jogador; carregar chunks sob demanda; placas de pressao; V8 a V12

## patchV.8 (ovos de spawn, folhas brancas)
- [x] `mcpack.inc` packSpawnEgg: ovo = `spawn_egg` tingido (cor 1) + `spawn_egg_overlay` tingido (cor 2) com as cores do 1.12 por mob; so quando o pack nao tem `<mob>_spawn_egg.png` (antes: base branca/sem textura)
- [x] Folhas: o pack 1.12 chama `leaves_spruce` etc. e as checagens procuravam `_leaves` => nao era cubo (sprite plano), sem tinta (branca). Agora usa `pk112(n)` em `asset_manager.inc` (pkIsCube), `pkblocks.inc` (gPkLeaf, tinta de bioma tambem no mundo) e `render_items.inc` (tinta do icone)
- Teste: sintaxe ok; test_piston 142/0, test_chest 42/0, test_egg 21/0
- Limites: (VIS) nao conferido no jogo; icones semi-transparentes NAO tratados (sem exemplo de quais blocos)
- [ ] Falta: ícones semi-transparentes (preciso de print/lista de blocos), V9 a V12, mapa convertido (reconverter, baus/placas/mobs, sob demanda)

## patchInstaller.2 (gerar o setup.exe com 2 cliques)
- [x] `installer/gerar-setup.bat`: instala MSYS2 + Inno Setup via winget se faltarem, roda `build-windows.sh --installer` no MSYS2 MinGW64 e abre `dist\`; README atualizado
- Teste: nenhum (sem Windows/Wine aqui); wizard do .iss ja era Avancar->Instalar->Concluir (pagina "pronto" e de grupo desligadas)
- Limites: (VIS) nao rodei no Windows; depende do winget e do caminho padrao C:\msys64
- [ ] Falta: testar no Windows, assinatura (README), V9 a V12

## patchV.9 (icones semi-transparentes)
- [x] `inventory_ui.inc` drawIcon: todo icone em cubo ganha fundo opaco (silhueta cinza escura) exceto vidro/gelo/slime/mel/folhas/agua/portal/tingidos; modelos de entidade (bau, cama, placa, shulker) ganham fundo opaco por quad
- Teste: sintaxe ok (icones sem teste headless)
- Limites: (VIS) palpite pelo codigo: fundo escuro .2 pode aparecer em bordas de blocos com furos (cacto, bigorna); se ficar escuro demais, trocar a cor ou o fator .2; modelos finos (tocha, alavanca, trilho) NAO mexidos
- [ ] Falta: conferir no jogo; V9(luz de lajes/escadas) a V12; mapa convertido

## patchBuild.1 (build completo pelo Linux)
- [x] `build-tudo-linux.sh`: apt (g++, mingw, nsis), baixa SDL2-mingw oficial, gera `./blockland`, `./blockland.exe` e `dist/setup.exe` em um comando
- [x] `installer/blockland.nsi` (NSIS, roda no Linux): mesmo comportamento do Inno (instala/atualiza, %LOCALAPPDATA%, atalhos, uninstall, preserva saves/profile/resourcepacks); `build-installer.sh` usa NSIS quando nao ha wine/iscc (ou INSTALLER=nsis)
- Teste: bash -n nos scripts; NAO rodei (apt sem acesso aqui; sem makensis nem mingw)
- Limites: (VIS) nao compilei o .nsi nem o cross-build; possiveis ajustes de sintaxe NSIS/URL do SDL2 na primeira execucao
- [ ] Falta: rodar no Linux/Windows e me mandar a saida de erro; V9 (luz laje/escada) a V12

## patchV.10 (luz de lajes e escadas)
- [x] `models.inc` emitXB: laje/escada (opacidade 15 no 1.12) ficavam com a PROPRIA celula sem luz => faces escuras/erradas; agora usam o maximo de ceu/bloco entre a celula e os 6 vizinhos (como o useNeighborBrightness do MC)
- Teste: sintaxe ok (malha sem teste headless)
- Limites: (VIS) nao conferido no jogo; a malha so refaz quando o chunk e remalhado (luz do vizinho mudando no chunk ao lado pode atrasar); sombreamento por direcao nao mexido
- [ ] Falta: V10 camas todas as cores + alcapao so carvalho; V11 vidros/paineis coloridos; V12 blocos fora da 1.12.2 + plantas; mapa convertido

## patchV.11 (camas todas as cores, alcapao so carvalho)
- [x] `inventory_ui.inc` drawIcon: cama no inventario = modelo 3D ModelBed (entBed, 2 pecas) nas 16 cores, so a textura muda (antes: sprite plano montado a mao)
- [x] `entmodels.inc`: cama cinza-clara tambem aceita `entity/bed/silver.png` (nome do pack 1.12) no modelo e no sprite
- [x] `redstone.inc`: removidos do registro/criativo/nomes os alcapoes de abeto, betula, selva, acacia e carvalho escuro (ficam so ferro e carvalho; os ja colocados continuam desenhando)
- Teste: sintaxe ok; test_chest 42/0, test_piston 142/0
- Limites: (VIS) cama na mao/chao continua sprite plano (so o inventario virou 3D); causa exata das cores erradas nao confirmada sem print; portas de outras madeiras mantidas (so alcapao foi pedido)
- [ ] Falta: V11 vidros/paineis coloridos; V12 blocos fora da 1.12.2 + plantas; mapa convertido

## patchV.12 (vidros tingidos e paineis coloridos)
- [x] `pkblocks.inc`: vidro tingido do pack 1.12 (`glass_white`...) agora e reconhecido via `pk112` => passada translucida (antes era tratado como vidro opaco/errado)
- [x] `redstone.inc`: icone do painel colorido nao achava a textura (procurava `block/white_stained_glass`, o arquivo e `block/glass_white`); agora compara pelo nome normalizado
- Teste: sintaxe ok
- Limites: (VIS) sem print nao confirmei se ha outro defeito de cor/modelo alem do nome antigo; modelo 3D do painel nao mexido (ja usa glass_pane_top_<cor> normalizado)
- [ ] Falta: V12 da fila (blocos fora da 1.12.2 + plantas; preciso do final da mensagem das plantas); mapa convertido

## patchV.13 (blocos fora da 1.12.2)
- [x] `redstone.inc`: removidos do registro/criativo/nomes PT as placas de pressao (abeto..carvalho palido, 11) e as placas de sinal (abeto..carvalho palido, 11) que so existem a partir do 1.13+; ficam placa de pressao de pedra/carvalho/leve/pesada e a placa (sign) de carvalho
- Teste: sintaxe ok; nenhuma receita usava as variantes removidas (chest.inc usa so rd(XB_PLATE,0/1) e rd(XB_SIGN,0)); colocados em saves antigos continuam desenhando
- Limites: so revisei o registro de redstone.inc e o item_data.inc (1.12 puro); pack/extras ja vem do Default-Java-1.12.2; outros blocos 1.13+ no codigo (se houver) precisam de lista ou print
- [ ] Falta: plantas completas (preciso do final da mensagem: "todas as plantas tem que..."); mapa convertido; ícones: shulker, painéis, baú, alavanca

## patchV.14 (icones: shulker, paineis de vidro)
- [x] `entmodels.inc` entLoadOne: qualquer textura de entidade com `light_gray` cai para `silver` (nome do pack 1.12) => shulker cinza-clara tambem carrega (antes so a cama)
- [x] `redstone.inc` rsIconQuads: painel de vidro/barras/painel colorido deixam de usar o modelo em cruz no inventario; viram sprite plano da textura, como no 1.12.2
- Teste: sintaxe ok (icones sem teste headless)
- Limites: (VIS) nao conferido no jogo; o modelo do shulker posto/icone ja existia (entShulker), sem print nao sei o que mais esta errado; alavanca NAO tratada (preciso de print)
- [ ] Falta: alavanca (print), plantas (final da mensagem), mapa convertido

## patchMap.2 (chunks do save comprimidos na RAM)
- [x] `world.inc`: `savedC`/`savedM` guardam o RLE (pares/trincas do arquivo) em vez do chunk inteiro; helpers savedEncB/EncM/DecB/DecM; `genImpl` decodifica so ao gerar o chunk (antes ~192 KB por chunk salvo => ~2 GB no mapa de 11.521 chunks)
- [x] `menu.inc` loadSavedChunks (BL16/17/18 e 8 bits antigo) monta o RLE ja com o remap de ids; meta so e recodificado se algum bloco virou pedra; `net.inc` serve/recv convertem de/para RLE
- Teste: sintaxe ok; test_chunk_sync 6/0, test_piston 142/0, test_chest 42/0 (testes ajustados para os helpers)
- Limites: (VIS) nao medi a RAM nem abri o mapa de 140 MB; o arquivo ainda e lido inteiro no carregamento (so a RAM residente cai); leitura sob demanda do disco nao feita
- [ ] Falta: reconverter o mapa (baus/placas/mobs/posicao do jogador); alavanca (print); plantas (final da mensagem); instalador testar no Windows
- Auditoria (patchMap.2): cercas (7 tipos) e portoes (6 madeiras) ja existem em redstone.inc; muro so cobblestone/mossy (como no 1.12.2) => item "muros e cercas de todas as madeiras" ja estava feito. Restante da fila precisa de print, do mapa original (/tmp/m) ou do pack Default-Java-1.12.2 (ptbr.inc, blocos faltantes)

## patchPlant.1 (plantas altas sem modelo e cinzas)
- [x] `models.inc`: o carregamento dos modelos de planta (cross) e das mudas procurava so o nome canonico no pack; o pack 1.12 usa `double_plant_*`, `tallgrass`, `flower_*`, `sapling_*` => sem textura o modelo era pulado (sem modelo/cinza). Agora procura pelo nome 1.12 via pk112Rev (girassol, lilas, grama alta, samambaia/grande, roseira, flores, mudas)
- [x] `render_items.inc`: tinta de bioma do icone tambem para `double_plant_grass/fern` e `tallgrass`
- Teste: sintaxe ok; test_piston e test_chest sem falhas (sem o pack nao testei o carregamento)
- Limites: (VIS) nao conferido no jogo nem com o pack; PEONIA (double plant `paeonia`) NAO existe no jogo: as 24 ids XB_PLANT (940..963) estao cheias e `24` esta fixo em varios arquivos (models/blockshape/blockupdate); precisa de faixa nova de ids
- [ ] Falta: peonia (2 ids novos + xbIsPlant/xbIsUpper); conferir no jogo

## patchPlant.2 (peonia, agua azulando entidades, DRI_PRIME) — soma o patchPlant.1
- [x] Peonia (double plant do 1.12): `main.cpp` XB_PEONY=1007/1008 (ids livres; `isRs` agora exclui esses 2); `models.inc` xbPIdx (indice 24/25), XB_PLANTS[26], modelo cross baixo/cima, item (`peony_top`/`peony`) e drop; `blockshape.inc` e `blockupdate.inc` (forma e hooks). Colocar, quebrar a de cima e suporte usam as mesmas regras das outras plantas altas
- [x] Agua azulando entidades: `sky_cycle.inc` entLightAt multiplicava a cor por azul (.55/.78) sempre que a celula amostrada era agua (barco/mobs/itens ficavam azuis so por encostar); removido (a passada translucida da agua ja tinge o que fica submerso)
- [x] DRI_PRIME no Linux: `vkgl.inc` — com DRI_PRIME definido e diferente de 0 a GPU Vulkan escolhida e a PRIMEIRA da lista (o Mesa device-select poe a pedida primeiro); antes escolhia so por tipo (discreta > integrada) e ignorava a variavel; loga a GPU escolhida. O caminho OpenGL ja respeita DRI_PRIME pelo Mesa
- Teste: sintaxe ok; test_piston 142/0, test_chest 42/0, test_chunk_sync 6/0
- Limites: (VIS) nao conferido no jogo nem com o pack; peonia nao gera no terreno (so no criativo/colocada); se ao colocar a peonia houver erro, ver `isRs`; DRI_PRIME nao testado (sem GPU dupla aqui); entidades dentro d'agua ficam so com a tinta da passada da agua
- [ ] Falta: geracao de chunks/performance (preciso de dados: logs/latest.log com o profiler, FPS e onde trava — carregando, andando, ou so o 1o carregamento); peonia no terreno (floresta de flores); conferir no jogo

## patchPlant.3 (peonia no terreno)
- [x] `models.inc`: manchas de flores da floresta agora geram peonia (no lugar de uma cornflower repetida, `FOR[6]`); dbl(XB_PEONY) poe baixo+cima
- Teste: sintaxe ok; test_piston 142/0, test_chest 42/0
- Limites: (VIS) terrenos ja gerados nao mudam; so chunks novos
- [ ] Falta: geracao de chunks/performance (log do profiler); mapa convertido (mapa original); alavanca (print); instalador no Windows

## patchCraft.1 (barcos, baus e cercas por madeira; cama dropa)
- [x] `chest.inc` packRecipes: barco de cada madeira (5 tabuas, "P P"/"PPP", 6 tipos); bau com tabuas de qualquer madeira; cerca (3) e portao das 5 madeiras nao-carvalho (tabuas do pack, nome 1.13 `X_planks` ou 1.12 `planks_X`)
- [x] `blockupdate.inc` xbNeighbor: ao quebrar uma metade da cama (ou porta), a cascata da outra metade sobrescrevia `gRsLastMeta` e o drop da metade quebrada pelo jogador saia como "metade de cima" (0). Agora o meta e restaurado depois da cascata => a cama dropa
- Teste: sintaxe ok; test_piston 142/0, test_chest 42/0, test_egg 21/0. Receitas com o pack nao testadas (sem o pack)
- Limites: (VIS)/sem pack: se a madeira nao tiver tabuas no pack a receita e pulada; quebrar a cama por explosao/pistao nao testado; o bau ja tinha receita (so carvalho) — agora todas as madeiras; cor da cama dropada = a da metade dos pes
- [ ] Falta: mais receitas (portas e escadas/lajes das outras madeiras, cama por cor, barco ja feito); geracao de chunks/performance (log do profiler); mapa convertido; alavanca; instalador no Windows

## patchCraft.2 (portas por madeira, cama por cor)
- [x] `chest.inc` packRecipes: porta de abeto/betula/selva/acacia/carvalho escuro (6 tabuas -> 3, variantes XB_DOOR 2..6); cama das 15 cores nao brancas (3 la da cor + 3 tabuas, variante = ordem dos corantes); assinatura do cache inclui porta e cama
- Escadas/lajes por madeira ja existiam (patchBlk.1); alcapao so de carvalho (1.12.2)
- Teste: sintaxe ok; test_piston e test_chest sem falhas. Receitas com o pack nao testadas (sem o pack)
- Limites: (VIS) a cama so aparece se a la colorida existir no pack com nome `<cor>_wool` ou `wool_colored_<cor>` (cinza-clara = light_gray/silver); se o jogo guardar la colorida de outro jeito, a receita e pulada
- [ ] Falta: reconverter o mapa (baus/placas/mobs/posicao; precisa do mapa original); chunks sob demanda; alavanca (print); icones transparentes de bau/mesa de encantamento; instalador testar no Windows; DRI_PRIME (sistema, o usuario resolve)

## patchPlant.4 (flores e matos invisiveis) — soma patchCraft.2
- [x] `models.inc`: o Default-Java-1.12.2.zip NAO tem `assets/minecraft/models/` (0 JSONs) e o jogo tambem nao traz `block/cross.json`; `xbLoadModel("block/cross")` falhava e o laco de plantas/mudas dava `continue` => nenhuma flor, mato, samambaia, planta alta ou muda tinha modelo. Agora `CROSS_JSON` embutido (2 planos cruzados, rescale 45, como o vanilla) e usado quando o pack nao tem o arquivo (cross e tinted_cross)
- Teste: sintaxe ok; test_piston e test_chest sem falhas. Verifiquei por script que o pack tem as texturas (tallgrass, flower_*, double_plant_*, sapling_*) e que pk112Rev as acha
- Limites: (VIS) nao conferido no jogo. Outros modelos tambem vindos do pack (tocha, alavanca, girassol com haste) continuam dependendo de models/ do pack; o girassol cai no cross simples se faltar
- [ ] Falta: conferir no jogo (flores/mato/mudas aparecem e dropam); se ainda sumirem, mande logs/latest.log

## patchPlant.5 (tochas sem modelo no pack)
- [x] `models.inc` xbLoadModel: alem do cross, fallback embutido para `block/torch`, `wall_torch`, `redstone_torch`, `redstone_wall_torch` (template_torch/_wall vanilla 1.12, parede inclinada -22.5) e `ladder`; usados so quando o pack nao tem models/ (o log mostra so 29 itens colocaveis com modelo)
- Teste: sintaxe ok; test_piston 142/0, test_chest 42/0
- Limites: (VIS) nao conferido no jogo; geometria da tocha de parede escrita de memoria do vanilla: se a inclinacao ficar para o lado errado, inverter o angulo em `template_torch_wall`
- [ ] Falta: modelos embutidos de trilhos (`block/rail*`), alavanca (`lever`, `lever_on`), funil, portas, alcapao, haste do end rod, tripwire hook, girassol (modelos vem do pack, que nao tem models/); conferir no jogo

## patchPlant.6 (trilhos e alavanca sem modelo no pack)
- [x] `models.inc` xbLoadModel: fallback embutido para trilhos (`rail`, `powered_rail[_on]`, `detector_rail[_on]`, `activator_rail[_on]`, `rail_corner`, cada um reto ou `_raised_ne` = rampa que sobe para o norte) e para `lever`/`lever_on` (base de pedregulho + haste a -45/+45 graus)
- Teste: sintaxe ok; test_piston 142/0, test_chest 42/0
- Limites: (VIS) nao conferido no jogo; geometria escrita de memoria do vanilla 1.12: se a rampa subir para o lado errado inverter o angulo em `_raised_ne`; se a alavanca ligada/desligada ficar trocada, inverter -45/45
- [ ] Falta: modelos embutidos de funil (`hopper`, `hopper_side`), `end_rod`, `tripwire_hook[_on]`, haste do girassol (`sunflower_top*`); conferir no jogo

## patchFlor.1 (fornalha bugada no criativo, arbusto morto, zip de texturas)
- [x] `mods.inc` pkFaceStem: o criativo (`block/furnace_front|Furnace`) caia no PNG solto `furnace_front_off` (sufixo `_off` nao era reconhecido) e mostrava um cubo escuro com a grade nas faces; agora `_front_off/_on/_horizontal/_vertical` contam como face solta e redirecionam para a fornalha de verdade; o bloco real tambem entra em gCreativeBase (sem repetir na aba Resource pack)
- [x] Arbusto morto (deadbush do 1.12): `XB_DEADBUSH=1009` (main.cpp; fora de isRs), modelo cross sem tinta, colocavel em solido, cede lugar como mato, forma sem colisao, hook de vizinho (quebra sem chao); nome PT ja existia
- [x] `assets/InventivetalentDev_minecraft-assets_1_12_2_textures.zip` adicionado (copia do zip enviado)
- Teste: sintaxe ok; test_piston 142/0, test_chest 42/0. Comparei o zip com o Default-Java-1.12.2: mesmas 1406 texturas (nada de flor faltando em textura)
- Limites: (VIS) fornalha e arbusto nao conferidos no jogo; arbusto morto nao gera no deserto (xbVeg so roda em topo de grama); nao dropa gravetos; vaso de flor (flower_pot) NAO feito (exige bloco com interacao)
- [ ] Falta: arbusto morto no deserto/mesa; vaso de flor com planta; cogumelos como planta colocavel (conferir); conferir no jogo

## patchHud.1 (coracao, coxa, bolha e XP do icons.png oficial)
- [x] `asset_manager.inc`: `rtIcons` = gui/icons.png do pack; `survival.inc` survDrawHud usa coracao (y=0, hardcore y=45), coxa (y=27) e bolha (y=18) do icons.png; sem pack cai nos desenhos antigos
- [x] XP novo: `gXpTot`, `xpLevel/xpToNext` (formula do Java), barra 182x5 (icons.png y=64/69) + numero verde do nivel; XP por mob morto (mobs.inc) e minerio (carvao, lapis, redstone, diamante em main_loop.inc); zera ao renascer
- Teste: sintaxe ok
- Limites: (VIS) posicoes dos icones (-17/-27) e frames de piscar do coracao nao conferidos; sem orbes de XP, sem frasco/encantamento, sem XP de fornalha/esmeralda/quartzo; XP nao salvo no save nem na rede
- [ ] Falta: patchCrop.1 (plantar sementes, crescer, colher; melancia, abobora, verruga, cana, cacau; girassol); salvar XP

## patchCrop.0 (portoes no criativo + flor do girassol)
- [x] `redstone.inc` case 14: sem models/ no pack o portao nao gerava malha e o criativo mostrava cubo de tabua; agora fallback embutido (2 postes, 2 travessas, miolo; aberto = folhas nos postes)
- [x] `models.inc`: girassol sem modelo do pack ficava so com a haste (sunflower_top nao tem amarelo); fallback com 2 quads (front amarela / back) que giram com o sol
- Teste: sintaxe ok; test_piston/chest sem falhas
- Limites: (VIS) tamanho/posicao da cabeca (plano x=.72, 0,8x0,8) e geometria do portao escritas de memoria do vanilla; icone do girassol no inventario usa sunflower_front (ja era)
- [ ] Falta: patchCrop.1 (plantar/crescer/colher + melancia, abobora, verruga, cana, cacau)

## patchCrop.1 (plantar, crescer e colher: trigo, cenoura, batata, beterraba)
- [x] `crops.inc` novo: enxada (clique direito em grama/terra com ar em cima) vira terra arada; semente de trigo/cenoura/batata/semente de beterraba planta em terra arada (XB_CROP, idade 0); `cropRandomTick` (chamado em runWorldTick, so host/solo) amostra 96 blocos em +-32x+-16 em volta do jogador, ~1/7 de chance de +1 estagio ate 7; `cropDrops` (drops.inc): verde devolve semente/raiz, maduro = produto (+ 0..3 sementes ou 1+binomial(3,.57)), batata venenosa 2%
- Teste: sintaxe ok; test_piston/chest sem falhas. Sem teste headless dedicado
- Limites: (VIS) tudo nao conferido no jogo; sem hidratacao (agua), sem luz minima, sem pisoteio, sem farinha de osso, enxada sem durabilidade, sem sementes de grama alta, crescimento so perto do jogador (convidado remoto nao faz crescer); terra arada nao volta a terra
- [ ] Falta: patchCrop.2: melancia/abobora (haste + fruto), verruga do Nether, cana-de-acucar, cacau; farinha de osso; sementes da grama alta; hidratacao

## patchRed.1 (alcapoes 1.12.2, dispenser/dropper, haste do pistao, funil)
- [x] `assets/creative/mc-1.12.2.cfg`: removidos alcapoes de abeto/betula/selva/acacia/carvalho escuro (so existem na 1.13; 1.12.2 tem carvalho e ferro)
- [x] `pk112.inc`: apelidos dispenser_front_horizontal, dropper_front_horizontal, piston_top_normal -> nomes usados pelo jogo (o criativo mostrava "Dispenser Front Horizontal" cru e cubo claro)
- [x] `redstone.inc`: haste do pistao estendido agora de madeira (rsRod, UV do centro de piston_top; antes pegava o corpo de pedra do piston_side); funil embutido (case 10) com hopper_outside/inside/top, bico para baixo ou de lado
- Teste: sintaxe ok; test_piston/chest sem falhas
- Limites: (VIS) nada conferido no jogo. NAO mexi na textura de tras do pistao: piston_bottom do pack 1.12 e mesmo pedra cinza com moldura escura; se ainda estiver errado mande print do pistao pegajoso fechado e estendido. Cubo de madeira com rebites (print 5) = frente do pistao; se aparecer em todas as faces, mande print
- [ ] Falta: conferir no jogo; patchCrop.2

## patchRed.2 (placas so de carvalho/pedra, pistao normal x grudento, faces do dispensador/ejetor)
- [x] `assets/creative/mc-1.12.2.cfg`: removidas placas de pressao de abeto/betula/selva/acacia/carvalho escuro/mangue/cerejeira/bambu/carmesim/distorcido/carvalho palido (1.12.2: pedra, carvalho, leve, pesada)
- [x] cfg + `redstone.inc` + `ptbr.inc` + `tools/gen_creative_1122.py`: pistao normal (piston_top_normal, nome "Piston") e grudento (piston_top_sticky) agora sao entradas separadas e independentes de alias
- [x] `pkblocks.inc`: FaceDef com os nomes do pack 1.12 (dispenser_front_horizontal / dropper_front_horizontal) -> icone com frente + lados/topo da fornalha, em vez da frente nas 6 faces
- Teste: sintaxe ok; test_piston/chest/egg sem falhas
- Limites: (VIS) nada conferido no jogo; causa do "dois grudentos" deduzida do codigo (sem print); dispensador/ejetor vertical usa a mesma frente
- [ ] Falta: conferir no jogo; patchCrop.2

## patchDur.1 (tochas, god rays estourados, durabilidade)
- [x] `models.inc`: `XModel m` era reaproveitado sem zerar: a tocha de redstone herdava os elementos da tocha de parede (modelo misturado). Agora `m = XModel()` antes de cada carga
- [x] `godrays.inc`: `pUniform1f(uGdAsp, asp)` estava DENTRO de um comentario (uAsp = 0): o raio ficava so por distancia vertical e estourava a tela em branco; agora numa linha propria
- [x] Durabilidade 1.12.2: `itemMaxDur` (inventory_ui.inc: madeira 59, pedra 131, ferro 250, ouro 32, diamante 1561; arco 384; armaduras couro/ferro/ouro/diamante). `itemWear/toolWear/meleeWear/armorWear` (survival.inc). Gasta: quebrar bloco (ferramenta 1, espada 2), golpe em mob/jogador (espada 1, resto 2), arco 1 por tiro, enxada 1 por arar, armadura max(1, dano/4) por golpe. Quebra = item some + som random/break. Criativo nao gasta
- [x] Barra de durabilidade (verde->vermelho) na hotbar, inventario, cursor e mao secundaria; dano salvo no player.dat (linha `d N`) e preservado ao mover pilhas / addItem
- Teste: tests/test_durab.cpp (19 verificacoes); test_piston/chest/egg sem falhas
- Limites: (VIS) nada conferido no jogo; shader branco: causa achada no codigo (uAsp), sem print; splitscreen: jogador 2 nao gasta ao quebrar bloco/bater (so arco); item solto no chao perde o dano (DropEnt sem d); isqueiro/tesoura/vara/escudo nao migrados para itemMaxDur; sem encantamento Inquebravel/Reparo na bigorna; armadura de convidado remoto nao gasta
- [ ] Falta: conferir no jogo; durabilidade no drop e no splitscreen; patchCrop.2

## patchFur.1 (fornalha funcional)
- [x] `furnace.inc` novo: conteudo por posicao (entrada/combustivel/resultado + tempos), fundir 200 ticks como o TileEntityFurnace 1.12.2, combustivel por tempo (carvao 1600, madeira 300, bloco de carvao 16000, balde de lava 20000 devolve balde vazio...), receitas (minerios, areia->vidro, pedra, carnes, peixes, batata, argila, tijolo, cacto, esponja, terracota->glazed), progresso recua 2/tick sem fogo, resultado so retira
- [x] Estado aceso = meta bit 2 do bloco: frente `furnace_front_on` (pkblocks.inc acha o tile) + luz 13 (world.inc, mesma tecnica da tocha de redstone apagada); frente (bits 0-1) preservada
- [x] Tela reaproveita a do bau (`gFurn`, gChRows=1): slots entrada/combustivel/resultado, chama e seta (retangulos), shift-click inteligente, numero/arrastar no resultado bloqueados; clique direito abre (agachado com item coloca bloco)
- [x] Quebrar solta o conteudo; salvar/carregar em saves/<mundo>/furnaces.dat (menu.inc, main_loop.inc)
- Teste: tests/test_furnace.cpp (52 verificacoes); test_piston/chest/egg sem falhas; sintaxe ok
- Limites: (VIS) tela, chama, seta, frente acesa e luz NAO conferidas no jogo; sem XP; sem funil/dispensador; convidado de MP nao abre (clique consumido); jogador 2 do splitscreen nao abre; fornalha so funde com chunk carregado; receitas do pack dependem dos nomes dos arquivos (fidv tenta aliases 1.12/1.13)
- [ ] Falta: conferir no jogo; XP; funil; fornalha no MP para convidado (NP_CHEST); splitscreen; alto-forno/defumador nao existem na 1.12.2

## patchFur.2 (fornalha para convidado no MP + diagnostico de carregamento)
- [x] `furnace.inc` + `chest.inc`: convidado abre a fornalha reaproveitando NP_CHEST/NP_CHESTREQ (mesmo formato do bau; slot 3 = chama, slot 4 = seta; NET_VER NAO mudou, cliente antigo ignora). Host manda conteudo+progresso, empurra o progresso a cada 10 ticks por 30 s (convidado repete o pedido a cada 10 s); convidado devolve so os itens que mudou; nao guarda o conteudo ao fechar; timeout de 2,5 s sem resposta fecha a tela
- [x] `menu.inc`: logs `[LOAD]` (formato, chunks, paleta) e aviso quando o chunks16.dat tem formato desconhecido (antes o mundo era regerado em silencio)
- [x] `tests/test_oldworld.cpp`: carrega um save inteiro (jogador, mobs, barcos, carrinhos, bau, fornalha), gera, malha 49 chunks, spawn e 400 ticks (rodar com ASAN)
- Teste: test_furnace 58 verificacoes (inclui MP: host->convidado, convidado->host, fechar); test_piston/chest/egg sem falhas; test_oldworld com ASAN sem erro nos 2 saves do zip + versoes BL17 e BL16 sinteticas
- TRAVA AO INICIAR MUNDO ANTIGO: NAO reproduzida. Os 2 saves do zip (BL18) e BL17/BL16 sinteticos carregam, geram, malham e rodam 400 ticks sem erro; o zip nao trouxe crash-report nem log da trava. Os logs [LOAD] novos mostram onde para
- Limites: (VIS) tela do convidado nao conferida; edicao simultanea host x convidado: vale a ultima gravacao (como o bau); splitscreen J2 nao abre; sem XP/funil
- [ ] Falta: log (logs/latest.log) e crash-reports/ da trava + qual mundo (versao do jogo que o criou); conferir no jogo; XP; funil; splitscreen

## patchFur.3 (XP da fornalha) + patchShader.1 (luz hemisferica e dither)
- [x] `furnace.inc`: XP por receita (tabela 1.12.2: ferro/redstone .7, ouro/diamante/esmeralda 1, carnes .35, argila .35, carvao/pedra/vidro .1...) acumulado em `FurnData.xp` a cada item fundido; ao retirar o resultado (clique, shift-click) entrega a parte proporcional (inteiro + chance da fracao, como o Java); criativo nao ganha; salvo em furnaces.dat (campo extra, saves antigos leem 0)
- [x] `sky_cycle.inc` (shader do mundo): luz ambiente hemisferica so de COR (topo frio do ceu, lados/base com rebote quente do chao, proporcional a luz do ceu e a hora do dia; so no modo "fancy", fora do Nether/agua) + dither de 1 nivel (ruido por pixel animado) que quebra as faixas do degrade da neblina, ceu e sombras suaves
- Teste: sintaxe ok; test_furnace 61/0 (3 novos de XP); test_piston 142/0, test_chest 42/0, test_egg 21/0
- Limites: (VIS) shader NAO compilado/visto no jogo (so a sintaxe C++; GLSL nao validado aqui) - se a tela ficar preta/sem mundo, ver logs/latest.log (erro de shader) e remover o bloco "patchShader.1"; convidado de MP recebe XP pela tabela (host nao desconta); quebrar a fornalha nao solta XP
- [ ] Falta: conferir no jogo (shader, XP); funil na fornalha; fornalha no splitscreen J2; patchCrop.2

## patchHop.2 (funil: aro flutuante)
- [x] `redstone.inc` case 10: funil embutido refeito so com texturas OPACAS (hopper_outside/hopper_inside); topo do aro nao usa mais `hopper_top` (tem buraco transparente 10x10); placa de base 16x16 + 4 paredes de 6px + piso inside + funil + bico (como o hopper.json 1.12.2)
- Diagnostico: renderizei o modelo e a malha real do chunk fora do jogo: 36 quads, todos dentro de y 0..1, sem nada acima do bloco. O aro fino flutuante da foto (cerca de 0,25 bloco acima do funil) NAO sai da malha do chunk nem do contorno de selecao (wireBox so desenha a caixa mirada); causa nao confirmada
- Teste: tests/test_hopper.cpp (21 verificacoes: sem quad com hopper_top, nada fora do bloco, y max = 1, meta 0/2..5, malha do chunk); test_piston 142/0, test_chest 42/0
- Limites: (VIS) nao conferido no jogo; se o aro flutuante continuar, mandar print do funil recem-colocado SEM item caido por perto (suspeito: item/entidade desenhada acima do bloco); o funil ainda NAO tem funcao (sem inventario, sem puxar/empurrar itens)
- [ ] Falta: conferir no jogo; funcao do funil (inventario 5 slots, puxar do bau/fornalha de cima, empurrar para baixo/lado)

## patchShader.2 (tela branca, buracos longe, shader a longa distancia)
- [x] `sky_cycle.inc`: normal por derivada (dFdx x dFdy) protegida contra vetor nulo (normalize(0)=NaN virava pixel/tela branca); guarda final NaN/inf -> cor da neblina + clamp 0..1; divisor da neblina protegido
- [x] Longa distancia: luz hemisferica agora some suave ate 400 (antes corte seco em 150 = linha visivel); perspectiva atmosferica proporcional ao fim da neblina (antes fixa p/ RAD 7)
- [x] `world.inc` (modo longe): face so e cortada se estiver coberta na coluna E nas 4 vizinhas (antes um beiral/penhasco cortava faces visiveis = buracos no terreno distante)
- Teste: sintaxe ok (GLSL nao validado aqui)
- Limites: (VIS) nao conferido no jogo; se ainda houver branco, mandar print + logs/latest.log (erro de shader). Cogitado e NAO feito: z-fighting longe (plano near 0.05); precisao de vWorld em coordenadas enormes
- [x] (extra) near 0.09 com RAD>8 (menos z-fighting longe); neblina um pouco mais densa em vales (y<72)
- [ ] Falta: conferir no jogo; instalador Windows; precisao de vWorld em coordenadas enormes; demais da fila

## patchShader.4 (nuvem enchendo a tela)
- [x] `mining_sky.inc`: video do usuario mostrou a tela toda de uma cor lisa (pessego do por do sol) com a camera na altura das nuvens (y 128..132). Opacidade das nuvens cai para ~12% quando a camera esta dentro/colada na camada e volta a 80% a 3+ blocos (lista refeita ao mudar o degrau)
- Teste: sintaxe ok
- Limites: (VIS) nao conferido no jogo; se o pessego vier de outra causa (neblina/ceu), mandar a posicao Y (F3) do momento
- [ ] Falta: conferir no jogo

## patchShader.5 (tela lisa no chao = pilha de GL estourando)
- Diagnostico (logs/latest.log do usuario): `GL ERROR @ Post render 1283: Stack overflow` todo quadro desde que o reflexo da agua ligou, e `0 chunks desenhados, 0 draw calls` = mundo some, so a cor do ceu. NAO era nuvem (patchShader.4 so ajuda dentro da camada)
- [x] `logger.inc`: `glStackHeal(onde)` le profundidade das pilhas modelview/projecao/attrib/client, loga 1x `[GL] pilha vazada em '<ponto>'` e DESFAZ o vazamento (pop ate a base)
- [x] `main_loop.inc`: chamado no inicio do quadro, apos o reflexo da agua e no fim do quadro (o log mostra onde vaza)
- Teste: sintaxe ok. Causa-raiz (push sem pop) NAO localizada por leitura: os pares push/pop do reflexo/sombras/godrays/ceu estao equilibrados no texto
- Limites: (VIS) nao conferido; se o log trouxer `[GL] pilha vazada em ...`, esse ponto indica o culpado
- [ ] Falta: mandar logs/latest.log depois do patch (linhas "[GL] pilha vazada")

## patchLight.1 (lua, nascer do sol, luz e sombra)
- [x] `shadows.inc`: de noite a LUA vira a fonte das sombras (mapa de sombra refeito na direcao oposta ao sol), forca pela fase (1/.75/.5/.25/0; lua nova fraca); `gSunDir` continua sendo o sol
- [x] `sky_cycle.inc` (shader do mundo e das entidades): luz direta da lua com tom prateado-azulado e realce ~3,7x maior que o do dia; sombras noturnas mais suaves/azuis; brilho (glitter) da lua na agua; halo da lua proporcional a fase
- [x] Nascer/por do sol: 2 camadas novas (faixa larga rosa-violeta + brasa dourada rente ao horizonte), alem da faixa original
- Teste: sintaxe ok (GLSL nao validado aqui)
- Limites: (VIS) tudo; sombras a noite custam o mesmo que de dia (se cair FPS, F7 desliga); se a tela ficar preta/sem mundo, ver logs/latest.log (erro de shader) e remover as marcas "patchLight.1"
- [ ] Falta: conferir no jogo; ajustar intensidade (2.6 e .10+.30*fase) conforme o gosto; estrelas cintilantes; raios de lua (god rays noturnos)

## patchCrop.2 (farinha de osso)
- [x] `crops.inc` cropUse: farinha de osso (item/bone_meal ou dye_powder_white) em trigo/cenoura/batata/beterraba: +2..5 estagios (beterraba +1 com 2/3 de chance), maximo 7; maduro nao gasta; sincroniza no MP (netSendBlock)
- Teste: tests/test_crop.cpp (364 verificacoes, 0 falhas); test_furnace 61/0, test_piston 142/0; sintaxe ok
- Limites: (VIS) nao conferido no jogo; sem particulas; o item so age em colheita (grama alta/mudas nao)
- [ ] Falta: melancia/abobora (haste + fruto), verruga do Nether, cana, cacau; sementes da grama alta; hidratacao, luz minima, pisoteio (nao ha estado "umido" na terra arada: XB_FARM bit0 = caminho)

## patchShader.6 (causa-raiz do vazamento de pilha + shader preso)
- Diagnostico (logs/latest.log apos patchShader.5): `[GL] pilha vazada em 'reflexo da agua': modelview 2` todo quadro. Causa: em main_loop.inc, no reflexo da agua, `skyShaderOff(); entDimReset(); glPopMatrix();` ficou ENGOLIDO por um comentario `//` na mesma linha da chama do jogador => sem pop (pilha estourava, mundo sumia) e o shader de entidade ficava LIGADO para o resto do quadro (nuvens/ceu cinza-brancos sem cor, como no print)
- [x] linha separada; glStackHeal (patchShader.5) continua como rede de seguranca
- Teste: sintaxe ok
- Limites: (VIS) nao conferido no jogo; hotbar com slots oliva no print nao investigado
- [ ] Falta: conferir se as nuvens voltaram ao normal e se o log nao traz mais "[GL] pilha vazada"

## patchFur.4 (tela da fornalha com os assets do pack)
- [x] `asset_manager.inc`: carrega `gui/container/furnace.png` do pack (rtFurn, 256x256)
- [x] `inventory_ui.inc` furnDrawPack: fundo 176x166 do pack (moldura dos slots, caixa do resultado, seta vazia); CHAMA = sprite (176,0 14x14) que encolhe de cima para baixo (formula do GuiFurnace: k = fracao*13); SETA = sprite (176,14 24x17) que enche da esquerda (largura = progresso*24+1). Slots da mochila/hotbar em y=84/142 e altura 166 (como o furnace.png). Sem o pack: continua o desenho antigo em retangulos
- Ja funcional (patchFur.1..3): fundir 200 ticks, combustivel por tempo (balde de lava volta vazio), shift-click, XP, luz/frente acesa, quebrar solta o conteudo, salvar, convidado MP
- Teste: sintaxe ok; test_furnace 61/0; test_chest sem falhas (UI sem teste headless)
- Limites: (VIS) coordenadas do furnace.png conferidas so pela documentacao do GuiFurnace; se chama/seta ficarem desalinhadas, mandar print
- [ ] Falta: conferir no jogo; funil na fornalha (puxar/empurrar); jogador 2 do splitscreen abrir a fornalha; alto-forno/defumador nao existem na 1.12.2

## patchLight.2 (sol atras das nuvens + feixes de luz volumetricos)
- [x] `sky_cycle.inc` drawSkyFull: disco e halo do sol escurecem conforme a fracao do sol tapada pela camada de nuvens (cloudSunOcc, suavizado, ate -93%); antes o sol/halo aditivos apareciam por cima das nuvens como um circulo brilhante (print do usuario). `main_loop.inc`: guarda a camera do quadro para isso
- [x] Shader do mundo: feixes de luz volumetricos - 8 passos no raio camera->pixel lendo a cascata 1 do mapa de sombra, so luz que realmente chega ao ar (buracos, copas, cavernas); tom dourado de dia e azul prateado a noite (lua); mais visivel em lugar fechado e olhando para a luz (espalhamento para frente); ate 26 blocos; so com sombras ligadas (F7)
- Teste: sintaxe ok (GLSL nao validado aqui)
- Limites: (VIS) tudo. A linha escura fina em volta do retangulo claro do 2o print parece o contorno de selecao do bloco (nao toquei); se for outra coisa, mandar print sem o cursor sobre o bloco. Custo: +8 leituras de sombra por pixel proximo (se cair FPS, F7)
- [ ] Falta: conferir no jogo; ajustar 0.22 (intensidade dos feixes); feixes no shader das entidades

## patchLight.3 (luz de tocha/lava no shader + Nether)
- [x] `sky_cycle.inc`: vertex shader passa `vWarm` (cor da fonte + peso da luz de bloco sobre a do ceu); fragment (completo e enxuto) aplica tremulo de chama por celula e brilho quente na cor da fonte (tocha dourada, lava laranja, redstone vermelha). So age onde a luz de bloco domina (cavernas, noite, Nether); entidades sem efeito
- [x] netherGrade (2 copias): brilho de lava/glowstone comeca em luminancia menor e pulsa mais forte; sombras ganham brasa vermelha fraca
- Teste: sintaxe ok (GLSL nao validado aqui)
- Limites: (VIS) tudo; se a tela ficar preta, ver logs/latest.log e remover as marcas "patchLight.3"; intensidades 0.30/0.07 ajustaveis
- [ ] Falta: conferir no jogo; luz de lava iluminando entidades; particulas de brasa no Nether

## patchBucket.1 (balde completo)
- [x] `main_loop.inc` bucketUse: balde vazio na vaca = balde de leite (so se a vaca estiver mais perto que o bloco); agua no Nether evapora (balde esvazia, sem fonte); balde de leite so segura
- [x] `main.cpp`/`pkblocks.inc`: gBktMilk (item/milk_bucket), pilha 1
- Ja existia: pegar fonte de agua/lava, colocar fonte, lava+agua -> obsidiana/pedregulho, criativo nao gasta, lava como combustivel, crafting (3 ferros em V, crafting.inc)
- Teste: sintaxe ok; test_furnace sem falhas (balde sem teste headless proprio)
- Limites: (VIS) tudo; beber leite, caldeirao, bezerro (nao filtrado) e MP do leite nao feitos
- [ ] Falta: beber leite, caldeirao, filtrar bezerro, sincronizar leite no multiplayer

## patchLight.4 (efeitos realistas no shader)
- [x] `sky_cycle.inc` (completo e enxuto): hora dourada (sol baixo esquenta o ar livre); translucidez de folhas/grama (luz amarelada); sombras frias de dia (tom azulado nas partes escuras ao ar livre); tonemap filmico ACES leve (28%). Tudo so no overworld e onde ha luz do ceu
- Teste: sintaxe ok (GLSL nao validado)
- Limites: (VIS) tudo; remover marcas "patchLight.4" se o shader falhar
- [ ] Falta: bloom, vinheta, reflexo especular em agua/gelo, ondas de calor no Nether

## patchLight.5 (mais efeitos no shader)
- [x] `sky_cycle.inc`: brasas piscando no netherrack; faces viradas para o sol mais claras (relevo, so shader completo); neblina quente perto de tocha/lava com a distancia
- Teste: sintaxe ok (GLSL nao validado)
- Limites: (VIS) tudo; remover marcas "patchLight.5" se o shader falhar
- [ ] Falta: bloom, vinheta (precisa uniform da tela), ondas de calor, reflexo em gelo

## patchBucket.2 (beber leite)
- [x] `main_loop.inc`: clique direito com balde de leite = bebe (zera veneno gPoisonT, devolve balde vazio; criativo nao gasta). Leite nao altera o mundo, entao nao precisa de sincronia no multiplayer
- Teste: sintaxe ok
- Limites: (VIS) tudo; nao ha outros efeitos de pocao no jogo para limpar
- [ ] Falta: caldeirao (encher/esvaziar com balde; hoje so decorativo), filtrar bezerro (nao existe estado de filhote em MobEnt); instalador: testar no Windows com Inno Setup/NSIS

## patchBucket.3 (caldeirao)
- [x] `main_loop.inc` bucketUse: balde de agua no caldeirao enche (nivel 3) e devolve balde vazio; balde vazio no caldeirao cheio devolve balde de agua; meta bits 2-3 = nivel (XB_DECOR var 0); criativo nao gasta; sincroniza no MP (netSendBlock)
- [x] `redstone.inc` case 32: desenha a agua (block/water_still) com 9/12/15 px conforme o nivel
- Teste: sintaxe ok; test_piston e test_chest sem falhas (sem teste headless do caldeirao)
- Limites: (VIS) tudo; chuva nao enche; so niveis 0 e 3 via balde (1..2 so desenham); lava no caldeirao e tintura de armadura nao existem; bezerro nao filtrado (sem estado de filhote em MobEnt)
- [ ] Falta: chuva enchendo, garrafa/pocao no caldeirao; shader: bloom, vinheta, ondas de calor, reflexo em gelo; brasas do Nether (particulas); instalador testar no Windows

## patchGfx.1 (liga/desliga shader do Nether + culling de folhas)
- [x] `screens.inc` (Detalhes): toggles "Shader do Nether" (d17) e "Culling de folhas" (d18), padrao ON; salvos em options.cfg (netherfx=, leafcull=); gfxTogglesSync() aplica na hora e ao iniciar
- [x] `sky_cycle.inc`: com d17 OFF o Nether usa uNether=2 (visual neutro: sem grade vermelha, brasas, neblina de calor, sem raios solares)
- [x] `world.inc`: folha com 4+ vizinhas folha nao gera faces voltadas para outra folha (so as expostas ao ar); mudar d18 remonta os chunks carregados (os de outra dimensao so ao recarregar)
- Teste: sintaxe ok; test_chest sem falhas (UI/malha sem teste headless)
- Limites: (VIS) tudo; botao "Concluido" desceu para h6+200 (se sair da tela em janela pequena, mandar print); com culling ON folhas densas podem parecer mais "vazadas" olhando por dentro da copa; limiar 4 em world.inc (leafN >= 4)
- [ ] Falta: conferir no jogo; culling por profundidade de 2 blocos; bloom/vinheta/ondas de calor/reflexo em gelo; brasas (particulas)

## patchRfl.1 (sombra de tocha nos pes + reflexo lateral + fade-in do reflexo)
- [x] `player_render.inc` torchFootShadow (chamada em main_loop antes dos drops): ate 2 fontes de luz de bloco (nivel >= 12: tocha, lava, fogo, glowstone) a < 7 blocos projetam mancha escura macia no chao, afastada da luz (mais comprida quanto mais perto); some ao ar livre de dia (escala por gSkyScale); so com chao a ate 2 blocos
- [x] `sky_cycle.inc` (agua, completo e enxuto): face LATERAL da agua (parede/queda) usa normal horizontal virada para a camera e reflete ceu/neblina em vez do reflexo planar (que so vale para a superficie); fresnel vale para as laterais
- [x] `world.inc`/`main_loop.inc`: Chunk::rborn; na passada do reflexo o chunk faz fade-in proprio (1.6 s, a partir do 1o quadro em que aparece no reflexo), misturando com a cor do ceu
- Teste: sintaxe ok; test_chest 42/0, test_egg sem falhas (GL/shader sem teste headless)
- Limites: (VIS) tudo; a sombra e so do jogador local (mobs/J2 nao), sem recorte por blocos (pode atravessar parede fina); reflexo lateral nao mostra blocos/entidades (so ceu/neblina); se a sombra aparecer flutuando, mandar print
- [ ] Falta: conferir no jogo; sombra de tocha em mobs e J2; reflexo lateral com cena real; reflexo de flores

## patchEmber.1 (brasas do Nether + sombra de tocha nos mobs)
- [x] `player_render.inc` netherEmbers: 48 faiscas aditivas subindo ao redor do jogador no Nether (so com "Shader do Nether" ligado); chamada em main_loop antes dos drops
- [x] torchFootShadow ganhou escala e e chamada para os 6 mobs mais proximos (ate 20 blocos)
- Teste: sintaxe ok (GL sem teste headless)
- Limites: (VIS) tudo; sombra do jogador 2 (splitscreen) nao feita; brasas sem colisao/luz
- [ ] Falta: sombra de tocha no J2; bloom, vinheta, ondas de calor, reflexo em gelo; reflexo lateral com cena real; reflexo de flores; culling de folhas por profundidade; chuva no caldeirao; poção

## patchShadow.1 (correcao: sombra de tocha so com sombras ligadas e na forma da projecao)
- Causa (print do usuario): a mancha oval aparecia mesmo com "Sombras e efeitos realistas: desligados" e nao lembrava a sombra do sol
- [x] `player_render.inc` torchFootShadow: so desenha com gShadowsOn (F7); forma = projecao real do corpo para a luz (ponta a h*H/(Hl-H) do pe, afastada da luz, alargando pela perspectiva; centro opaco, bordas em penumbra; some pelo comprimento); mobs usam a altura propria (MD[type].h)
- [x] `world.inc`: gShadowsOn declarado aqui (removido de shadows.inc) para uso antes do include
- Teste: sintaxe ok; test_chest 42/0, test_egg 21/0
- Limites: (VIS) tudo; sem recorte por blocos (a sombra nao sobe degrau/parede); a sombra do sol ja existente continua separada
- [ ] Falta: conferir no jogo; sombra recortada por degraus; sombra no J2

## patchWx.1 (clima + folhas menos "vazadas")
- [x] `weather.inc` (novo): clima limpo/chuva/tempestade com ciclo (chuva 10-20 min, limpo 10-150 min); chuva em bioma quente, neve em bioma frio (`climateAt` t<0.25); so overworld; chuva/neve desenhadas so onde ha ceu aberto (altura por coluna, cache 0,5 s); raio visual (clarao + traco) na tempestade
- [x] Ceu e neblina cinza/escuros com a chuva, luz do ceu menor; `/weather clear|rain|thunder [segundos]` (apelidos clima/chuva); host sincroniza nos bits 2-3 do NP_TIME (sem mudar NET_VER)
- [x] Chuva enche o caldeirao (ceu aberto, bioma nao frio, ~1 nivel/min)
- [x] `world.inc`: culling de folhas so remove folha 100% cercada de folhas (antes 4+ vizinhas) = copa volta a ter profundidade
- Teste: sintaxe ok; test_weather 9/0 (novo); test_chest 42/0; test_egg 21/0
- Limites: (VIS) tudo de GL/ceu; sem som de chuva/trovao; raio nao causa dano/fogo; chuva nao apaga fogo nem hidrata plantacao; clima nao e salvo no save; operador convidado nao muda o clima (so o host); chuva atravessa folhas (conta como bloco: nao passa)
- [ ] Falta: variacao de cor em biomas (Wx.2); gerador de terreno (Terrain.1); neve acumulando; sons; salvar clima

## patchCrash.1 + patchWx.2 + patchTerrain.1 (crash em mundos antigos, cor de bioma, terreno)
- Causa do crash (log 00.56.02 + crash-report; pilha rsShape <- xbNeighbor <- build): meta de mundo antigo em blocos do redstone.inc lia fora das tabelas: tocha/alavanca (`rsSupported` case 7: RS_DX[f-2] com f 6/7) e portinhola/porta (`RS_WOOD[ty]` com ty 7, tabela de 7). Reproduzido com ASAN
- [x] `redstone.inc`: `(f-2)&3` e RS_WOOD[8] (tipo 7 = carvalho)
- [x] `menu.inc` loadSavedChunks: id fora do registro atual (>= PK0+PK.size()) vira pedra e o meta do trecho e descartado (antes so valia com paleta diferente)
- [x] `tests/test_xbfuzz.cpp` (10752 combinacoes id x meta em rsShape/rsQuads/xbNeighbor, com ASAN) e `tests/test_meshfuzz.cpp` (build() com ids x metas; sem pack so ids ate 1056). Rodar com `-fsanitize=address` e ASAN_OPTIONS=detect_leaks=0
- [x] `terrain112.inc` biomeTints/biomeWaterTint: cor da grama/folha varia DENTRO do bioma (clima continuo `climateAt` + manchas finas); agua um pouco mais turva/verde em clima umido-quente
- [x] `world.inc` genImpl: pedregulho musgoso na taiga, arbusto morto no deserto, vitoria-regia em manchas no pantano (so se o modelo/pack existe: `xbHasModel`)
- Teste: sintaxe ok; test_xbfuzz e test_meshfuzz sem estouro (ASAN); test_weather 9/0, test_chest 42/0, test_piston 142/0, test_crop 364/0
- Limites: (VIS) cores, boulders, lilypads, arbusto (sem pack os blocos extras nao existem no teste); o 2o crash (log 01.36.11, 5 min depois de carregar "Novo Mundo (4)") nao veio com crash-report: se repetir, mandar crash-reports/*.txt; terreno base (terrain112: alturas/rios/biomas) NAO foi alterado; mapa de bioma continua 11 tipos
- [ ] Falta: reconferir crash do Novo Mundo (4); sugar cane/rios/lagos/montanhas mais altas no gerador; variacao de cor em outros blocos (nao-grama); clima: som, salvar, neve acumulando

## patchTerrain.2 (florestas menos densas e sem arvores coladas)
- [x] `world.inc` genImpl: densidade da floresta modulada por ruido grande (clareiras e trechos fechados, 45%..145% de 10 arvores/chunk) em vez de uniforme
- [x] canGrow como o MC: nao planta arvore se 1 bloco em volta (alturas 1..3 acima do chao, dentro do chunk) tem algo que nao seja ar/folha/planta (tronco dentro de tronco, pedra, agua) = fim de troncos colados/duplicados
- [x] Vilas: ja existiam variantes por bioma (deserto/savana/taiga/planicie em `village_build.inc`, mesma regra de bioma do MC 1.12); nada alterado
- Teste: sintaxe ok; test_meshfuzz (ASAN) ok; test_chest 42/0
- Limites: (VIS) densidade e aparencia; a checagem so olha dentro do chunk (arvores de chunks vizinhos ainda podem encostar na borda); jungle continua densa (50/chunk, como o MC) mas agora sem troncos colados; estradas da vila ainda usam COBBLE no deserto (cobR); terreno base nao alterado
- [ ] Falta: conferir no jogo; rios/lagos/montanhas; cana-de-acucar; checagem de arvores na borda do chunk

## patchWx.3 (cor de bioma visivel no mundo)
- Sintoma (usuario): particulas da grama com a cor certa, mas o mundo sem diferenca entre biomas. Malha e particulas usam o mesmo `biomeTints`; a causa mais provavel e o shader (luz/tonemap) achatar o desvio de cor
- [x] `terrain112.inc` biomeTints: desvio em relacao a planicie ampliado 1.8x; variacao por clima .38->.6 e manchas .22->.3 (vale para malha e particulas, continuam iguais)
- Teste: sintaxe ok (visual sem teste headless)
- Limites: (VIS) causa nao confirmada sem o jogo; se continuar igual, mandar print de 2 biomas e `logs/latest.log`; se ficar exagerado, baixar 1.8f
- [ ] Falta: conferir no jogo

## patchWx.4 (cor de bioma: shader nao uniformiza mais o verde)
- [x] `sky_cycle.inc` (2 fragment shaders): o realce de verde somava (0.10,0.12,0) fixo a todo pixel verde, aproximando biomas diferentes; agora e um aumento de saturacao (mantem o matiz de cada bioma)
- Teste: sintaxe ok (GLSL nao validado aqui)
- Limites: (VIS) causa nao confirmada; se continuar igual, mandar print de 2 biomas e `logs/latest.log`; se ficar saturado, baixar 0.22 em `sky_cycle.inc`
- [ ] Falta: conferir no jogo; cor de bioma em outros blocos

## patchTerrain.3 + patchWx.5 (arvores na borda do chunk + clima salvo)
- [x] `world.inc` genImpl: lista de posicoes candidatas das 16 areas vizinhas (mesma sequencia aleatoria); arvore descartada se houver candidata "anterior" (ordem fixa) a 1 bloco: evita troncos colados atraves da borda e e igual nos dois chunks
- [x] `menu.inc`/`main_loop.inc`: clima (`weather`, `wxleft`) salvo em level.cfg e restaurado ao abrir o mundo (save antigo = limpo)
- Teste: sintaxe ok; test_weather 9/0; test_chest 42/0; test_meshfuzz ok
- Limites: (VIS) densidade pode cair um pouco na floresta; candidatas descartadas mesmo que a vizinha nao tenha sido plantada
- [ ] Falta: conferir no jogo; rios/lagos/montanhas; clima: som, convidado mudar, raio com dano

## patchTerrain.4 + patchInv.5 (encostas de montanha + cursor da rolagem do inventario)
- Causa do cursor sumido (pack 1.12): o scroller (232,0 / 244,0) esta em `creative_inventory/tabs.png`, nao em `tab_items.png` (la o pixel e transparente: a textura carregava mas ficava invisivel)
- [x] `asset_manager.inc`: rtScr[0/1] recortados de tabs.png
- [x] `world.inc` genImpl: em bioma de montanha, ressalto >=4 blocos em volta vira pedra/cascalho (25%), 3 blocos vira pedra em ~60%; sem grama e sem arvore nos paredoes
- Teste: sintaxe ok; test_chest 42/0; test_meshfuzz ok
- Limites: (VIS) tudo; cana-de-acucar nao existe como bloco no jogo (precisa de modelo novo); lagos/rios/masmorras ja vem do porte do MC (worldgen.inc)
- [ ] Falta: conferir no jogo; cana-de-acucar (bloco novo); inclinacao so olha dentro do chunk

## patchWx.5b (raio com dano e fogo)
- [x] `weather.inc`: raio causa 5 de dano a quem esta a <3 blocos (nao no criativo) e ateia fogo no ponto atingido (2/3 das vezes, so host/solo, se a base aceita fogo)
- Teste: sintaxe ok; test_weather
- Limites: (VIS); raio e local a cada cliente (nao sincronizado no MP); nao atinge mobs nem acende TNT; sem som
- [ ] Falta: raio sincronizado, raio em mobs/criper carregado, sons

## patchOutline.1 (contorno das plantas altas)
- Sintoma (print): contorno da grama alta curto/estreito, com vao entre as duas metades e fora do desenho da planta
- [x] `blockshape.inc`: gSh.tall (cada metade de planta alta: grama alta, samambaia grande, girassol, lilas, roseira, peonia) agora .15..85 x 0..1 (antes usava a caixa da flor, altura .6)
- Teste: sintaxe ok
- Limites: (VIS); flores/grama baixa seguem a caixa do MC (flor .3-.7 h.6; grama .1-.9 h.8) que pode parecer menor que o modelo; sem deslocamento aleatorio XZ do MC
- [ ] Falta: conferir no jogo; se flores/grama baixa ainda parecerem erradas, mandar print

## patchRed.3 (conexao da poeira como no MC 1.12.2 + curvas com bracos)
- [x] `redstone.inc` rsLink: poeira liga no repetidor so nos eixos de entrada/saida (antes: qualquer lado) e no observador so pela saida (costas); resto igual (tocha, alavanca, botao, placa, comparador, bloco de redstone, poeira em degrau)
- [x] `redstone.inc` modelo da poeira: curva/T/cruz = ponto central + meia linha por lado ligado (antes desenhava um "+" completo mesmo com 2 lados em L); 1 lado ou lados opostos = linha inteira como antes
- Teste: sintaxe ok; test_piston, test_minecart 39/0, test_meshfuzz; test_redstone precisa do pack (SKIP aqui)
- Limites: (VIS) tudo; o sentido de rotacao dos bracos (N->L->S->O) segue a convencao do xbBake e nao foi visto; trilhos: nenhuma mudanca (porte do MC com 39 testes passando; sem relato concreto do erro)
- [ ] Falta: conferir no jogo; trilhos: dizer o que falha (print/descricao)

## patchOrg.1 (organização do projeto)
- [x] Scripts `build-*.sh` e `make-appimage.sh` → `scripts/` (`cd "$(dirname "$0")/.."`; `build.sh` continua na raiz).
- [x] `HANDOFF.md`, `LEIAME*.txt` → `doc/` (antigos em `doc/historico/`); `genskins.py` → `tools/`.
- [x] `src/test_boat_net.cpp` e `src/test_team.cpp` → `tests/`; cópias velhas de test_conn/nick_sync/playerlist em `src/` removidas (as de `tests/` são as novas).
- [x] Removidos `crash-reports/` e `tests/saves/`; criado `.gitignore`; README com a nova árvore.
- Teste: sintaxe do jogo OK; `bash -n` em todos os scripts. Limites: scripts não rodados de verdade (sem SDL2/mingw aqui).
- [ ] Falta: `src/*.inc` continuam planos (86 arquivos); agrupar em subpastas exige mexer em todos os `#include` do main.cpp.

## patchWx.6 (operador convidado muda o clima)
- Causa: `/weather` de operador convidado mandava o pacote NP_TIME, mas o host so aplicava hora/ciclo e ignorava o clima (ficava so no convidado ate o proximo envio do host).
- [x] `net.inc` netTimeRecv: host aplica `gWeather` do operador (timer padrao do ciclo; duracao em segundos digitada pelo convidado nao viaja) e re-transmite a todos. NET_VER inalterado (so logica do host).
- Teste: test_weather 12/0 (3 novas: operador aplica, nao-operador ignorado, sem bit de comando ignorado); test_chest 42/0; sintaxe ok.
- Limites: (VIS) nao testado com 2 instancias reais.
- [ ] Falta: raio sincronizado/em mobs, sons de chuva/trovao, neve acumulando, cana-de-acucar (bloco novo), rios/lagos/montanhas

## patchBuild.1 (build.sh com flags por plataforma)
- [x] `build.sh`: `--linux`, `--windows`, `--android`, `--rpi`, `--all` (combinaveis); `--pack`, `--debug`, `--release`, `--abis=A,B`, `--console`, `--installer`, `--help`. Sem plataforma = a do sistema. Uma falha nao interrompe as outras; resumo ok/falhou no fim. `TARGET=windows ./build.sh` continua valendo.
- [x] `scripts/build-android.sh` aceita `--debug|--release|--abis=` alem do formato antigo (`release arm64-v8a`). `scripts/build-all.sh` removido (o build.sh o substitui).
- Teste: `bash -n` em todos; flag invalida e --debug+--release recusadas; --all sem SDL2/SDK mostra erro de cada plataforma e o resumo.
- Limites: nenhuma plataforma foi compilada de verdade aqui (sem SDL2, mingw, SDK Android).
- [ ] Falta: testar `./build.sh --linux/--rpi/--android` na sua maquina; sons de chuva/trovao (nao ha arquivo de som de clima em assets/sounds)

## patchBuild.2 + patchWx.7 (barra de progresso nos builds + raio fere mobs)
- [x] `build.sh`: barra de progresso geral com %, spinner, tempo e resumo final (tamanho do binario/APK). A % e ESTIMADA (compilar e um passo unico): tempo decorrido contra a duracao da ultima compilacao da plataforma (`logs/.build-times`; 1a vez usa um chute). Log de cada plataforma em `logs/build-<plataforma>.log` (ultimas linhas aparecem se falhar). `--plain` (ou sem terminal / NO_COLOR) = linhas simples com %. Precisa de bash 4+ (Linux/MSYS2; o bash 3.2 do macOS nao serve).
- [x] `weather.inc`: `wxBoltStrike` (jogador, mobs a <3 blocos levam 5 de dano, fogo); so host/solo mexe em mobs.
- Teste: test_weather 13/0 (nova: raio fere os 2 mobs proximos, nao o distante); test_egg e test_chest sem falhas; interface do build testada com scripts falsos (terminal e --plain).
- Limites: (VIS) nenhum build real rodado; mobs nao pegam fogo nem criper carregado ainda.
- [ ] Falta: raio sincronizado no MP (efeito visual so no host/solo para mobs), criper carregado, sons de chuva/trovao (faltam arquivos), neve acumulando, cana-de-acucar, rios/lagos/montanhas

## patchRail.1 (trilho nao entrava no chao com mato)
- Causa provavel: `railPlace` so aceitava o alvo exatamente AIR; o chao do mundo esta cheio de grama alta/samambaia, e mirar nelas fazia o alvo ser o bloco ACIMA da planta (ou a propria planta, recusada). Os blocos normais ja substituiam mato (`placeBlock`), o trilho nao.
- [x] `minecart.inc` railPlace: aceita AIR, liquido e mato (`xbReplaceable`: grama, samambaia, arbusto seco; flor nao, como no MC) + checagem de limites do mundo; railUse: mirar no mato coloca o trilho no lugar dele.
- Teste: test_minecart 46/0 (4 novas: grama e samambaia cedem, flor e pedra nao, sem chao nao); test_piston 142/0; test_chest 42/0; sintaxe ok.
- Limites: (VIS) nao visto no jogo; se ainda falhar em outro lugar (ex.: sobre laje, neve, areia), mandar print/descricao de onde.
- [ ] Falta: conferir no jogo; trilho sobre neve (camada de neve nao e substituida)

## patchProp.1 (conversor de mods modconv + Decocraft convertido + leitor props.inc)
- [x] `conversor/modconv/` (core, pack, csmodel, handlers generic + decocraft, preview, db/): converte mod 1.12.2 -> BLP1 (props.bin + atlas). Cada execução registra lacunas em `db/unknown.json` e `db/history.json`.
- [x] Decocraft 2.6.3.7 convertido em `mods/decocraft/`: 1076 modelos, 3119 itens (variantes), 39 018 caixas, 5 páginas de atlas; nomes e abas em pt-BR.
- [x] `src/props.inc`: leitor de `props.bin` (`propsLoad/propsLoadAll`, `propsFaceQuad`); incluído em main.cpp, ainda não chamado no jogo.
- Teste: `tests/test_props.cpp` 25/0 (mod real; mutação na ordem de UV foi pega); `tests/test_modconv.py` 8/0 (jar sintético); `g++ -fsyntax-only` do main.cpp sem erros. Não rodei o `tests/run.sh` inteiro. Preview à vista (creeper, relógio, garrafa, vitral) conferido em imagem.
- Limites: (VIS) orientação de rotações CraftStudio só conferida em 6 modelos (Cavalo de Pau parece embaralhado); lado esquerdo/direito e chiralidade não verificados com texto. Nada disso aparece no jogo ainda.
- [ ] Falta (patchProp.2): desenhar no jogo (id de bloco para props, malha com a textura do atlas próprio, colisão/seleção pela bbox, rotação por `facingFromYaw`, luz `light`, aba por categoria no criativo, nome pt-BR). Limite atual de mods (136 ids) não comporta 3119 itens: precisa de faixa nova de ids.
- [ ] Falta: comportamentos (cadeira, cama, baú, porta, lâmpada ligável, partículas); texturas `textureOverride` ausentes do jar (245); `faceCulling`, `colorSecondary`; handlers para o próximo mod enviado.

## patchProp.1b (ids de props + aba Decocraft no criativo)
- [x] Faixa propria de ids `PROP0=32768..65535` (main.cpp); `isProp/propItem/propName` em props.inc; `propsLoadAll()` agora e chamado (via `registerPackTab`, mods.inc). Uma aba por pacote (`mods/<nome>`), com todos os itens (3119 no Decocraft); nome em itemName; busca funciona por nome.
- Teste: NAO compilado aqui (sem SDL no ambiente); so revisao de tipos (TabInfo/ModDef).
- Limites (VIS): icone provisorio (tile do graveto); item de prop nao e colocavel (isBlk falso) ate o Prop.2.
- [ ] Falta (Prop.2): icone do atlas proprio, colocacao (bloco/entidade), malha, colisao, rotacao. Salvar ids em chunk ja cabe em 16 bits.

## patchPortal.1 (portal do Nether sem textura)
- [x] `nether.inc`: tile do portal (e do fogo) alocado ANTES dos 4 tiles do portal do End; margem do atlas 64->8 so para a tira animada; log em stderr se faltar tile ou o png.
- Teste: nao compilado (sem SDL). (VIS) conferir portal animado; se continuar sem textura, ver a linha "portal: sem tile livre..." em logs/latest.log (mostra quantos tiles restam).
- [ ] Falta: se o atlas estiver mesmo estourado, reduzir tiles do Chisel/entidades.

## patchProp.2 (props do Decocraft: icones 3D + colocaveis)
- [x] `src/propsys.inc` (novo): prop = entidade estatica `PropEnt` (nao bloco). Malha a partir de `props.bin` com as paginas de textura do mod (carregadas sob demanda, `loadTexturePNG`); icone 3D isometrico em `drawIcon` (`propDrawIcon`, so faces voltadas ao observador, modelo ampliado p/ caber no slot).
- [x] Colocar com botao direito (celula adjacente a face mirada, rotacao em passos de 22,5 graus com a frente virada para o jogador, 1 prop por celula, nao coloca em cima do jogador, modelo que desce abaixo de y=0 e apoiado no chao); remover com botao esquerdo (criativo: some; sobrevivencia: solta o item). Colisao pela caixa do modelo (`propCollBoxes` em `collBoxes`; `nocoll` e itens finos como tapetes nao colidem). Salvar/carregar `saves/<mundo>/props.dat` (chave mod/id estavel).
- Teste: `test_propsys` 22/0 (3119 itens: malha, icone dentro de 16x16, colocar, colisao, rotacao, remover, salvar); `test_props` 25/0 (agora inclui main.cpp); sintaxe do jogo OK. Nao rodei o jogo.
- Limites (VIS): visual/GL nao conferido. Sem rede (cada jogador ve os proprios props), sem splitscreen; mao e item solto no chao mostram o modelo reduzido a caixa unitaria (VIS: posicao/escala na mao nao conferida); contorno ao mirar usa a caixa do modelo; luz do item (`light`) e `beh` (cadeira, cama, lampada, bau, porta) sem efeito; faces com textura por referencia (vanilla) saem cinza; 1a abertura de cada pagina de textura (4096x4096) pode dar um engasgo.
- [ ] Falta: Prop.3 (comportamentos), rede, luz, pick block (botao do meio), Prop.5 rotacao CraftStudio.

## patchProp.2b (transparencia dos props)
- [x] Passada translucida em `drawProps` (blend, sem escrever profundidade, props do mais longe ao mais perto); textura com >=25% dos texels visiveis de alfa parcial e detectada ao carregar a pagina (102 de 2688 no Decocraft). Icones tambem usam blend.
- [x] O atlas do Decocraft so tem alfa 0/255 (vidro nao vem translucido da textura): `mods/decocraft/translucent.txt` define por nome de item a opacidade (hoje so `Glass Block Window` 0.6; editavel sem recompilar).
- Teste: test_propsys 24/0. (VIS) nao visto no jogo.
- [ ] Falta: decidir quais outros itens de vidro (mesas, armarios, vitrines) ganham opacidade (so parte do modelo e vidro); props-bloco (janelas/paredes) que deveriam encaixar na grade como bloco.

## patchProp.2c (pick block + encaixe)
- [x] Botao do meio num prop (criativo) pega o item; props de bloco inteiro (largura/profundidade ~1 bloco: janelas, paredes) so giram em 90 graus. Teste: test_propsys 24/0 (VIS nao visto no jogo).

## patchProp.3 (colisao por parte, empilhar props, conversor com hierarquia)
- [x] `propsys.inc`: colisao/selecao por PARTE do modelo (ate 24 caixas, rotacionadas) em vez de uma caixa unica; painel de espessura zero (janela/placa) engrossa p/ 1/16 (antes nao colidia nem recebia o raio). `box` = uniao das partes.
- [x] Empilhar: `propPlaceOnProp` (botao direito mirando um prop): em cima = ponto mirado em 1/8 de bloco sobre a parte atingida; ao lado = encostado, mesmo nivel da base. Removida a regra "1 prop por celula": agora so nao pode sobrepor prop/bloco/jogador. Agachado + clicar bloco = posicao livre em 1/8.
- [x] Conversor: `csmodel.py` ganhou hierarquia (pos/quat/escala do filho relativos ao pai) com 7 leituras candidatas do vinculo; `decocraft.py` escolhe a de menor score (centrado, apoiado, altura plausivel) e grava no RELATORIO. Cache de modelo agora por (asset, escala) e variante usa a propria escala (antes herdava a do pai/primeiro item = tamanhos errados). `csdump.py` imprime os nos de um modelo.
- Teste: test_propsys 62/0; test_props 25/0; test_chest 42/0; test_modconv 8/0; test_cstree 10/0; sintaxe ok.
- Limites: sem o .jar aqui, NAO reconverti o Decocraft: `mods/decocraft/props.bin` e o antigo, entao lanterna/tamanhos so mudam apos rodar o conversor (ver abaixo). A hierarquia e escolhida por score, nao confirmada com os bytes reais. (VIS) tudo no jogo.
- Como aplicar: `python3 conversor/modconv/modconv.py Decocraft.jar --out mods` e ler `mods/decocraft/RELATORIO.md` ("hierarquia escolhida"). Se disser flat/nenhuma melhora: `python3 conversor/modconv/csdump.py Decocraft.jar <assetID da lanterna>` e mandar a saida.
- [ ] Falta: reconverter com o jar; props-bloco (janela/parede) encaixando na grade ao empilhar; Prop.3 comportamentos (cadeira, cama, lampada, bau, porta).

## patchProp.3b (Decocraft reconvertido: offset do no nao leva escala)
- [x] Causa real da lanterna/tamanhos: em `boxes_of` o `off` (pivo) era multiplicado pela escala do no (0,25 em muitos modelos) -> pecas derivavam em diagonal. Agora `centro = pos + rot(off)`. A hierarquia (u16 final = 0 em todos os nos) nao era a causa; o conversor segue escolhendo `flat`.
- [x] `mods/decocraft/` reconvertido do jar real: lanterna de papel centrada (x/z +-0,38); modelos de 15 blocos de altura sumiram (max 6,8); modelos fora de centro 292 -> 262, UV fora da textura 230 -> 1.
- Teste: test_props 25/0, test_propsys, test_modconv 8/0, test_cstree 10/0.
- Limites: (VIS) nada visto no jogo. Alguns modelos continuam assimetricos por natureza (cadeira, bancada).
- [ ] Falta: conferir lanterna/tamanhos no jogo; Prop.3 comportamentos.

## patchProp.4 (vidro: nuvem, culling, translucidez por vidraca; lampada e porta)
- [x] Nuvem aparecendo por cima do vidro: os props translucidos eram desenhados ANTES das nuvens (e nao escrevem profundidade). `drawProps(..., only)`: opacos no lugar de antes, translucidos num bloco novo logo apos as nuvens e antes da agua (`main_loop.inc`).
- [x] Translucidez por QUAD: so as faces grandes de caixas finas (vidraca) ficam translucidas; moldura/cabo ficam opacos (antes o item inteiro ficava translucido ou nada). `mods/decocraft/translucent.txt` ganhou regras Window/Janela/Vidro/Glass/Vitrine/Pane (0,55).
- [x] Culling de vidro como no MC: face do vidro colada em vidro IGUAL (mesmo item, mesma rotacao, celula vizinha) nao e desenhada (`PropEnt::nb`, `propNbAll`). So rotacoes de 90 graus e faces no limite da celula.
- [x] Comportamentos: `SwitchableLamp` liga/desliga (botao direito, escurece quando apagada; estado salvo); `Door:N`/`LinkedModel:N` troca pelo modelo vinculado (porta abre/fecha, arquivos de gabinete etc.). Agachado + botao direito = empilhar. Conversor grava o modelo vinculado em `beh` (`Door:1199`); `mods/decocraft/` reconvertido.
- Teste: test_propsys 72/0 (novos: vidraca x moldura, culling por vizinho/rotacao, lampada, porta, salvar `on`), test_props 25/0, test_chest 42/0, test_modconv 8/0, test_cstree 10/0, sintaxe ok.
- Limites: (VIS) nada visto no jogo. Lampada apagada nao reduz a luz do mundo (so escurece o modelo). Janelas de varios blocos (ex. 2,5 de largura) nao foram redimensionadas.
- [ ] Falta: Chair (sentar), Bed (dormir/spawn), Container (bau), ParticleEmitter (castical), Faucet/Shower, RecordPlayer, TimedModel; luz da lampada no mundo; tamanho de vidro "bloco" (preciso saber qual item); Prop.5 (rotacao CraftStudio, 245 texturas embutidas, colorSecondary/faceCulling).

## patchProp.5a (Decocraft: hierarquia por id; presentes)
- [x] Conversor: `id` do no = (pai<<16)|proprio. `csmodel.par_pair/has_pair/compose_pair`: filho = pivo do pai + rot(q_pai, off_pai + pos); escala nao herdada. Usado em 777 de 1077 modelos (handlers/decocraft.py). Tampa e lacos dos Presentes agora assentam sobre a caixa.
- [x] Decocraft reconvertido (mods/decocraft/*). Metricas: modelos abaixo de y=-0,05: 749 -> 250; score total dos modelos com hierarquia 2428 -> 858 (621 melhores, 51 piores); fora de centro 185 -> 209.
- Teste: test_cstree 12/0, test_modconv 8/0, test_propsys 72/0, test_props 25/0.
- Limites (VIS): nada visto no jogo. 51 modelos pioraram no score (ex.: assets 1966, 2312, 1312, 3235): conferir.
- [ ] Falta: Prop.5b Chair+Bed; Container; Particle+Faucet; Prop.6 (rotacao CraftStudio, 245 texturas, colorSecondary/faceCulling); Prop.7.

## patchProp.5b (Decocraft: Chair e Bed)
- [x] Chair (470 itens): botao direito senta (`gSeated`, `propSeatFor`: topo da maior caixa horizontal a 0,25-0,9 de altura, centrada nela); `propSeatTick` em main_loop.inc segura posicao/velocidade; agachar/pular levanta; prop removido levanta.
- [x] Bed (240 itens): botao direito define spawn (`gSpX/Y/Z`) e pula a noite (>=12500) como o bloco de cama.
- Teste: test_propsys 81/0 (antes 72/0), sintaxe ok.
- Limites (VIS): sem pose sentada (corpo fica em pe, baixado 0,1 do topo); sem animacao de dormir/tela escura; sem rede/splitscreen; sentar nao gira para a frente da cadeira; cama nao exige duas metades nem checa monstros.
- [ ] Falta: Prop.5c Container (94); Particle (29) + Faucet/Shower; RecordPlayer, TimedModel, Decobench, VariableRendering; luz da lampada no mundo; Prop.6; Prop.7.

## patchProp.5b2 (Decocraft: raiz do modelo como pai; janelas/vidros)
- [x] Conversor: a RaizNode (id<0) agora e pai dos nos (pos/rotacao propria da raiz estavam sendo descartadas). Janelas de 1 bloco (Single Block Window: 1x1, antes 1,4 x 1,4 deslocada) e demais vidros voltam ao tamanho certo. Score total (modelos com hierarquia) 858 -> 345; modelos abaixo do chao 250 -> 43 (originalmente 749); fora de centro 283 (muitos legitimos: janelas 2x, 4x ancoradas na borda).
- Teste: test_cstree 12/0, test_modconv 8/0, test_propsys 81/0, test_props 25/0. Decocraft reconvertido (mods/decocraft/*; translucent.txt preservado).
- Limites (VIS): nada visto no jogo; janelas finas ficam na borda +Z da celula (ancora do modelo).
- [ ] Falta: Prop.5c Container; 5d Particle/Faucet/etc; Prop.6; Prop.7. Se ainda houver vidro errado: nome do item.

## patchProp.5b3 (props de parede)
- [x] `propWallSide` (modelo fino em Z <=0,35 e encostado numa borda: 253 itens encostam em +Z = janelas/portas, 184 em -Z = quadros/placas/cortinas/guirlandas) + `propWallPlace`: mirando o LADO de um bloco, escolhe a rotacao (so 0/90/180/270) cuja borda de tras fica rente ao plano da parede e encosta o prop nela. Antes: rotacao pelo yaw do jogador (qualquer 22,5 graus) e janela de +Z ficava longe da parede.
- Teste: test_propsys 130/0 (antes 81/0): 2 tipos x 4 lados x 3 yaws rentes ao plano.
- Limites (VIS): heuristica por tamanho do modelo (123 itens finos nao encostados, como lampadas de mesa, ficam como antes); so vale ao mirar bloco (nao ao mirar outro prop); no teto/chao segue o yaw; agachado mantem a posicao livre ao longo da parede.
- [ ] Falta: Prop.5c Container; 5d Particle/Faucet/etc; Prop.6; Prop.7. Se algum prop de parede ainda ficar virado: nome do item.

## patchProp.5c (Decocraft: Container)
- [x] Container (94 itens): botao direito abre a tela do bau (27 slots) como bau virtual: chave `(x, y+PCH_OFF, z)` na mesma `gChests`; salva/carrega junto de `chests.dat`; remover o prop solta o conteudo (`propContainerRemoved`). Codigo no fim de `chest.inc` + ramo em `propUse`; `main_loop.inc` abre a tela via `gPropOpenChest`.
- Teste: test_propsys 139/0 (antes 130/0), test_chest 42/0. Fecha ao se afastar (8 blocos) ou se o prop sumir.
- Limites (VIS): tela mostra o titulo/aparencia do bau comum; so local (sem rede: convidado nao ve o conteudo); 2 Containers na mesma celula e mesma altura compartilham o conteudo; sem tampa animada; sem comparador.
- [ ] Falta: Prop.5d Particle (29) + Faucet/Shower; RecordPlayer, TimedModel, Decobench, VariableRendering; luz da lampada no mundo; Prop.6; Prop.7.

## patchProp.6 (Decocraft: sentar de verdade, rede, translucidez por face)
- [x] Cadeira: pose sentada (reaproveita `Pose.sit` do bote: pernas para a frente), corpo virado para a FRENTE da cadeira (`gSeatYaw` = giro*22,5 + 180), pes a `topo - 0,586` (olho = tampo + ~1,0; antes o corpo ficava em pe), quadril um pouco atras do centro do assento; ao levantar sai na frente/lado/tras onde couber (senao em cima). Vale em 1a/3a pessoa, sombra e reflexo.
- [x] Rede (NET_VER 29, `NP_PROP`): colocar/remover/trocar (porta, lampada) aplicam local e vao ao host, que repassa; convidado que entra recebe todos os props (`propNetSyncPeer`); cama de convidado pede ao host para pular a noite (op 4); cadeira ocupada por outro jogador nao deixa sentar; jogadores remotos sentados aparecem sentados e virados (`NP_STATE` leva sit + yaw no fim).
- [x] Bug de rede achado: ids de item/drop iam em `int16` e props (ids 32768..35886) estouravam: item na mao de outro jogador e drop de prop apareciam errados. Agora `uint16` (held e NP_DROP).
- [x] Container (94) funciona na rede: convidado pede o conteudo ao host (NP_CHESTREQ com a chave virtual y+PCH_OFF), edita e o host guarda; chest.inc.
- [x] Texturas: a translucidez era decidida pela TEXTURA inteira (>=25% de texels parciais): moldura opaca de mesa/aquario/vaso com vidro ia para a passada do vidro (sem escrever profundidade, depois das nuvens). Agora decide por FACE (`PropQuad.trc`, >=5% de texels parciais na area de UV da face; `gPropTexA` guarda o alfa so das ~100 texturas translucidas).
- Teste: test_propnet 29/0 (novo: direcao/altura do assento em 60 cadeiras x 6 giros, ocupado por remoto, NP_PROP add/swap/del/sync/cama/chave desconhecida, uint16, translucidez por face), test_propsys 140/0, test_chest 42/0, test_props 25/0, test_boat_net 20/0, test_conn/host_leave/chunk_sync/furnace ok. (test_chest2 e test_cmd_player ja falhavam antes do patch.)
- Limites (VIS): nada visto no jogo. Sentado: bracos seguem a pose do bote (para a frente), nao ha animacao de sentar/levantar; sem dormir na cama (so spawn + pular a noite); props so sincronizam por UDP sem reenvio (pacote perdido = prop fantasma ate reentrar; blocos tem reenvio, props ainda nao); 245 itens sem textura de override no jar e 230 faces com UV fora da textura (fixadas na borda) so se resolvem reconvertendo com o .jar.
- [ ] Falta: Prop.5d Particle (29) + Faucet/Shower; RecordPlayer, TimedModel, Decobench, VariableRendering; luz da lampada no mundo; Prop.6 (rotacao CraftStudio, colorSecondary/faceCulling, 245 texturas); Prop.7 (reenvio de pacote perdido, splitscreen).

## patchMod.1 (conversor generico: rotacao de variante, multipart, mais parents)
- Feito: `blockstate` com `x`/`y` agora gira as caixas em torno do centro do bloco (antes ignorava); `multipart` junta todas as partes sem `when`; parents vanilla novos `block/cross`, `slab`, `slab_top`, `carpet`; lacunas novas: `multipart condicional ignorado`, `rotacao de UV de face ignorada`. `pack.qmul` adicionado.
- Teste: `tests/test_modconv2.py` (4) + `test_modconv` (8) passam. Sentido do giro em X nao conferido no jogo (VIS).
- Falta: um prop por estado do blockstate (hoje 1 modelo por bloco), `when` do multipart, rotacao de UV de face, stairs/fence/pane, animacao alem do 1o quadro, itens sem blockstate.

## patchMod.2 (props empilhados tortos; alcapao com redstone)
- Feito: `propPlaceOnProp` (propsys.inc) agora herda o giro (`rot`) do prop de baixo ao empilhar/encostar; antes usava o yaw do jogador e o de cima ficava girado/torto. Alcapao (ferro e madeira) agora abre/fecha por redstone: `rsTrapCheck` (redstone.inc) chamado de `xbNeighbor` (blockupdate.inc); antes nada ligava o alcapao a redstone. Ferro continua sem abrir por clique.
- Teste: `test_irontrap` (5), `test_propsys` (141, +1 de empilhar com yaw diferente), `test_update` (76). Colisao/textura do alcapao nao testadas (precisam do pack).
- VIS: alcapao reagindo a alavanca/botao/placa no jogo; empilhar monitores com o jogador olhando noutra direcao.
- Falta: avaliar o alcapao ao colocar (hoje so reage quando um vizinho muda); energia via bloco solido fortemente energizado; shift para empilhar com giro livre; portas/portoes tambem nao reagem a redstone.

## patchMod.3 (empilhar com giro livre)
- Feito: agachado (shift) + botao direito num prop: o novo prop usa o yaw do jogador (passos de 22,5 graus) em vez de herdar o giro do de baixo. `propPlaceOnProp(..., freeRot)` em propsys.inc; `main_loop.inc` passa `P.sneak`. Sem shift continua herdando (patchMod.2).
- Teste: `test_propsys` 142 (0 falhas), +1 de giro livre. VIS: conferir no jogo.
- Falta: portas/portoes com redstone; alcapao ao colocar; estados do blockstate no conversor; Prop.5d/6b/7.

## patchMod.4 (conversor: tradutor de acoes do Forge 1.12.2)
- Feito: `conversor/modconv/forge.py` (novo) + gancho em `core.convert`: varre `.class` (constantes) e classifica chamadas do Forge pela TABLE; traduz recipes JSON (shaped/shapeless/smelting, ore dict), loot_tables e sounds.json para `receitas.txt`/`drops.txt`/`sons.txt` + `forge_acoes.json`; lacunas em RELATORIO/db.
- Teste: `tests/test_forge.py` (9), `test_modconv` (8), `test_modconv2` (4). Nao testado com .jar real (so sintetico).
- Falta (jogo): ler `receitas.txt` (crafting.inc: Recipe/sh), `drops.txt` (blockDrops) e `sons.txt` no loader de mods (`mods.inc`); resolver nomes vanilla (minecraft:planks) para ids do pack. Falta (conversor): valores de setHardness/setLightLevel via bytecode, estados do blockstate.

## patchMod.5 (blocos de luz colorida; pendencias na raiz)
- Feito: `mods/luzes_coloridas/` (mod.txt + 32 texturas 16x16 do colored_lamp.png do Colored Lights): 16 cores x acesa (light=15) / apagada (light=0). `PENDENCIAS.md` na raiz com todas as pendencias; este arquivo passa a ser so historico.
- Teste: nenhum automatizado (formato igual a mods/exemplo). VIS: abrir o criativo e ver a aba "Luzes Coloridas".
- Falta: motor de luz RGB.
- 2o pedido: armas — ver patchMod.7.

## patchMod.7 (conversor: itens e armas)
- Feito: `generic.py` converte `models/item/*.json` em props: modelo 3D com `elements` (guns do MrCrayfish: 22 itens, ate 68 caixas, texturas vanilla por nome) e icone plano `item/generated|handheld` (caixa 1x1 de 1px; Techguns: 308 itens). Copia `guns/*.json` para `armas.json`. Saida: `mods/cgm`, `mods/techguns`.
- Teste: `test_modconv` 8, `test_modconv2` 4, `test_forge` 9 passam; conversao real dos 2 jars sem erro. Nao testado no jogo (VIS).
- Falta: ver PENDENCIAS.md (raiz), item Armas/Techguns.

## patchMod.8 (pendencias novas + thuttech reconvertido)
- Registrado em PENDENCIAS.md: armas sem textura e mal apoiadas na mao; regra de grid para todo mod (Thut's Tech fora da grade); Thut's Tech sem Device Linker/Elevator Rail. `mods/thuttech` reconvertido com o conversor novo (inclui Device Linker).

## patchProp.7 (itens planos Techguns/cgm; luz do dia em props na mao)
- Feito: `PropMesh.flat` (1 caixa fina 1x1: 319 itens Techguns/cgm) desenha o icone como sprite 2D de frente (`propBuildIcon`) em vez da laje isometrica; props 3D (cubos, maquinas, guns com varias caixas) usam pose de bloco (`isBlockLike`), planos usam pose de item vanilla. `propDrawUnit` multiplica a cor por `gEntDim`/`gEntHurt`: o prop na mao, no chao e em 3a pessoa agora escurece/clareia com dia, noite e caverna como `drawMesh`.
- Teste: `test_propflat` (5, novo). `test_propsys` tem 9 falhas ja existentes (assume pack 0 = Decocraft; com Cgm/Techguns instalados o pack 0 e o Cgm).
- VIS: icones Techguns no inventario/hotbar; item na mao ao anoitecer. Armas 3D (cgm) ainda sem pose propria (cano para frente).
- Falta: `test_propsys` achar o Decocraft por nome do mod (nao por indice 0); pose de mao propria das armas; `propDrawUnit` com luz propria de tocha/lampada.

## patchMod.9 (portas/portoes com redstone; alcapao ao colocar; test_propsys)
- Feito: `rsOpenCheck` (redstone.inc): portao e porta (ferro e madeira) abrem/fecham com alavanca/botao/placa/tocha/fio; a porta olha as duas metades e muda as duas sem disparar ganchos. Ligado em `xbNeighbor` e `rsAddedXb` (blockupdate.inc); o alcapao tambem e avaliado ao colocar. `test_propsys` acha o Decocraft pelo tamanho do pack (nao mais indice 0).
- Teste: `test_irontrap` 11 (+6: portao, porta de ferro 2 metades, alcapao ao colocar), `test_propsys` 142/0 falhas, `test_update` 76, `test_props` 25. VIS: porta/portao no jogo com alavanca e placa.
- Falta: energia por bloco solido fortemente energizado (alcapao/porta); colisao/textura do alcapao de ferro sem teste; porta de madeira aberta por redstone nao toca som.

## patchWorld.1 (conversor de mundo Minecraft 1.12.2 + mods -> Blockland)
- Feito: `conversor/mundo/` (`mc2bl.py`, `mapping2.py`, `mapping_base.py`, `pk112py.py`, `survey_mundo.py`): converte overworld + Nether para `saves/<NOME>/` (BL18 com nomes crus do pack 1.12.2, level.cfg, dim.txt, conversao.txt). Mundo `MAPA DO ESTADO` (11.521 chunks + 3.098 do Nether, 350 mi de blocos, 248 blocos de mods) convertido: 0,06% virou pedra por falta de equivalente. Novos vs conversor antigo: escada de mao, bau (dir/tipo), carpete, vinha, vitoria-regia, portao, placas, neve em camadas (= carpete branco), fogo.
- Teste: `tests/test_mapping2.py` (OK); formato BL18 validado por leitor Python (todos os chunks somam 65536). O jogo NAO abriu o save (VIS).
- Falta: tile entities (baus com itens, placas com texto, estandartes, cabecas); alavanca/funil/pistao/repetidor/fio de redstone; escada (forma curva/canto); biomas reais (o jogo gera pelo seed); props de mods (cfm, bibliocraft, littletiles) como props do Decocraft; chisel com variantes exatas (usar mods/chisel quando existir); mundos de outras dimensoes (DIM4).

## patchMod.8 — porte em massa dos mods (17 pacotes, +1957 itens)
- Feito: `generic.py` agora (a) varre `models/item/**` em subpastas (IC2 6→500, Aquaculture 19→77, cfm 336→432, vehicle 68→90), (b) segue a cadeia de parents até `item/generated|handheld` (IC2 `ic2:item/default`), (c) ignora maiusculas em modelos/texturas (StorageDrawers 35→69), (d) multipart so condicional usa a 1a parte. Novos pacotes em `mods/`: aquaculture, architecturecraft, bibliocraft, biomesoplenty, blockcraftery, cfm (Furniture), ic2, ironfurnaces, littletiles, notenoughwands, quark, rustic, storagedrawers, travelersbackpack, upsidedowndimensionmod, vehicle, waystones. Cada um vira aba (paginada de 10 em 10) e props como o Decocraft.
- `props.inc`: ordem dos pacotes = cgm, decocraft, techguns, thuttech primeiro (ids existentes nao mudam), depois os demais em ordem alfabetica. **Adicionar pacote novo no meio do alfabeto desloca ids dos que vem depois.**
- Teste: test_props 25, test_propsys 142, test_modconv 8 (headless). (VIS) nada visto no jogo.
- Falta: ironchest/bonsaitrees (blockstate `forge_marker` + TESR, sem modelo JSON), BiblioCraft/Chisel-like com TileEntity, IC2 maquinas (so o modelo de item/bloco simples; sem logica), LittleTiles/vehicle/Waystones (logica de codigo), Quark/BoP: blocos que sao vanilla-like (cubos) poderiam virar blocos reais em vez de props; receitas/drops/sons gerados mas o jogo ainda nao os le; mods so de logica/UI (JEI, Hwyla, JourneyMap, WorldEdit, VeinMiner, etc.) nao tem blocos.

## patchMod.9 — IronChest + itens sem modelo JSON
- Feito: `handlers/ironchest.py` (subclasse do generic): 8 baus (Ferro, Ouro, Diamante, Cobre, Prata, Cristal, Obsidiana, Terra) viram props `beh=Container`, modelo = base 14x10x14 + tampa 14x5x14 + trinco, UV do layout 64x64 de `textures/model/chest/*.png`. `generic.py`: fallback para texturas de `textures/item[s]/**` sem json de modelo (icone plano) + gancho `extra()`. Reconversao total: 2148 itens em 18 pacotes (ironchest 0→26, cfm 432→511, upsidedown 16→35, rustic 148→170, quark 180→192...). `bonsaitrees` segue com 0 itens (nao instalado).
- Teste: test_props 25, test_propsys 142, test_modconv 8. (VIS) nada visto no jogo.
- Limites: Container de prop tem 27 slots (baus de ouro/diamante deveriam ter 81/108); orientacao do trinco/UV do topo e da base sem conferir (VIS); baus de shulker do IronChest (112 texturas) nao portados; upgrades de bau so existem como item, sem a acao de melhorar.
- Falta: slots por tipo de bau (CHEST_SLOTS fixo em chest.inc), bonsaitrees, cubos simples de BoP/Quark como blocos reais.

## patchMod.10 (texturas vanilla embutidas; itens de mod nao empilhaveis; 4 mods novos; conversor)
- Causa do "posicionavel sem textura": o jogo NAO resolve textura de prop por nome (`propsys.inc`: `pt.ref` -> -1), e o conversor deixava `minecraft:blocks/x` so como nome. Agora `conversor/modconv/vanilla.py` le `assets/*minecraft-assets*textures*.zip` (ou `--vanilla <zip>`) e embute a imagem no atlas (aliases `block/`->`blocks/`, 1o quadro de animacao). cfm 110->2 refs, rustic 17->1, quark 15->2, vehicle 43 etc. Reconvertidos 18 mods (ids de item identicos aos antigos, conferido).
- Pilha: `PropItem.stack` + `propStackHeur` (props.inc): espada/picareta/machado/pa/enxada/elmo/peitoral/calca/bota/arco/escudo/martelo/chave/mochila/arma (rifle, pistola...) = 1; moveis (Decocraft, cfm, bibliocraft) sempre 64. `maxStack` (main.cpp) consulta `propStackOf` para ids de prop. Itens nativos (isTool/isArmor) ja eram 1.
- Mods novos portados: light_blocks (39), chisel (17), zombie_rush (21), invisiblights (4). ATENCAO: pacote novo no meio do alfabeto desloca os ids numericos dos seguintes (saves usam `mod/id`; rede = mesma build dos dois lados).
- Conversor: `--vanilla`, estatistica `vanilla_embutida` no relatorio.
- Teste: `test_propstack` (5698, novo), `test_props` 25, `test_propsys` 142/0, `test_modconv` 8. (VIS) nada visto no jogo.
- Falta: mods sem blocos JSON (bonsaitrees, neonium, MCA) nao portados; Chisel so 17 (variantes vem de codigo); tintura de grama/folha (texturas cinzas ficam cinza); heuristica de pilha por nome pode errar em itens raros (ajustar `tok[]`); campo `stack` real no props.bin (hoje so heuristica); aba criativa de mods com muitos itens.

## patchMod.11 (tintura de grama/folha nas texturas embutidas)
- Feito: `vanilla.tint` (conversor): texturas cinzas de folha/grama/samambaia/vinha/lirio multiplicadas pela cor do bioma planicie (grama 91BD59, folha 77AB2F, abeto/bétula/lirio proprias). So tinge se a imagem for quase cinza (nao mexe em texturas ja coloridas). Vale para texturas do jar e vanilla embutida. Reconvertidos BoP, cfm, quark, rustic (ids iguais).
- Teste: `test_modconv` 11/0, `test_props` 25, `test_propstack` 5698. (VIS) conferir folhas/grama de BoP e quark.
- Falta: cor por bioma real (so planicie); `tintindex` do modelo nao lido (tinge por nome); bonsaitrees/neonium/MCA; campo `stack` no props.bin.

## patchMod.12 (arma/ferramenta de mod na mao)
- Causa: item de mod 3D (arma) usava pose de BLOCO (45 graus, escala .4) e item plano de arma branca usava a pose generica na 3a pessoa. Agora `propHeldKindOf` (props.inc, por tokens do id): 2 = arma de fogo (gun/rifle/pistol/shotgun/smg/launcher/blaster...), 1 = arma branca/ferramenta (sword/pickaxe/axe/hammer/wrench...); carregador/municao/cano/peca = 0.
- Arma 3D (`propGunXf`, propsys.inc): transformacao de mao do `display` do modelo MrCrayfish (1a pessoa trans [-3,5;8,5;0] escala .8; 3a pessoa trans [0;6;-7]; cano = +Z), com recentro/normalizacao de `propDrawUnit`. Ligada em `displayXf` (3a pessoa) e no ramo de 1a pessoa de `player_render.inc`. Plano (sprite) arma branca/ferramenta: pose `handheld` da 3a pessoa (igual ferramenta nativa).
- Teste: `test_propstack` 5707/0 (+9), `test_propflat` 5, `test_propsys` 142/0. (VIS) nada visto: conferir cano para frente e altura da arma em 1a/3a pessoa (cgm: assault_rifle, rifle...).
- Falta: `display` por item nao e lido pelo conversor (usa os valores do cgm para todas as armas 3D); pose de bracos erguidos ao segurar arma de fogo; Techguns tem modelo .obj (so icone plano); armaduras de mod vestiveis (patchMod.13); blocos de mod como blocos reais, nao props (patchMod.14).

## patchMod.13 (armaduras de mod vestiveis)
- Conversor: `conversor/modconv/armor.py` (ligado em `core.convert`): detecta item *helmet/chestplate/leggings/boots, acha as texturas de camada no jar (`<material>_layer_N`, `<material>_N`, busca aproximada por prefixo; pastas `textures/armor|armors|models/armor|models`) e grava `mods/<mod>/armor.txt` (`id|slot|tex1|tex2|pontos`) + `armor/*.png`. Pontos por material (couro/pano 1-3-2-1, ouro 2-5-3-1, diamante/nano/quantum/neptunio 3-8-6-3, resto ferro 2-6-5-2). Sem textura achada = usa o ferro nativo. 92 armaduras (68 com textura): IC2 23, Techguns 56, Aquaculture 4, UpsideDown 6, ZombieRush 3.
- Jogo: `armorSlotOf(id)` (main.cpp) aceita item nativo ou prop com `aslot`; `armorOk`, equipar com shift-clique, `armorPoints` (defesa) e o desenho no jogador (`armorTexOf`, player_skin.inc) usam isso. `propArmorTex` (propsys.inc) carrega o PNG (64x32 completado para 64x64, que e o UV da malha de armadura). Armadura de mod = pilha 1.
- Teste: `test_propstack` 5990/0 (+283), `test_inv_keys` 9, `test_propsys` 142/0, `test_props` 25. (VIS) nada visto: vestir no inventario (slots), cor/UV da camada, defesa na barra.
- Falta: camadas "overlay" (quantum_1_overlay), armaduras Techguns com modelo proprio (so textura de camada vanilla); equipar por clique direito com a armadura na mao; armadura de mod no jogador remoto (`worn` nao vai pela rede); durabilidade; blocos de mod como blocos reais (patchMod.14).

## patchProp.8 (reenvio de pacote UDP perdido para props; teste de rede atualizado)
- Feito: `propsys.inc` (`gPropRep`, `propRepQueue`, `propNetRepeatTick`): ADD/DEL/SWAP de props sao reenviados mais 2 vezes (a cada 150 ms), como os blocos (`gEditRepeat`). Op nova na mesma celula (x,z,giro) descarta a pendente antiga, entao um DEL nao e desfeito por um ADD atrasado. `net.inc` chama o tick junto de `netEditRepeatTick`. Formato de pacote igual: NET_VER continua 29.
- Teste: `test_propnet` 35/0 (+5 de reenvio; corrigido o teste antigo, que assumia Decocraft = pacote 0 e 3119 itens exatos; agora acha o pacote "Decocraft" e soma o deslocamento de ids), `test_propsys` 142/0, `test_conn` 33/0, `test_irontrap` 11/0. Alcapao de ferro ja estava correto (patchMod.2): nenhum defeito encontrado nos testes. (VIS) nada visto no jogo.
- Falta: BED nao e reenviado (so pula a noite); splitscreen (jogador 2 nao senta nem usa props); Prop.5d (ParticleEmitter, Faucet/Shower, RecordPlayer, TimedModel, Decobench, VariableRendering); Prop.6b (rotacao CraftStudio, 245 itens sem textura de override, UV fora da textura so reconvertendo com o .jar); energia por bloco solido fortemente energizado (alcapao/porta).

## patchProp.9 (luz de props no mundo)
- Feito: props com `light>0` (167 itens do Decocraft: 157 de nivel 15) agora emitem luz no motor de luz dos chunks. `world.inc`: `gPropLightV` + mapa local `rPl` lido por `rEm`; `propsys.inc`: `propLightRebuild()` (chamado em `propNbAll`, ao ligar/desligar lampada local e por rede) refaz so os 9 chunks em volta quando a lista muda. SwitchableLamp so emite ligada.
- Teste: `test_propnet` 41/0 (+6), `test_propsys` 142/0, sintaxe do projeto limpa. (VIS) nada visto: conferir lampada ligando/apagando e luz sobre blocos vizinhos; custo do `rPl` por chunk so existe perto de props luminosos.
- Falta: Prop.5d restante (ParticleEmitter 29, Faucet 9, ShowerHead/Handles, RecordPlayer 2, TimedModel 4, Decobench 1, VariableRendering 51); dormir de verdade na cama; Prop.6b; splitscreen de props.

## patchMod.14 (porte dos mods restantes do 1-mods_para_converter.zip)
- Rodado o conversor nos 25 jars ainda nao portados. Com conteudo: `coloredlights` (6 itens: Color Card, po colorido e brilhante), `thutcore` (9), `baubles` (1: anel). Instalados em `mods/<mod>/` (props.bin + atlas + mod.txt + RELATORIO.md). Pacotes novos entram em ordem alfabetica, entao os ids numericos dos pacotes seguintes mudam (saves usam `mod/id`; rede = mesma build nos dois lados).
- Sem nada a portar (bibliotecas/clientes/utilitarios sem blocos ou itens proprios): AutoRegLib, Baubley-Elytra, Bloodmoon, CreativeCore, ImprovedMobs, RenderLib, TenshiLib, mcjtylib, worldedit, ido, collective, obfuscate, Chameleon, PTRLib, ItemPhysic, neonium (so codigo), bonsaitrees (sem modelo JSON, so receitas).
- Nao instalados de proposito: `naturescompass` (32 "itens" sao quadros da animacao da bussola, nao itens), `mcarebornlegacy` (1 item "Baby"), `carryon` (1 "tile"), `mysticallib` (1 "test_item"): lixo de conversao.
- Teste: `test_props` 25/0, `test_propstack` 6006/0, `test_propsys` 142/0, `test_propnet` 41/0. (VIS) nada visto no jogo.
- Falta: bonsaitrees/neonium (precisam de handler proprio, sem JSON); bussola do Nature's Compass como 1 item; logica de codigo dos mods (so itens/blocos como props).

## patchProp.10 (efeitos de props: castical, torneira, chuveiro)
- Feito: `propFxTick` (propsys.inc, chamado no inicio de `tickMobs`, 20 Hz, so props a ate 24 blocos, max 40 emissores e 600 particulas): ParticleEmitter (29 itens) solta fumaca fina do topo; Faucet (9) e ShowerHead (1) soltam gotas de agua caindo quando ligados. Clique direito liga/desliga Faucet/ShowerHead/ShowerHandles (`on`, vai pela rede como a lampada). Nascem desligados (`propMake`). `Puff` (mobs.inc) ganhou `kind` (0 fumaca, 1 agua azul que cai).
- Teste: `test_propnet` 47/0 (+6), `test_propsys` 142/0, `test_irontrap` 11/0, `test_mob_drown` 6/0; sintaxe limpa. (VIS) nada visto: posicao do bico/chama e quantidade das particulas foram estimadas pela caixa do modelo.
- Falta: ShowerHandles ligar o ShowerHead vizinho (hoje cada um liga so a si); chama animada do castical e cor por item; RecordPlayer 2, TimedModel 4, Decobench 1, VariableRendering 51; dormir de verdade; Prop.6b; splitscreen de props. Props antigos salvos com Faucet ja colocados voltam ligados (on=1 salvo antes do patch).

## patchProp.11 (alcapao/porta/portao energizados por bloco solido)
- Feito: `rsConsumerPowered` (redstone.inc): alcapao, porta e portao (ferro e madeira) agora abrem tambem quando um bloco solido VIZINHO esta fortemente energizado (alavanca presa nele, tocha de pe embaixo, repetidor); `xbNeighbor` (blockupdate.inc) reavalia os consumidores colados ao bloco solido quando algo ao lado dele muda (antes so reagia a fonte direta).
- Teste: `test_irontrap` 15/0 (+4: alavanca presa no bloco, desligar, alavanca desligada, aviso de vizinho), `test_update` 76/0. `test_redstone` deu SKIP (pack de recursos fora do zip enviado; rode no seu projeto). (VIS) nada visto no jogo.
- Falta: porta de madeira aberta por redstone nao toca som; energia fraca (poeira) por bloco solido; dormir de verdade na cama; Prop.5d restante (ShowerHandles->ShowerHead, RecordPlayer, TimedModel, Decobench, VariableRendering); Prop.6b; splitscreen de props.

## patchProp.12 (dormir de verdade na cama de prop)
- Feito: `propSleepTick` (propsys.inc, chamado em `main_loop.inc` junto de `propSeatTick`): usar uma cama Decocraft DE NOITE deita o jogador no colchao (preso, sem andar), a tela escurece (`gSleepA` + `drawFadeBlack`), apos 5 s (100 ticks) pula a noite (convidado pede ao host via `NP_PROP` BED); agachar/pular acorda sem pular a noite; de dia so define o spawn. Nao senta e dorme ao mesmo tempo.
- Teste: `test_propnet` 55/0 (+8), `test_propsys` 143/0 (teste da cama atualizado), `test_conn` 33/0. (VIS) nada visto: o jogador fica em PE sobre a cama (sem pose deitada nem camera baixa); altura exata sobre o colchao.
- Falta: pose deitada (Pose.sleep) e camera baixa; som de porta (nao existe asset de som de porta em `assets/sounds`: falta o arquivo .wav + linha em `events.cfg`); acordar ao levar dano; exigir todos os jogadores dormindo; Prop.5d restante (ShowerHandles->ShowerHead, RecordPlayer, TimedModel, Decobench, VariableRendering); Prop.6b; splitscreen de props.

## patchProp.13 (ShowerHandles liga o chuveiro)
- Feito: `propUse` (propsys.inc): usar um registro (ShowerHandles) alterna ele e liga/desliga os ShowerHead a ate 3 blocos (horizontal e vertical), com `propNetSwap` para os outros jogadores. Antes cada peca ligava so a si.
- Teste: `test_propnet` 59/0 (+4: liga o proximo, nao liga o distante, desliga). (VIS) nada visto no jogo.
- Falta: pose deitada (Pose.sleep) e camera baixa; som de porta; acordar ao levar dano; Prop.5d restante (RecordPlayer 2, TimedModel 4, Decobench 1, VariableRendering 51); Prop.6b; splitscreen de props.

## patchProp.14 (cama: pose deitada, camera baixa, acordar com dano)
- Feito: `Pose.sleep` (player_render.inc): corpo deitado de costas (gira -90 em X, +0,1 de altura); `main_loop.inc`: corpo vira para o giro da cama (`gSleepYaw`), olho a 0,35 acima do colchao enquanto dorme (3a pessoa e F5 mostram a pose); `survival.inc`: `gHurtWake` marca dano aplicado e `propSleepTick` acorda sem pular a noite.
- Teste: `test_propnet` 62/0 (+3). Sintaxe limpa. (VIS) nada visto: lado da cabeca (assumido -Z do modelo da cama) e altura do corpo.
- Falta: jogadores remotos nao veem a pose deitada (precisa campo novo em NP_STATE + NET_VER); exigir todos dormindo; som de porta; Prop.5d restante (RecordPlayer 2, TimedModel 4, Decobench 1, VariableRendering 51); Prop.6b; splitscreen de props.

## patchProp.15 (Bancada de Artesanato abre a mesa de trabalho)
- Feito: `propUse` trata `Decobench` (deco_508, 1 item): botao direito abre a mesa 3x3 normal (`gPropOpenTable` -> `gTable`/`openTable` em main_loop.inc).
- Teste: `test_propnet` 64/0 (+2); sintaxe limpa. (VIS) nada visto.
- Falta: RecordPlayer (Vitrola deco_108, Jukebox deco_556: o jogo nao tem tocar-disco nem jukebox de bloco; precisa de sistema de disco + audio); TimedModel 4; VariableRendering 51 (so cores/variantes, sem jar nao da para saber); cama deitada para remotos; Prop.6b; splitscreen de props.

## patchTitle.1
- Feito: panorama do titulo com camera baixa (chao+3,5), pitch oscilante 9+-11 graus, FOV 100; estado de shader/F7/dim/luz dinamica congelado durante o panorama (sempre meio-dia, sem agua realista/sombras), shader enxuto com F7; teto ~120 FPS no titulo (anti-lag).
- Teste: compila (sintaxe). (VIS) tudo.
- Falta: terreno identico ao oficial (seed/bioma escolhido a mao), desfoque do panorama, fix por assets se lag persistir.

## patchTitle.2
- Feito: terreno do titulo esculpido (panoCarve: rio/lago largo na camera, colinas com pedra nos barrancos, arvores) sobre a seed fixa; nuvens no panorama; pitch -5..15 graus; camera em SEA+3,5.
- Teste: compila (sintaxe). (VIS) tudo; custo do panoCarve na abertura do titulo (~15 mil colunas via setB).
- Falta: ajustar formas ao print do usuario apos ver no jogo, desfoque, agua realista no titulo nao existe (de proposito).

## patchTitle.3
- Feito: cor de grama/folha/agua do panorama fixa (planicie) via gPanoTint (terrain112.inc), corrigindo o tom azul-esverdeado/escuro do bioma da seed.
- Teste: compila (sintaxe). (VIS)
- Falta: se a agua ainda escurecer, investigar shader de agua; desfoque.

## patchTitle.4
- Feito: a cada abertura do titulo o panorama muda (seed do mundo aleatoria, fase/amplitude/frequencia do rio, ruido das colinas, densidade de arvores, direcao inicial da camera; camera sempre sobre o lago).
- Obs: itens "acordar ao levar dano" e "ShowerHandles->ShowerHead" ja estavam feitos (patchProp.13/14); as linhas Falta antigas estao defasadas.
- Teste: compila (sintaxe). (VIS)
- Falta: desfoque do panorama; ajuste fino das formas.

## patchNether.1
- Feito: fortaleza do Nether (nether.inc: fortCell/fortColumn/fortBlock/netherFortress): celulas 96x96, 40% com fortaleza; grade 3x3 de corredores de 5 (pontes abertas / cobertos por trecho de 16), pilares ate y=28, sala central 15x15 com plataforma (arena dos blazes). Deterministica por coordenada. Tijolo = block/nether_bricks (fallback BRICKS).
- Teste: tests/test_fortress.cpp (13/13). test_nether segfault em [salvar/carregar] ja existia antes (conferido na copia original).
- Falta (VIS): ver no jogo; sem cerca de tijolo, escadas, bau, nether wart, spawner; blazes = patchNether.2.

## patchNether.2
- Feito: Blaze (M_BLAZE=9, mobs.inc/hostile.inc): modelo ModelBlaze (cabeca + 12 varetas orbitando), voa/paira 2,2 acima do alvo, rajada de 3 bolas de fogo (5 de dano + fogo no jogador, 5 s de pausa), imune a fogo, dropa blaze_rod 0-1, ovo, /spawn blaze. NET_VER=30.
- Fortaleza (nether.inc): cerca de tijolo sobre as muretas das pontes, escadas em volta da plataforma, spawner no centro (block/mob_spawner) que gera 1-3 blazes a cada 10-30 s com jogador a <16 blocos (max 6), 4 baus nos cantos da sala com loot (ouro/ferro/silex/linha/diamante).
- Teste: tests/test_fortress.cpp 23/23; test_mobsync 25/25; test_mob_near 8/8.
- Falta (VIS): tudo no jogo (varetas, brilho do blaze fullbright, escala/altura, escadas com meta de lado).
- Falta: nether wart (nao existe no jogo), som do blaze, bolas de fogo nao sincronizadas na rede (convidado nao ve/leva), blaze nao pega fogo ao jogador remoto, spawner sem particulas.

## patchNether.3
- Efeitos visuais do Nether (shader + brasas, opcao d17) desligados por padrao: screens.inc, world.inc. Reativar em Opcoes (d17=1). (VIS)

## patchNether.3 — Nether 1.12.2 + fortaleza por pecas + porco-zumbi
- Feito: terreno = ChunkGeneratorHell (ruido vanilla OctNoise, grade 5x17x5, lava y<32, superficie soul sand/cascalho y60-65, MapGenCavesHell) em `nether112.inc`; decoracao (fogo, glowstone, quartzo, magma) dentro do chunk. Fortaleza = MapGenNetherBridge (1 por regiao 16x16 chunks, 1/3 de chance) + pecas (cruzamento, ponte, fim de ponte, sala, escadas, trono c/ spawner, entrada, corredores, escada, varanda, sala de verruga) em `nether_fort.inc`. Quantidade: `fortress=0..100` em options.cfg ou `/fortaleza N` (33 = vanilla, 0 = nenhuma; vale p/ chunks novos). Textura do porco-zumbi: `entity/zombie_pigman.png` (pack 1.12.2) antes do caminho novo.
- Teste: tests/test_fortress.cpp (26), test_nether, test_portal_net.
- VIS / limites: layouts das pecas reescritos de memoria (nao sao copia exata do vanilla); sem verruga do Nether (bloco inexistente: sala usa soul sand); sala de verruga so tem 1 saida; chunks ja salvos nao mudam.

## patchNether.4
- Feito: porco-zumbi usa o mesmo modelo biped64 do zumbi (bracos/pernas voltaram); efeitos do Nether (shader do ceu + brasas) removidos (gNetherFx sempre false, toggle tirado de Detalhes); verruga do Nether (XB_CROP tipo 4, planta em soul sand com item nether_wart, 3 estagios, drop 2..4, gerada na sala de verruga); nascentes de lava (WorldGenHellLava); decoracao atravessa a borda dos chunks (4 chunks-fonte + cache de terreno vizinho; ~12 ms/chunk); sala de verruga com 2a saida (corredor a esquerda).
- VIS: textura/modelo da verruga (nether_wart_stage_N), porco-zumbi no jogo.
- Falta: cogumelos do Nether (nao existem como bloco no jogo).

## patchFix.1
- Feito: madeiras por especie (abeto/betula/selva/acacia) em arvores e vilas — o pack 1.12 chama log_spruce/planks_acacia, o codigo procurava spruce_log (sempre caia no carvalho); plantacoes: texturas de todos os estagios (pk112 so mapeava wheat_stage_7) e modelo crop do vanilla (y -1..15 = altura da terra arada); fornalha do criativo = a funcional (gFurnId; no pack 1.12 gFurnId era 0 e o menu criava uma copia so com textura).
- VIS: vilas de deserto/savana/taiga com material proprio; plantacoes na altura da terra.
- Falta: vidro (culling/contorno) — aguardando exemplo do que esta errado.

## patchFix.2
- Feito: contorno (selecao) de painel/barras/cerca/muro = UMA caixa (uniao poste+bracos, como o getBoundingBox do MC 1.12; colisao continua com varias caixas); face de cima do painel some quando ha o mesmo painel em cima (cullface do modelo do MC: coluna continua sem fio entre fileiras). Teste: tests/test_pane.cpp.
- VIS: janelas de vidro (painel), contorno ao mirar.
- Vidro em bloco ja cortava faces entre blocos iguais (world.inc: transKind); se ainda houver diferenca com o Java, preciso do print/caso.

## patchFix.3
- Feito: vilas sem agua (peca fora estrada com >30% das colunas <= SEA sai; vila com >25% molhada nao existe — gVilWetPct; antes havia vila 98% oceano perto do spawn na seed 1); arvores nao nascem dentro/perto das pecas da vila (vilTreeBlocked, margem 4, mesma regra nos dois chunks da borda); luz de escadas/lajes POR FACE (vizinho na direcao da normal, como o MC; sem vizinho iluminavel usa o maximo da celula+6 vizinhos).
- Teste: tests/test_village_gen.cpp (86), test_stairs 202/0, test_slabtex 18/0. test_village_conn agora acha 0 cercas (a unica vila das seeds 1-6 era oceano; nao e regressao).
- VIS: iluminacao de escadas/lajes no jogo (nao ha teste de malha com luz); vilas costeiras.

## patchStairs.1 + patchMech.1 + patchTerrain.5
- Feito: contorno de selecao de escada/multi-caixa = contorno do conjunto em L (wireShape, voxeliza 16^3 e desenha so arestas de quina); gelo/gelo compacto .98 e slime .8 de atrito; teia (x.25/.05/.25, sem colisao); alma de areia x.4; cacto machuca 1 de dano; lagos de superficie (world.inc, bacia elipsoidal, fundo de areia).
- Teste: tests/test_mech.cpp (6), test_stairs 202/0, test_village_gen 86/86.
- VIS: contorno das escadas (todas as orientacoes/metades), teia sem colisao, lagos (raros: ~2 por 144 chunks perto da origem).
- Falta: cana-de-acucar (sem modelo de planta), masmorras, minas abandonadas, aumentar a frequencia dos lagos, gelo derretendo.

## patchMob.1
- Feito: knockback do sprint nos mobs reduzido (extra .5 -> .22 horizontal, vertical <= .42); movimento residual < .005 zerado; atrito do chao do mob usa o bloco (gelo escorrega).
- Teste: test_mobsync 25/0, test_mob_near 8/0, test_mech 6/0.
- VIS: sensacao do recuo; "travado ao arrastar" — moveMob ja segue Entity.moveEntity do 1.8.9; se persistir, preciso do caso (qual mob, em que situacao).

## patchMob.2 + patchMech.2
- Feito: mob empurrado/arrastado gira o CORPO para a direcao do movimento (30%/tick, updateDistance do Java) e a cabeca mantem o olhar (hyaw compensado, +-75); terra arada pisoteada ao cair (chance = queda - 0,5).
- Teste: test_mech 8/0 (inclui vaca empurrada contra parede), test_mobsync 25/0, test_mob_near 8/0.
- Do video: o jogador atravessa a vaca presa na parede (empurrao suave de 0,05/tick, igual ao vanilla); nao mudei isso.
- Falta: teia zerar queda; confirmar no jogo (VIS) se o arrasto ficou natural.

## patchVil.1
- Feito: reposicao natural de aldeoes (a cada 30 s, vila ate 96 blocos do jogador com < 3 aldeoes perto ganha 1; teto 3 e filtro de chao em vilSpawnPending); ovo de blaze na aba Diversos (todos os 10 mobs agora tem ovo no criativo; eggMob/mobEggItem ja cobriam todos).
- Teste: test_mech 9/0.
- Limite: centros das vilas (gVilCenters) nao sao salvos; em mundo recarregado so repoem vilas geradas na sessao. VIS: aldeoes reaparecendo.
- Falta: salvar gVilCenters no save; reproducao/portas/camas de aldeao; profissoes.

## patchPane.1 + patchMob.3
- Feito: painel/barras sem conexao = 2 placas finas com textura inteira (noside do 1.12) em redstone.inc; aranha (M_SPIDER=10: modelo, IA noite/neutra de dia, mordida 2, drop fio, ovo, spawn, /summon aranha); NET_VER=31.
- Teste: so sintaxe (compila). (VIS) painel e aranha.
- Falta: pernas da aranha com angulo/animacao, sons, escalar paredes, veneno da cave spider, pane ligado (arms) conferido no jogo.

## patchPortal.2 + patchInv.2
- Feito: portal/fogo achavam `nether_portal.png`/`fire_0.png`; pack 1.12 usa `portal.png`/`fire_layer_N.png` (fallback em netherStrip, nether.inc). Aba Diversos lista todos os ovos `item/*_spawn_egg` do catalogo (inventory_ui.inc).
- Teste: so sintaxe. (VIS) portal roxo animado; ovos na aba. Pode haver ovo duplicado (porco/vaca/ovelha/galinha).
- Falta: spawn real dos mobs sem modelo (so aranha, blaze etc. existem).

## patchInv.3
- Feito: sem ovos duplicados (porco/vaca/galinha/ovelha do pack = mesmos mobs, eggMob); fogo usa fire_layer_0/1 pelo fallback do patchPortal.2.
- Teste: so sintaxe. (VIS) fogo animado (bloco, mob, tela), ovos.
- Falta: modelos/IA para morcego, enderman, lobo etc. (ovos ainda sem efeito).

## patchMob.4
- Feito: husk (zumbi, textura husk, imune ao sol) e aranha da caverna (aranha 70%, sempre hostil) + ovos, drops, /summon husk|cavespider; NET_VER=32. Nao spawnam naturalmente.
- Teste: so sintaxe. (VIS) modelos/escala.
- Falta: stray, enderman, lobo, lula, slime, morcego; veneno da cave spider; spawn natural de husk (deserto).

## patchInv.5
- Causa: pack 1.12 nao tem PNG por mob, entao ovos nao eram itens; gPkByFile so tem block/. Agora asset_manager.inc cria 39 itens `item/<mob>_spawn_egg` (textura gerada) e a aba Diversos varre PK; eggMob/mobEggItem usam mobPkItem (antes pkFind = 0 para item/).
- Teste: so sintaxe. (VIS) ovos coloridos na aba Diversos; mobs sem modelo nao spawnam nada.
- Falta: modelos/IA dos mobs restantes.

## patchVil.1
- Causa provavel: escada da vila na borda do chunk calculava o canto sem o vizinho (chunk ainda inexistente) e nao remalhava. Agora village_build.inc registra em gVilEdge e vilEdgeTick (blockupdate.inc) remalha escada + vizinhos. Logica de canto ja passa em test_stairs (202/0).
- Teste: sintaxe + test_stairs. (VIS) dobras do telhado nas vilas; se persistir dentro do chunk, a causa e outra (mandar print + coordenada).
- Falta: luz de escadas/lajes (sintoma nao detalhado).

## patchTnt.1 + patchBio.1
- TNT/creeper: explosao usa blockDrops (tabela real, meta de blocos XB), ignora liquido/fogo/portal, chance 1/forca (hostile.inc). Antes dropOf devolvia o proprio id (lava, portal, variantes) = item estranho.
- Bioma: cor de grama/folha usa id vanilla 1.12 (biomeRawAt) com temperatura/chuva reais e colormap[t][chuva*t]; mesa e pantano com cor propria; sem ruido de clima proprio; amplificacao 1.8 -> 1.5 (terrain112.inc).
- Teste: test_tnt 17/0. (VIS) cores por bioma; o fator 1.5 pode precisar de ajuste com os shaders.
- Falta: mistura de bioma em raio maior (MC), grama de roofed forest, luz de escadas/lajes (aguardando sintoma).

## patchSombra.1 + patchTnt.2 + patchZoom.1
- Sombra: entBlobShadow (player_render.inc) = renderShadow do 1.8.9 (quad preto com textura circular gerada, so em blocos cubo cheio, alpha = (1 - dist^2/256 - altura/2)*0.5). Jogador, mobs e jogadores remotos. So com F7 (shader) DESLIGADO.
- TNT/creeper: HBOOMS (mobs.inc) = EntityHugeExplodeFX: 6 nuvens "largeexplode" por tick durante 8 ticks (Puff.kind 2, cinza escurecendo). Desenho procedural (sem explosion.png).
- Zoom: segurar C = FOV/4 suave, sensibilidade do mouse proporcional (gZoom). SDL_SCANCODE_C nos stubs.
- Teste: sintaxe + test_tnt 17/0. (VIS) sombra, nuvem da explosao, zoom.
- Falta: partículas da explosão nao aparecem em convidados (so o host roda explosionAt; precisa pacote, NET_VER); zoom no celular/gamepad; roda do mouse ajustando zoom; mais mobs.

## patchPerf.1 + patchMob.1
- Perf: causa do mundo lento/travando = terrain112.inc: gBioRawSeed era `int` e a seed e 64 bits; a comparacao nunca batia e o cache de bioma (gBioRaw) era LIMPO a cada biomeRawAt (~66 us, 9x por canto x 289 cantos por chunk = ~250 ms/chunk no teste, ~900 ms no PC do log). Agora int64_t. Medido em bench: build() 250 ms -> 4 ms por chunk; biomeTints 289 chamadas 224 ms -> 0,1 ms. Deve resolver tambem o panorama do menu (6,5 s) e o "Preparing spawn area" (26 s).
- Mob: vaca-cogumelo (M_MOOSH): partes/IA/drops/sons da vaca, textura entity/cow/mooshroom.png, ovo item/mooshroom_spawn_egg, /summon mooshroom|vacacogumelo, balde de leite funciona.
- Teste: test_mooshroom 4/0, test_mobsync 25/0, sintaxe. test_biomes ja nao compilava antes (GC_PLAINS/gBioFrom).
- Falta: cogumelos vermelhos nas costas do mooshroom; spawn natural so em ilha de cogumelo; (VIS) mooshroom no jogo; tosquiar/sopa de cogumelo.

## patchTnt.3 + patchZoom.2
- Rede: NP_EXPL (host -> convidados: dim + x,y,z da explosao); convidado so desenha a nuvem (HBOOMS). NET_VER 32 -> 33 (tambem cobre o mob M_MOOSH = tipo 13).
- Zoom: segurar C + roda do mouse ajusta o zoom (gZoomTgt .05..1, padrao .25).
- Teste: sintaxe, test_tnt 17/0, test_conn 33/0, test_mooshroom 4/0. (VIS) nuvem em convidado (2 instancias).
- Falta: zoom no celular/gamepad; cogumelos nas costas do mooshroom; spawn natural em ilha de cogumelo; mais mobs.

## patchBio.2
- Mistura de bioma: terrain112.inc biomeGrid = cor de grama/folha por bloco -> gaussiana separavel (raio 8, sigma 4) uma vez por chunk (grade 17x17, cache gBTCache, limpo ao trocar de seed). Chao (world.inc biomeTintsChunk) e plantas (biomeTints bilinear) leem a MESMA grade. Antes: media 3x3 espacada de 3 blocos (borda dura). Mantidos: amplificacao 1.5x, mesa, pantano.
- Flores: caule/folha verde viram tile separado (verde = g > r+12 e g >= b+10), tingido pelo bioma como a grama; petalas sem tinta (models.inc, 2 tiles extras por flor). Vale para flores e plantas duplas (girassol, lilas, roseira, peonia).
- Teste: sintaxe, test_mooshroom 6/0 (inclui grade suave), build ~4 ms/chunk (sem perda). (VIS) borda de bioma e caule das flores.
- Falta: cor da AGUA por bioma ainda usa 3x3 (biomeWaterTint); se o verde-escuro do print for sombra de nuvem e nao bioma, mandar outro print sem F7.

## patchEnd.1
- Feito: sombra (entBlobShadow) nao aparece em 1a pessoa (camMode 0) e tem borda mais nitida; ovo do dragao (`block/dragon_egg`) cai como areia e teleporta ao clique (eggTeleport, falling.inc); Ender Dragon (src/dragon.inc): voa em circulo no End, mergulha no jogador (10 de dano), 200 HP, cabeca dano total/corpo 1/4+1, morre em 10 s e surge o ovo sobre pilar de bedrock em (0,topo,0); flag em saves/<mundo>/dragon.dat.
- Teste: so sintaxe (compila). (VIS) tudo.
- Falta: sincronia de rede do dragao, flechas/explosoes ferindo o dragao, cristais do End, olhos/respiracao (dragon_eyes/fireball), portal de saida, XP, barra de vida.

## patchNet (multiplayer em 3 passadas; NET_VER 33 -> 34)
- Passada 1 (patchNet.1): raio sincronizado: so o host sorteia (jogador aleatorio no mundo), manda NP_BOLT, convidado desenha e leva o dano local (weather.inc). Ender Dragon na rede (dragon.inc): host manda NP_DRAGON todo tick (pos/yaw/asa/vida/morte), convidado so desenha; golpe de convidado = NP_DRAGONHIT (host aplica, max 40); alvo = jogador mais proximo no End; mordida em convidado = NP_DRAGONBITE (a vitima aplica dano 10 + recuo).
- Passada 2 (patchNet.2): bolas de fogo do blaze: host manda NP_FBALL ao disparar; convidado simula a sua e leva dano/fogo sozinho (resolve "blaze nao pega fogo ao jogador remoto"). Pose deitada (cama) visivel nos remotos: byte sit do NP_STATE virou flags (bit0 sentado, bit1 dormindo) + yaw; gSleepT/gSleepYaw movidos para net.inc.
- Passada 3 (patchNet.3): armadura (4 ids uint16, incl. de mod) enviada no NP_STATE e desenhada no jogador remoto. Operador convidado ja mudava o clima (patchWx.6, conferido).
- Teste: test_netsync (novo, 10/0: receptores de dragao, raio e bola de fogo); regressao test_conn 25/0, test_propnet 33/0, test_mobsync 64/0. Sintaxe ok.
- Limites (VIS): nada visto rodando com 2 clientes. Dragao so simula com o HOST no End (se so o convidado estiver no End ele nao se mexe); bolas de fogo do host nao atingem convidados (cada um simula a copia); UDP sem reenvio nas novas mensagens (perda = 1 quadro, o proximo tick corrige; NP_BOLT/NP_FBALL perdidos nao repetem).
- Falta: exigir todos dormindo para pular a noite; raio em mobs/creeper carregado; flechas/explosoes no dragao; splitscreen de props; efeitos de overlay de armadura de mod.

## patchEnd.1 (portal do End visivel)
- Feito: tiles do portal do End alocados ANTES do portal do Nether/fogo e sem exigir 64 livres (com atlas cheio o portal sumia); sem tiles livres reaproveita um (visivel, sem paralaxe); topo das camadas em 12/16 (.75); animacao so com 4 tiles distintos.
- Teste: tests/test_endportal.cpp (8 checks: tiles, malha, abrir com 12 olhos, entrar -> End).
- Limites (VIS): aparencia no jogo; falta confirmar com o pack real.
- Falta: nada alem do VIS; fila segue em Prop.5d.

## patchStronghold.1 (stronghold 1.12.2, src/stronghold.inc)
- Feito: MapGenStronghold.generatePositions (128 em aneis, mesmo Random do Java); layout StructureStrongholdPieces com java.util.Random (pesos/maximos, Library depth>4, PortalRoom depth>5, Corridor de ligacao, filhos X/Z/normal, pendentes aleatorios, markAvailableHeight, repete ate ter sala do portal; seed por estrutura como MapGenBase). Construcao: Straight, Corridor, StairsStraight, Stairs/Start, Left/Right, Crossing, ChestCorridor, PortalRoom completa (12 molduras com 10% de olho, lava, escada, spawner, portal se os 12 tem olho). Gerada antes do populate.
- Teste: tests/test_stronghold.cpp (aneis, 12 estruturas sem sobreposicao exceto Corridor, sala do portal no mundo). test_nether ja segfaulta em "salvar/carregar" no original.
- Limites: sem ajuste ao bioma nas posicoes (findBiomePosition); aleatoriedade dos blocos por hash da celula, nao pelo Random de populacao; portas de madeira/ferro e grades viram vao/moldura (sem porta/barras); bau do ChestCorridor vazio; spawner sem mob de traca. (VIS)
- Falta: PrisonHall (celas), RoomCrossing (5 tipos), Library (estantes), mobilia da Crossing (lajes aproximadas), portas/barras/botao, loot, silverfish (monster egg e spawner), ajuste ao bioma.

## patchEnd.1 + patchSombra.2
- Feito: moldura do End: busca tolerante do id (block/end_portal_frame[_top|_side|prefixo]) -> caixa 13/16, topo/lados cortados, clique direito com Olho do Ender encaixa e abre o portal; textura do olho com fallback. Sombra de jogador/mobs: disco 16x16 NEAREST alinhado a grade, 2 niveis de alfa, nao some ao pular (alcance 12 blocos, clareia devagar).
- Teste: so sintaxe. (VIS) tudo.
- Falta: se a moldura continuar cubo cheio, falta a textura end_portal_frame* no pack (nao existe no Default-Java-1.12.2.zip enviado).

## patchEnd.3
- Feito: barra de vida do Ender Dragon no topo (dragonBar em dragon.inc, chamada em drawHud); XP ao matar (12000 na 1a vez, 500 depois).
- Teste: sintaxe + test_netsync 10/0. (VIS) barra e XP.
- Falta: cristais do End (cura do dragao), flechas/explosoes ferindo o dragao, olhos/respiracao, sincronia de rede completa.

## patchEnd.4
- Feito: texturas reais do 1.12 se chamam endframe_top/side/eye (ja estao no pack); busca do id da moldura agora tenta block/endframe*; olho = endframe_eye. Pack original restaurado (as texturas falsas do patchEnd.2 saem).
- Teste: sintaxe. (VIS)

## patchEnd.5
- Feito: cristais do End (dragon.inc): 10, sobre o fogo/bedrock dos pilares; vidro+nucleo girando (textura endercrystal.png); curam o dragao 1 HP/8 ticks; clique/golpe destroi (remove o fogo, fere o dragao se perto).
- Teste: sintaxe + test_netsync 10/0. (VIS) tudo.
- Falta: raio de cura visual (endercrystal_beam), explosao real, flechas, rede (cristais so locais), base de bedrock sob o cristal.

## patchEnd.6
- Feito: raio de cura (endercrystal_beam.png, fita cruzada cristal->dragao, rola em v); explosao real de forca 6 ao destruir o cristal (via crystalsTick(P) -> explosionAt).
- Teste: sintaxe + test_netsync 10/0. (VIS)
- Falta: flechas em cristal/dragao, rede dos cristais, olhos/respiracao do dragao, base de bedrock sob o cristal.

## patchEnd.7
- Feito: flechas do jogador acertam cristais (destroi) e dragao (cabeca dano cheio, corpo/4+1) via arrowHitEnd (dragon.inc), gancho em bow.inc arrowStep.
- Teste: sintaxe + test_netsync 10/0 + test_arrow_pick. (VIS)
- Falta: olhos do dragao (dragon_eyes) e respiracao; rede dos cristais; base de bedrock sob o cristal.

## patchEnd.8
- Feito: cristais do End no convidado: crystalsTick roda antes do retorno do convidado em dragonTick; cristal morre quando o bloco de fogo some (sincronizado por netSendBlock). Sem pacote novo, NET_VER igual.
- Teste: sintaxe + test_netsync 10/0. (VIS) com 2 clientes.
- Falta: cura do dragao so no host (certo); dano do cristal ao dragao quando o convidado destroi; olhos/respiracao; base de bedrock.

## patchEnd.9
- Feito: BUG corrigido: crystalsTick/crystalsTick usavam getC com coordenadas de bloco (getC e por chunk) => cristais nunca apareciam; agora cystChunk(). tests/test_crystal.cpp novo (9/0): aparece sobre o fogo, cura, flecha mata, fogo apagado mata, golpe mata, sai do End zera.
- Base de bedrock ja existe (fogo fica sobre bedrock no endGen).
- Falta: olhos/respiracao do dragao; dano ao dragao quando convidado destroi; grades de ferro dos 3 pilares maiores.

## patchEnd.10
- Feito: gaiola de barras de ferro (5x5, 4 de altura, teto) nos 3 pilares guardados (ord<3, raio 2) em endGen (nether.inc); tests/test_endcage.cpp novo (6/0): 10 fogos, 3 gaiolas, barras conectadas, 10 cristais.
- Falta: olhos/respiracao do dragao; dano ao dragao quando convidado destroi cristal; melee atravessa as grades (vanilla bloqueia).

## patchEnd.11
- Feito: golpe no cristal bloqueado por grade/bloco no caminho (raySolidDistance em crystalHit); olhos do dragao: drawDragon = 2 passadas (modelo + dragon_eyes.png aditivo, sem escrever profundidade). test_crystal 10/0, test_netsync 10/0.
- (VIS) olhos alinhados ao modelo (mesmos UV do dragon.png).
- Falta: respiracao/bola de fogo do dragao (dragon_fireball); dano 10 HP quando convidado destroi cristal.

## patchBox.1
Feito: cama (UV das pontas, icone claro), shulker (base ate y8), end rod (modelo embutido), painel de vidro (cruz com espessura, poste so nos lados sem braco), slime no criativo (REDSTONE), girassol (icone = flor). Teste: sintaxe + test_pane. VIS: tudo. Falta: cama na mao, variacao de posicao das madeiras.

## patchBox.2
Feito: troncos de todas as madeiras com eixo X/Y/Z como o carvalho (gBlkAxis/gPkLog usavam nome bruto do arquivo; agora pk112). Teste: sintaxe. VIS: abeto/bétula/selva/acácia/carvalho-escuro deitam ao clicar na lateral. Falta: cama na mao.

## patchBox.3
Feito: icones de entidade (cama/shulker/bau) com fundo na cor media dos texels opacos da face (nao preto, nao transparente). Teste: sintaxe. VIS: icones de shulker sem preto, camas opacas.

## patchOpt.1
Feito: Graficos Rapidos|Bonitos (sem Fantasticos), Interface=escala GUI, FPS max, Brilho, view bob, Nuvens OFF, Shaders (sombras/qualidade) salvos em options.cfg; sombra blob 128x128 suave; blocos em queda com sombreamento por face; shulker guarda 27 slots (chestStores). Teste: sintaxe, test_chest 42/0, test_netsync 10/0. VIS tudo.
Falta: icone do sinalizador (rsIconQuads), icones nitidos, agua empurra itens, animacao da agua, submenus Qualidade/Detalhes/Desempenho/Animacoes/Diversas.

## patchOpt.2
Feito: agua empurra itens soltos (flowFull*0,014 em drops.inc); agua animada (frames do pack, waterAnimTick em player_skin.inc, atlas niveis 0-4, respeita Animacoes>Agua animada); sinalizador: item do pack deixa de ser cubo (icone/mao/chao usam o modelo); icones com filtro LINEAR no nivel 0 (sem mipmap borrado). Teste: sintaxe, test_dropflow 3/0 (novo), test_chest 42/0. VIS tudo.
Falta: submenus Qualidade/Detalhes/Desempenho/Animacoes/Diversas (so ligados a Nuvens/FPS/Brilho/Interface/Graficos); agua animada so no backend OpenGL; shulker mantem conteudo no item ao quebrar.

## patchAud.1
Feito: Musica & Sons: botao de dispositivo (cicla padrao/dispositivos SDL, reabre o audio), Todos os sons/Mudo (gSndMuted no mixer), Atualizar sons (limpa cache e recarrega events.cfg); mudo e dispositivo salvos em options.cfg (sndmute/snddev). Sliders ja valiam via events.cfg cat=. Teste: sintaxe, test_chest 42/0. VIS/ouvido.
Falta: categorias 2 (blocos musicais), 3 (clima), 8 (ambiente) so valem se o evento tiver cat= no events.cfg; volume da musica do menu; sons de mobs alem de galinha/vaca/creeper/porco/esqueleto/zumbi.

## patchFix.1 (nuvens, cama, estandarte, slime, sombra, escada)
Feito: nuvens usam a mascara real de environment/clouds.png (mining_sky.inc, antes ruido inventado); estandarte: arquivo certo entity/banner_base.png (entmodels.inc; antes caia no cubo de la); slime: miolo verde opaco atras das faces translucidas no icone (inventory_ui.inc); cama: icone do inventario usa o modelo 3D 64x64 (antes sprite 16x16 sintetizado = pixelado); escada de carvalho escuro: UV do icone recuado meio texel (borda branca vinha do tile vizinho); sombra: PCF das entidades 2,3x mais estreito, cascata E0 2048 e E1 1536/16 blocos, ambas atualizam todo quadro (sky_cycle.inc, shadows.inc).
Teste: compila (sintaxe); test_bucket_pack passa. Tudo (VIS).
Falta (nesta ordem, pedido do usuario): patchSign.1 texto/edicao de placas (nao existe no codigo); patchRail.2 trilhos/carrinhos funcionando; patchBucket.4 conferir baldes no jogo (logica ja existe: test_liquid) - precisa sintoma; slime como bloco no MUNDO (se ainda parecer plano, diga onde).

## patchBucket.4
Feito: causa dos baldes sem logica = o pack 1.12 registra item/bucket_empty|water|lava|milk (nome cru) e pkblocks.inc procurava item/bucket, item/water_bucket... => ids 0. Agora aceita os dois nomes. Teste: test_bucket_pack (3), test_liquid. (VIS)
Falta: patchSign.1 (texto de placas), patchRail.2 (trilhos/carrinhos). Conferir outros itens com nome cru 1.12 (flint_and_steel ok).

## patchRail.2
Feito: trilhos/carrinhos nao eram posicionaveis porque o pack 1.12 registra block/rail_normal|golden|detector|activator e item/minecart_normal|chest|furnace (nome cru) e minecart.inc comparava com os nomes novos. Agora usa pkCanon (railKindOfItem, cartTypeOfItem, cartItemId, drop do trilho). Teste: test_minecart (48, +2 casos de nome cru). (VIS)

## patchSign.1
Feito: sign.inc novo: texto de 4 linhas nas placas. Editor abre ao colocar a placa e ao clicar com o direito (Sneak ignora); Enter proxima linha, setas, Backspace (UTF-8), Esc conclui; limite 90 unidades; texto 3D preto na tabua (1/96 bloco, mesma rotacao do modelo; em pe e na parede); poda ao quebrar; salvar saves/<mundo>/signs.dat. Ganchos: main.cpp, menu.inc (save), main_loop.inc (load, teclado, TEXTINPUT, uiMode, UI, desenho, clique). Teste: test_sign (11). (VIS)
Falta: rede (texto da placa nao vai aos outros jogadores; novo pacote + NET_VER); orientacao do texto na parede e brilho/cor por cor de tinta; placa colocada por comando/mod nao tem texto; splitscreen.

## patchSign.2
Feito: texto das placas na rede. NP_SIGN (x,y,z + 4 linhas); enviado ao concluir a edicao (vazio = remove); host repassa (repasse generico); convidado novo recebe todas por signNetSyncPeer. NET_VER 35. Teste: test_sign (13), test_propnet (64) sem regressao. (VIS)
Falta: reenvio de UDP perdido (como props); cor de tinta/brilho; splitscreen (jogador 2 nao edita placa).

## patchDoc.1 (organizacao das pendencias e do MEGA_PROMPT)
- Feito: `PENDENCIAS.md` (raiz): nota solta do topo virou item de PRIORIDADE (mods fora do Decocraft: grade, props indevidos, itens inutilizaveis); nova secao 6 com a lista de desejos (multiplayer, PvP, mobs, End, blocos, chat/UI/som, Android). MEGA_PROMPT reescrito: NET_VER 35, feitos removidos (Prop.1-15, Prop.8 reenvio UDP, instalador).
- Falta: reconferir cada item da secao 6 com grep (varios tem historico parcial: afogamento patch5.76, perola patch5.62, Tab patch 3E, placas Sign.1/2, alcapao Red.1/Mod.9).

## patchFix.4 (textura do alcapao de ferro)
- Feito: `redstone.inc` (caso 13) pedia `block/iron_trapdoor_face`, nome que NAO existe no pack 1.12.2 (o arquivo e `blocks/iron_trapdoor.png`); agora pede `block/iron_trapdoor`. Alias antigo em `mods.inc` (CA) mantido.
- Teste: sintaxe do projeto limpa; `test_irontrap` e `test_update` rodados abaixo. (VIS) conferir a textura do alcapao de ferro posto e aberto.
- Falta: se o item continuar "so textura" no inventario, o problema e o item->bloco em `pkblocks.inc`/`placement.inc` (nao reproduzido nos testes).


## patchAud.OGG (jogo passa a usar .ogg; TNT e mobs com som)
- Feito: `src/vorbis.inc` (decodificador Ogg Vorbis I proprio, sem libs; bate com o ffmpeg em +-1 LSB em mono/estereo/musica). `sndGet`/`sndFileOk` (audio.inc) leem `.ogg` primeiro e `.wav` so como reserva; sons do pacote 1.12.2 (1085 .ogg) colocados em `assets/sounds/` no lugar dos .wav (apagados).
- Feito: TNT: `tnt.fuse` (random/fuse ao acender, host e convidado) e `explode` (random/explode1-4, vol 4 = ouvido de longe) em `events.cfg`; tocado em `explosionAt` (hostile.inc: TNT, creeper, carrinho) e no convidado em `NP_EXPL` (net.inc; NET_VER nao mudou).
- Feito: mobs: tabela `mobSndFor` (audio.inc) com os 14 mobs do jogo (porco, galinha, vaca, ovelha, aldeao, pigman, zumbi, esqueleto, creeper, blaze, aranha, husk, aranha da caverna, vaca-cogumelo) com os nomes reais do 1.12.2 (say/hurt/death).
- Teste: `test_audio` (+ pares .ogg/.raw do ffmpeg, rodar da raiz do jogo), `test_tnt`, `test_conn`, `test_propnet` passam. Musica ogg (3 min) leva ~1 s para decodificar ao iniciar (VIS: conferir engasgo).
- Falta (audio): passos/quebrar/colocar por material de bloco (hoje so pedra/grama), porta, fogo, agua, lava, botao do menu, bau/fornalha/poçao, dano do jogador, comer/beber, flechas, trovao/chuva, sons ambiente (caverna), discos de musica (`records/`), mobs ainda sem tabela (lobo, gato, morcego, ghast, golem, slime, enderman...) quando existirem.

## patchFix.5 (slime so em Redstone; grama mais rapida)
- Feito: bloco de slime saiu da aba Decoracao e fica so em Redstone (`assets/creative/mc-1.12.2.cfg` + `tools/gen_creative_1122.py`).
- Feito: dureza do bloco de grama 0,6 -> 0,3 (`blockHardness`, `mining_sky.inc`). Assumido "grama = bloco de grama"; mato (high grass) nao mexido.
- Teste: sintaxe limpa; `test_mech` passa. (VIS) conferir abas do criativo e o tempo de quebrar a grama.
- Nota: no 1.12.2 original o slime fica em Decoracao; mantido Redstone a pedido do usuario.

## patchFix.6 (mato)
- Feito: mato/flores/mudas/arbusto seco (pack) com dureza 0 e qualquer ferramenta (`pkblocks.inc`); bloco de grama voltou a 0,6 (desfaz o ajuste do patchFix.5). (VIS) conferir quebra do mato.

## patchTouch.1 — pick block Android momentâneo
- `src/touch.inc` (`touchFrame`): botões laterais pick/trocar/soltar (TA_PICK/TA_SWAP/TA_DROP) soltam sozinhos após 160 ms; uma ação por toque, sem estado preso mesmo se o FINGERUP se perder. (VIS) conferir no Android.
- AINDA FALTA: fila 2 (reposicionar botões, `src/android.inc`/`touch.inc`), 3 (mods individuais), 4 (alçapão de ferro), 5 (junkbox), 6 (TNT na água), 7 (mods não-Decocraft).

## patchTouch.2 — joystick Android maior e afastado da borda
- `src/touch.inc` (`touchLay`): base do joystick 76–104 un. (era 64–88), dica a 24 un. da borda esquerda e 16 do fundo, zona de toque 42% x 75% da tela. Ações já ficam agrupadas à direita; chat segue no topo. (VIS) sem print: ajustar números conforme o aparelho.
- AINDA FALTA: filas 3, 4, 5, 6, 7.

## patchMod.9 — desligar mods individualmente
- `src/mods.inc` (`gModsOff`, `modIsOff`, `modDirNames`, filtro em `loadMods`), `src/props.inc` (filtro em `propsLoadAll`), `src/screens.inc` (Mod List agora lista as pastas reais de `mods/`; clique liga/desliga; `optSave` grava `modsoff=` em options.cfg). Mod desligado não registra blocos/itens/props nem aparece no inventário; vale ao reiniciar.
- Testado: sintaxe + parse de `modsoff=` isolado. (VIS) tela Mod List (tecla M no título) e inventário sem o mod.
- Atenção: mundos salvos com props de mod desligado perdem esses props até religar; ids de prop dependem da ordem dos pacotes (patchMod.8).
- AINDA FALTA: filas 4, 5, 6, 7.

## patchTrap.1 — itens soltos de alçapão agora colocam
- Causa: o pack traz `block/iron_trapdoor` e `block/trapdoor` como itens soltos (aba Resource pack/busca) sem ligação ao bloco: textura no inventário mas nada ao clicar. Os itens do criativo (aba Redstone) já funcionavam. Correção em `rsMapItems` (`src/redstone.inc`): ferro = var 0, madeira = var 1 (XB_TRAP). Teste: `tests/test_trapitem.cpp` (rodar da raiz, com o pack).
- AINDA FALTA: filas 5 (Junkbox), 6 (TNT na água), 7 (mods não-Decocraft).

## patchTnt.1 — TNT acesa arrastada por água/lava
- `tntLiquidPush` (`src/falling.inc`), chamada em `tntTick`: soma o fluxo (`flowVector`) das células tocadas e empurra 0,014 (água) / 0,007 (lava). Bloco de TNT parado não anda. Teste: `tests/test_tntwater.cpp`. (VIS: velocidade da correnteza; MP convidado segue só recebendo a posição do host.)
- AINDA FALTA: fila 5 (Junkbox, confirmar Barrel x Junk Box Decocraft), fila 7 (mods não-Decocraft).

## patchJuke.1 — toca-discos do 1.12.2
- `src/jukebox.inc` (novo; incluído em `main.cpp`; `jukeInit` em `falling.inc`; clique direito em `main_loop.inc`; `jukeTick` junto de `sndTick`). Bloco = `block/jukebox` ou `block/jukebox_side` do pack; disco dentro = metadado 1..12 (sincroniza/salva como bloco). Disco na mão insere e toca `assets/sounds/records/<nome>.ogg`; clique de novo ejeta; quebrar solta o disco. Teste: `tests/test_juke.cpp`.
- **O zip enviado NÃO tem `assets/sounds/records/*.ogg`** (11, 13, blocks, cat, chirp, far, mall, mellohi, stal, strad, wait, ward): sem eles o disco entra/sai mudo. Copiar do 1.12.2 (`assets/minecraft/sounds/records/`).
- (VIS) o bloco `jukebox_side` do pack é colocável/clicável? Convidado em MP não ouve a música (só o meta sincroniza); decodificar disco leva ~1 s. Disco não toca de novo ao recarregar o mundo (ejeta/reinserir toca).
- AINDA FALTA: Junkbox (5), mods não-Decocraft (7), música parar/pausar a de fundo, redstone (comparador), MP som.

## patchJuke.2 — música de fundo cala + toca-discos em rede
- `gJukeBusy` (`audio.inc`): enquanto o disco toca, a música de fundo para e não recomeça. `jukeNetEdit` (`jukebox.inc`, chamada em `NP_BLOCKS` de `net.inc`): quando outro jogador insere/ejeta, o disco toca/para aqui também (sem duplicar o item). Formato de rede igual: `NET_VER` não mudou.
- (VIS) MP: convidado ouve o disco do host e vice-versa; música de fundo volta depois do disco.
- AINDA FALTA: `records/*.ogg` (não vieram no zip), Junkbox (5; nada no código, preciso saber Barrel x Junk Box Decocraft), mods não-Decocraft (7).

## patchMus.1 — mais músicas + discos (assets)
- `sndMusicList`/`sndMusicStart` (`audio.inc`): playlist por contexto, sorteio sem repetir a última, pula faixa que não existe. Superfície: calm1-3, hal1-4, nuance1-2, piano1-3; criativo: + creative1-6; Nether: nether1-4; End: end/end. `gMusicCtx` ajustado em `main_loop.inc` (troca de dimensão/modo troca a playlist). `/sound play N` toca a N-ésima. Assets: `assets/sounds/music/game/**`, `music/menu/menu1-4` (ainda sem uso), `assets/sounds/records/*.ogg` (12 discos do toca-discos).
- Teste: `tests/test_musdec.cpp` (raiz do jogo; 10 faixas ok). (VIS) decodificar leva 1-5 s (end/end ~5 s, ~160 MB de PCM) = engasgo possível; faixa do End `boss`/`credits` e música do menu (`music/menu`) ainda não ligadas.
- AINDA FALTA: música do menu, boss do dragão, Junkbox (decidir), mods não-Decocraft (7), resto do áudio (§3 do prompt).

## patchAud.2 — sons por material, dano e comer
- `sndMatOf` (`drops.inc`): pedra/grama/cascalho/areia/madeira/lã/neve/vidro (blocos base + nomes do pack). `SND_BRK/PLC/STP` (`audio.inc`) ligam material -> evento. Passo agora pelo bloco sob os pés (`gSndStepMat`, ajustado em `main_loop.inc`). Quebrar/colocar usam `dig/*` (vidro: `random/glass#`). `player.hurt` / `player.fall.small|big` em `survDamage` (`survival.inc`); mordidas `eat` a cada 4 ticks e `burp` ao terminar (`eatTick`, `bow.inc`).
- `assets/sounds/events.cfg` atualizado (e o texto padrão embutido em `audio.inc`). **Substituir o events.cfg do jogo por este.** Teste: `tests/test_audmat.cpp` (raiz, com `assets/sounds` completo).
- (VIS) volume dos passos (0,35) e dano; passo de escada/água/agachar ainda sem som próprio.
- AINDA FALTA (áudio): porta, baú, fogo, água/lava/nadar, botões do menu, poção/beber, flechas (hit), chuva/trovão, caverna, forja/anvil, XP orb, mobs novos. Depois: música do menu, Junkbox (decidir), mods não-Decocraft (7).

## patchAud.3 — porta, baú, botão/alavanca, isqueiro, fogo, lava, respingo
- `events.cfg` (+ texto embutido): `door.open/close`, `chest.open/close`, `button.click(.wood)`, `lever.click`, `fire.ignite`, `fire.ambient`, `lava.ambient`, `lava.pop`, `splash`. Ganchos: `rsUse` (porta/alçapão/portão/botão/repetidor, `redstone.inc`), `xbToggleLever` (`models.inc`), `chestOpenAt`/`chestClosed` (`chest.inc`), `flintUse` (`nether.inc`), `sndWorldAmbient` (`jukebox.inc`, chamada em `main_loop.inc`: fogo/lava por perto a cada 0,5 s + respingo ao cair na água). `test_audmat` agora checa 20 eventos.
- (VIS) volumes; som de porta/baú de OUTRO jogador em MP não toca (só quem age); porta aberta por redstone sem som.
- AINDA FALTA (áudio): botões do menu, beber poção, flecha acertando, chuva/trovão, caverna, forja/bigorna, orbe de XP, nadar, passo na escada/água, mobs novos. Depois: música do menu, Junkbox (decidir), mods não-Decocraft (7).

## patchWx.8 — chuva nova + sons de chuva e trovão
- `wxDraw` (`weather.inc`): gotas = faixas voltadas para a câmera com cauda que some (gradiente de alpha), inclinadas pelo vento (mais forte na tempestade), 3 gotas por coluna num raio de 10 (antes 2 em 8), fade por distância, respingo em anel no chão/água. Neve: flocos de tamanho variável e movimento em 2 eixos.
- Som: `wxRainSound` toca `rain1-8` em sequência (emenda 0,25 s antes do fim), volume pela intensidade, 30 % sob teto, mudo em bioma frio (neve). Trovão: `wxBoltStrike` agenda `weather.thunder` (`thunder1-3`) atrasado por distância (até 2,5 s) e mais baixo longe. Eventos `weather.rain`/`weather.thunder` no `events.cfg` e no padrão embutido. Teste: `tests/test_rain.cpp`.
- (VIS) aparência da chuva (largura/alpha das faixas, densidade, FPS com ~1300 gotas); volume da chuva; respingo na água. Sem sons de chuva batendo em telhado/folhas por bloco.
- AINDA FALTA (áudio): caverna (`ambient/cave`), botões do menu, poção, flecha, forja/bigorna, XP, nadar, escada, mobs novos. Depois: música do menu, Junkbox (decidir), mods não-Decocraft (7).

## patchLogic.1 + Aud.4 + Mob.9 + Suf.1
- **Lâmpada de redstone** e **bloco de notas** (`src/logic.inc`, ganchos `blockHooksSet`): lâmpada acende/apaga com `rsBlockPowered` (host/solo decidem, convidado recebe o bloco), lit solta a apagada (`blockDrops`); bloco de notas: clique direito troca a nota 0–24 e toca, energia toca uma vez; instrumento pelo bloco de baixo (madeira baixo, areia/cascalho caixa, vidro chimbal, pedra bumbo, resto harpa). Teste: `tests/test_logic.cpp`.
- **Passos correndo+pulando:** `sndTick` acumula distância também no ar e toca 1 passo ao pousar (`gSndOnG`); antes só contava com o pé no chão.
- **Mobs na água:** `wetT` (0,75 s) impede o sol de reacender zumbi/esqueleto que pula na água (`mobs.inc`, `hostile.inc`). Lava/fogo continuam queimando. (Interpretação: queimar na água era o bug.)
- **Sufocar na parede:** 8 pontos em torno dos olhos como o Java (`survival.inc`); encostar o rosto numa quina já dá dano.
- (VIS) lâmpada acesa dá luz e troca de textura; nota/instrumento; passos correndo; mob no rio de dia. Lâmpada apaga na hora (Java: 4 ticks de atraso).
- AINDA FALTA (lógica): dropper/dispenser, relógio/clock, observer, comparador, hopper/funil. Áudio: caverna, menu, poção, flecha, forja, XP, nadar. Depois: música do menu, Junkbox (decidir), mods não-Decocraft (7).

## patchTrap.2 + Aud.5 — alçapão/porta sobre o jogador, caverna
- `unstuckPlayer` (`physics.inc`) só empurra o jogador para fora de **cubo cheio** (como `pushOutOfBlocks` do 1.12.2): alçapão/porta/laje/escada/cerca abertos ou fechados em cima de você não te movem; a colisão já deixa andar livre (port do `calcOffset`). Teste: `tests/test_unstuck.cpp`. (VIS) fechar alçapão/porta em você; sair andando. Vidro conta como cubo cheio (no Java não empurra). Mobs ainda usam outra rotina.
- Caverna: `ambient.cave` (`events.cfg` + embutido) a cada 20–60 s abaixo de y 56 sem céu aberto (`sndWorldAmbient`, `jukebox.inc`).
- AINDA FALTA: dropper/dispenser, relógio, observer, comparador, funil; áudio: botões do menu, poção, flecha, forja, XP; música do menu; Junkbox (decidir); mods não-Decocraft (7).

## patchLogic.2 — dropper e dispenser
- `src/logic.inc`: `dispFire`/`dispNeighbor`/`dispOpen`. Container de 9 slots (tela de 1 linha, reaproveita `chestAt`/`netSendChest`/salvar de baús: `chestAt` aceita `gDropId`/`gDispId`; ids resolvidos em `logicInit` e registrados em `pkblocks.inc`). Energia (borda de subida, bit 8 do meta) solta 1 item de slot aleatório na frente (meta&3: 0 N,1 L,2 S,3 O); dropper com baú na frente empurra para dentro dele; vazio = clique. Quebrar solta o conteúdo (`world.inc` → `chestOnRemoved(..., false)`). Clique direito abre (`main_loop.inc`). Host/solo decidem. Testes: `tests/test_logic.cpp` (45).
- (VIS) a tela é 1 linha de 9 (não a grade 3x3 do Java); só 4 direções (sem cima/baixo); dispenser só solta (não atira flecha, não esparrama bola de neve/ovo, não coloca balde/água, não acende TNT); itens com dano/NBT: NBT vai, dano não. Funil/hopper para dentro/fora ainda não existe. Baú-armadilha/redstone comparador não lê.
- AINDA FALTA: dispenser com comportamentos (flecha, TNT, balde, ovo, poção...), tela 3x3, direção cima/baixo; relógio, observer, comparador, funil; áudio: menu, poção, flecha, forja, XP; música do menu; Junkbox (decidir); mods não-Decocraft (7).

## patchLogic.3 — funil (hopper) + pacote acumulado
- `src/logic.inc`: `hopperOne`/`hopperTick`. Funil = `block/hopper_outside` do pack; container de 9 slots (mesma tela de 1 linha do dropper/dispenser); a cada 8 ticks puxa 1 item do container de cima (baú, dropper, dispenser, funil) ou item solto sobre ele, e empurra 1 para o container de baixo; energia desliga. Só funis a até 12 blocos do jogador; só host/solo. Quebrar solta o conteúdo. Teste: `tests/test_logic.cpp` (49).
- (VIS) bloco do pack `hopper_outside` colocável/clicável; saída só PARA BAIXO (sem apontar para o lado); não puxa/empurra fornalha nem funil-carrinho; textura do funil continua cubo.
- Este zip é **acumulado**: contém todos os patches da sessão (TNT na água, Juke.1/2, Mus.1, Aud.2/3/4/5, Wx.8, Logic.1/2/3, Trap.2, Mob.9, Suf.1) + assets de música/discos.
- AINDA FALTA: funil apontando para o lado/fornalha, dispenser com comportamentos, tela 3x3, relógio, observer, comparador; áudio: menu, poção, flecha, forja, XP; música do menu; Junkbox (decidir); mods não-Decocraft (7).

## patchSombra.2 — sombra redonda igual ao 1.12.2
- `src/player_render.inc` (`entBlobShadow`, `mobShadowSize`), `src/main_loop.inc` (chamadas): refeita pela formula do `Render.renderShadow` do 1.12.2: textura 32x32 pixelada (GL_NEAREST, no lugar do disco suave 128x128), alfa = (1 - dist²/256 - altura/2) * 0.5 * brilho da luz (continuo, nao 2 niveis), so cubos inteiros ate `size` abaixo dos pes (some ao pular alto, como no Java), nao desenha com luz fraca. Raio = shadowSize do renderer (porco/vaca/ovelha .7, galinha .3, aranha 1.0, caverna .7, resto .5). ATRASO corrigido: mobs e jogadores remotos agora usam posicao interpolada (px..x com partialTicks), antes usavam a do tick. Sem shader (F7 desligado), como antes.
- Testado: sintaxe. (VIS) tamanho/escurecer por luz, sombra sumindo no pulo, mobs sem atraso.
- AINDA FALTA: mobs filhotes com metade do raio; ler `textures/misc/shadow.png` do pack quando existir (hoje gerada no codigo); sombra de slime (0.25*tamanho), cavalo .75, boat/minecart/item; Junkbox (decidir); mods nao-Decocraft (7); dispenser com comportamentos, relogio, observer, comparador; audio: menu, pocao, flecha, forja, XP.

## patchSombra.3 — sombra le shadow.png do pack
- `src/player_render.inc` (`entBlobShadow`): tenta `misc/shadow.png` do pack via `packRGBA` (quadrada); sem ele, usa a textura gerada. Filhotes: o jogo nao tem mobs filhotes, item removido. Slime/cavalo/barco nao existem em `MD[]`: entram junto com o mob (usar `mobShadowSize`).
- Testado: sintaxe. (VIS) com o pack Default-Java-1.12.2 a sombra deve ficar identica a do Java.
- AINDA FALTA: Junkbox (decidir: Barrel 27 slots ou decorativo do Decocraft); mods nao-Decocraft (7); dispenser com comportamentos, relogio, observer, comparador; audio: menu, pocao, flecha, forja, XP.

## Junkbox = toca-discos (decisao do usuario)
- "Junkbox" era o toca-discos (jukebox), nao Barrel nem Junk Box do Decocraft: ja feito em patchJuke.1/2 + Mus.1. Item 5 da fila encerrado. Sobra so (VIS): disco nao toca de novo ao recarregar o mundo; comparador nao le o disco.
- AINDA FALTA: mods nao-Decocraft (7); dispenser com comportamentos, relogio, observer, comparador; audio: menu, pocao, flecha, forja, XP.

## patchAud.6 — arco, XP, menu, bebida, fornalha
- `assets/sounds/events.cfg`: eventos `bow.shoot`, `bow.hit`, `orb`, `levelup`, `ui.click`, `drink`, `furnace.crackle`. Ganchos: `bowShoot` e flecha cravada (`bow.inc`), `survAddXp` (orbe / subir de nivel, `survival.inc`), balde de leite (`main_loop.inc`), estalo de fornalha acesa (`furnace.inc`, 1 em 30 ticks), clique de botao do titulo (`menu.inc`) e das telas de opcoes (`screens.inc`).
- Testado: sintaxe, `test_juke` e `test_furnace`. (VIS) volumes, e se o clique do titulo nao toca duas vezes.
- AINDA FALTA: audio do menu de pausa/inventario e musica do menu; pocao (nao ha pocoes no jogo), flecha acertando mob/jogador, forja de bigorna; mods nao-Decocraft (7); dispenser com comportamentos, relogio, observer, comparador.

## patchLogic.4 — dispenser dispara flecha e acende TNT
- `src/logic.inc` (`dispFire`): dispenser com flecha no slot sorteado dispara a flecha (vel 1,1, imprecisao como o Java, cravavel/recolhivel); com TNT acende a TNT (pavio 80) na frente. Dropper e outros itens: igual a antes (solta o item). Teste novo em `tests/test_logic.cpp` (51 verificacoes, 0 falhas).
- (VIS) flecha do dispenser nao fere o proprio jogador? (dono = -1 como a do arco); recolher a flecha em survival.
- AINDA FALTA: dispenser com balde (agua/lava), ovo/bola de neve, fogo/isqueiro, pocao, bone/lã; tela 3x3; direcao cima/baixo; observer e comparador (precisam de bloco direcional novo: nao ha texturas observer_* nem comparator_* no projeto, so no pack 1.12.2; falta modelo/face); relogio (item); funil lateral; mods nao-Decocraft (7); audio pausa/inventario/menu musica.

## patchLogic.5 — dispenser com balde
- `src/logic.inc` (`dispFire`): balde de agua/lava esvazia na celula da frente (so se for ar; agua no Nether evapora e esvazia), balde vazio enche numa FONTE de agua/lava da frente; o slot vira o balde trocado. Testes em `tests/test_logic.cpp` (53 verificacoes, 0 falhas).
- (VIS) dispenser vira fonte de agua que escorre; sons.
- AINDA FALTA: dispenser com ovo/bola de neve (jogar), fogo/isqueiro, pocao, bone; tela 3x3; direcao cima/baixo; observer e comparador (sem texturas/bloco direcional); relogio (item); funil lateral; mods nao-Decocraft (7); audio pausa/inventario/musica do menu.

## patchClock.1 — relogio animado pela hora do mundo
- `src/compass.inc` (`clockFrame`, `clockUpdate`), chamado em `main_loop.inc` junto da bussola; `src/item_data.inc` (pilha 1). Mesma tecnica da bussola: troca o conteudo do tile `item/clock_00` por um dos 64 quadros `clock_00..63` do pack. Quadro = angulo celeste do 1.12 (0 = meio-dia, 32 = meia-noite); no Nether/End gira sem sentido. Conferido a formula (6000->0, 12000->13, 18000->32, 0->50).
- Testado: sintaxe e formula. (VIS) relogio no inventario/mao/chao acompanha a hora; `/time set` muda o quadro; funciona em Vulkan/GLES (usa o mesmo upload da bussola).
- AINDA FALTA: receita do relogio (4 ouro + redstone) se ainda nao existir; dispenser com ovo/neve/fogo/osso, tela 3x3, direcao cima/baixo; observer e comparador (sem texturas/bloco direcional); funil lateral; mods nao-Decocraft (7); audio pausa/inventario/musica do menu.

## patchClock.2 — receitas do relogio e da bussola + arredondamento
- `src/chest.inc` (`packRecipes`): relogio (4 ouro + redstone em cruz) e bussola (4 ferro + redstone em cruz); a assinatura do cache inclui os ids. `src/compass.inc`: `clockFrame` com epsilon (meia-noite dava quadro 31 em vez de 32). Teste novo `tests/test_clock.cpp` (7 verificacoes, 0 falhas).
- (VIS) fabricar na mesa de trabalho; se o pack ativo nao tem `item/clock_00`/`item/compass_00` a receita some.
- AINDA FALTA: dispenser com ovo/neve/fogo/osso, tela 3x3, direcao cima/baixo; observer e comparador (sem texturas/bloco direcional); funil lateral; mods nao-Decocraft (7); audio pausa/inventario/musica do menu.

## patchBug.1 — /give pegava item errado + testes velhos
- `src/chat_debug.inc` (`findItem`): busca parcial escolhia o PRIMEIRO item que continha o texto ("/give stone" dava Minerio de redstone); agora escolhe o melhor (comeco do nome > comeco de palavra > meio, nome mais curto) e aceita o id em ingles do pack (`block/stone`, `item/iron_ingot`, `item/clock_00`) como match exato.
- `tests/test_cmd_player.cpp`: atualizado para as regras de op (convidado sem op: /puxar e /give negados; convidado op pode). 26 verificacoes, 0 falhas. `test_cmd_nick` 12/0, `test_nick_p2_chat` 29/0. `test_chest2` passa (43/43) quando a pasta `saves/` existe (o segfault era ela ausente no zip).
- (VIS) `/give stone`, `/give iron_ingot`, `/give pedra` no jogo com o pack; se algum item pt-BR comum ainda cai no item errado, mandar o nome.
- AINDA FALTA (bugs): mods nao-Decocraft sem grade/itens como prop (PRIORIDADE); mods no MP; textura das armas; placa (host nao ve texto, camera, Android); areia caindo no MP; ovo sem dano/perola; sons dos botoes (feito titulo/opcoes; falta pausa/inventario).

## patchBug.2 — blocos/icones de mods fora do Decocraft
- `src/propsys.inc` (`propGridBlock`, `propModFlat`, `propPlace`): prop de bloco inteiro (modelo ~1x1x1) de mod que NAO e Decocraft agora ignora a posicao livre (agachado) e sempre fica no centro da celula (so 4 direcoes, ja existia); icone plano de mod (so textura, ex. os 308 icones do Techguns) nao e mais posicionavel como prop. Decocraft intacto.
- Testes: `tests/test_propgrid.cpp` (5 verificacoes, pack sintetico), `test_propsys` 143/0, `test_propnet` 64/0 (rodar de dentro de `tests/`).
- (VIS) blocos de mods (cfm, bibliocraft, architecturecraft...) encaixam; itens-icone nao colocam mais; se algum item plano de mod precisava ser colocado (quadro/poster), avisar o nome.
- AINDA FALTA (prioridade): blocos de mod virarem BLOCOS reais da grade (colisao, luz e vizinhanca de bloco, hoje ainda props); baus de mods como containers; REMOVER do inventario os itens so-textura (hoje so deixam de ser posicionaveis); mods no MP; armas funcionais.

## patchBug.3 — ajuste do patchBug.2 (so sprite de item vira nao-posicionavel)
- Levantamento (~1600 itens "planos" de mods fora do Decocraft: techguns 281, ic2 506, biomesoplenty 200, quark 145, cfm 107, aquaculture 84...): quase todos sao ITENS de verdade (varas, peixes, armaduras, livros, varinhas, polvora). Comportamento (`beh`) vem vazio em todos, entao nao da para separar item de bloco por ele.
- `src/propsys.inc` (`propModFlat`): so e "nao posicionavel" o SPRITE gerado pelo conversor (laje centrada em z, espessura ~0,06). Coisa fina de verdade, como `quark_iron_ladder` (z=0,45, espessura 0), continua posicionavel. `tests/test_propgrid.cpp`: 6 verificacoes, 0 falhas; `test_propsys` e `test_propnet` sem regressao.
- DECISAO PENDENTE (usuario): tirar do inventario criativo os sprites de item sem funcao? Sao itens legitimos (peixe, vara, armadura), so nao tem uso no jogo ainda. Sugestao: manter, e implementar uso real por mod (aquaculture pesca, armaduras, comida) em vez de remover.

## patchGfx.2 / patchPerf.1 / patchPano.1
- Panorama do titulo sem pedra exposta: camada de terra/grama desce abaixo do vizinho mais baixo (`panoCarve`, main_loop.inc). (VIS)
- Graficos Rapidos/Bonitos agora mudam o jogo: Rapido = folhas opacas (tiles do atlas, `leafGfxApply` em compass.inc) + culling total folha-folha (`gLeafFast`, world.inc); Bonito = original. Remalha ao alternar. (VIS)
- Panorama gerado em fatias de 8 ms por quadro (`panoBegin/panoStep`) = sem congelar ao voltar ao titulo. (VIS)

## patchPerf.1 / patchLog.1 (congelamento + logs)
- Causa dos quadros de 1,5-2,6 s do latest.log: a faixa de musica (OGG, minutos) era decodificada na thread principal em `sndMusicStart`. Agora decodifica em thread (`sndMusicThread`/`sndMusicPoll`, audio.inc); `sndMusicStop` cancela. (VIS: musica entra ~1-3 s depois, sem engasgo)
- perf.inc ignorava quadros > 1 s em silencio: agora loga `[LAG] quadro de N ms (>1 s)` com as causas. `perfNote()` registra causas (decodificacao de audio > 25 ms etc.) e sai junto do [LAG].
- "Can't keep up" agora mostra tela, chunks e causas. Aviso de som ausente agora 1x por som, com evento e caminho.
- Tabela do vorbis thread-safe. No log do usuario faltam damage/, fire/, liquid/ em assets/sounds (so 226 .ogg no zip).

## patchTouch.3 (Android: controles)
- Botoes maiores e afastados da borda (pular 50, agachar 42, coluna lateral 36, topo 22, margem 18; joystick maior e mais afastado). `touchLay` em touch.inc. (VIS)
- Botao de inventario = slot da hotbar (widgets.png, 22x22) acinzentado, COLADO na direita da hotbar; area de toque maior (`invHit`). Sem textura: visual antigo.
- Slot da mao esquerda usa o sprite do widgets.png (29x24, `rtOffL`); reserva: slot da hotbar. `inventory_ui.inc`. (VIS)
- Pick block ja e momentaneo (patchTouch.1: solta sozinho em 160 ms); nenhum toggle preso.

## patchLog.2 (alçapão de ferro + auditoria da fila)
- Verificado com o pack real: os 4 itens de alçapão (block/iron_trapdoor, _face, oak_trapdoor, trapdoor) estão em `gXbItem` e `rsPlace(XB_TRAP, 0/1)` coloca no chão e na parede (`tests/test_trapplace.cpp`). Não reproduzi o bug; se acontecer, o log traz `[PLACE] item ... nao esta ligado a bloco colocavel`.
- Fila antiga já feita: desligar mods (patchMod.9), TNT primed na água (patchTnt.1, `tntLiquidPush`), pick momentâneo (patchTouch.1).
- AINDA FALTA: Junkbox (decidir Barrel 1.12.2 x Junk Box do Decocraft), mods não-Decocraft (grade/rotação/MP), sons damage/fire/liquid (arquivos não vieram no zip), resto do áudio.

## patchPE.1 — HUD touch estilo PE 1.1.5
- touch.inc: joystick flutuante trocado por direcional de 5 botoes (cruz; centro = agachar, losango), botoes maiores (24-36 unidades GUI); topo: chat, pausa, depois F5/F1/F3/luz/Tab; pular/coluna lateral mantidos. Compila e passa nos testes headless (sintaxe). (VIS)
- FALTA: menu inicial estilo PE (Play/Settings/skins, sem launcher/conquistas/login) no Android e Rpi; opcoes do launcher abaixo das skins; coracoes no topo-esquerdo e fome no topo-direito como no PE (conferir survival).

## patchPE.2 — titulo e Configuracoes estilo PE
- menu.inc: `peUi()` (Android, Rpi, ou PC com F8/`BL_PE=1`); titulo PE: logo, Jogar/Multijogador/Configuracoes/Mods, skin+nome a direita com cabide (abre perfil/skins), sem Conquistas/Login/Realms. screens.inc: pagina `peset` (categorias + painel, placeholder; sliders reais so em Video). main_loop.inc: sem launcher inicial no modo PE. Compila (sintaxe). (VIS)
- FALTA: layout das listas de mundos/multijogador/opcoes em PE; coracoes/fome do HUD (conferir survival.inc); categorias de Configuracoes com opcoes reais.

## patchPE.3 — menu PE como principal
- menu.inc: `peUi()` sempre true (titulo PE padrao no PC tambem); titulo/menu deslocados a esquerda; boneco 3D (drawPlayerPreview) segue mouse/dedo, maior; cabide abaixo abre perfil/skins. main_loop: sem launcher inicial. screens.inc: Configuracoes PE com Video (video+detalhes+animacoes+qualidade+desempenho+diversas) e Jogo (chat+sons) em lista unica rolavel (roda do mouse); moldura sem faixa cinza. Compila (sintaxe). (VIS)
- FALTA: rolagem por arrasto no touch; categorias Teclado/Controle/Toque/Perfil sao placeholder; paginas internas (mundos, multi) em PE; coracoes/fome no HUD.

## patchPE.4 — ajustes do menu PE
- Configuracoes: sem paineis cinza (panorama escurecido atras), opcoes em 2 colunas na tela toda, sem titulo duplicado; titulo: nome acima da cabeca desenhado depois do boneco, sem item na mao. Compila (sintaxe). (VIS)
- FALTA: scroll por arrasto no touch; Teclado/Controle/Toque/Perfil placeholder; paginas internas em PE; coracoes/fome no HUD.

## patchPE.5 — seletor de skins estilo PE
- Titulo: boneco menor (33% da altura). Cabide abre pagina `peskin` (screens.inc): previa 3D grande que segue mouse/dedo, nome da skin, campo de nome + "Escolher arquivo...", carrossel de miniaturas (setas < >, clique seleciona), "Opcoes do launcher..." (abre `launcher`) e "Concluido" abaixo. Panorama escurecido atras. Compila (sintaxe). (VIS)
- FALTA: miniaturas em corpo inteiro (hoje so o rosto); scroll touch em Configuracoes; categorias placeholder; paginas internas PE; coracoes/fome no HUD.

## patchPE.6 — Configuracoes completas + HUD PE
- screens.inc: rolagem por arrasto (touch/mouse; botoes da lista agem ao soltar); categorias reais: Teclado e Mouse (controles), Controle (gamepad), Toque (HUD touch ON/OFF = F8), Perfil (botao Skins + customizacao da skin), Jogo, Video. survival.inc/touch.inc: com HUD touch ligado, coracoes no topo esquerdo, fome e bolhas no topo direito (XP continua acima da hotbar). Compila (sintaxe). (VIS)
- FALTA: miniaturas de skin em corpo inteiro; paginas internas (mundos, multi, mods) em PE; barra de armadura no HUD PE (abaixo dos coracoes).

## patchPE.7 — tela de skins: nome da skin acima da cabeca, boneco menor (screens.inc). Compila (sintaxe). (VIS)

## patchPE.8 — tela de skins: previa 2D nitida em painel escuro (sombra no chao, nome acima da cabeca, tamanho proporcional). Titulo continua 3D. Compila (sintaxe). (VIS)

## patchSkin.1
- Tela de skins (peskin): fundo de terra, paineis com moldura, preview 3D girando (arrastar gira), nome da skin na dica do carrossel. Menu principal: cabide removido, o boneco e o botao (contorno ao passar o mouse). (VIS)

## patchOpt.3
- Todas as opcoes de video/animacao/qualidade/desempenho/diversas/fov persistem em options.cfg (opt.* / osl.*). Volume (m0..m8) aplica ja no menu; chuva/trovao = cat 3 (Clima), caverna = cat 8 (Ambiente). Ligadas: Ceu, Estrelas, Sol e Lua, Altura das nuvens, Nevoa (OFF) e inicio, Chuva e neve OFF, Arvores. FALTAM: v4,v7,v9-v11, d7,d10-d16, an.1-16, an.p, q*, p*, x*. (VIS)

## patchUI.1
- Versao agora v1.1.0.19rc (`BLOCKLAND_VERSION` em src/main.cpp; instalador le os digitos). **REGRA: SEMPRE mudar a versao ao atualizar (a cada patch/zip).**
- GUI Auto no desktop um passo menor (tela cheia 768p = 2x). Titulo: Sair embaixo de Configuracoes/Mods; contorno rigido (silhueta dilatada por stencil) no boneco ao passar o mouse. (VIS)

## patchLoad.1 / patchSkin.2
- Skin arrastada agora funciona no menu (DROPFILE tratado antes da cadeia gScreen em main_loop.inc); area de soltar na tela peskin (screens.inc) substitui textos inuteis; clique na area abre o seletor.
- Boneco do titulo estilo Bedrock (gPreviewDist/suavizacao em player_render.inc, menu.inc).
- Loading simples sem logo: "loaded: arquivo" + barra embaixo (splash.inc, hooks splashNote em png.inc/audio.inc); mais rapido. (VIS) tudo.

## patchSkin.3
- 6 skins novas em assets/skins (unnamed 128x128 convertida p/ 64); default.png = cinza com rostinho feliz (tambem defaultSkin() em player_skin.inc).
- loadSkinPNG (player_skin.inc): aceita 128x128/256x256/128x64 etc. (reduz por media); usado em loadSkin, skinScan, skin2Preview.
- Launcher antigo removido: pagina "launcher" = alias de "peskin"; J2 (nome+skin) agora na tela de skins quando splitOn(). Inicio nao abre mais o launcher.
- Titulo: duplo clique no boneco abre skins/opcoes; aviso com seta na 1a vez (profile.cfg hint1=1). (VIS) posicao do aviso e do J2.

## patchTouch.4
- Botao de inventario do HUD touch: botao de pedra com icone de bau (touch.inc, pChest), no lugar do slot escuro com "...". (VIS)

## patchAud.7
- Clique de menu tocava em qualquer toque (main_loop.inc) alem do proprio botao (som duplo). Removido o som generico; cada botao toca 1x (screens.inc/menu.inc). (VIS)

## patchUI.1 — clique de inventario so em abas; sem botao pick (touch); panorama proprio; skins 3D; galeria Android
- Som: removido o "ui.click" generico do inventario (tocava em qualquer clique); agora so em abas/setas. (VIS)
- touch.inc: botao pick removido (acerto e desenho). Panorama do titulo: `panoFill`/`panoHeight` (main_loop.inc) geram terreno proprio via `gPanoGen` em `genImpl` (nao usa mais gerador do mundo + carve). (VIS)
- Skins removidas: exploradora, mago, mineiro, aventureiro, astronauta, ninja, pirata. Carrossel: cabeca 3D com overlay (`uHead3D`, `gSkinHead3`) e fundo em degrade. (VIS)
- Galeria Android: C++ pronto (`skinPickFile` via JNI -> `blPickSkin`); falta colar `tools/android-galeria/SkinPickerPatch.java` na Activity (android/ nao veio no zip).

## patchTouch.5 — layout Android: direcional maior e afastado, botoes de acao maiores, chat no canto superior esquerdo (touch.inc) (VIS)

## patchLeves.1 — escadas/entidades de bloco/porta/sombra/vidro/concreto
- models.inc: luz suave por VERTICE em escada, laje, bau, cama, placa, estandarte (media da celula da frente + 3 do canto, so celulas que passam luz); respeita `gSmoothMode` (OFF = antigo). Sem AO nesses modelos. (VIS)
- Hitbox da escada conferida por `tests/test_stairbox.cpp`: caixas exatas (0-1 x 0-1, meia altura). Travar a cabeca = colisao igual ao vanilla (beiral de escada acima da cabeca); se persistir, mandar video com a posicao exata.
- player_render.inc: `uiFeetShadow` (misc/shadow.png) sob os pes do boneco (titulo + skins); `entShadowTex()` extraida. (VIS)
- Porta: some enquanto o jogador esta dentro (`gDoorHide`, models.inc + main_loop.inc). (VIS)
- Vidro: place/step usam random/glass (nao ha dig/glass no pack) com pitch/vol ajustados; vidracas = material vidro.
- falling.inc: `*_concrete_powder` cai como areia (`gPowder`). Falta: virar concreto na agua.

## patchBuild.1 — ./build (atalho de build.sh) pergunta a plataforma (padrao Linux, Enter); compilacao mais rapida: -O1 por padrao (--release/--pack = -O2), -pipe, ccache e linker mold/lld/gold se instalados (scripts/fastcc.sh)

## patchSkin.4 — sombra dos pes acompanha o corpo (elipse da pegada, girada com o yaw; sombras falsas largas removidas da tela de skins), destaque do carrossel de skins cinza (era azul), blur no panorama do titulo (blurScreen em menu.inc, chamado no main_loop), fade-in do boneco do titulo junto com o menu (blurRedraw), luz suave dos degraus de escada/topo de laje (models.inc, camada da propria celula), "Opcoes" do jogo abre a mesma tela de Configuracoes do titulo (peset) (VIS)

## patchLeves.2 — blocos inteiros ao lado de escada/laje nao escurecem mais: celula de escada/laje usa o maximo da luz dos 6 vizinhos na media de vertices (world.inc cellInfo) (VIS)

## patchSkin.5 / patchLeves.3 — titulo: boneco 3D removido, cabeca 3D do jogador como botao quadrado ao lado de Configuracoes/Mods abre perfil/skin (1 clique); escada/laje: sombreamento lateral igual ao dos blocos inteiros (X .8, Z .65) (VIS)

## patchSkin.6 — titulo centralizado, sem cabeca ao lado de Mods; aviso (hint2) aponta para Configuracoes: launcher antigo descontinuado; Configuracoes > Perfil so com Skins e nome (sem capa/mangas/calcas/chapeu); Configuracoes com fundo de terra (sem panorama) (VIS)

## patchSkin.7 — Configuracoes > Perfil abre direto a tela de skins (launcher novo); faixa do carrossel de skins neutra (era azulada) (VIS)

## patchSkin.8 — blur do titulo por reducao 1/2->1/4 + ampliacao bilinear (sem fantasmas/faixas) (VIS)

## patchRpi.1 — Criar Novo Mundo (criar/criar2) com layout compacto em duas colunas quando sh < 240 (Raspberry), sem sobrepor os botoes de baixo (VIS)

## patchInv.1 — inventario criativo no toque (gInvTouch) estilo Pocket Edition: moldura bege, miolo/slots escuros, abas laterais (esq. = linha 1, dir. = linha 2); hotbar selecionada + toque num item do inventario tira o item do slot (inventory_ui.inc) (VIS)

## patchInvTouch.1 (v1.1.0.20rc) — inventario no celular com fundo de terra
- [x] (VIS) Modo toque (`gInvTouch`): fundo de terra em tela cheia (`invDirtQuad`, menu.inc), painel de terra (`uPanel`), slots escuros translucidos com borda marrom (`uBox`), abas laterais de terra, textos claros com sombra; vale para sobrevivencia, mesa, bau/fornalha e criativo. Flag `gInvDirt` so liga durante o desenho do inventario.
- [x] (VIS) Area de toque dos slots = celula inteira 18x18 (sem zona morta entre slots) em `hitSlot`.
- [x] Corrigido no layout sem textura: seta/resultado/escudo seguem o layout do pack (rtInv).
- Nao compilado aqui (sem SDL2 no ambiente): compile com `./build.sh` e confira no celular.

## patchInvTouch.2 + patchPorta.2 (v1.1.0.21rc)
- [x] Inventario no celular sem tema de terra: mesmos assets/painel do PC (painel PE bege desligado; escala ajustada para abas em cima/embaixo). Area de toque 18x18 mantida.
- [x] Celular: slot selecionado + toque na paleta do criativo (mesmo em celula vazia) apaga o slot, como o Java.
- [x] Porta: luz por face (vizinho na direcao da normal, como laje/escada) para as duas metades terem o mesmo brilho; so fica invisivel quando FECHADA com o jogador dentro (aberta continua visivel).
- Nao compilado aqui (sem SDL2): compile e confira (VIS).

## patchInvTouch.3 (v1.1.0.22rc)
- [x] Criativo no celular: layout adaptado de volta (abas laterais, escala maior: so painel+abas laterais limitam) com os assets do PC (painel creative_*.png do pack, botao do pack nas abas). Mantida a correcao: slot selecionado + toque na paleta apaga o slot. (VIS)

## patchSplit.1 — tela inicial, bug de mira no split, split de 2 a 4 jogadores
- menu.inc: logo+menu centralizados (titleLogoBox), frase "Faça de tudo!" sem sobrepor botao. (VIS)
- main_loop.inc: viewEye do J1 restaurado apos as passadas dos extras (J1 quebrava bloco na posicao do J2).
- splitscreen.inc: gSX[3] (J2..J4), SX/SxSel, layouts 2/3/4, split2Cycle (F12/Esc), alvo de mobs/flechas/explosoes/pistoes em todos.
- Falta: J3/J4 sem bote/chat; skin no perfil so do J2; Android sem split; conferir test_shapes linha 54, test_drown, test_tnt.

## patchSplit.2 — visao individual por tela (FOV, balanco de camera, balanco da mao)
- splitscreen.inc: SplitP ganhou fovMod/prevFovMod, cbY/pcbY/cbP/pcbP e hb[4] (balanco da mao); split2Tick1 atualiza os tres (FOV x1.15 ao correr, view bob, handBobTick com yaw/pitch do proprio jogador).
- main_loop.inc: swapView troca esses pares por jogador (antes um unico s2Fov era trocado 2x: FOV de J2 virava o de J1 e o par de J1 saia invertido; gHandBob era um so, entao girar a camera de um mexia o braco do outro).
- Testado: sintaxe ok; test_split 61/61 (no sandbox precisa de stub de SDL_GetTicks no link). (VIS)
- Falta: estados de UI individuais (placa, inventario, pausa, chat); J3/J4 bote/chat; skin do perfil so do J2; Android sem split; test_shapes/test_drown/test_tnt.

## patchSplit.3 — jogadores do split screen visiveis para a rede (NET_VER 36)
- net.inc: pacote novo NP_XSTATE (k + ligado + corpo do NP_STATE; netXPacket/netSendStateX/netSendXGone); netStateParse (corpo do NP_STATE reaproveitado); mapa visual remotesX (chave 1000+id*4+k) desenhado em netDraw/netDrawTags/sombra, animado em netTick, expira em 8 s, apagado no NP_BYE; nametag "<nick> (J2/J3/J4)". Host repassa como qualquer pacote.
- splitscreen.inc: split2Tick1 envia o estado de cada extra todo tick (item na mao, armadura, arco, fogo, sentado no bote); split2SetN avisa "saiu" ao desligar/reduzir; extra morto some.
- Extras na rede NAO entram em PvP, mobs, colisao de colocar bloco (so visual). Skin: aparecem com a skin padrao.
- Testado: sintaxe ok, test_split 64/64 (inclui ida e volta do pacote). (VIS)
- Falta: skin de cada extra na rede; extras alvejaveis por PvP/mobs de outro cliente; barco/carrinho (proximo patch: 2 por barco, P3/P4 e carrinho).

## patchSplit.4 — UI por viewport, PvP no split/multiplayer, sombras dos jogadores do split (v1.1.0.23rc, NET_VER 37)
- main_loop.inc: inventario/pausa/placa/morte do J1 aparecem so no viewport do dono (dual sempre ligado no split); mouse real vira coordenada local (`uiRectNow`, `uiWinSize`, `uiSync`); extras nao congelam mais com a UI do J1; pausa nao zera o tempo no split; so o controle do J1 pausa.
- gamepad.inc: cursor do controle em coordenadas locais (evento marcado which=0x7F000001); J2 nao comanda mais o menu do J1 quando J1 usa teclado.
- pvp.inc/splitscreen.inc/net.inc: `pvpAttackK` (J1, extras e jogadores/extras remotos), `split2PvpHit`, `split2PvpReceive`; NP_PVP alvo = id | ((k+1)<<4); NET_VER 37.
- Sombras: blob (sem shader) e mapa de sombras (F7) para os outros jogadores do split; pause_menu.inc usa gVpX/gVpY.
- Testado: sintaxe ok, test_split 71/71 (7 novos de PvP), test_propnet 64/64. (VIS) nada visto no jogo.
- Falta: inventario aberto por J2-J4 ainda para o J1; pausa propria de J2-J4; chat/placa de extras; flechas entre jogadores do split; nick do extra na mensagem de morte via rede.

## patchMenu.1 — menu principal
- patchMenu.2: "fixo" = mesmo terreno sempre (semente fixa em panoBegin); panorama continua girando. Faixa inferior, painel e espaçamentos reajustados (titleLogoBox, peTitleRects).
- Texto amarelo preso ao canto inferior direito do logo, escala com o logo (lw/260) e é limitado às bordas da janela. (VIS)

## patchCfg.1 — Configurações redesenhadas (screens.inc)
- Página `peset` reescrita: 5 categorias (Geral, Vídeo, Ambiente, Áudio, Controles), seções com título amarelo, painéis lateral/cabeçalho/conteúdo, categoria atual destacada. Só opções com efeito no jogo (fov, v0–v3, v5, v6, v8, d0–d6, d8, d18, an.0, m0–m11, gfxapi).
- Removidas as páginas mortas: opcoes, skin (capa), chat, musica, twitch, video, detalhes, animacoes, qualidade, desempenho, diversas, snooper (opções sem efeito: Realms, Super Secret, anáglifo, OptiFine etc.).
- Corrigido: "Água animada" mostrava OFF por padrão mas o jogo usa ON (an.0 def=1).
- AINDA: opções de chat (c0–c9) e sensibilidade/inverter mouse da página `controles` (k0–k2) não são lidas pelo jogo; código morto em menu.inc (ramo antigo do título, `goOther`) e helpers (`snpRows`, `LANGS`...) ainda no arquivo. (VIS) conferir rolagem/cliques em janelas pequenas e Android.

## patchMenu.3 — título estilo MCPE
- Removidos degradê em faixas (causava listras), painel escuro, faixa inferior e sombra do logo. Logo maior, botões 24 de altura/230 de largura, fundo com blur duplo, splash amarelo no canto inferior direito do logo (menu.inc `titleLogoBox`/`peTitleRects`/`drawTitlePE`, main_loop.inc). Sem boneco. (VIS)

## patchPad.1 — cursor do controle + boneco no inventário (split)
- `drawPlayerPreview`/`Spin` (player_render.inc): scissor agora soma a origem do viewport (era a causa da caixa preta do boneco no 2º viewport do split).
- Cursor do controle desenhado (pixel art `assets/textures/gui/cursor_pad.png`, 27x27) no viewport do jogador do controle; fim do `main_loop.inc`; `gpCurT` em `gamepad.inc` (no split fica sempre visível; fora do split some após 4 s sem uso). (VIS)

## patchPad.2 — cursor grudento + mão do J1
- Cursor do controle prende nos slots (gamepad.inc `gpUiFrame`, `gpSlotPoints` em main_loop.inc): direcional pula para o slot vizinho; stick prende no slot e solta ao empurrar além de 1 raio; imã a 0,55 raio. Vale p/ inventário, baú, bancada, fornalha (slots de `activeSlots`). Grade do criativo (abas) não prende ainda. (VIS)
- Bug da mão: com o inventário do J2 aberto, `inv` guarda o do J2 e a mão do J1 lia o slot errado (mão vazia). `held` do J1 agora vem do inventário real dele (main_loop.inc, ~l.877).

## patchNet.1 — convidado acende TNT e usa ovo de spawn (NET_VER 38)
- net.inc: NP_MOBSPAWN (tipo, x y z, yaw, dim) e NP_TNTREQ (x y z, dim), convidado -> host; o host valida (tipo < M_N, dim igual, limites, max 400 mobs) e aplica spawnMob/tntIgnite. main_loop.inc: ovo de convidado manda pacote; nether.inc: isqueiro de convidado em TNT manda pedido.
- Compila; sem teste headless de rede para isso (VIS). test_split segfaulta tambem no src original (nao e regressao).

## patchSplit.5 — chat por viewport + teclado virtual so com controle
- chat_debug.inc: drawChat(w,h,own): o log aparece em cada viewport; campo de texto/rolagem so no viewport do dono (J1 = 0, J2 = 1 via gChatAs2). main_loop.inc: drawChat por viewport; teclado virtual desenhado no viewport do dono (glViewport + split2Rect). gamepad.inc/screens.inc: gChatPad (chat aberto pelo controle) - oskActive so mostra o teclado virtual nesse caso (T no teclado nao mostra mais).
- Compila (VIS). Falta: clique do mouse no teclado virtual em split usa coordenadas da janela; pausa/placa individuais seguem globais.

## patchNet.2 — indicador "digitando" (NET_VER 39)
- net.inc: NP_TYPING (1 byte), gNetTyping, " ..." no nome acima do jogador que esta com o chat aberto (some ao fechar ou apos 60 s). main_loop.inc: setChat avisa abrir/fechar (nao para comandos '/').
- Compila (VIS). Falta: mods em MP, voltar na mesma dimensao, areia caindo em MP, luz da lava, players escuros no End.

## patchCmd.1 — kill @e leva itens, TNT acesa e blocos em queda
- chat_debug.inc: /kill @e chama killLooseEnts (falling.inc): limpa DROPS (avisa a rede via netSendDropSet), TNTS e gFallEnts; so host/jogo local. Conta no total de entidades.
- Compila (VIS). Falta: tp @e (hoje so move jogadores); luz da lava/players escuros no End e areia em MP/dimensoes dependem de ver o jogo (VIS).

## patchCmd.2 — tp @e x y z
- chat_debug.inc: /tp @e <x> <y> <z> (aceita ~) leva mobs vivos e itens no chao ao destino (so host/local, entCmdLocal em falling.inc), alem dos jogadores. Falta: /tp @e <nick> e mover TNT/blocos em queda/barcos. (VIS)

## patchOsk.1 — teclado virtual melhorado
- screens.inc (oskKeys/oskDraw): nova fileira de acentos do portugues (á é í ó ú ã õ ç ê ô ! ?) com maiusculas; altura OSK_H=152 (definida em chat_debug.inc); dica de botoes encurtada quando o viewport e estreito (antes cortava nas bordas no split). chat_debug.inc: com o teclado aberto o historico do chat sobe e fica visivel acima dele.
- Compila (VIS). Posicao: continua no viewport de quem abriu (J1 = vp 0, J2 = vp 1, mesmo rect do inventario). Falta: clique do mouse no teclado em split usa coordenadas da janela.

## patchFire.1 — mobs queimam de verdade ao sol, nao na chuva, fogo no modelo vanilla
- mobs.inc (mobFireTick): causa do 'fogo sem dano': o sol renovava e.fire=160 todo tick e o dano usava fire%20==0, que nunca fechava enquanto no sol. Agora contador proprio e.burn: 1 de dano por segundo queimando.
- hostile.inc: chuva (gRain/gThunder >= 0,2, bioma nao frio) nao acende zumbi/esqueleto e apaga o fogo de quem esta a ceu aberto (wxCold declarada antes).
- nether.inc (drawFireWH): Render.renderEntityOnFire 1.8.9: planos de frente para a camera (gFireCamYaw, set em main_loop antes de drawMobs), escala largura*1,4, camadas de 0,45 que estreitam 0,9x e avancam 0,03, fire_0/fire_1 alternados e espelhados. Assinatura nova: (largura, altura, yawDaEntidade); chamadores em mobs.inc (drawMobFire), net.inc e main_loop.inc ajustados.
- Compila (VIS). Falta: ignicao depende do mapa de luz do chunk (c->lc) no bloco da cabeca; capacete nao protege ainda; fogo dos extras do split (J2-J4) nao desenha.

## patchNet.3 — convidado volta na mesma dimensao e posicao ao reentrar
- menu.inc: mpResRecord/mpResGet/mpResLoad (saves/<mundo>/mp_players.txt: nick dim x y z). net.inc: netResRecordId grava ao sair (BYE, timeout, nick fantasma), a cada 8 s e no netStop do host (netlan.inc); ao sincronizar o convidado (ready && !synced) o host manda NP_TPTO com flag extra 1 (volta de onde parou; mensagem propria). Sem novo tipo de pacote: NET_VER continua 39 (TPTO ganhou 1 byte opcional no fim; versao antiga ignora).
- Compila (VIS). Falta: testar com nick auto 'Jogador N' (nao grava), convidado em dimensao nao gerada no host, inventario/vida do convidado nao sao salvos pelo host (player_guest.dat e local).

## patchStronghold.2 — celas, salas de cruzamento, biblioteca e grades
- stronghold.inc: PrisonHall (parede central com iron bars/XB_PANE e celas), RoomCrossing com 5 tipos (p.f0: pilar com tochas, balaustrada, fonte de agua, plataforma com bau, vazio), Library (estantes nas paredes/colunas, mesas, teias via sbRandPut; sem 2o andar/escada), porta tipo 2 vira grade de ferro. Layout (rng/pecas) inalterado.
- Teste: test_stronghold 1737/0. Visual (VIS): disposicao dos moveis e aproximada (nao copia o Java bloco a bloco).
- Falta: portas de madeira/ferro de verdade e botao, loot dos baus, silverfish (monster egg/spawner), biblioteca com 2o andar e escada, ajuste ao bioma, Random de populacao.

## patchStronghold.3 — /locate stronghold
- chat_debug.inc (locateCmd): /locate stronghold|fortaleza da a posicao [x, ~, z] (centro do chunk) e a distancia da fortaleza mais proxima entre as 128 posicoes (shNearest em stronghold.inc); so na superficie. Compila (VIS). Lembrete: sem ajuste ao bioma as posicoes podem cair em oceano (como a etapa 1 ja avisava).

## patchEnd.12 — dragao ModelDragon 1.12.2, 1 cristal cura, coro natural, enderman (NET_VER 40) (VIS)
- dragon.inc: modelo refeito com os pivos/caixas/UV do ModelDragon (corpo, 5 vertebras de pescoco, cabeca com mandibula/narinas/escamas, 12 de cauda, asas com ponta, patas de 3 ossos) + animacao por ringBuffer (yaw/Y dos ultimos 64 ticks) e animTime em ciclos; escala 1/16 (era 2x). Cura: so UM cristal (o mais proximo, ate 32 blocos) cura 1 HP/10 ticks e so dele sai o raio; destruir esse cristal fere o dragao em 10.
- nether.inc: coro natural (growTreeRecursive/generatePlant) so nas ilhas externas, 0..4 tentativas/chunk, em end stone; plantas ligadas (bits de meta) + flor (meta 0xFF, modelo em cruz em redstone.inc). Fica dentro do chunk (ramos que cruzam a borda sao recusados).
- Enderman (M_ENDERMAN): modelo 64x32 + olhos, 40 de vida, neutro: encarar a cabeca por 5 ticks ou bater o irrita; 7 de dano; teleporta (encarado de perto, agua, acertado de longe, longe do alvo); agua machuca; dropa perola; ovo; /summon enderman; spawn no End (1-4, ate 6/jogador) e raro no overworld.
- AINDA FALTA: comportamento do dragao por fases (pouso no portal, sopro, bola de fogo, investida), animacao de morte (dragon_exploding), enderman pegar/colocar blocos, mandibula aberta/tremor ao irritar, enderman sem som (pack sem sounds/mob/endermen), abobora na cabeca, destruir planta de coro ao quebrar o apoio/crescer, item chorus_flower colocavel, test_split segfault (ja ocorria no original).

## patchEnd.13 — fases do dragao, morte dissolvendo, enderman completo, coro vivo (VIS)
- dragon.inc: fases (circula / investida / passada com bola de fogo / aproximacao / pousado no portal com sopro 3 de dano / decolagem); morte pairando com dragon_exploding dissolvendo; chorusScan: coro sem apoio quebra, flores crescem; chorusFix recalcula ligacoes.
- Enderman: boca aberta + tremor irritado, pega/solta blocos (grama, terra, areia, cascalho; so host), desvia de projetil (sem dano) teleportando. Item flor de coro colocavel (var 7, meta 0xFF).
- AINDA FALTA: sons do enderman (pack sem sounds/mob/endermen), abobora na cabeca, bola de fogo do dragao sem nuvem de sopro (usa a do blaze), fase do dragao nao sincroniza no multiplayer (convidado so ve posicao), enderman nao solta o bloco carregado ao morrer, test_split segfault (ja no original).

## patchPart.1
- Particula cinza ao quebrar mato: plantas XB usam xbTile + tint do bioma (mining_sky.inc partLook) e alpha test em drawParts.
- Menu de pausa: texto "?e???U" era ponteiro solto em splitLabel (splitscreen.inc); redesenho em pause_menu.inc (titulo "Jogo pausado" escala 1.5, dica Esc, grupos espacados). (VIS)

## patchSplit.6
- Split: nome e skin proprios de J2, J3 e J4 (profile.cfg nick3/nick4/skin3/skin4; tela peskin com uma linha por jogador). Skins dos extras enviadas pela rede (NP_SKIN, dono = 16+cliente*4+k; remotesX usa netSkinTexFor). Sem mudanca de formato de pacote antigo: NET_VER mantido. (VIS)

## patchFire.2
- Chamas desenhadas nos jogadores extras do split (J2-J4) na tela local: SplitPoseD.fire (splitscreen.inc) + q.fire em main_loop.inc (so fora da passada de sombras). Remotos ja tinham (net.inc). Compila (VIS).

## patchPause.2
- Pausa: removidos titulo "Jogo pausado" e dica "Esc para voltar"; Conquistas e Estatisticas agora abrem paginas (tempo de sessao, blocos quebrados/colocados; 5 conquistas simples) (pause_menu.inc, main_loop.inc, screens.inc).
- Performance: split reduz a distancia de nevoa/culling por tela (2 jogadores 80%, 3-4 jogadores 60%) em fogEndD (screens.inc). (VIS)

## patchMenu.4
- Menu principal: botoes 200x20 (Jogar, Multijogador, Configuracoes|Mods 98x20, Sair) na mesma grade do menu de pausa, sem esticar textura, grupos com respiro (menu.inc peTitleRects).
- Fundo do titulo: preto enquanto o panorama gera / splash ativo; ao ficar pronto, fade-in de 0,7 s (0,15 s ao voltar do jogo) (main_loop.inc). (VIS)

## patchFog.1
- Neblina radial (vFog = length(eye) em vez de |z|): a borda nao gira/segue a direcao da camera; curva suave (55% smoothstep) em sky_cycle.inc. Android (glesnative.inc) e caminho sem shader seguem planares. (VIS)

## patchFog.2
- Montanhas de longe: neblina com altura (acima de y=70 reduz ate 50% da distancia efetiva), perspectiva atmosferica mais fraca (0,27 -> 0,16) e sem clarear a cor; inicio da neblina de 35%-80% (antes 20%-80%) (sky_cycle.inc, screens.inc). (VIS)

## patchChunkAnim.1
- Chunks novos sobem do subsolo (ease-out, 1.1 s) + fade 0.7 s em main_loop.inc (chunkRise; solido, agua, vidro). BLOCKLAND_NOCHUNKANIM=1 desliga. Versao 1.1.0.24rc. (VIS)

## patchChunkAnim.2
- Chunk que sai do alcance afunda (fantasma, 1.1 s) e ao voltar sobe de novo (born reiniciado ao sair de viewC) em main_loop.inc. Versao 1.1.0.25rc. (VIS)

## patchFog.3
- Neblina mais atmosferica (shader, sky_cycle.inc): curva mais suave e cedo (pow 1.3), haze 0.16->0.34 com exp mais denso, mais neblina em vales (abaixo de y=74), longe esfria para azul (Rayleigh). Versao 1.1.0.26rc. (VIS)

## patchPerf.2
- Shaders (sky_cycle.inc): LOD de textura por distancia (bias de mip: 12 chunks=1 nivel/metade dos pixels, 16=2, 20=3, max 3; so caminho com shader); recorte alfa das folhas varia suave com a distancia (0.3 perto -> 0.1 longe, 6..56 blocos) em vez de estourar. Versao 1.1.0.27rc. NAO feito: geracao de chunks em thread (caches globais gBioTile/gBTCache/g112 nao sao thread-safe), occlusion/cave culling, malha compacta. (VIS)

## patchPerf.3
- Geracao de chunks ~3x mais rapida (14,3 -> 4,7 ms/chunk no bench, hash dos blocos IDENTICO => terrenos/saves iguais): terrain112.inc PerlinImp::grad por tabela sem ramos, floor/fade do eixo Y hoistados, ruido em O3 via pragma (build padrao e -O1). Bench: tests/bench_gen.cpp (imprime hash + ms/chunk). Versao 1.1.0.28rc. Proximo: gen em thread, malha (build) e GLStack::get.

## patchJungle.1 + patchPerf.4
- Selva 1.12.2: WorldGenMegaJungle (tronco 2x2, copa larga, galhos, vinhas; altura 10+nextInt(3)+nextInt(20)) em trees_mc.inc; BiomeJungle.getRandomTreeFeature (1/10 carvalho grande, 1/2 arbusto de selva c/ folha de carvalho, 1/3 gigante, resto selva pequena); vinhas no tronco e penduradas nas folhas (WorldGenTrees vines); treePut aceita meta (vinha) e o spill guarda a meta (worldgen.inc). Copa da gigante: createCrown(2) = camadas 5/4/3 (lembrado de memoria do 1.12.2, conferir com o vanilla). Sem cacau (nao existe o bloco). Teste: tests/test_jungle.cpp. (VIS)
- Luz e malha (world.inc) em O2 por pragma: malha 5,6 -> 3,8 ms/chunk no bench (tests/bench_mesh.cpp; compilar tudo em -O2 daria 2,5). Versao 1.1.0.29rc. Proximo: gen em thread, GLStack::get, culling de oclusao.

## patchVoice.1 (voice chat + criativo touch)
- Voice chat: `src/voice.inc` (novo). Mic SDL (8 kHz mono, quadros de 40 ms, u-law, porta de ruido), pacote `NP_VOICE` (NET_VER 41; host repassa), mistura em `sndCallback` (`voiceMix`), volume/pan por distancia (8..48 blocos). So o J1 (teclado) fala/ouve; split sem voz. V liga/desliga (PC); botao de microfone no HUD touch (`TA_VOICE`, `pMic`, ao lado do chat; aceso = ligado). Mic desliga sozinho ao sair da partida. Android pede RECORD_AUDIO por JNI (sem Java novo), mas o MANIFEST precisa declarar a permissao: `tools/android-voice/LEIA-ME.txt`.
- Criativo (touch/PE): com abas de mods (varias paginas) o topo reserva 44 unidades (`invLayout`), setas "<" ">" e "1 / N" deixam de cobrir o titulo; abas da direita 4 unidades mais a esquerda (`PE_RSHIFT`); botao X (`touchCloseRect`) 10 unidades mais a esquerda.
- Teste: `tests/test_voice.cpp` (1744 checks). (VIS): inventario touch, V/botao mic, voz entre 2 maquinas, Android real (permissao), Pi 3B. AINDA FALTA: controle de volume/limiar/dispositivo no menu de Configuracoes, indicador de "falando" no nome, /voice no chat, voz por dimensao.

## patchTex.1 (Android: textura legada apos reinstalar)
- Causa: `androidPrepare` (android.inc) pulava arquivo de MESMO TAMANHO e o carimbo do build (build-android.sh) usava so caminho+tamanho => textura nova com o mesmo tamanho nunca substituia a antiga; arquivos que o APK novo nao tem mais ficavam em assets/textures.
- Agora: carimbo = hash do CONTEUDO dos assets + identidade do APK instalado (mtime+tamanho de base.apk via JNI getPackageCodePath) => reinstalar/atualizar SEMPRE revalida; arquivo so e pulado se tamanho E conteudo forem iguais; ao revalidar, apaga texturas de assets/textures/ que nao existem mais no APK (resourcepacks, skins, mods, saves e options ficam). (VIS: celular que ainda tem a textura antiga; limpar uma vez "Android/data/<pacote>" tambem resolve.)

## patchGen.1 (blocos invisiveis + gerador 1.12.2)
- Invisiveis: `build()`/`computeLight` usavam `chunkTop` em cache (`maxY`); blocos escritos direto em `c->b` (sem `setB`) acima do topo velho ficavam fora da malha = invisiveis com hitbox/contorno. Agora `chunkTopFresh` reescaneia o chunk e os 8 vizinhos a cada malha. (VIS: causa provavel; se o bloco do print continuar sumido, mande o id/coordenada com F3.)
- Biomas 1.12 (por id vanilla via `biomeRawAt`): floresta de betulas (27/28/155/156) so betula; floresta densa (29/157) 2/3 carvalho-escuro 2x2 (`mcDarkOak`, textura big_oak) e 20 arvores/chunk; taiga gigante (32/33/160/161) abeto gigante 2x2 (`mcMegaSpruce`) 1/3, 10 arvores/chunk, arvores tambem sobre podzol/terra grossa; mesa (37/38/39) cacto e arbusto morto na areia vermelha.
- Teste: `tests/test_gen112.cpp`. AINDA FALTA (1.12): cogumelos gigantes (ilha de cogumelo/floresta densa), espinhos de gelo (140) e gelo compacto, flores por bioma (floresta de flores 132, plains M), `test_biomes.cpp` esta defasado (GC_PLAINS/gBioFrom), arvores aproximadas (copa do carvalho-escuro e do abeto gigante nao sao 1:1), bioma "river/deep ocean/stone beach" sem decoracao propria.

## patchEndPortal.1 (portal do End = ceu de galaxia) — versao v1.1.0.30rc
- Antes: 4 lajes 16x16 no atlas com estrelas procedurais grandes (repetiam por bloco e nao pareciam o 1.12.2). Agora o End portal NAO entra na malha: `emitXB` (models.inc) so anota a celula (`endPortalNote`) e `drawEndPortals` (src/endportal.inc, chamado em main_loop.inc junto de drawPistonAnims) desenha: laje de fundo com `environment/end_sky.png` x(.13,.15,.20) + 12 camadas ADITIVAS (glBlendFunc ONE,ONE; sem escrever profundidade) com `entity/end_portal.png` (256x256), cada uma com escala/rotacao/deslocamento e cor propria (R .1-.6, G .4-.9, B .5-1, peso 5/(18-i), mesma semente 31100 do MC), empilhadas entre y+.45 e y+.75 => paralaxe. UV em coordenadas de MUNDO (sem repetir por bloco). So GL 1.x imediato: GL/GLES/Vulkan(vkgl) iguais. Sem o pack usa textura procedural.
- `kEndPortalLegacy` (nether.inc) = true volta ao portal antigo (tiles no atlas). Teste: `tests/test_endportal.cpp` (registro/mesh). Composicao conferida por simulacao em Python com as texturas reais (fundo azul-marinho quase preto, estrelas teal/roxas finas). (VIS: brilho, paralaxe, desempenho; ajustar pesos em endportal.inc se ficar escuro/claro demais.)
- Versao do jogo: BLOCKLAND_VERSION v1.1.0.30rc (cobre patchVoice.1/2, patchTex.1, patchGen.1 e este).

## patchSword.1 (v1.1.0.31rc)
- Criativo + espada na mao nao quebra bloco (jogador 1: main_loop.inc, extras do split: splitscreen.inc). So sintaxe verificada; test_split segfaulta tambem no codigo original neste ambiente (nao e regressao). (VIS)
- Pick block Android ja era momentaneo (patchTouch.1, touch.inc:86) — item 2 da fila fechado.
- Bump do J2 (recuo vindo do J1): lidos pvp.inc, hostile.inc, mobs.inc, splitscreen.inc, bow.inc — todas as origens usam a posicao/olho de quem bate; causa NAO encontrada, precisa reproduzir no jogo.

## patchPigLava.1 (v1.1.0.32rc)
- Mobs em lava: flag `MobEnt::inLava` (mobs.inc), arrasto 0,5 / gravidade -0,02 (handleLavaMovement), boia (swimming AI 80%/tick), sem dano de queda. Porco-zumbi nada na lava; demais mobs usam a mesma fisica (o fogo deles segue como antes). So sintaxe verificada (testes headless segfaultam neste ambiente). (VIS)

## patchMenuSnd.1 (v1.1.0.33rc)
- Clique (`ui.click`) ao tocar campo de texto, slider e linha de lista (screens.inc, pageEvent). Carrossel de skins ja tinha. So sintaxe verificada. (VIS)
- Codigo morto do launcher (`launcherOpen`, `gLFirst`, `skinStep`): NAO e morto — gLFirst/skinStep/launcherOpen ainda sao usados (screens.inc:412/478/757). Nao removido. A reconferir: toggle de mods por mod (fmt 5 em screens.inc, `gModsOff`) parece existir — validar no jogo se persiste e se o mod desligado some do inventario.

## patchSnd.1 (v1.1.0.34rc)
- Relatorio de sons faltando em `SONS_FALTANDO.md` (raiz). 11 arquivos referenciados nao existem (caverna, chuva, trovao, fogo, lava, dano).
- player.hurt/fall.small/fall.big e splash agora usam random/classic_hurt e random/splash (provisorio) em events.cfg + padrao embutido (audio.inc). So sintaxe verificada. (VIS)

## patchPvpOrigin.1 (v1.1.0.35rc, NET_VER 42)
- Bump do J2: no multiplayer o receptor calculava a origem do recuo pela posicao do jogador PRINCIPAL do atacante (remotes[from]), mesmo quando quem bateu era o extra do split. Agora NP_PVP leva (x,z) do olho de quem bate (pvp.inc pvpAttackK; flecha em bow.inc usa a posicao da flecha) e pvpReceive/split2PvpReceive usam isso. Formato mudou: NET_VER 41 -> 42 (todos precisam da mesma versao). So sintaxe verificada. (VIS)

## patchSplitBoat.1 + patchSplitChat.1 + patchSignUI.1 (v1.1.0.36rc)
- Bote para J2/J3/J4: `gBoatX[3]`/`gBoatXP[3]`/`gBX[3][5]` (boat.inc; `gBoat2` virou macro de gBoatX[0]); montar/dirigir/desmontar agachando/sair ao morrer por extra (splitscreen.inc); render e troca de viewport usam gBoatX[k] (main_loop.inc). Varios jogadores no mesmo bote: o motorista e o J1, senao o menor k. (VIS)
- Chat para J3/J4: `gChatK`/`gP2ChatReqK` (gamepad.inc); nick por extra (xNickK), teclado virtual/viewport do extra certo, so o dono do chat para de jogar (split2Act). Continua local quando o extra e convidado (host so aceita o nick do proprio convidado). (VIS)
- Placa: editor fechava no mesmo quadro (signTick no desenho + restauracao de gSignEd ~1574); agora signTick roda na logica. Editor usa a textura entity/sign.png (tabua 2,2-26,14 e poste 2,16-4,30; retangulo so se o pack nao tiver). So sintaxe verificada. (VIS)
- NAO coberto: J2..J4 colocarem placa/editarem texto (so o J1 abre o editor); android continua sem split.

## patchFallWeb.1 (v1.1.0.37rc)
- Teia zera a queda (Entity.setInWeb): `physInWeb` em physics.inc (mechSlow usa o mesmo helper) + survival.inc. Queda na agua/escada ja zerava (MLG ok pelo codigo). So sintaxe verificada. (VIS)
- Auditoria: toggle de mods por mod JA existe (options.cfg `modsoff=`, mods.inc:44, props.inc:112, tela de mods; vale ao reiniciar). Codigo do launcher (`launcherOpen`/`gLFirst`/`skinStep`) NAO e morto.

## patchMob.5 (v1.1.0.38rc) — aranha removida
- spawnMob ignora M_SPIDER/M_CSPIDER (ovo, /summon, spawn natural, rede); spawn noturno troca aranha por esqueleto; ovos continuam no criativo (so nao spawnam nada ate o mob novo). Enum/ids mantidos (rede/save). VIS.

## patchMob.6 (v1.1.0.39rc) — esqueleto foge do sol
- hostile.inc: skyExposed/skelFleeSun; de dia (sem chuva) o esqueleto exposto ao ceu corre ate um ponto coberto (raio 12, relatado a cada 1 s) antes de atacar; sem sombra por perto segue a IA normal; MobEnt.sunFlee em mobs.inc. VIS.

## patchPvp.1 (v1.1.0.40rc) — pérola do End
- egg.inc: cooldown de 1 s so para a perola (ovo segue 0,5 s); pearlFx = 32 particulas roxas + evento pearl.tp na origem e no destino do teleporte; events.cfg: pearl.tp (placeholder random/pop grave, falta o .ogg do enderman). Ovo ja nao causa dano (so empurra). Falta: ovo/perola acertando jogador remoto. VIS.

## patchPvp.2 (v1.1.0.41rc) — perola do End arremessavel
- BUG achado: egg.inc pearlId() usava pkFind (gPkByFile so indexa "block/..."), entao a perola nunca era reconhecida (nao arremessava nem teleportava). Agora usa mobPkItem e nao cacheia 0. VIS.

## patchFix.1 (v1.1.0.42rc) — pkFind("item/...")
- worldgen.inc: pkFind agora acha itens do pack (antes so "block/..."): conserta receitas de relogio/bussola (chest.inc), bola de fogo do ghast/blaze (hostile.inc), loot dos baus da fortaleza (sela, espada de ouro, isqueiro, armadura de cavalo; nether_fort.inc) e verruga do Nether (redstone.inc). VIS.

## patchMob.7 (v1.1.0.43rc) — canto dos mobs
- mobs.inc mobPush: variacao de velocidade horizontal por tick causada pelo empurrao entre entidades limitada a 0,15 (jogador e cada mob); a soma de varios mobs empilhados nao arremessa mais. Empurrao de 1 par (0,05+0,05) fica igual ao vanilla. Falta: so pushPair; knockback/explosao nao passam por aqui. VIS.

## patchMech.2 (v1.1.0.44rc) — cacto e tesoura
- drops.inc: item caido em cima/dentro/ao lado do cacto some; falling.inc: gancho do cacto quebra (e solta o item) se a base some ou se um bloco solido fica colado nele (nao confere o tipo da base: areia vermelha ok); item_data.inc: tesoura empilha ate 64 (so o stack: tosquiar ovelha NAO existe; ovelha sem estado tosquiado). Falta: tosquiar/cortar folhas-teia-la, fogo sem particulas + fumaca ao apagar. VIS.

## patchFire.1 (v1.1.0.45rc) — apagar fogo
- nether.inc fireSmoke: fizz (evento fire.extinguish) + 10 fumacas subindo ao apagar o fogo por agua acima ou soco (main_loop breakAt). Fogo ja NAO tem particulas (nao existe emissor); nao mexi no modelo. VIS.

## patchWx.9 (v1.1.0.46rc) — chuva esconde nuvens
- mining_sky.inc drawClouds: com chuva as nuvens 3D vao sumindo em 8 passos (alpha*(1-gRain)) e ficam um pouco cinza; gRain>=0,94 nao desenha; transicao suave porque gRain ja e interpolado em weather.inc. O resto do clima (chuva densa, ceu cinza, raio com dano/fogo, trovao) JA existia; falta raio segmentado/ramificado e fog mais denso. VIS.

## patchCmd.1 (v1.1.0.47rc) — /fill
- chat_debug.inc: /fill (alias /preencher) x1 y1 z1 x2 y2 z2 <bloco> [replace [filtro] | hollow | outline | keep | destroy]; ~ relativo; max 32768; ar/air; so op em MP; chunks nao carregados ignorados; blocos de meta/estado (degrau, escada) saem com meta 0. Falta: /setblock, /clone, estado de bloco. VIS.

## patchEnd.13 (v1.1.0.48rc) — portal de saida do End
- dragon.inc dragonEggSpawn(first): fonte de bedrock (raio 3,5) + portal de saida (XB_PORTAL meta 2, raio 2,5; entrar leva ao spawn via endTravel ja existente) + pilar central de bedrock; ovo so na 1a morte (XP 12000 ja existia). Sincroniza por netSendBlock. VIS. Falta: tochas no pilar, creditos/poem, respawn do dragao por cristais.

## patchEnd.14 (v1.1.0.49rc) — creditos apos matar o dragao
- dragon.inc: creditsStart/drawCredits/creditsSkip (texto proprio em PT, rolagem sobre fundo preto); dispara so na 1a morte; Esc/Enter pula; ao fim dragonTick chama endTravel (volta ao spawn). main_loop.inc: tecla (antes do pauseOpen), uiMode inclui gCredits, desenho apos a tela de morte. VIS. Falta: poema do End (nao copiado por ser protegido), conquista "O Fim", clique/gamepad pulando, creditos no convidado MP (so quem roda dragonTick ve).

## patchCmd.2 (v1.1.0.50rc) — /setblock e /clone
- chat_debug.inc: cmdBlockArg (bloco ou bloco:meta 0..15); /setblock x y z <bloco[:meta]> [replace|keep|destroy]; /clone x1 y1 z1 x2 y2 z2 dx dy dz [replace|masked] (copia id+meta via pilha, aceita sobreposicao, max 32768, ~ relativo). So op em MP, entram no autocompletar. VIS. Falta: estado simbolico `[facing=north]` e meta no /fill, /clone com filtro/move, dimensao.

## patchFix.2 (v1.1.0.51rc) — /fill com meta + neblina na chuva
- chat_debug.inc: /fill aceita bloco:meta (usa cmdBlockArg; meta so no bloco novo, nao no filtro). main_loop.inc: neblina (inicio e fim) encolhe ate 50% com gRain no Overworld (piso 24). VIS. Falta: estado simbolico [facing=..].

## patchWx.10 (v1.1.0.52rc) — raio estilo Minecraft
- weather.inc wxDraw: raio com 40 passos aleatorios (RNG proprio por gBoltSeed, igual no convidado; nao mexe mais em srand/rand), 3 camadas de brilho aditivo, ate 4 ramos laterais e fim ancorado no ponto de impacto. Dano/fogo/trovao inalterados. VIS. Falta: fog/flash do ceu mais forte, conferir sombras de nuvem na chuva.

## patchEnd.15 + patchWx.11 (v1.1.0.53rc)
- main_loop.inc: clique do mouse (e A do gamepad, que vira clique) pula os creditos do End. weather.inc: flash do raio mais forte (cor do ceu .55->.8, escala do ceu .4->.6). VIS. GUI scale x4 ja existia (v6 0..4), nao mexi. Tochas do pilar do End adiadas: orientacao dos ids XB_TORCH+1..4 nao conferida.

## patchEnd.16 (v1.1.0.54rc) — cristal do End com o modelo do Minecraft
- dragon.inc crystalsDraw: ModelEnderCrystal 1.12.2 em textura 64x32 (base 12x4x12, vidro 8x8x8 duas vezes a 0.875, nucleo), escala 2, giro f*3, sobe/desce f1, inclinacao 60 graus em (.7071,0,.7071), como RenderEnderCrystal. Centro do cristal = topo do bedrock+2: hitbox (crystalHit, sph) e inicio do raio de cura ajustados. Altura/orientacao da base conferidas so por conta (derivacao do codigo vanilla). VIS. Falta: iluminacao do modelo pelo bloco, raio de cura no mesmo ponto vanilla.

## patchPhys.1 (v1.1.0.55rc) — areia/cascalho nao cospe o jogador
- physics.inc: gNoLiftUntil + modo "soft" em unstuckPlayer: depois que um bloco caindo pousa (falling.inc fallTickAll seta 60 s), ate o jogador ficar livre so ha empurrao LATERAL (passo .05/quadro, como pushOutOfBlocks); sem saida lateral = sufoca no lugar (nunca sobe). tp/respawn fora dessa janela continuam subindo a coluna. Limite conhecido: um tp para dentro de bloco ate 60 s apos qualquer pouso tambem so empurra de lado. VIS. Colisao com blocos nao-solidos ja estava correta no codigo (blockshape.inc: ar/agua/XB sem colisao; pack = cubo por padrao): a "trava de entrar em blocos" nao foi reproduzida sem o jogo; precisa de exemplo (qual bloco, onde).

## patchWater.1 (v1.1.0.56rc) — agua escorre na borda de chunk novo
- world.inc: waterEdgeWake chamado ao fim de gen(): para cada borda com vizinho carregado, agua encostada em ar (mesma altura, y 1..127) entra na fila da agua (waterSchedule) e flui para o espaco. Antes o fluxo so acordava por troca de bloco vizinho. Levantamento: observador, comparador e repetidor JA existem (redstone.inc: rsObsTick, rsCompTick, XB_REPT) ao contrario do que o prompt diz; falta auditar clock/lampada. VIS: conferir que nao inunda bordas de mar/lago nem pesa ao explorar.

## patchCrop.1 (v1.1.0.57rc) — terra molhada e crescimento do vanilla
- crops.inc: XB_FARM meta bit0 = caminho (igual antes), bits1..3 = umidade 0..7. farmTick (BlockFarmland.updateTick): agua num raio 4 (x/z) e y..y+1, ou chuva com ceu aberto -> 7; sem agua cai 1 por tick aleatorio; 0 e sem plantacao em cima -> vira terra. cropRandomTick: crescimento por getGrowthChance (base 1, terra de baixo 1/3 molhada, vizinhas /4, fileira+coluna ou diagonal da mesma cultura /2; chance 1/(25/f+1)); verruga 1/10. redstone.inc: textura farmland_moist com umidade 7. Sem checagem de luz (>=9) ainda. VIS: conferir que a terra seca/molha e que nao vira terra de aldeia sem agua cedo demais (so quando amostrada, ~72 s).

## patchMP.1 (v1.1.0.58rc) — mundo novo do host nao herda mais o estado do anterior (convidado)
- Causa raiz: o convidado sempre carrega o save temporario `saves/_mp` e netPrepareGuestWorld (netlan.inc) apagava so `chunks16.dat`; chests.dat, furnaces.dat, props.dat, signs.dat, boats/carts, player (inventario), chunks16n/e.dat, dim.txt e dragon.dat do host anterior eram relidos no mundo novo. Correcao: `fs::remove_all("saves/_mp")` + recriar antes de gravar o level.cfg. Sem mudanca de pacote (NET_VER segue 42). Lado do host: resetWorldData + loaders (cada um limpa o seu container) e netStop ja limpam o estado; nada a mudar. VIS: host abre mundo A, convidado entra e pega item/abre bau; host sai e abre mundo B; convidado reentra: inventario inicial, sem baus/props do A. Obs.: inventario do convidado nao persiste entre sessoes no MP (comportamento novo, igual ao vanilla sem save por UUID).

## patchShear.1 (v1.1.0.59rc) — tesoura funcional
- inventory_ui.inc: shearsId() (acha "item/shears" em PK; nao congela 0), gShearBreak, durabilidade 238. survival.inc: toolWear gasta 1 ao quebrar bloco com tesoura.
- drops.inc blockDrops: com gShearBreak, folhas (isLeafB), teia, tallgrass/fern/dead_bush/vine soltam o proprio bloco (set em breakAt, main_loop.inc).
- mobs.inc: MobEnt.sheared; ovelha tosquiada nao desenha a camada de la, nao solta la ao morrer e volta a ter la (1/1000 por tick, em cima de grama). main_loop.inc shearUse: botao direito na ovelha (raio vs AABB, bloco na frente bloqueia): 1-3 la, gasta 1, som de la.
- Limites: convidado nao tosquia (estado dos mobs vem do host; sem pacote); tosquia do host nao aparece nos convidados; ovelha sempre branca (sem cor) e tosquiada nao e salva; tesoura nao acelera a quebra de folha/teia/la; tesoura nao funciona no split (splitscreen.inc:167 chama blockDrops sem gShearBreak); vidro/trilho/pe de planta XB nao tratados. VIS: tosquiar ovelha, ver o corpo sem la, esperar rebrotar na grama; quebrar folha/teia com tesoura e ver o bloco no chao; barra de durabilidade.

## patchMLG.1 (v1.1.0.60rc) — amortecimento com agua (MLG)
- Causa: P.inWater e calculado no INICIO do tick (main_loop.inc:959, antes de mover). Ao pousar no mesmo tick em que a agua passou a existir sob o jogador (balde no ultimo instante) ou em agua rasa (1 bloco), o valor ainda era false e survival.inc cobrava a queda. Correcao (survival.inc, bloco de queda): `P.onGround && inWaterNow(P)` reavalia com a posicao nova e zera gFall, como o Entity.updateFallState do 1.12.2. O balde ja colocava a fonte na hora (bucketUse). Sem mudanca de rede. VIS: cair de 30 blocos e colocar balde de agua rente ao chao: 0 de dano; cair em poca de 1 bloco: 0 de dano; cair em chao seco: dano normal. Falta: mobs ainda nao tem amortecimento por agua rasa (nao auditado).

## patchEgg.1 (v1.1.0.61rc) — ovo e perola batem em jogadores
- egg.inc eggStep: caixa de colisao (largura .6 + folga .3, como os mobs) para jogadores remotos (remotes/remotesX, mesma dimensao, vivos, nao espectador) e, nas copias de ovo alheio (owner != -1), tambem para o jogador local e extras do split. Nunca bate no proprio dono. Ao bater: particulas; ovo do dono tenta chocar (eggHatch), perola do dono teleporta ate o ponto de impacto (gPearlTp). Sem dano nem recuo (EntityPlayer ignora dano 0 em 1.8.9/1.12.2). Sem pacote novo: NET_VER segue 42 (o dono decide; copias remotas so quebram). gPvpP movido de pvp.inc para egg.inc (forward).
- Limites: a copia remota e simulada com a posicao que este PC conhece do jogador, pode divergir um pouco do dono (so visual); ovo do P1 nao bate no P2 do proprio split. VIS: dois PCs: jogar ovo e perola em cima do outro jogador (quebra nele, perola leva o dono ate la); o alvo ve o ovo sumir nele; ovo e perola continuam atingindo mobs e blocos.

## patchBaby.1 (v1.1.0.62rc) — mobs bebes (porco, galinha, vaca, ovelha, mooshroom)
- mobs.inc: MobEnt.baby/babyAge; mobW(e)/mobH(e) (metade) substituem MD[x.type].w/.h em todos os usos (9 arquivos: colisao, mira, flecha, ovo, tesoura, balde...); spawnMob(..., baby=false); render com glScalef(.5) (cabeca nao e maior como no vanilla); cresce em 24000 ticks; mobDrops: filhote nao solta nada; galinha filhote nao poe ovo.
- Origem: ovo de galinha que choca (egg.inc) = pintinho; spawn natural de animais (hostile.inc) 10% filhote. Balde nao ordenha vaca filhote; tesoura nao tosquia ovelha filhote.
- Rede SEM subir NET_VER: NP_MOBS usa os bits 8 (filhote) e 16 (tosquiada) do byte de flags (mobsync.inc); convidado agora VE tosquia e filhote do host (fecha o limite do patchShear.1 de exibicao; convidado ainda nao tosquia).
- Falta: reproduzir com trigo (corações, filhote), filhote segue a mae, cabeca proporcional, XP, som agudo, salvar filhote (mobs nao sao salvos), ovo de spawn de filhote. VIS: ovo de galinha (1/8) gera pintinho pequeno; animais pequenos nascem as vezes; filhote some drop; host+convidado veem o mesmo tamanho.

## patchSponge.1 (v1.1.0.63rc) — esponja molha e para de absorver
- falling.inc spongeAbsorb: (1) so esponja SECA (meta 0) absorve; (2) trava de reentrada gSpongeBusy (cada agua removida disparava o gancho neighbor da propria esponja, ainda seca, que recomecava a absorver em cadeia); (3) se o pack nao tiver bloco molhado, a propria esponja vira meta 1 (molhada, nao absorve mais) em vez de ficar seca para sempre; (4) busca tambem "block/sponge_wet". Causa exata nao reproduzida sem o jogo: as 3 hipoteses (id molhado 0, reentrada, esponja seca reabsorvendo) ficam todas cobertas. VIS: colocar esponja na agua = textura molhada e a agua some UMA vez; a agua ao redor nao e mais sugada; esponja seca perto de agua corrente absorve ao receber atualizacao. Falta: esponja molhada solta o item seco ao quebrar (meta perdida), secar no Nether/fornalha ja existe so na fornalha, textura da meta 1 igual a seca (so ocorre sem bloco molhado no pack).

## patchRpiOpt.1
Pi: sem neblina, folhas sempre opacas (reaplica se atlas recarregar), luz plana, split desativado, padrao 4 chunks, 1 chunk gerado+1 malha por quadro, pre-decode de sons. Compilacao NAO testada (sem SDL/GL no ambiente). (VIS) FPS no Pi.

## patchRpiOpt.2
Pi: gerar chunk e malhar em quadros alternados, orcamento max 8 ms. Thread de chunks NAO feita (globais W/getC/gMeshFar nao sao thread-safe).

## patchRpiOpt.3
Pi: worldgen/trees/genImpl em O2. (anel R+1 testado e descartado: luz precisa dos 8 vizinhos)

## patchRpiOpt.4
Pi: sem glFinish (env BLOCKLAND_GLFINISH=1 liga), mip NEAREST_MIPMAP_NEAREST, [PERF2] ceu/entidades/hud a cada 5 s. Corrigido crash do patch 1 (folhas: 3 faces, nao 6).

## patchRpiOpt.5
Pi: neblina removida de verdade (shader enxuto usava gl_Fog mesmo com GL_FOG off: start/end enormes fora d agua), sem faixa de nascer/por do sol (sky_cycle.inc) e sem god rays.

## patchRpiOpt.6
Pi: janela 400x250 (BL_WIN_*), mobs/itens alem do raio de render nao desenham nem simulam (mobs: so SP), spawn menos (animais 6, hostis 5), UI small p/ altura<300, PERF2 ticks.

## patchRpiOpt.7
Pi: sem anim/fade de chunks, grama longe verde, UI escala 1, inventario mobile, gamepad generico auto-mapeado, Chisel no Pi (BLOCKLAND_NOCHISEL=1 desliga), chat sem duplicar com OSK, Ctrl+A (placa/busca/campos), ceu OFF mantem sol/lua, tela cheia 400x250 (upscale bilinear, mouse escalado), carga de texturas do pack em 4 threads.

## patchPad.9
Gamepad: varredura de controles ja conectados 1x/s (splash.inc drenava os eventos de conexao; so funcionava replugando).

## patchRpiOpt.8
Pi: fog linear estilo Alpha (25%..100% do alcance), sem fantasmas de chunk nem agua animada, inventario enche a tela (resolucao interna reduzida por fator ao abrir: gLowF/invFitFactor, UI escala 1). Inclui patchPad.9.

## patchRpi.3
Build Pi: padrao cortex-a7/neon-vfpv4 + RPI_DOCKER em scripts/build-rpi.sh; Android: touch reseta ao voltar do segundo plano; Pi: resolucao interna ~100k px adaptada a proporcao (gBW/gBH), grama fast, inventario mobile com cursor quando ha controle (gInvPad), malha urgente ao quebrar bloco, cursor do controle reescalado 2x corrigido, direcional 4 sentidos nos menus, gamma alto (ltab), hotbar mais alta. (VIS)

## patchRpi.4
Cursor do controle gruda nos botoes de menu (gBtnReg em uButton/pauseButton; gpUiFrame usa gBtnSnap quando nao ha slots). (VIS)

## patchRpi.5
Pi: sem particulas (addPart/drawParts); Ceu OFF + Sol/Lua ON sem halos e com blend screen; audio tenta outros dispositivos/drivers; flags -fno-math-errno -fno-trapping-math + gc-sections. (VIS)

## patchRpi.6
getC com cache de 16 entradas (getCReset em menu.inc ao W.clear); ltab/ltabB com tabela para niveis inteiros. (VIS: FPS/tempo de malha no Pi)

## patchRpi.7
Pi: sem halos/camadas extras do ceu (modo imediato), domo 12 segmentos; luz: pula ar na varredura de emissores. (VIS)

## patchRpi.8
Pi: cropRandomTick 48 amostras, 12 vozes de audio, flags sem stack-protector/fortify + unroll. (VIS)

## patchRpi.9
Pi: distancia de render automatica (perf.inc/main_loop.inc): <25 FPS com GPU no limite = -1 chunk (max -3, min 2), >=32 FPS por 3 janelas = +1. (VIS)

## patchBuild.1 (v1.1.0.64rc)
- Quebrar bloco: overlay de rachadura igual ao vanilla (`drawCrack`, mining_sky.inc): blend (DST_COLOR, SRC_COLOR), cor (1,1,1,.5), alpha test > 0.1, polygon offset -3 (antes SRC_ALPHA = linha branca). (VIS)
- Pi: quebrar bloco refazia 9 chunks (luz dos 8 vizinhos) e o Pi malhava todos com urgencia. Agora so vizinhos que a luz alcanca (Manhattan <= 15, `setBM` world.inc) e urgencia so no chunk do bloco (`gUrgCx/gUrgCz`, main_loop.inc). Medido no x86: build de chunk = 2,6 ms (luz 1,7). (VIS no Pi)
- Build reorganizado: tudo em `build/` (build.sh, linux.sh, windows.sh, android.sh, rpi.sh, lib.sh, installer/, libs/, instalar-dependencias.sh); `./build.sh` na raiz e atalho. Saida SEMPRE empacotada em `executaveis/`: Blockland-setup-windows-{64,32}bits.exe, Blockland-linux-{64,32}bits.AppImage, Blockland-raspberrypi-32bits.AppImage, Blockland-android-{64,32}bits.apk. Flags: --android64/32 --windows64/32 --linux64/32 --rpi --all (+ --android/--windows/--linux = 64+32). Nada foi executado aqui alem de `bash -n`, apply.py e AppImage simulado (sem SDL/mingw/docker/NDK). (VIS)
- Android: manifesto/recursos do projeto em `data/android/` (manifest.cfg + overlay/ + apply.py, chamado depois do android/setup.sh); a pasta android/ nao precisa ir no zip.
- tests/stubs/SDL.h: declaracoes de joystick que gamepad.inc usa. `test_split.cpp` nao compila (gBoat2P/gB2 inexistentes: teste defasado do patchSplit.1).

## patchPane.3
- Painel/barras sem conexão = só o poste 2x2 (modelo e colisão/seleção; antes cruz inteira); dá pra colocar painel em cima de painel encostado no jogador.
- Contorno de seleção: preto 40% (alpha .4, 2px), como o drawSelectionBox do MC (`wireBox`/`wireShape`, `player_render.inc`). (VIS)

## patchRpiCull.1
- Culling por frustum de entidades (`entOutView`, `gFrs/gFrsOn` em `frustum.inc`; preenchido no main_loop, inclui reflexo): mobs, itens, botes, carrinhos e props fora da tela não desenham (antes só corte por distância). Desliga com BLOCKLAND_NOCULL=1 e na passada de sombras de entidade. (VIS: nada some ao girar a câmera/ nos cantos da tela, mob grande/ghast ok)
- NÃO resolvido: micro lag ao quebrar bloco no Pi (remalha do chunk inteiro + luz 48x48, ~1 malha urgente + vizinhos). Próximo passo: medir com [PERF] no Pi e remalhar só a seção 16x16x16 alterada.

## patchBreak.3
- Pi: `sndPrewarm` (audio.inc) agora decodifica TODOS os `block.*`/`step.*` (até 220 arquivos) antes dos outros comuns; antes eram 40 arquivos em ordem aleatória, então quebrar/colocar um material novo decodificava o .ogg na hora (60-190 ms = micro lag). Compila com -DBL_RPI. (VIS no Pi: 1ª quebra de cada material sem travada; loading um pouco mais longo)
- Se ainda travar: causa seria a malha urgente (build+luz do chunk, ~12 ms de orçamento); próximo passo = remalhar só a seção 16^3 alterada.

## patchRpiCull.2
- `entBlobShadow` e `torchFootShadow` (player_render.inc): não desenham fora da tela (`entOutView`); `entBlobShadow` não chama mais `glGetIntegerv(GL_TEXTURE_BINDING_2D)` (consulta que sincroniza CPU/GPU, 1x por mob por quadro; agora restaura o atlas). (VIS: sombra sob os pés some/aparece certo, textura do chão ok após sombras)
- Ideias NÃO feitas (risco sem Pi): GL_CULL_FACE no terreno (plantas/painéis são 2 faces), limitar uploads de VBO por quadro (draw usa nv novo com VBO velho), remalha por seção 16^3.

## patchBFC.1
- Corte de faces traseiras (GL_CULL_FACE) no terreno sólido, **só no Pi** (`BL_RPI`, desliga com `BLOCKLAND_NOBFC=1`; no PC liga com `BLOCKLAND_BFC=1`; Android/Vulkan não usam). Sem reflexo da água (cena espelhada).
- Malha sólida do chunk dividida: `[0,cubeN)` cubos/lava (1 face, com culling) + `[cubeN,nv)` modelos de 2 faces (plantas, painéis, trilhos... = tudo que passa por `emitXB`/livro encantado) desenhados SEM culling; `Chunk::cubeN/xOff[17]`; frustum por seção vale para os dois trechos (`bindDraw`, main_loop.inc). +1 chamada de desenho por chunk.
- A tabela `Q` do mesher tem topo/base em ordem CW vista de fora (laterais CCW): topo/base invertidos no `quad()`, no topo da lava e no olho do Ender. Base da lava já estava certa.
- `tests/test_bfc.cpp` (22 checks: 1119 quads de cubo todos voltados para fora, painel no 2º trecho, offsets coerentes). `tests/stubs/SDL_opengl.h` ganhou glCullFace/glFrontFace.
- (VIS no Pi) nada some: topo/base/lados do terreno, lava (topo, base, lados), folhas, cacto, olho do Ender; plantas/painéis/tochas visíveis dos 2 lados; água/reflexo normais; FPS.

## patchRpiBuild.1
build/rpi.sh: compile() terminava com status 1 (teste && da checagem de glibc) e o set -e abortava o build mesmo com binario gerado -> return 0. Flags: -mtune=cortex-a53 -fno-semantic-interposition -fvisibility=hidden. (VIS) desempenho no Pi nao medido.

## patchRpiPerf.1
Base: logs do Pi (desenho chunks ~11 ms com so ~50 mil vertices; hud 4-5 ms). Pi usa indices de 16 bits nos quads (era uint32 => conversao na CPU do Mesa/VC4 a cada draw; main_loop.inc QIdx/bindQuadIB). HUD: filtro do atlas trocado 1x por quadro em vez de 2x por icone (inventory_ui.inc gIconLin/iconFilterRestore; restaura apos drawHud). Sintaxe ok com e sem BL_RPI. (VIS) comparar [PERF] desenho/hud no Pi.

## patchRpiPerf.2
Pi: text() (font.inc) monta os retangulos dos glifos num buffer (gGlyphBuf) e desenha sombra+texto com 2 glDrawArrays (antes centenas de glVertex2f por string; fonte e feita de retangulos). Salva/restaura estado de arrays (glPushClientAttrib) e desliga VBO. So BL_RPI desktop GL. Sintaxe ok. (VIS) conferir texto igual (sombra, acentos) e [PERF2] hud.

## patchRpiPerf.3
Pi: terreno opaco sem discard (VC4 mantem early-Z). assets_atlas.inc: gTileCut/tileIsCut (tile com texel alpha<255). world.inc: quads de tiles recortados (folha, mato, overlay) vao para o 2o trecho da malha (xbQ, sem culling). sky_cycle.inc: FSPiO (sem discard), gSkyProgOpq, rpiSolidProg(). main_loop.inc bindDraw: cubos com programa opaco, 2o trecho e agua com o normal. So BL_RPI. Sintaxe ok com/sem. (VIS) conferir folhas/mato/grama lateral sem texel preto; FPS e [PERF] desenho.

## patchRpiPerf.4
Pi: drawChunks do solido em 2 passes (gPiPhase): 1 = todos os cubos opacos com o programa sem discard, 2 = todos os 2os trechos (recortados) com o normal; 1 troca de programa por quadro em vez de 2 por chunk (main_loop.inc, bindDraw/drawChunks). Fantasmas/fade e agua seguem fase 0. Sintaxe ok com/sem BL_RPI. (VIS) igual ao patch 3 visualmente; conferir [PERF] desenho.

## patchDeathDrop.1
Bug: ao morrer, o jogador coletava de volta os itens que acabara de soltar (corpo morto parado no mesmo lugar; o delay de coleta e 20 ticks). drops.inc: `tickDrops` nao coleta com `gDead`. Splitscreen ja tinha a guarda (`alive`).
- Teste: sintaxe ok com e sem -DBL_RPI. Teste manual: morrer em sobrevivencia com itens; eles devem ficar no chao.

## patchRpiOpt.7
Pi: os logs mostram 115 decodificacoes de .ogg na thread principal (~10 s no total, 50-170 ms cada: mob/cow, mob/pig...). audio.inc: depois do `sndPrewarm`, `sndPlay`/`sndPlayAt` usam `sndGetBg` (thread de fundo, so com -DBL_RPI); o som novo e pulado na 1a vez e toca nas seguintes. `sndAsyncPump()` publica os prontos 1x por quadro (main_loop.inc). Jukebox/chuva continuam sincronos.
- Teste: sintaxe ok com e sem -DBL_RPI. (VIS) no Pi: conferir que os [LAG] "audio decodificado" somem. Limite: travadas de 50-100 ms em multiplos de 16,7 ms sem causa no profiler nao foram mexidas aqui (ver patchRpiOpt.8).

## patchRpiOpt.8
Pi, so com -DBL_RPI onde indicado (os logs mostram travadas de ~95 ms 1x/s sem causa no profiler, HUD = ~4,5 ms/quadro):
- `main_loop.inc`: `gpScanAll()` (SDL_NumJoysticks + abrir controles) rodava a cada 1 s para sempre; agora 1x/s so nos 20 s iniciais e depois a cada 30 s (o hot-plug continua por evento).
- `menu.inc`: `savePlayer()` (autosave a cada 5 s) monta o texto na memoria e so grava no cartao SD se mudou.
- `audio.inc`: threads de fundo (musica e decodificador de sons) com prioridade baixa (`SDL_SetThreadPriority`) para nao disputar CPU com render/audio.
- `survival.inc`: coracoes e fome do HUD (ate ~40 quads, 1 glBegin/glEnd cada) em UM glDrawArrays (`survIconQ`/`survIconFlush`; mesmo padrao de `text()` em font.inc).
- `tests/stubs/SDL.h`: declara `SDL_SetThreadPriority` (existe no SDL2 real).
- Teste: sintaxe ok com e sem -DBL_RPI. (VIS) no Pi: coracoes/fome identicos ao antes (inclusive piscar ao levar dano e tremer com vida baixa); comparar `[PERF2] hud` (antes ~4,5 ms) e se as travadas de ~95 ms 1x/s sumiram.

## patchRpiOpt.9
Pi: o log travava o jogo. Nos logs enviados, dos 479 quadros "sem causa" (50-117 ms, `resto` alto) 338 vem logo apos uma linha `[LAG]` e 130 logo apos o bloco `[PERF]` (5 linhas a cada 5 s): `logPutLine` fazia `fflush` de TODA linha no terminal (stdout) e no cartao SD, e o custo caia no quadro seguinte (que gerava outro `[LAG]`: efeito cascata, ~1 por segundo).
- `logger.inc` (so -DBL_RPI): terminal INTERATIVO (tty) so recebe ERROR/FATAL (BLOCKLAND_LOGSTDOUT=1 volta a imprimir tudo; stdout redirecionado, como nos testes, recebe tudo); arquivo com flush no maximo 1x/s (`logFlushIfDue()`, chamado em `perfFrameEnd`) e na hora em ERROR/FATAL; crash report/saida seguem usando o anel de 48 linhas e `fclose`. Outros alvos: comportamento antigo.
- Teste: sintaxe ok com e sem -DBL_RPI. (VIS) no Pi: `[LAG] ... resto` sem causa deve sumir quase todo; se precisar ver o log no terminal, usar BLOCKLAND_LOGSTDOUT=1 (ou `tail -f logs/latest.log`).

## patchGenSlice.1 (geracao de chunk em etapas: sem pico de 90-150 ms)
Problema: `gen()` gerava um chunk inteiro num quadro so (88-150 ms no Pi nos logs). Agora o mesmo codigo roda em 14 etapas (`genStage`, world.inc), sem threads (mesmo resultado e mesma ordem em Linux/Windows/Android/Pi e no multiplayer).
- `world.inc`: `GenJob` + `genStage()` (etapas: 0 ruido do terreno, 1 preenchimento/superficie, 2 cavernas, 3 fortaleza, 4 alturas dos vizinhos (`floorOf`, 1 vizinho por passo), 5-7 populacoes {cx-1,cx}x{cz-1,cz} (3 primeiras), 8-10 4a populacao em 3 sub-etapas, 11 spill+encostas+lagos, 12 arvores, 13 decoracao+vilas+publicacao em W). O chunk so entra em `W` na ultima etapa.
- `gen()` continua sincrono (testes, mobsync do host, Nether/End, loading) e termina antes qualquer job em andamento. `stream(..., slice=true)` (so o jogo, ligado com `gGenSlice`: padrao no Pi e Android; `BLOCKLAND_GENSLICE=1/0` forca em qualquer plataforma) roda etapas ate 1,5x o orcamento do quadro e so inicia outro chunk quando o anterior acaba (1 job por vez => spill de arvores/`gFloor` na mesma ordem de antes).
- `resetWorldData()` aborta o job (`genJobAbort`); `dimSwitch()` termina o job antes de trocar de dimensao (`genJobFinishNow`). Mundo salvo/baixado do host (`savedC`), Nether, End e panorama seguem o caminho antigo (nao sao sliceable).
- `terrain112.inc`: `terrain112A/B` (terrain112() = A+B). `worldgen.inc`: `populatePart(part, sub, JR2*)` (populateChunk = todas as partes).
- Provas (tests/): `test_genslice.cpp`: mundo em etapas == mundo sincrono (hash, spill, chunk so aparece no fim, gen() no meio de um job, abort); `test_genstream.cpp`: `stream(slice=true)` == `stream(slice=false)`. `bench_gen.cpp` com o codigo novo = hash `a6b4b9d1794941e4`, o MESMO do codigo original (geracao bit a bit identica). Pior etapa no x86: ~3 ms contra ~5,5 ms do chunk inteiro (antes 100%); no Pi deve cair de 90-150 ms para ~25-40 ms no pior caso.
- `-DGEN_PROFILE` liga os contadores `gPopT` (tempo por secao da populacao) usados em test_genslice.
- Limites: (VIS) no Pi: carregar chunks andando rapido pode ficar um pouco mais lento que antes (menos pico, mesma carga ao longo de varios quadros); se faltar chunk, subir o fator 1,5 de `runJob` em `stream()`.

## patchGeral.1 (v1.1.0.65rc) — aba Geral das Configuracoes menos vazia
- `screens.inc` (`pageBuild`, categoria Geral): alem de Perfil e Idioma agora tem nome do jogador atual, **Jogo** (botao "Mods e conteudo..." abre a tela de mods), **Janela** (botao Tela cheia, so desktop; empurra a tecla F11, `pageAction` id `ps.fs`), **Atalhos** (F11, F12, F7, F8, V, M, N) e **Sobre** (versao, renderizador, distancia de renderizacao). Nenhuma opcao nova de jogo: so atalhos para o que ja existia.
- Testado: sintaxe ok com e sem -DBL_RPI. (VIS): conferir a aba Geral nas resolucoes pequenas (rolagem pela roda do mouse; no touch a rolagem por arrasto ainda e placeholder) e se o botao Tela cheia alterna e atualiza o texto.
- Falta: confirmar as teclas F7/F8/F12 no jogo (tiradas do codigo/historico), Android sem o botao de tela cheia.

## patchPerf.1 (v1.1.0.66rc) — "Modo desempenho": otimizacoes do Raspberry como opcao em qualquer plataforma
- `main.cpp`: `gPerf` (ativo na sessao; lido de `options.cfg` `perfmode=` em `perfModeLoad()`, 1a linha do `main`, antes de atlas/malhas) e `gPerfNext` (escolha salva para a proxima sessao). Com `-DBL_RPI` (`BL_PERF_LOCK 1`) `gPerf` e SEMPRE true e a opcao fica travada (so texto na tela). Macros `BL_SMOOTH_FORCE`, `BL_RPI_FASTGRASS`, `BL_CLOUD_DEF`, `BL_NO_FANCY` agora dependem de `gPerf` fora do Pi.
- Interface: Configuracoes > Video > "Desempenho" (`screens.inc`, id `ps.perf`; salva em `optSave`). Mudar vale ao REINICIAR.
- Virou runtime (`#ifdef BL_RPI` -> `gPerf`): sem luz suave/AO, folha so com face para o ar, folha opaca (atlas + `compass.inc`), grama de longe 1 quad, sem particulas, nuvens 2D (raio 14, sem pre-passe), domo do ceu com 12 segmentos, sem faixa de nascer/por do sol, sem god rays, sem agua animada, sem animacao/fade de chunk e sem fantasmas, culling de visao/chunk sem folga, neblina linear simples, `sFar`=2, spawn 5x5, mobs/itens longe congelados e nao desenhados, menos animais/hostis, sem luz dinamica de itens, sem sombras, sem panorama no titulo (fundo liso), distancia de render max 5, nuvens OFF por padrao, ajuste automatico da distancia de render por FPS (alvo ~30).
- NAO virou opcao (continua so com `-DBL_RPI`, e por que): atlas 1024x2048, janela/resolucao interna 400x250 e escala da UI/inventario (hardware/layout do Pi); shader enxuto do VC4 e programa sem `discard`/2o trecho da malha (`gTileCut`, `rpiSolidProg`); indices 16 bits; filtro NEAREST_MIPMAP_NEAREST do atlas; batching de texto/HUD por `glDrawArrays` (`font.inc`, `survival.inc`); audio (buffer 2048, 12 vozes, prewarm, thread de decodificacao, prioridade baixa); log com flush 1x/s; `glGetError`/`glFinish`; teclado virtual sempre; split desativado/4 chunks; crescimento de culturas pela metade (muda jogabilidade); `BLOCKLAND_NOCHISEL`.
- Testado: sintaxe ok com e sem `-DBL_RPI`; `test_genslice` e `test_update` passam (modo normal inalterado por construcao). `test_rpi_atlas` falha por falta do resource pack no sandbox. Nada foi rodado com `gPerf=true` fora do Pi. (VIS): ligar o modo no desktop, reiniciar e conferir o visual (nuvens 2D, folhas opacas, sem sombras) e FPS; no Pi, que a opcao aparece travada e nada mudou.
- Falta: decidir se audio/log/UI do Pi tambem viram opcao; versao Android (gPerf false por padrao; ajuste automatico de distancia la continua sempre ativo).

## patchMouse.1 (v1.1.0.67rc) — sensibilidade e inverter mouse (opcoes que existiam so na tela, sem efeito no jogo)
- `main_loop.inc` (SDL_MOUSEMOTION com `grab`): olhar usa `gsf("q1")*2` (0-200%, padrao 100%, minimo 10%) e `gsi("q2")` inverte o eixo vertical. Antes a tela antiga "Controles" tinha `k0`/`k1` que nada lia.
- `screens.inc`: ids `k0`/`k1` -> `q2`/`q1` (persistem em `options.cfg` via `okId`); Configuracoes > Controles ganhou a secao "Mouse" (Inverter mouse, Sensibilidade do mouse).
- Testado: sintaxe ok com e sem -DBL_RPI. (VIS): sensibilidade em 50/100/200%, inverter, persistir ao reabrir. Nao vale para o J2-J4 do split (controle/teclado proprios) nem para o toque.
- Falta: `k2` (Modo Touchscreen) e `k3` (Redefinir Teclas) da tela antiga continuam sem ligacao verificada; sensibilidade do gamepad (`gpsens`) nao persiste (id fora do `okId`).

## patchPerf.2 (v1.1.0.68rc) — efeitos individuais e sensibilidade do controle
- `screens.inc` (Video > "Efeitos"): **Particulas** (`d20`, `gOptParticles` em main.cpp, lido em `mining_sky.inc`; sincronizado em `gfxTogglesSync`), **God rays** (`d21`, `main_loop.inc`) e **Animacao de chunks** (`d22`, `chunkAnimOn` em `main_loop.inc`). Persistem em `options.cfg`. Com o Modo desempenho ativo os tres ficam desligados (aviso na tela); no Raspberry (`BL_PERF_LOCK`) a secao vira so texto.
- Controles: "Sensibilidade do controle" (`gpsens`) agora tambem na aba Controles e **persiste** (`okId`).
- Testado: sintaxe ok com e sem -DBL_RPI. (VIS): cada opcao desliga/liga o efeito na hora (chunk animation vale nos chunks novos), valores ficam ao reabrir.
- Falta: opcoes para agua animada/nuvens ja existem (Ambiente); sombras, luz suave e folhas opacas seguem por Qualidade/Modo desempenho; F3/F1 (dbg/hideHud) sao locais do `main` e nao tem toggle nas Configuracoes.

## patchCfg.2
- Configurações: páginas reorganizadas em 2 colunas (`gb` meia largura, `fw` slider largura total em `screens.inc`); removidos textos desnecessários (Jogador: Steve, atalhos, dica de mods, distância de render. duplicada). Compila + test_split. (VIS)

## patchCfg.3
- Configurações > Geral: "launcher" embutido (nome + área de soltar skin + boneco 3D à direita + carrossel de skins à direita), seguido de Jogo (idioma, mods, tela cheia, atualizar skins), Tela dividida (J2-J4, só com split) e Sobre. `geralLay` em `screens.inc`; `drawSkinDrop`/`drawSkinCarousel` compartilhados com a tela `peskin`. Nome salvo ao sair do Geral. Compila. (VIS)
- Falta: arrastar no boneco do Geral para girar (só gira sozinho); teste headless de `screens.inc` não existe.

## patchCfg.4
- Geral: arrastar no boneco gira (não rola; pausa o giro automático). Nova seção "Tela dividida" sempre visível (Jogadores Desligada/2/3/4, Divisão cima-baixo/lado a lado, atalho J2-J4). Ambiente: nova seção "Nether" (Efeitos do Nether `d17`, Fortalezas `fort` 0/15/33/60/100%). Compila. (VIS)

## patchWld.1
- Lista de mundos: botão "X" (excluir, abre a confirmação) em cada linha; direcional do controle navega pelas linhas e pelos "X" (`gBtnReg`) e rola a lista para mostrar a linha sob o cursor. "Fortalezas do Nether" saiu de Configurações > Ambiente e foi para Criar Mundo > Mais Opções (`fortsel`, `fortSelIdx`); "Efeitos do Nether" passou para Terreno. Compila. (VIS)

## patchCfg.5
- Nome do jogador: campo vazio mostra "Steve" em cinza (placeholder; nome continua vazio = padrão). Carrossel de skins: fundo azul trocado pelo mesmo fundo escuro translúcido + moldura do painel do boneco 3D (vale para Geral e tela de skins). Compila. (VIS)

## patchGPU.1
- Otimização sem perda de qualidade (o log mostrava GPU-bound: ~1,7 M vértices/quadro, "espera da GPU (swap)" 25 ms). (1) Culling de cavernas: conectividade por secao 16^3 calculada no `build` (`chunkConnCompute`, `world.inc`) + BFS de secoes a partir da camera por quadro (`main_loop.inc`); secoes inalcancaveis pela vista nao sao desenhadas (solido, agua e modelos; conservador). `BLOCKLAND_NOCAVE=1` desliga. (2) Culling de costas (BFC) ligado por padrao tambem no PC GL (antes so no Pi); `BLOCKLAND_NOBFC=1` desliga; desligado no passe de sombras. Compila; `test_cave` 14/14. (VIS) conferir: nenhum chunk/secao sumindo (principalmente ao abrir caverna, cavar, nadar, split), faces de blocos invertidas/ausentes, sombras (F7).
- Falta (ideias para mais FPS): vertice compacto (V tem 36 bytes: cor em float), oclusao por colina, LOD de chunks distantes.

## patchTree.1
- Pontinhos pretos nas folhas/mato: texels transparentes tinham RGB preto e o mip trilinear misturava esse preto. `bleedRGB` (assets_atlas.inc) espalha a cor dos texels opacos para os transparentes (nivel 0 e mips; alfa intacto). (VIS)
- Falta: forma das arvores (geracao), vertice compacto/LOD para FPS, picos de "resto" ~50 ms a cada ~20 s (causa nao achada).

## patchFps.1
- perf.inc: ajuste automatico da distancia de render agora tambem no PC (-1 chunk se <50 FPS e GPU no limite, ate -4; volta +1 apos 15 s a >=58 FPS). (VIS)
- Falta: vertice compacto (36->20 bytes), LOD de chunks distantes, picos de ~50 ms a cada ~20 s.

## patchFps.2
- gpScanAll (varredura de controles) so a cada 120 s depois dos 20 s iniciais (hot-plug ja vem por evento). Sondas [SONDA] no log se gpScanAll/savePlayer passarem de 4 ms, para achar os picos de "resto". (VIS)

## patchLeaf.1
- leafGfxApply reenviava os tiles de folha por compassUploadTile (sem bleed): o patchTree.1 nao valia para folhas. Agora compassUploadTile(tile, true) sangra a cor e, de longe (mip>=2, ~30+ blocos), a folha fica opaca e 18% mais escura (menos ruido/pisca e menos alpha-test). (VIS)
- Log pos-Fps.2: picos de resto sumiram (sem [SONDA]); FPS ~50 com -4 chunks, GPU no limite (~1,5 M vertices, swap 8-11 ms).
- Falta: vertice compacto, LIGAR LOD de chunks distantes, vegetacao baixa (mato/flores) tem muitos vertices.

## patchFps.3
- main_loop.inc bindDraw: LOD barato — chunks a >72 blocos do olho nao desenham o 2o trecho da malha (mato, flores, modelos de 2 faces); cubos e folhas continuam. Fora de agua/reflexo/sombra. (VIS)
- Falta: vertice compacto, LOD de geometria de cubos distantes.

## patchFps.4
- perf.inc/main_loop.inc: distancia do LOD de plantas (gFarSkipD) 72 blocos, cai para 48 quando o ajuste automatico ja cortou >=2 chunks. (VIS)
- Descartado: MSAA/vsync (ja desligados); shader de chunk ja e leve. Falta: vertice compacto, LOD de cubos, passe sem discard para blocos opacos (hoje so no Pi).

## patchLeaf.2
- world.inc: faces folha-folha (internas da copa) vao para o 2o trecho da malha (como plantas): de longe (>gFarSkipD, 48-72 blocos) so as faces expostas ao ar, opacas (mip>=2, patchLeaf.1) = culling; de perto copa cheia. Valido em PC/Android/Vulkan (Pi mantem o caminho proprio). (VIS)
- Ordem de desenho ja e perto->longe (early-Z ok); MSAA/vsync desligados; shader leve. Falta: vertice compacto, LIGAR LOD de cubos, passe sem discard no PC.

## patchFps.5
- mobs.inc/drops.inc: corte de entidades por distancia tambem no PC (mobs 80 blocos, itens no chao 48; o frustum ja era cortado). Vsync e MSAA ja eram desligados por padrao no PC (SetSwapInterval(0), sem atributos de multisample); Android/Pi mantem vsync por calor/limitador. (VIS)

## patchPanel.1 (paineis com culling, cacto so no Pi, icone de plataforma no nick)
- [x] `screens.inc`: Configuracoes — widget fora da area rolavel (y<28 ou >=sh-8) nem e desenhado; o que cruza a borda e recortado no pixel (scissor); hover nao vale fora da area
- [x] `world.inc`: otimizacao do cacto empilhado (sem topo e base internos) so com `BL_RPI`; fora do Pi so tira a base do de cima (sem pisca)
- [x] `assets/ui/platform/{windows,linux,android,raspberry}.png` (64x64); `net.inc`: `BL_PLAT`, `platOf`, `platTex`; plataforma no fim do NP_HELLO e no NP_ROSTER (**NET_VER 43**); icone 8x8 na nametag e na lista Tab (`playerlist.inc`)
- Testes: sintaxe ok; test_conn 33/0; test_playerlist igual ao original (5 falhas ja existentes)
- [ ] (VIS) botoes cortados nas Configuracoes; icones na nametag/Tab; icone na tela inicial/pausa ainda nao

## patchPanel.2 (painel de vidro: uma camada so)
- [x] `redstone.inc` (casos 19/22): o braco do painel/barras era uma caixa de 2px com frente e fundo (duas camadas sobrepostas pela transparencia); agora e 1 plano em x=8 (borda ate o centro, visivel dos dois lados) + faixa fina de cima; poste so como tampa quando ha conexao; sem conexao continua o poste de 4 faces (item/cruz). A faixa de cima ainda some com painel em cima (`models.inc`)
- Testes: sintaxe ok; test_pane 8/8, test_redstone igual ao original (pack ausente)
- [ ] (VIS) painel visto de frente e de tras, cantos, fim de parede, coluna empilhada, barras de ferro, colorido; iluminacao/sombra nas faces (a face unica pega o sombreamento de um lado so)

## patchPanel.4 (modelo do painel de vidro refeito)
- [x] `redstone.inc` (casos 19/22): cada braco = 1 folha fina sem rotacao e com 1 face (N/S em x=8 face leste; L/O em z=8 face sul), UV automatica do MC => textura continua pelo bloco (o video mostrava cada metade espelhada: faces de frente e fundo no mesmo plano); faixa fina de cima por braco + tampa do poste; sem conexao = poste 2x2 de 4 faces
- Testes: sintaxe ok; test_pane 8/8
- [ ] (VIS) por tras aparece a imagem espelhada inteira (esperado); sem face de ponta no fim da parede (2px vanilla); sombreamento so de um lado; barras de ferro/coloridos

## patchSeam.1 (linhas rosadas nas bordas dos blocos + ajuste de distancia)
- world.inc: UVs dos quads de bloco com margem (UVIN, .004 do tile): a amostragem na borda exata pegava a linha do tile vizinho do atlas (pontinhos rosados/claros no topo das laterais). (VIS)
- perf.inc: ajuste automatico de distancia so corta se GPU/desenho >= 40% do quadro; antes cortava ate -4 chunks por travadas de swap/driver (DRI_PRIME) sem ganho.
- Nao resolvido (log): picos de ~50 ms em "resto (fisica, mobs, UI)" a cada ~10 s e espera de swap de 100-200 ms (hibrida AMD + Wayland). Investigar com --perf se persistir.

## patchSeam.2 (carga do Chisel em paralelo)
- asset_manager.inc `loadChisel`: pre-passada lista os tiles unicos e decodifica os PNGs em ate 4 threads (como o resto do pack); antes eram ~2000 PNGs em serie (~12 s da carga). Resultado identico (mesma ordem de ids). (VIS: conferir tempo de "Resource pack carregado" no log.)

## patchSeam.3 (sonda dos picos de "resto")
- perf.inc `PROBE`/`probeReport` + checkpoints em main_loop.inc (inicio, antes-mobs, antes-fluidos, antes-hud, antes-swap, fim). Quadro > 40 ms dentro do loop gera `[SONDA2] ... trecho mais lento: A -> B = X ms`. Se houver [LAG] sem [SONDA2], o tempo foi FORA do corpo do loop (espera de evento/limitador/compositor). Nao muda comportamento. (VIS: mandar o log.)

## patchSeam.4 (listras coloridas no terreno distante)
- assets_atlas.inc: anisotropia desligada no atlas (causa das listras: filtro alongado cruza a borda do tile do atlas). Mipmap trilinear (NEAREST_MIPMAP_LINEAR, 4 niveis) continua; MSAA segue pelo options.cfg.
- world.inc: margem de UV 0,02 do tile (MSAA extrapola UV na borda). (VIS) Solucao definitiva futura: gutter (borda duplicada) de 1 texel por tile no atlas.

## patchSeam.5 (performance: geracao em fatias no PC, ajuste de distancia, sombras so de perto)
- main_loop.inc: `gGenSlice = true` em todas as plataformas (log: 1 chunk gerado = 63-95 ms de travada no PC).
- perf.inc: `gVsyncOn`; com vsync ligado a espera do swap NAO conta como GPU no limite (log: swap 12-17 ms = ritmo do compositor; derrubava a distancia -4 sem ganho). chat_debug.inc atualiza em /vsync.
- sky_cycle.inc: sombras/tintura de sol so ate ~90 de distancia (fade 48-90), antes ate 120: menos custo no fragment shader e menos vazamento de luz nas bordas (suspeita das listras amarelas/rosadas, que sao COR de sol/lua e nao de textura). (VIS)
- test_split.cpp usa nomes antigos (gBoat2P, gB2): nao compila desde o patchSplit.1 (nao e deste patch). test_mech 9/9 ok.

## patchSeam.6 (Wayland: limitador no lugar do vsync; sonda de etapa de geracao)
- main_loop.inc: no Linux/Wayland (PC) usa swap interval 0 + limitador na taxa do monitor (gSoftPace/gSoftHz em perf.inc); log: swap esperava 15-50 ms por quadro com vsync do driver (PRIME). /vsync 1 ou BLOCKLAND_VSYNC=1 voltam ao vsync do driver. (VIS)
- world.inc: `[GEN]` loga etapas de geracao > 25 ms (uma etapa ainda leva ~46 ms).
- Listras: reduzidas pelo patchSeam.5 (confirmado pelo usuario).

## patchCraft.3 (receitas completas + qualquer madeira) / patchSurv.1 (fontes de itens) / patchGfx.2 (nitidez)
- `craft_extra.inc` (novo, incluido antes de chest.inc; chamado no fim de `packRecipes`): ~150 receitas da 1.12.2 que faltavam, com ids pelo NOME do pack (aliases 1.12/1.13; ingrediente/resultado ausente = receita pulada, nunca vira "vazio"): pao, biscoito, bolo, torta, ensopados, cenoura dourada, melancia, feno, pepitas, blocos de 9, TNT, pistao/grudento, dispenser, dropper, funil, repetidor, comparador, observador, alavanca, tocha de redstone, sensor, gancho, bloco de notas, jukebox, lampada, baus (trapped/ender), placas de peso, alcapao de ferro, trilhos (energizado/detector/ativador), carrinhos (+bau/fornalha/TNT/funil), tesoura, vara de pesca, bigorna, caldeirao, stand (resultado = item do pack; funcional no proximo patch), garrafa de vidro, mesa de encantamento, bau do Ender, farol, quadro, armor stand, escudo, livro e pena, flecha espectral, po/bastao de blaze, olho do Ender, bola de fogo, tijolos, quartzo, arenito, pedras polidas, terra grossa, tintas (misturas + flores), la/carpete/vidro/vidraca/terracota/concreto em po/estandarte coloridos, tochas de abobora.
- `crafting.inc`: `craftResult` tenta ids exatos e, se nada casar, troca qualquer tabua (`gPlankSet`: todas as madeiras do pack) por `PLANKS` — palito, mesa, ferramentas, tocha, placa etc. funcionam com abeto/betula/selva/acacia/carvalho escuro (antes so carvalho: survival impossivel fora de floresta de carvalho). Tronco e "madeira" de cada especie -> 4 tabuas da propria especie.
- `inventory_ui.inc consumeCraft`: balde de leite volta como balde vazio (bolo).
- `drops.inc blockDrops` (patchSurv.1): grama alta solta semente de trigo (1/8); esmeralda/quartzo do Nether/melancia(3-7 fatias)/neve(4)/argila(4)/glowstone(2-4 po) soltam o item certo.
- `main.cpp`/`assets_atlas.inc` (patchGfx.2): `options.cfg sharp=<-2..1>` (padrao -0,5) = bias de LOD do atlas (so PC GL). Aniso do atlas continua desligado (listras). (VIS)
- Teste: `tests/test_craft_extra.cpp` (26, pack falso, nao precisa do resource pack); test_chest 42/0, test_chest2 43/43, test_piston 142/0, test_furnace 61/0, test_inv_keys 9/0, test_inv_keys2 9/0.
- Limites: (VIS) nomes de arquivo do pack real nao conferidos (sem o pack aqui): se uma receita nao aparecer, o nome do item no pack difere dos aliases; sem cana-de-acucar no mundo (papel/acucar/bolo so com o item de outro lugar), sem slime/ink sac/spider eye/ghast tear/cacau como drop; aniso desligado.

## patchGfx.3 — alpha-to-coverage no passe solido (PC GL, MSAA ligado): bordas de folhas/plantas suaves. BLOCKLAND_NOA2C=1 desliga. (VIS)

## patchLinux.1 — SDL_VIDEO_WAYLAND_ALLOW_LIBDECOR=0 por padrao no Linux (SIGABRT do GLib: chave antialiasing). (VIS)

## patchLinux.2 — AppRun e main: SDL_VIDEODRIVER=x11 (se DISPLAY) + LIBDECOR_PLUGIN_DIR vazio; build/lib.sh com "$tool" entre aspas (caminho com espaco). (VIS)

## patchLinux.3 — build/lib.sh gera tambem Blockland-linux-*bits.run (autoextraivel, sem FUSE; cache em ~/.cache/Blockland). (VIS)

## patchLinux.4 — AppRun: logs/saves/crash-reports na RAIZ do projeto quando o AppImage/.run esta em <projeto>/executaveis (BL_RUN no .run). (VIS)

## patchInv.1 — busca do inventario: UTF-8 (c cedilha/acentos), Ctrl+A com destaque azul + Ctrl+C/X/V, Backspace apaga caractere inteiro; 1-9 sobre um slot na aba Pesquisar vai para a hotbar (como o Java) sem digitar. test_invsearch 8/8.
## patchPerf.1 — PC: teto do orcamento de carga 8 ms (RAD>10; senao 11) e menos geracoes/malhas por quadro (menos picos de upload com chunks altos). (VIS: medir FPS)

## patchIntro.1
- Abertura de 5 s (`introScreen`, `main_loop.inc`): fade-in 1 s, panorama borrado, logo, barra verde quadrada no canto inferior direito (`splashBar`, `splash.inc`), fade-out e titulo. Sem Mojang. Pulada com --host/--join.
- Loading de inicio usa a mesma barra (canto, quadrada). `drawLoad` (`menu.inc`) refeita: fundo borrado do titulo (`gBlurTex`), "Iniciando o servidor", barra verde quadrada com % e dica em caixa.
- (VIS) conferir: abertura, geracao de terreno, resolucoes/Android. Fundo e o panorama do titulo (nao ha vila gerada).

## patchIntro.2
- Tela UNICA de abertura (`splash.inc`): fundo em cache + logo no canto superior esquerdo + barra flat no canto inferior direito; fade-in 1 s, carga real move a barra, `splashHold` completa 5 s no total e faz fade-out. `introScreen` (patchIntro.1) removida (era a tela duplicada).
- Cache: `cache/intro_bg.png` (480 px) gravado 1x por execucao no titulo (`introBgGrab`, panorama borrado), carregado no inicio (`introBgLoad`); 1a execucao usa degrade escuro. `drawLoad` (geracao de terreno) usa o mesmo cache (`introBgDraw`), titulo alinhado a esquerda, barra flat.
- (VIS) conferir: 1a execucao x 2a, resolucoes, Android. Adicionar `cache/` ao .gitignore.

## patchIntro.3
- Geracao de mundo usa terreno DIFERENTE do menu: 2o panorama (semente `gPanoSeedBase`=0xB10C1A4D) borrado e salvo 1x em `cache/load_bg.png` (`bgCapture`, `main_loop.inc`; textura `gLoadBgTex`). Sem cache na 1a execucao: gera na abertura (alguns s a mais, so 1a vez); depois so le o PNG.
- `drawLoad`: logo centralizado; dicas no estilo console (caixa larga azul-petroleo com contorno branco fino, 8 dicas PT).
- (VIS) conferir 1a execucao (tempo extra), fundo da geracao, caixa de dica. Apagar `cache/` forca regerar.

## patchIntro.4
- Bug: `cache/load_bg.png` saiu com logo+barra do loading "assados" (splashPoll redesenhava no meio da captura). Captura agora com `gSplashLoading=false`; arquivo renomeado para `cache/terreno_load.png` (o antigo e ignorado; pode apagar `cache/load_bg.png`).

## patchIntro.5
- 3 paisagens em cache/: `terreno_abertura.png` (abertura, semente 0xA11CE5E5), `terreno_load.png` (geracao de mundo), `menu_bg.png` (panorama do menu, gravado ao ver o titulo). 1a execucao gera as duas primeiras (alguns s a mais). Apagar os PNGs antigos (`intro_bg.png`, `load_bg.png`).
- Voltar do jogo ao menu: enquanto o panorama regenera mostra `menu_bg.png` na hora (`bgScreen`), sem tela preta nem fade preto.
- "Carregando" da abertura com ". . ." animado (largura fixa).
- (VIS) conferir: voltar do jogo ao menu (fundo imediato; a troca para o panorama vivo deve ser imperceptivel), abertura com paisagem propria.

## patchIntro.6
- Bug do patchIntro.5: comentario `//` no fim da linha do titulo (`main_loop.inc`) engoliu o `else { drawPanorama... }` -> panorama nao era desenhado e o menu pintava por cima do quadro anterior. Trocado por `/* */`. Licao: nunca `//` no fim de linha com mais codigo adiante.

## patchMP.2 — multiplayer: resposta pelo mesmo IP + logs de rede
- Host Linux com 2+ interfaces na mesma rede (log do usuario: 192.168.0.105 e .103): a resposta (WELCOME/BLA) saia com o IP de origem escolhido pelo kernel, diferente do IP que o convidado usou; firewall do Windows descarta isso. `net.inc`: `netRecvFrom`/`netSendFrom` (IP_PKTINFO, so Linux; Windows segue com sendto/recvfrom), `NetPeer::lip`, `netTo` e o repasse usam o IP certo; `netlan.inc`: descoberta (BLA) idem; `netPktInfoOn` no host.
- Logs `[NET]` em logs/latest.log: host aberto + interfaces, consulta de descoberta, HELLO (nick, interface, plataforma), versao recusada (mostra as duas), WELCOME, recusado, sem resposta em 7 s.
- Testado: sintaxe, test_lan 38/38, test_conn 33/33, test_host_leave 6/6. (VIS) conferir Linux<->Windows em maquina real. Windows como HOST multi-interface ainda sem PKTINFO (WSASendMsg).

## patchBuild.1 — build/windows.sh: `$RES` sem aspas quebrava o link com pasta com espaco (ex.: "Area de trabalho"): ld procurava "/home/x/Area", "de", "trabalho/.../res.o". Agora `${RES:+"$RES"}`. (VIS: rodar ./build.sh em pasta com espaco)

## patchPerf.1
- [x] main_loop.inc/perf.inc/chat_debug.inc: deteccao de swap estrangulado (1 Hz) -> swap 0 + limitador. (VIS) conferir FPS com 2+ instancias.

## patchPerf.2
- Deteccao de swap travado (compositor 1 Hz) vale tambem com /vsync 1 manual (limiar 400 ms) e em soft-pace: se swap>400 ms x3 com swap 0, SwapWindow so 1x/s (gSwapSkip).
- Ajuste automatico de distancia ignora swap>300 ms (nao e GPU). Dica [PERF] nao sugere mais /vsync 1 nesse caso.

## patchPerf.3
- gSwapGapMs: intervalo de SwapWindow no modo gSwapSkip = 1 s em foco, 3 s sem foco (perf.inc, main_loop.inc).

## patchPerf.4
- main_loop.inc: maxTk (2 ticks se gPfOtherMs>25, senao 5); sMaxGen<=3/sMaxMesh<=2 se gPfOtherMs>33. Testes: tnt_perf 6/6, clock 7/7, collision 86/86.

## patchPerf.5
- perf.inc: PROBE faz glFinish se BLOCKLAND_PROBE_GPU setado; tests/stubs/SDL_opengl.h: glFinish.

## patchMP.2
- menu.inc guestStash/guestRestore; saveCurWorld (convidado) chama guestStash; netlan.inc netPrepareGuestWorld chama guestRestore. Testes: player_save, netsync, chunk_sync ok.

## patchUI.1
- splash.inc introBgLoad: assets/ui/loading_bg.png substitui gIntroBgTex e gLoadBgTex (fallback: cache/terreno_*.png).

## patchLoad.2 — fundo da geracao de terreno
- [x] `splash.inc introBgLoad`: carrega `assets/ui/gerando_bg.png` em `gLoadBgTex` (tela "Gerando area de spawn"); prioridade sobre `loading_bg.png`/cache; sem o arquivo, cai no comportamento anterior. (VIS) conferir o fundo e o escurecimento embaixo.

## patchMenu.4 — parallax do titulo
- [x] `main_loop.inc drawPanorama`: camera do panorama acompanha o mouse (`gMX/gMY`), suavizada, +-3 graus yaw / +-1,5 pitch; sem efeito nas capturas de cache (dt=0). (VIS) ajustar amplitude se ficar demais/de menos. Falta: background novo do titulo (precisa da imagem do usuario).

## patchFps.6 — vertice compacto: INVIAVEL como proposto (sem mudanca de codigo)
- Testado: `V` = 5 floats + RGBA8 (24 bytes) compila sem erro, MAS o alfa do vertice carrega a luz EMPACOTADA em float (`packLight`, ~21 bits: cc*4096 + ceu*256 + bloco*4, /2097152; `world.inc` ~733/828) lida pelos shaders de GL/GLES/Vulkan; em 8 bits a luz seria perdida. Cor RGB sozinha caberia em 8 bits.
- Para 36->28/24 bytes seria preciso: luz em canal proprio (ex.: atributo uint32 ou 2o par de bytes) + mudar os 3 shaders (blockland.vert, kGnVS, Vulkan) + `glColorPointer`/`VkVertexInputAttributeDescription`/`GnV`/`VkV` (main_loop 148/336/1474/1591, render_items 103, vkgl 19/227/344). Nao da para validar sem rodar (VIS). Nao refazer sem decidir isso.
- Falta: LOD de cubos distantes, passe sem discard no PC.

## patchLoad.3 — layout da geracao de terreno
- [x] `menu.inc drawLoad`: logo, titulo, barra e dica agora numa coluna a 75% da largura (direita, sobre o ceu); logo menor (max 190), barra e dica ate 40% da largura. Castelo livre na esquerda. (VIS) conferir se o texto cabe em telas pequenas.

## Auditoria (patchAudit.1)
- Item 7 Q/F touch: pendencia antiga estava defasada (ver acima). Abertos que dependem do usuario: titulo (imagem de fundo), item 1 lava x tocha (descrever), item 3 drops (qual item), porta da vila (video). Grandes/VIS: LOD de cubos, passe sem discard no PC, NBT de bau no pick, animacao do pistao.

## patchFps.6b — LOD de cubos distantes
- [x] `main_loop.inc` (`sFar`): a malha em modo longe (`gMeshFar`: sem modelos, faces enterradas cortadas) agora comeca a 80% da distancia da nevoa (antes 92%) e vale a partir de RAD>6 (antes >8). Chunks reconstroem ao cruzar a faixa (histerese ja existe). (VIS) conferir se escadas/cercas/tochas somem cedo demais ao longe; se sim, subir o .80 para .88. Medir FPS com RAD 8-12.
- Falta: passe sem discard no PC; vertice compacto so com redesenho da luz (ver patchFps.6).

## patchMenu.5 — fundo do titulo
- [x] `assets/ui/titulo_bg.png` (castelo) vira o fundo do titulo, com parallax (zoom 1,08, mouse/dedo) e 30% mais escuro (`splash.inc titleImgDraw`, `main_loop.inc` ramo SC_TITLE). Apagar o arquivo volta ao panorama 3D (+ parallax do patchMenu.4). (VIS) conferir leitura do logo/botoes sobre a imagem.

## patchAudit.2 — decisoes (sem mudanca de codigo)
- Passe sem discard no PC: NAO feito. A maquina de 2 fases (`gPiPhase`, `rpiSolidProg`, `FSPiO`) e so do Pi (`BL_RPI`); no PC exigiria variante sem discard dos 3 shaders de `sky_cycle.inc` (411/510/705) + 2 passadas por chunk, sem como validar aqui, e GPUs de PC perdem pouco com discard. Retomar so se o profile mostrar fragmento como gargalo.
- Piston: animacao ja implementada (so conferir no jogo). CORRECAO: NBT de BAU no pick JA EXISTE (`Slot::nbt`, Ctrl+botao do meio, save/rede NET_VER 22, `tests/test_pick_nbt.cpp`); a nota anterior estava errada. Abertos so: shulker, tooltip com itens do bau, copiar no touch/controle, convidado colocar bau cheio, teste com convidado real (VIS).

## patchChest.1 — tooltip de bau com conteudo
- [x] `chest.inc chestSlotTip` + `inventory_ui.inc` (tooltip): bau com NBT mostra na 2a linha "Contem: 64x Pedra, 3x Tocha +2" (ate 3 tipos, pilhas somadas). (VIS) conferir no inventario/bau/hotbar-aberta.
- [x] marcados como feitos: titulo com imagem + parallax (patchMenu.4/.5), LOD de cubos (patchFps.6b), fundo e layout da geracao (patchLoad.2/.3), Item 7 Q/F touch, pick de bau com NBT, animacao do pistao (so VIS).
- [ ] Abertos: shulker no pick; copiar conteudo no touch/controle; convidado colocar bau cheio; passe sem discard no PC (adiado); lava x tocha, drops, porta da vila (dependem do usuario).

## patchChest.2 — shulker no pick
- [x] Shulker no pick (Ctrl+meio) ja funcionava: `chestStores` inclui tipos 3..18. Teste novo em `tests/test_pick_nbt.cpp` (shulker com conteudo copia; vazio nao). Comentario velho de `chestPickNbt` corrigido. Ender continua de fora (conteudo e do jogador). (VIS) colocar a shulker de volta e abrir.

## patchBreed.1 / patchTouch.9 — reproducao + sem pick no celular
- [x] Reproducao: botao direito com a comida certa num animal adulto (vaca/ovelha: trigo; porco: cenoura/batata/beterraba; galinha: sementes) liga o modo amor (30 s, consome 1; criativo nao consome). Dois da mesma especie em amor a ate 8 blocos andam um ate o outro; a <1,6 blocos nasce um filhote, recarga de 5 min, 1-7 de XP. Filhote alimentado cresce ~10% mais rapido. `mobs.inc breedTick`, `main_loop.inc breedUse/breedFood`. So host/offline. `tests/test_breed.cpp` (9/0).
- [x] Pick block removido do touch (`touch.inc`, `main_loop.inc`); controle e Ctrl+meio continuam.

## patchUnload.1 — descarregador de chunks
- [x] `main_loop.inc chunkUnload`: chunks alem de RAD+6 de TODOS os jogadores (local, P2-4 da tela dividida, convidados) saem da RAM: blocos/meta vao para `savedC/savedM` (RLE), VBOs/luz/malhas liberados, tirados de `W`. Roda a cada 6 quadros, ate 3 por vez (sem engasgo). Ao voltar, `gen()` decodifica de `savedC` (igual a carregar do save). `gEpoch++` invalida cache de visiveis/fantasmas (guardam Chunk*).
- [x] `menu.inc writeChunkFile`: o save agora grava W + savedC (chunks descarregados e chunks do save ainda nao revisitados). Antes so gravava W: chunks do save que o jogador nao revisitou eram PERDIDOS no proximo salvar. Vale para as 3 dimensoes (`dimSavedC/dimSavedM` em nether.inc).
- Testes: `tests/test_unload.cpp` (12/0); regressoes test_chunk_sync, test_oldworld, test_mobsync, test_pick_nbt OK.
- (VIS) medir RAM/FPS andando longe e voltando; salvar/sair/entrar e conferir construcoes antigas; ajustar margem `RAD + 6` se o ir-e-voltar regenerar demais.

## patchBreed.2 — filhote segue o adulto
- [x] `mobs.inc breedTick`: a cada 10 ticks, filhote parado vai ate o adulto mais proximo da mesma especie (3-10 blocos). `tests/test_breed.cpp` 12 checks. Falta: coracoes (particula), salvar filhote, mooshroom.

## patchSave.1 — autosave do mundo
- [x] `main_loop.inc`: `saveCurWorld` a cada 5 min jogando (nao com bau aberto, geracao em andamento nem como convidado). O RLE ja e em threads. (VIS) medir o tempo no log "Mundo salvo em X ms" e ver se a pausa incomoda; ajustar o 300000.

## patchMP.3 — convidado interage com mobs (NET_VER 44)
- [x] Novo pacote `NP_MOBUSE` (convidado -> host: uid i32, acao u8 [1 comida, 2 tesoura], item i32). Host (`main_loop.inc mobUseRecv`) valida: remetente conhecido e nao espectador, mob vivo a <=10 blocos, comida certa / tesoura / ovelha adulta nao tosquiada; aplica amor, crescimento do filhote ou tosquia (la cai pelo NP_DROP de sempre). Convidado (`shearUse`/`breedUse`) mira nos mobs espelhados, manda o pacote e gasta item/durabilidade local. NET_VER 43 -> 44 (os dois lados precisam do mesmo build). `tests/test_mobuse.cpp` (7/0).
- Limites: o convidado gasta o item mesmo se o host recusar (raro: mob mudou de estado no meio); tosquia/amor so valem se o host tem o mob carregado.
- MP ja era host-autoritativo (blocos, drops, baus, mobs, PvP, chunks, tempo, chat). Lacunas conhecidas que restam: convidado colocar bau cheio (pick NBT), balde em vaca pelo convidado (ordenha), sela/montaria, ovos de spawn ja existem (NP_MOBSPAWN).

## patchShader.3 — shader mais leve (mesma imagem) + anel do sol atras de blocos
- [x] `sky_cycle.inc` (shader principal): PCF das sombras sem sin/cos por amostra: o angulo aleatorio do pixel vira (cos,sin) 1x e as 4+8 amostras (mesmas posicoes, Vogel pre-calculado em constantes) sao so rotacionadas. Antes ~12 sincos por chamada de cascata (ate 3 cascatas + entidades). Resultado numerico igual.
- [x] normal da face (`cross(dFdx,dFdy)`) calculada 1x por fragmento (era 3x: sombra, ambiente, agua) e fora de ramos divergentes.
- [x] lens flare (`FFS`): `halo` (anel do sol) e `haze` agora multiplicam pela visibilidade do sol (terreno na frente = sem anel) e o anel nao pinta pixels de blocos em primeiro plano (12% de vazamento); `pow(x,2)` virou x*x; atan sem uso removido.
- (VIS) O GLSL nao pode ser compilado aqui (so o C++): se o shader falhar ao compilar o jogo cai no caminho sem shader e o log mostra o erro; conferir sombras (mesma penumbra), anel do sol atras de montanha/casas, e o FPS com sombras ligadas. Nao mexi em god rays (22 amostras) nem nos feixes volumetricos (8 amostras) para nao mudar o visual; candidatos se ainda pesar: god rays em meia resolucao, feixes com 6 passos.

## patchSlim.1 (NET_VER 49)
- Opcao `armsel` (Auto|Normais|Finos) em screens.inc (Geral e peskin); main_loop.inc aplica (slim/slimAuto, buildModel, gSkinNetDirty); F6 removida
- net.inc: NP_SKIN +1 byte slim; SkinSlot.slim; slimFromPx (extras do split); netSlimFor; NetSkinSwap(tex, slim) remonta o modelo do remoto e restaura
- tests/test_skin_sync.cpp atualizado (+flag, +checks slim); test_nick_sync: 11 falhas JA existiam no codigo original

## patchOpt.4
- F7/F9/F10 removidas; Shaders: sh2 = God rays, sh3 = Lens flare (gGodClick/gFlClick em asset_manager.inc, aplicados em main_loop.inc); textos sem "(Fx desliga)"

## patchSplash.1
- menu.inc: menuSplash() (lista de 40 frases, escolhida 1x por sessao) nos 2 pontos que desenhavam "Faça de tudo!"; no titulo classico o splash encolhe para caber (lw*.62)

## patchSplit.5
- Pedido do usuario: split fora do menu de pausa; START abre/fecha por controle (cada controle separado, 2x START no mesmo = so uma janela)
- gamepad.inc: gXPadId[3], gpIsExtra, gpPadFor novo; defaults Pausa=Select, Chat=LB; gpLoad corrige cfg com START; START nao e mais rebindavel
- splitscreen.inc: split2PadStart (entrar/sair, troca com o ultimo extra); split2Frame1 usa gpPadFor(gS2i + 2)
- main_loop.inc: START dos controles fora de menus; removidos F12, aplicacao de splitscr por quadro, cliques PB_SPLIT/PB_SPLITMODE; pause_menu.inc/screens.inc ajustados
- tests/test_split_start.cpp novo (12 checks); test_split.cpp NAO compila (gBoat2P/gB2, ja era assim)
