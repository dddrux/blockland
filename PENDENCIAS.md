> **REGRA FIXA: a cada atualizacao/patch, mudar a versao (`BLOCKLAND_VERSION` em src/main.cpp) e registrar aqui.**

# PENDENCIAS (raiz do jogo) — lista unica do que falta
## ★ META PRIORITARIA (pedido do usuario): DAR PRA ZERAR O JOGO — portal do End PRIMEIRO
Cadeia completa que precisa funcionar de ponta a ponta no survival (cada elo: `grep` + teste headless + conferir no jogo):
1. Portal do End: bloco = ceu de galaxia como o 1.12.2 — **FEITO no patchEndPortal.1 (VIS)** (`src/endportal.inc`: 12 camadas aditivas com entity/end_portal.png + end_sky.png).
2. Nether: portal de obsidiana, fortaleza, blazes -> varas -> po de blaze; endermen -> perolas -> olhos do Ender. (existem blaze/enderman/perola/olho; conferir drops, receitas e a fortaleza do Nether gerando)
3. Fortaleza (stronghold.inc, `/locate`): lançar olho aponta a fortaleza; sala do portal com 12 molduras; colocar olhos acende o portal (nether.inc:189). Conferir o ciclo inteiro no jogo.
4. End: plataforma de obsidiana, ilha principal, torres com cristais, dragao (dragon.inc, barra de vida) — conferir luta completa: cristais curam, dragao pousa/sopra, morre.
5. **FEITO (patchEnd.13, VIS)**: portal de saida + ovo + 12000 XP -> spawn.
6. **FEITO (patchEnd.14, VIS)**: creditos simples apos a 1a morte do dragao.
7. Sobrevivencia sem travar a progressao: fome, armadura/encantamentos/pocoes (brewing) ainda parciais — ver §6; multiplayer (convidado na luta do dragao) em src/PENDENCIAS.md.
Ordem de trabalho combinada: (1) feito -> (5) portal de saida+ovo+XP -> (6) creditos -> (2)(3)(4) auditoria de ponta a ponta.
Historico por patch: `src/PENDENCIAS.md`. Ao terminar um item, apague a linha daqui e registre o patch la.
VIS = nunca visto rodando no jogo; so "compila e passa nos testes headless". Nada dos patches Light/Bucket/Gfx/Rfl/Ember/Shadow/Wx/Terrain/Prop/Mod foi visto no jogo.

## 0. Audio (patchAud.OGG feito; resto)
- [ ] (VIS) Sons por material de bloco (passo/quebrar/colocar: madeira, terra, areia, vidro, folha, lã), porta, fogo, agua/lava, botoes do menu, bau/fornalha/pocao, dano do jogador, comer/beber, flechas, chuva/trovao, ambiente de caverna, discos (`records/`). Os .ogg ja estao em `assets/sounds/`; falta ligar em `events.cfg` + ganchos.
- [ ] Mobs novos precisam de linha em `mobSndFor` (audio.inc) ao serem adicionados (lobo, gato, morcego, ghast, golem, slime, enderman...).

## 1. Mods e conversor (patchMod)
- [ ] **PRIORIDADE (pedido do usuario):** blocos de TODOS os mods menos o Decocraft estao bugados: nao seguem a grade de blocos e qualquer coisa e posicionavel; varios baus/itens se comportam como props sem dever. Pesquisar como cada um funciona no 1.12.2, corrigir, e REMOVER do inventario os itens inutilizaveis (so textura).
- [ ] **Armas (MrCrayfish Gun Mod = `mods/cgm`, Techguns = `mods/techguns`)**: convertidos so os VISUAIS (patchMod.7). Falta o FUNCIONAL no jogo: disparo (raycast/projetil), municao e recarga, dano, cadencia, mira/luneta, silenciador, granada/bazuca/missil, efeitos (flash, som); `armas.json` (estatisticas do cgm: dano, cadencia, municao) e `sons.txt` ja sao gerados mas o jogo nao le. Usar `bow.inc`/`pvp.inc` como base.
- [ ] Techguns: os modelos 3D das armas/armaduras sao codigo Java (ModelBase), nao JSON: so os 308 icones planos viraram props; 5 modelos sem elementos, 14 blockstates sem modelo simples, 18 modelos `.obj` de blocos nao convertidos (BLP1 so tem caixas: precisa de suporte a malha), 2 multipart condicionais. Armaduras, mobs, NPCs, estruturas e livros Patchouli nao portados.
- [ ] cgm: texturas das armas sao vanilla por nome (concrete, anvil...): conferir no jogo se `ref` resolve pelo pack (VIS); bancada (workbench) sem GUI/crafting.
- [ ] (parcial, patchProp.7: icones planos 2D e luz do dia na mao feitos; VIS) **BUG armas (relatado pelo usuario, patchMod.7):** (1) o mod de armas esta SEM TEXTURA no jogo (cgm e techguns): conferir se `ref` por nome (`minecraft:blocks/concrete_gray` etc.) resolve pelo pack/atlas em `props.inc`/`propsys.inc`; no techguns checar os 308 icones planos; (2) a arma NAO e apoiada corretamente na mao: `propDrawUnit` encolhe o modelo a caixa unitaria e centra; falta pose de mao propria (posicao, escala, giro, cano para frente, tamanho relativo da arma), como a tocha em `player_render.inc`/`main_loop.inc`; itens planos devem ficar como item vanilla (em pe na mao).
- [ ] **REGRA GERAL: todo mod tem que seguir a GRID do jogo** (pode girar, mas encaixa na grade de blocos). Os blocos do Thut's Tech (Elevator, Controller, Block Cannon) nao seguem a grid (props com posicao livre de 1/8 de bloco, `propSnap`). Fazer: prop de bloco inteiro (bbox ~1x1x1, o que vem de blockstate de mod) vira bloco de verdade ou ao menos centra no bloco-alvo, bloqueia a celula e gira so em passos de 90 graus (`propRotFor`/`propPlace`); conversor marca esses itens (`grid: true`). Vale para todos os mods novos; Decocraft continua livre.
- [ ] **Thut's Tech: falta item** — o Device Linker (`item/devicelinker`) nao foi convertido no 1o envio (corrigido no conversor do patchMod.7; reconverter) e o Elevator Rail (`tile.liftRail`) nao tem blockstate/modelo no jar: criar um modelo simples ou deixar so no relatorio.
- [ ] Portar o mod Chisel 1.12.2: so os assets foram enviados (`Chisel_1_12_2_assets_organizados.zip`); falta o `.jar`.
- [ ] Motor de luz colorida (Colored Lights): blocos `mods/luzes_coloridas` existem (16 cores, acesa/apagada); falta luz RGB no motor (`biome_light.inc`, `lightEmit`) e no shader.
- [ ] Jogo ler `receitas.txt`, `drops.txt`, `sons.txt` do conversor (`mods.inc`, `crafting.inc`, `blockDrops`); resolver nomes vanilla (`minecraft:planks`) para ids do pack.
- [ ] `forge.py`: nomes ofuscados SRG (`func_177229_b`) na TABLE; valores de setHardness/setLightLevel via bytecode.
- [ ] Conversor generico: um prop por estado do blockstate, `when` do multipart, rotacao de UV de face, stairs/fence/pane, animacao alem do 1o quadro, itens sem blockstate. Sentido do giro em X das variantes (VIS).
- [ ] Thut's Tech (elevador, canhao): so decorativo; TileEntity/GUI/rede nao traduzidos. Thut Core e Colored Lights nao tem blocos.
- [ ] Limite de 136 blocos+itens somando todos os mods (`mods/README.md`): conferir com os mods novos.

## 2. Redstone / blocos
- [x] Portas e portoes com redstone: feito no patchMod.9 (VIS).
- [ ] Alcapao: avaliar energia ao colocar (hoje so quando um vizinho muda); energia por bloco solido fortemente energizado; colisao/textura do alcapao de ferro sem teste. Sintoma original nao detalhado: perguntar se ainda ha defeito. Falta `tests/test_irontrap` cobrir orientacao e colisao.
- [ ] Vidracas/cercas e laje/escada de arenito (item 2 da fila de pendencias antiga); laje de arenito: texturas de cima/baixo (`blockland-laje-arenito`).
- [ ] Baus e estandartes: baú funcional como no Minecraft, estandartes sem nome de bigorna (3 partes; `chest.inc`, `redstone.inc`); `test_chest2` ja falhava (linha 62, segfault).
- [ ] Aba Transporte: trilhos, carrinho de mina, limites de pilha (`blockland-transporte`).

## 3. Decocraft (props)
- [ ] Prop.5d: ParticleEmitter 29 (castical), Faucet 9/ShowerHead/ShowerHandles (agua), RecordPlayer 2, TimedModel 4, Decobench 1, VariableRendering 51; luz da lampada no mundo; dormir de verdade na cama (hoje so spawn + pular a noite).
- [ ] Prop.6b: rotacao CraftStudio (Cavalo de Pau embaralhado); 245 itens com textura de override ausente do jar; colorSecondary/faceCulling/colorAccentPrimary; 230 faces com UV fora da textura (hoje fixadas na borda; o certo e repetir; so reconvertendo com o `.jar` do Decocraft); props-bloco (janela/parede) encaixando na grade ao empilhar.
- [ ] Prop.7: lateral esquerda/direita com texto; itens que ainda caem no generico; reenvio de pacote UDP perdido para props (prop fantasma ate reentrar); splitscreen (jogador 2 nao senta nem usa props).
- [ ] Prop.6 VIS: cadeira (pose, altura, levantar), rede (NP_PROP, Container, sentar remoto), translucidez por face.
- [ ] Empilhar VIS: sem shift herda o giro do prop de baixo; com shift giro livre.
- [ ] Perguntas ao usuario: (1) qual vidro do Decocraft nao tem tamanho de bloco (nome)? Janelas grandes (Window 2, Bay Window) sao assim de proposito. (2) Quais itens ainda tem textura errada (nome)? Os atlas estao integros.
- [ ] O `.jar` do Decocraft e necessario para reconverter (`mods/decocraft/` ja vem convertido; `props.json` nao tem `beh`, so o `props.bin`).

## 4. Pendencias do HANDOFF (pedido "faca o que falta", ordem sugerida)
- [ ] 1 chat e nick: feitos /me, /say, /kill, edicao do chat, erro de nick no launcher, times. Falta Parte 3 do nick: /nick, /list, nick como alvo de comandos; /weather.
- [ ] 3 split pelo Esc; 4 animais e colisao de mobs; 5 vilas (geracao ruim, `/locate` nao acha vilas, `/tp` e `/locate` nao podem travar); 6 resource packs; 7 World Edit; 8 docs de mods; 9 launcher e menu; 10 Android.
- [ ] Modos de jogo: espectador, hardcore, aventura como no Minecraft (inclusive multiplayer), F3+F4 alternando.
- [ ] Arco funcional como no Minecraft (flechas, carregar, dano, multiplayer); flecha de esqueleto igual ao 1.12.2 (sem recolher).
- [ ] Tocha com animacoes e modelo 3D na mao.
- [ ] Chunks transparentes embaixo da agua (oceano feio) e buracos nos chunks.
- [ ] Sons: som de bigorna ao quebrar grama, ovo/pegar/dropar item, sistema facil de linkar audios, som em toda a UI, salvar e sair.
- [ ] GPU: jogo nao usa a Radeon dedicada no Linux com `DRI_PRIME=1` (`patchGPU`); foco primeiro nisso, depois a fila.
- [ ] Sombras/shaders: otimizacao e correcao (`shadows.inc`, `sky_cycle.inc`).
> Estes itens vem das anotacoes de pedidos anteriores; o status real de cada um nao foi reconferido neste chat. Conferir com `grep "^## patch" src/PENDENCIAS.md` antes de refazer.

## 5. Fila geral (depois do foco Decocraft)
- [ ] Sombra redonda no chao (patchSombra.2 feito, VIS): slime/cavalo/barco/carrinho/item; conferir no jogo atraso, tamanho e brilho.
- [ ] Luz/sombras/shader (VIS), clima (Wx), terreno 1.12.2 identico, portal/trilho, splitscreen, gamepad, Raspberry Pi 3B, Android, monitor/WebDisplays. Instalador Windows: FEITO (so mexer se pedido).
- [x] Falhas ja existentes: resolvidas no patchBug.1 (`test_cmd_player` era teste velho: convidado sem op nao usa /give; `test_chest2` so falha sem a pasta `saves/`, que o zip nao traz: crie `saves/` antes). `test_pkaudit` segue exigindo o pack.
- [ ] Nao inventar prioridade: perguntar em uma linha.

## 6. Lista de desejos (usuario; status NAO reconferido, fazer `grep` em src/PENDENCIAS.md antes de cada item)
Ordem = grupos do MEGA_PROMPT secao 7.
### 6.1 Multiplayer
- [ ] Mods no MP: convidado crasha ou nao ve bloco/prop do mod.
- [ ] Convidado nao spawna mobs nem acende TNT.
- [ ] Mega auditoria do multiplayer (host e convidado).
- [ ] Sair e voltar na MESMA dimensao (host e convidado).
- [ ] Areia caindo: animacao falha no MP e em outras dimensoes.
- [ ] Luz da lava ilumina jogadores; jogadores mais escuros no End.
- [ ] Indicador sobre a cabeca quando o jogador digita.
### 6.2 PvP / combate / itens
- [ ] Mobile nao ataca pessoas (PvP).
- [ ] Ovo sem dano no PvP; perola do End nao funciona (ha historico patch5.62: reconferir).
- [ ] `kill @e` / `tp @e` contam itens como entidade.
- [ ] Armadura funcional; todos os encantamentos; todas as pocoes; suporte de pocoes nao pega.
- [ ] TNT impulsiona jogador no criativo; sons e otimizacao da TNT (lag ~0).
- [ ] Amortecer queda: escada, agua, teia.
### 6.3 Mobs
- [ ] Todos os mobs; Ghast no Nether.
- [ ] Mob no mesmo barco que o jogador fica de lado (1.12.2).
- [ ] Todos os carrinhos funcionais; sombras do F7 erradas ao dirigir carrinho.
### 6.4 End
- [ ] Dragao: modelo feio; comportamento 1.12.2; barra de vida (linha branca, aparece de longe).
- [ ] Void real no End; cama explode no End/Nether.
- [ ] Gerador do End identico ao 1.12.2; cidades; estruturas novas (base 1.12.2); clarões do End.
- [ ] Luz laranja do End sem suavizacao nas bordas.
### 6.5 Blocos / redstone / mundo
- [ ] Elytra com partes pretas; modelo do fogo igual ao Minecraft; textura do funil com buracos.
- [ ] Particulas com o tom da luz do bloco; apagar fogo solta particula de fogo (nao de quebrar bloco).
- [ ] Alcapao de ferro: textura no inventario mas nao posicionavel (test_irontrap 11 passa; investigar so o item->bloco).
- [ ] Ejetor/liberador; redstone piscando (clock); lampada acende energizada; redstone mais 1.12.2.
- [ ] Ficar DENTRO de blocos; sufocar com areia sem teleporte; fechar alcapao dentro de si.
- [ ] Colheita completa; pao; plantacoes dropam.
- [ ] Placa: host nao ve texto; camera bugada ao editar; Android nao sai da tela; nao editar depois; criacao igual ao Java 1.12.2.
- [ ] Estandartes 3D no inventario; terreno atras do menu.
### 6.6 Chat / UI / som
- [ ] Emojis; acentos na busca e no chat; sintaxe EXATA do chat oficial; Tab identico; todos os comandos.
- [ ] Voice chat; mais skins e remover a padrao; sons dos botoes do menu bugados; sons em tudo.
### 6.7 Android
- [ ] Chat com teclado proprio; QWERTY estilo Gboard; facilitador de comandos.
- [ ] Controles maiores e longe da borda; voo melhor; UX adaptada; selecao de skins escaneando imagens.
- patchTnt.1 (TNT na água) feito; falta: Junkbox (5), mods não-Decocraft (7).
- patchJuke.1 (toca-discos) feito; falta: records/*.ogg, mods não-Decocraft.
- patchJuke.2 feito; falta: records/*.ogg, mods não-Decocraft.
- patchMus.1 feito (playlist + records); falta: menu music, mods não-Decocraft.
- patchAud.2 feito (material/dano/comer); falta áudio: porta, baú, fogo, água, chuva, caverna, menu; mods.
- patchAud.3 feito (porta/baú/fogo/lava/splash); falta áudio: menu, poção, flecha, chuva, caverna, forja, XP, nadar; mods.
- patchWx.8 feito (chuva nova + som); falta áudio: caverna, menu, poção, flecha, forja, XP, nadar; mods.
- patchLogic.1/Aud.4/Mob.9/Suf.1 feitos; falta: dropper/dispenser, observer, comparador, áudio caverna/menu/poção, mods.
- patchTrap.2 (alçapão sobre jogador) + Aud.5 (caverna) feitos; falta: dropper/dispenser, observer, áudio menu/poção/flecha, mods.
- patchLogic.2 (dropper/dispenser) feito; falta: dispenser comportamentos, observer, comparador, funil, áudio menu/poção/flecha, mods.
- patchLogic.3 (funil) feito; zip ACUMULADO. falta: funil lateral, dispenser comportamentos, observer, comparador, áudio menu/poção/flecha, mods.
- patchHop.1 (textura do funil) feito; zip acumulado. falta: modelo vanilla do funil, funil lateral, dispenser comportamentos, observer, comparador, áudio, mods.
- patchAud.6 feito (arco, XP, clique do titulo/opcoes, leite, fornalha); falta audio: pausa/inventario, musica do menu, flecha em mob, bigorna.
- patchLogic.4 feito (dispenser: flecha e TNT); falta: dispenser balde/ovo/neve/fogo, tela 3x3, observer, comparador, relogio, funil lateral, mods (7), audio pausa/inventario.
- patchLogic.5 feito (dispenser balde); falta: dispenser ovo/neve/fogo, tela 3x3, observer, comparador, relogio, funil lateral, mods (7), audio pausa/inventario.
- patchClock.1 feito (relogio animado); falta: receita do relogio, dispenser ovo/neve/fogo/3x3/direcao, observer, comparador, funil lateral, mods (7), audio pausa/inventario.
- patchClock.2 feito (receitas relogio/bussola); falta: dispenser ovo/neve/fogo/3x3/direcao, observer, comparador, funil lateral, mods (7), audio pausa/inventario.
- patchBug.2 feito (mods: bloco inteiro na grade, icone plano nao posiciona); falta blocos reais, baus de mods, remover itens so-textura, MP.

- patchPE.1: HUD touch estilo PE (direcional, botoes maiores) em src/touch.inc. Falta: menu inicial PE no Android/Rpi.
- patchPE.2: titulo + Configuracoes estilo PE (menu.inc, screens.inc, main_loop.inc); F8/BL_PE=1 testa no PC. Falta: paginas internas em PE, coracoes/fome no HUD.
- patchPE.3: menu PE padrao (PC incl.), boneco 3D, Configuracoes com opcoes de video/jogo rolaveis. Falta: scroll touch, categorias placeholder, paginas internas PE.
- patchPE.4: Configuracoes PE sem paineis, 2 colunas; nome do boneco corrigido.
- patchPE.5: seletor de skins PE (peskin), boneco do titulo menor.
- patchPE.6: Configuracoes PE completas + scroll por arrasto + HUD PE (coracoes/fome no topo). Falta: miniaturas corpo inteiro, paginas internas PE, armadura no HUD.
- patchPE.7: nome da skin acima da cabeca e boneco menor na tela de skins.
- patchPE.8: previa 2D da skin em painel (screens.inc).

## patchSkin.1
- Tela de skins (peskin): fundo de terra, paineis com moldura, preview 3D girando (arrastar gira), nome da skin na dica do carrossel. Menu principal: cabide removido, o boneco e o botao (contorno ao passar o mouse). (VIS)

## patchOpt.3
- Todas as opcoes de video/animacao/qualidade/desempenho/diversas/fov persistem em options.cfg (opt.* / osl.*). Volume (m0..m8) aplica ja no menu; chuva/trovao = cat 3 (Clima), caverna = cat 8 (Ambiente). Ligadas: Ceu, Estrelas, Sol e Lua, Altura das nuvens, Nevoa (OFF) e inicio, Chuva e neve OFF, Arvores. FALTAM: v4,v7,v9-v11, d7,d10-d16, an.1-16, an.p, q*, p*, x*. (VIS)

## patchUI.1
- Versao agora v1.1.0.19rc (`BLOCKLAND_VERSION` em src/main.cpp; instalador le os digitos). **REGRA: SEMPRE mudar a versao ao atualizar (a cada patch/zip).**
- GUI Auto no desktop um passo menor (tela cheia 768p = 2x). Titulo: Sair embaixo de Configuracoes/Mods; contorno rigido (silhueta dilatada por stencil) no boneco ao passar o mouse. (VIS)
- [ ] (VIS) patchSplit.4: conferir inventario/pausa so no viewport do dono, PvP split/multiplayer e sombras. Falta: inventario aberto por J2-J4 ainda para o J1; pausa propria de J2-J4; chat/placa de extras; flechas entre jogadores do split.

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
- Compila (VIS). Falta: ignicao depende do mapa de luz do chunk (c->lc) no bloco da cabeca; capacete nao protege ainda; (fogo dos extras do split: feito no patchFire.2).

## patchNet.3 — convidado volta na mesma dimensao e posicao ao reentrar
- menu.inc: mpResRecord/mpResGet/mpResLoad (saves/<mundo>/mp_players.txt: nick dim x y z). net.inc: netResRecordId grava ao sair (BYE, timeout, nick fantasma), a cada 8 s e no netStop do host (netlan.inc); ao sincronizar o convidado (ready && !synced) o host manda NP_TPTO com flag extra 1 (volta de onde parou; mensagem propria). Sem novo tipo de pacote: NET_VER continua 39 (TPTO ganhou 1 byte opcional no fim; versao antiga ignora).
- Compila (VIS). Falta: testar com nick auto 'Jogador N' (nao grava), convidado em dimensao nao gerada no host, inventario/vida do convidado nao sao salvos pelo host (player_guest.dat e local).

## patchStronghold.2 — celas, salas de cruzamento, biblioteca e grades
- stronghold.inc: PrisonHall (parede central com iron bars/XB_PANE e celas), RoomCrossing com 5 tipos (p.f0: pilar com tochas, balaustrada, fonte de agua, plataforma com bau, vazio), Library (estantes nas paredes/colunas, mesas, teias via sbRandPut; sem 2o andar/escada), porta tipo 2 vira grade de ferro. Layout (rng/pecas) inalterado.
- Teste: test_stronghold 1737/0. Visual (VIS): disposicao dos moveis e aproximada (nao copia o Java bloco a bloco).
- Falta: portas de madeira/ferro de verdade e botao, loot dos baus, silverfish (monster egg/spawner), biblioteca com 2o andar e escada, ajuste ao bioma, Random de populacao.

## patchStronghold.3 — /locate stronghold
- chat_debug.inc (locateCmd): /locate stronghold|fortaleza da a posicao [x, ~, z] (centro do chunk) e a distancia da fortaleza mais proxima entre as 128 posicoes (shNearest em stronghold.inc); so na superficie. Compila (VIS). Lembrete: sem ajuste ao bioma as posicoes podem cair em oceano (como a etapa 1 ja avisava).

## patchEnd.12 (feito, VIS): dragao com modelo 1.12.2, 1 cristal cura, coro natural, enderman — ver src/PENDENCIAS.md (AINDA FALTA: fases do dragao, enderman pega blocos/sons).

## patchVoice.1 (feito; VIS)
Voice chat (src/voice.inc, V ou botao de microfone no Android; so J1; NET_VER 41) + criativo touch (setas de pagina nao cobrem o titulo, abas da direita e X mais a esquerda). Falta: menu de voz (volume/limiar/dispositivo), indicador 'falando', /voice, Android real (manifest: tools/android-voice/LEIA-ME.txt). Detalhes em src/PENDENCIAS.md.

## patchRpi.1 (Raspberry Pi 3B em hardware real: preto + 1-3 FPS) (VIS)
- Causa pelo log (`logs-rpi`, VC4/Mesa 24.2, 869 MB RAM): shader de SOMBRAS (`sampler2DShadow` em cascatas) compilava e devolvia sombra total (tela preta) + pesado; 161 chunks/361 mil vertices por quadro (desenho 117 ms); Chisel 2042 blocos = 21 s de carga; `glGetError` a cada quadro; spawn 7x7 = 26 s.
- [x] `sky_cycle.inc`: `BL_RPI` usa shader enxuto (`VSPi/FSPi`: luz ceu/bloco + neblina, sem luzes dinamicas, sem pow, sem bias de mipmap), nunca o de sombras
- [x] `main.cpp`/`screens.inc`: max 5 chunks, padrao 3 (49 chunks em vez de 169); `main_loop.inc`: malha sem plantas/faces enterradas a partir de 2 chunks; spawn 5x5
- [x] `asset_manager.inc`: sem Chisel no Pi; `logger.inc`: `logGlErrors` desligado no Pi (`BLOCKLAND_GLDEBUG=1` liga)
- Teste: `tests/check_rpi.sh` (compila com `-DBL_RPI` + test_piston 142/0, test_chest 42/0); PC: sintaxe + piston ok. `test_split` ja nao compilava antes (gBoat2P/gB2), nao e regressao
- Limites: SEM Pi para testar; shader e FPS so no hardware (VIS). Se ainda ficar preto, rode com `BLOCKLAND_GLDEBUG=1` e mande o log (procure "Raspberry: shader enxuto ... FALHOU")
- [ ] Falta: medir FPS real; texturas/atlas menores no Pi; mobs/entidades; 60 FPS alvo

## patchRpi.2 (build do Pi com docker sem permissao)
- [x] `scripts/build-rpi.sh`: escolhe o motor (docker/podman) que responde a `info`; docker sem permissao cai para podman ou explica `usermod -aG docker`

## patchRpi.3 (Pi: sem textura, nao cria mundo, 12 FPS) (VIS)
- Causas pelo `log2`: (1) atlas 512x2384 > 2048 (limite do VC4) => glTexImage2D INVALID_VALUE => nenhuma textura; (2) `AppRun` fazia `cd` dentro do AppImage (somente leitura) => nem `logs/` ("so console") nem `saves/` (criar mundo falha); (3) quadro ~90 ms com so 20-60 mil vertices = custo fora da malha (HUD/nuvens/sons).
- [x] `main.cpp`: no Pi atlas 1024x2048 (ATL=64, ATLR0=123, ATLR=128), mesma capacidade de tiles; `assets_atlas.inc`: loga o atlas e erro se passar de GL_MAX_TEXTURE_SIZE
- [x] `scripts/build-rpi.sh` e `make-appimage.sh`: AppRun roda em `$BLOCKLAND_HOME` ou `~/.local/share/Blockland` (saves, logs, options, profile gravaveis; `assets` = link para o AppImage)
- [x] `screens.inc`/`main_loop.inc`: nuvens desligadas por padrao no Pi (`BL_CLOUD_DEF`)
- Teste: `test_rpi_atlas` (PC 5/0 e Pi 6/0: <=2048, POT, tiles dentro do atlas), test_slabtex Pi 18/0, test_piston 142/0, test_chest 42/0; sintaxe PC e Pi ok
- Limites: sem Pi; (VIS) texturas, criar mundo e FPS so no hardware. Os 12 FPS NAO foram atacados a fundo: sem perfil do "resto (fisica, mobs, UI)" ~70 ms; so removi nuvens. Mande o novo log (procure "Atlas de texturas 1024x2048")
- [ ] Falta: medir/perfilar o "resto" no Pi (HUD em modo imediato, decodificar som 70 ms na thread principal, mobs); usar os saves em `~/.local/share/Blockland`

## patchBuild.1 (v1.1.0.64rc) — FEITO (VIS)
Overlay de rachadura vanilla; Pi malha so o chunk do bloco quebrado; build em `build/` com saida em `executaveis/` (7 alvos); Android em `data/android/`. Detalhes em `src/PENDENCIAS.md`. AINDA FALTA: rodar `./build.sh --all` numa maquina com mingw/docker/NDK (nada disso foi executado aqui); conferir o FPS ao quebrar no Pi; crack em blocos nao-cubicos (laje/escada) ainda usa o cubo inteiro.
