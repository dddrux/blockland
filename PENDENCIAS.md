# PENDENCIAS (raiz do jogo) — lista unica do que falta
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
