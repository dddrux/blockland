# Estrutura do código

O núcleo original foi preservado e dividido em unidades de inclusão para facilitar manutenção sem mudar a arquitetura do Claude.

- `main.cpp`: includes + composição do programa.
- `core_noise.inc`: constantes, ruído e utilitários básicos.
- `assets_atlas.inc`: atlas em memoria (512x2176) + textura "ausente"; sem texturas procedurais/placeholder.
- `world.inc`: chunks, geração, malha e fluxo da água.
- `png.inc`: decoder PNG.
- `player_skin.inc`: skins e geometria do personagem.
- `render_items.inc`: meshes de blocos/itens e transformações.
- `inventory_ui.inc`: inventário, fonte e HUD/menu.
- `player_render.inc`: renderização do personagem e mão.
- `main_loop.inc`: eventos, física, câmera e loop principal.

Os `.inc` são incluídos pelo `main.cpp`, então não há dependências novas de linkagem nem necessidade de CMake.
- `physics.inc`: fisica do jogador portada do Minecraft Java 1.8.9 (AABB, step, agachar na borda, agua).
- `inventory_ui.inc`: inventário reescrito no estilo do Java 1.8.9 (escala inteira de GUI, painel/slots/hotbar/tooltip em unidades do Java, shift-click, arrastar para dividir, duplo clique, tecla 1-9, criativo 195x136).
- Criativo: abas (Construcao, Decoracao, Materiais, Alimentos, Pesquisa, Ferramentas, Combate, Inventario), rolagem e busca por texto.
- Texturas de blocos/itens: so do resource pack (assets/resourcepacks). Itens novos: ids 48+; blocos novos: ids 15..31. Todos os blocos/itens do pack viram itens extras (ids 1000+). Atlas 512x2176.
- Voo criativo: dois toques em Espaco (physics.inc, Phys::flying).

## v33
- `trees_mc.inc`: árvores originais do MC 1.12.2 (carvalho, carvalho grande, bétula, abeto, pinheiro, acácia, selva, arbusto), ligadas ao `treePut`/spill de `worldgen.inc`.

## v34
- Árvores: posição/quantidade por chunk no estilo BiomeDecorator (treesPerChunk por bioma, +10% extra), em vez de ruído por hash.

## v35
- `worldgen.inc`: lagos de água e lava (WorldGenLakes) na população, antes dos minérios.

## v36
- `worldgen.inc`: masmorras (WorldGenDungeons, sem baú/spawner), 8 tentativas por população após os lagos.

## v37
- `village.inc`: vilas. Localização e crescimento do layout portados do MapGenVillage/StructureVillagePieces 1.12.2 (pesos, limites, tamanhos, ordem dos sorteios); conteúdo das peças ainda simplificado.

## v39
- `/locate <bioma|vila>` (chat_debug.inc); nascentes de água/lava em pedra (worldgen.inc).

## v40
- Girassol (metade de cima) usa o modelo do pack com a flor; tocha de redstone (XB_RTORCH 934..938, luz 7); item/drop das tochas conferidos pelo nome do arquivo.

## v19
- `font.inc`: fonte 5x7 com minúsculas e acentos do português (UTF-8). `text()` aceita opacidade e sombra opcional (0 = sem sombra).
- `chat_debug.inc`: chat (T ou /), comandos, F3 e normalização do inventário.
  - Chat: Enter envia, Esc fecha, setas cima/baixo = histórico, PageUp/PageDown ou roda = rolar, Ctrl+V cola.
  - Comandos: `/gamemode <0|1>` (ou `/gm`), `/give <item> [qtd]`, `/tp x y z` (aceita `~`), `/clear`, `/pos`, `/cls`, `/ajuda`. F4 continua alternando o modo.
  - F3: fps e tempo de quadro (mín/máx + gráfico), chunks e malhas, posição, direção, velocidade, bloco mirado, memória do processo, RAM, VRAM (NVIDIA/AMD), CPU, GPU, versão do OpenGL e do SDL.
- Boneco do inventário (`drawPlayerPreview`): projeção ortográfica como o `GuiInventory` do Java; cabeça e corpo apontam para o mouse a partir dos olhos.
- Inventário: `sanitizeInv()` corrige pilhas vazias/acima do limite e armaduras no slot errado todo quadro; nomes agora em português com acento.

## v20
- Inventário: criativo e sobrevivência agora têm o mesmo painel (176x166); paleta 9x6 com barra de rolagem fina.
- `biome_light.inc`: clima (temperatura/umidade) em manchas grandes; grama (só topo + faixa verde lateral) e folhas recebem cor por bioma com gradiente suave.
- Folhas: texturas com furos (alpha), alpha test ligado, faces entre folhas desenhadas (estilo "fancy") e decaimento sem tronco a até 4 blocos (folhas colocadas pelo jogador não decaem).
- Luz por voxel (céu + bloco/glowstone) propagada por BFS numa região 3x3 chunks, luz suave por vértice e oclusão ambiente (AO) com diagonal do quad escolhida pelo par mais escuro. Chunks só são malhados com os 8 vizinhos carregados (anel extra gerado em `stream`).

## v21
- Desempenho (DRI_PRIME): malha dos chunks em VBOs (GL 1.5, fallback para arrays), reenvio só quando o chunk muda; simulação da água só visita células ativas. FPS destravado (`SDL_GL_SetSwapInterval(0)`), `/vsync 1` liga.
- `mods.inc`: pasta `mods/<nome>/mod.txt` (ver `mods/README.md`). Ids 120..255. Cada mod tem sua aba; setas ◀ ▶ rolam as abas do criativo.
- Árvores: carvalho (4-6), alto, redondo, fino, pinheiro (frio) e arbusto (quente seco), escolhidos pelo clima; densidade varia com a umidade. Folhas com ~32% de pixels transparentes.

## v22
- Grama (topo e camada verde da lateral) e folhas agora são **cinza** e multiplicadas por um colormap (`assets/textures/colormap/grass.png` e `foliage.png`, x = temperatura, y = umidade). Texturas verdes antigas são convertidas para cinza ao carregar. A lateral da grama é desenhada como base de terra + sobreposição tingida (como o Minecraft); ícones/mão usam a cor padrão de planície.
- Biomas: planície, floresta, deserto, savana, selva, pântano, taiga, tundra e montanhas (com neve e cascalho em altitude); praia/oceano pela altura. Cada um tem superfície, árvores e densidade próprias. Altura do mundo CH=96. F3 mostra o bioma.

## v23
- Corrigido: o HUD desligava o alpha test, então pixels transparentes (folhas, camada verde da grama) saíam pretos e piscavam. Agora o alpha test é religado a cada quadro; a camada verde da grama usa a mesma geometria da base (GL_LEQUAL), sem z-fighting.
- Pixel art do inventário/hotbar: a margem de UV estava em unidades erradas depois que o atlas cresceu (distorcia ~1px); agora é 0,02 texel.
- `drops.inc`: itens no chão (física do EntityItem 1.8.9): quebrar bloco em sobrevivência dropa; Q solta 1, Ctrl+Q solta a pilha; clicar fora do inventário solta o item da mão (direito = 1). Gira e flutua em 3D, junta pilhas, coleta na caixa expandida (1,0.5,1), delay 10/40 ticks, some em 5 min.
- Fog linear 38-108 com horizonte igual ao céu em degradê (domo); chunks novos aparecem com fade-in de 1,2 s (glBlendColor).

## v22 — água estilo Minecraft + desempenho
- Água (`world.inc`, `main.cpp`): port do `BlockDynamicLiquid` 1.8 guiado por eventos. `setB` acorda a água vizinha, então quebrar um bloco debaixo d'água enche o buraco (antes ficava um buraco de ar bugado). Fonte/fluxo 1..7/caindo (`WATER_FALL` = id 40), escolha do caminho com queda mais próxima (até 4 blocos), fonte infinita (2 fontes + piso), fluxo some sem fonte. Passo de 0,25 s, no máx. 800 células por quadro.
- Malha da água: altura de cada canto = média dos 4 blocos que o dividem (`BlockFluidRenderer`), cantos compartilhados => superfície lisa, sem degraus. Lados só contra ar.
- Correnteza no jogador (`physics.inc`: `flowVector`/`waterPush`, 0,014 como o Java).
- Desempenho: (1) `stream` com orçamento de ~3 ms/quadro (1 tarefa pesada por quadro no máx.); (2) luz e malha só até a altura ocupada (`chunkTop`/`rTop`); (3) mudança só de água refaz apenas a malha de água, usando luz em cache (`Chunk::lc`, `wdirty`), e sólido/água sobem em VBOs separados; (4) corte de chunks fora do campo de visão horizontal e além da névoa.

## v23 — inventário
- Boneco do inventário: segura o item da hotbar selecionado (`gHeldSel`) e inclina o corpo com o mouse (`-atan(dy/40)*20`, como o GuiInventory do Java).
- Título "Fabricação" movido para cima (88, 9).
- Fonte proporcional (`glyphMetrics` em `font.inc`): cada glifo usa só as colunas ocupadas + 1 px; espaço = 4.
- Abas do criativo redesenhadas (`drawTabs`): contorno, realce e sombra consistentes; a aba selecionada apaga a borda do painel sem deixar linhas soltas.
- v23.1: abas desenhadas pela posição (só TAB_INV embaixo); título 'Fabricação' removido.
- v23.2: seta de fabricação redesenhada e slot de saída 18x18 (área de clique inalterada).

## v24
- Folhas piscando: faces de folha contra folha ficavam no mesmo plano (duas faces opostas, culling desligado => z-fighting). Agora só uma das duas é gerada (`world.inc`); 
- `mining_sky.inc`: mineração do 1.8.9 em sobrevivência (segure o botão esquerdo): tempo = dureza x (1,5 | 5 sem ferramenta adequada) / velocidade da ferramenta (madeira 2, pedra 4, ferro 6, ouro 12, diamante 8); minérios/obsidiana exigem o nível certo de picareta para dropar. Rachaduras em 10 estágios, partículas de quebra (4x4x4) e nuvens que derivam. Criativo continua quebrando na hora.
- v24.1: removida a "bola" no céu (o domo, raio 200, era cortado pelo plano far; agora domo = 100 e far = 200). Nuvens refeitas: pré-passe de profundidade (só a superfície mais próxima é pintada, sem listras), sem fade por alpha (a névoa do mundo apaga as distantes), blobs maiores e mais altas.
- v24.2: partículas no estilo EntityDiggingFX (ticks de 20 Hz interpolados, gravidade 0,04, arrasto 0,98, atrito 0,7, vida 4..40 ticks, caixa 0,2, brilho 0,6 x luz local) com cor do bioma na grama (camada de cima) e nas folhas; partículas de batida na face enquanto minera (1/tick). Nuvens: máscara 256x256 que se repete (1 texel = célula 12x12), deriva 0,03 bloco/tick para -X, alpha 0,8, sombreamento 1.0/0.7/0.9/0.8. Braço: segurar o botão sem alvo não balança mais; minerando, o balanço reinicia a cada 0,2 s indo só até metade (como o swingItem por tick).

## v25 (HUD touch)
- Botões maiores e pixelados (cantos recortados, ícones em bitmap), pular alinhado ao direcional, botão de inventário com o tamanho da hotbar, círculo pixelado de quebra (`touchBrkProg`), botão X de sair no chat/inventário (só no modo celular, F8) e novo visual de painéis/slots do inventário (`uPanel`/`uBox`).
- `mcpack.inc`: ponte com resource packs do Minecraft (zip/pasta -> atlas) + armadura vestida (ver player_skin.inc).

## v26 — menu de pausa
- `pause_menu.inc`: menu de pausa do Java (GuiIngameMenu) em pt-BR: "Menu do jogo", Voltar para o jogo, Conquistas | Estatísticas, Opções..., Salvar e sair para o título. Layout em unidades da GUI (escala inteira), fundo em degradê 0xC0101010→0xD0101010, hover amarelo (0xFFFFA0) como o Java. Só "Voltar para o jogo" funciona; os outros são placeholders.
- Esc abre/fecha o menu (também ao perder o foco da janela); o jogo congela enquanto está aberto (só no modo solo; com outro jogador conectado a simulação continua). No menu só valem Esc e F3.
- Botões: `assets/textures/gui/widgets.png` (256x256, formato do Java: normal v=46, destacado v=66, desativado v=86). É a textura padrão do jogo e não depende do resource pack; se o arquivo faltar, `genWidgetsFallback()` (asset_manager.inc) recria a mesma arte.

## v27 — Rethoughted GUI como base da interface
- Texturas do pack **Rethoughted GUI** (CatFromNet) copiadas para `assets/textures/gui/rt/` e integradas ao jogo (não dependem do resource pack ativo): botões (normal/destacado/desativado, nove fatias 20/4/20/4), hotbar 182x22 + seletor 24x23, mira 15x15 (mistura invertida), fundos `inventory`, `crafting_table` e `creative_items/inventory/search` (176x166 e 195x136 dentro de 256x256), abas do criativo 26x32, rolagem, tooltip (fundo borda 9 + moldura borda 10) e ícones dos slots vazios.
- `inventory_ui.inc`: helpers `uSpr`/`uTexRegion`/`uNine`; hotbar, painéis, abas, rolagem, tooltip e botões usam as texturas. Textos de título ficam claros (`UI_TXT`) por causa do fundo escuro. Se algum arquivo de `rt/` faltar, o desenho antigo em código é usado.
- Inventário de sobrevivência no layout da textura: fabricação 2x2 em (98,18), resultado em (154,28) e **mão secundária** em (77,62) (novo slot, também na aba Inventário do criativo).
- `pause_menu.inc` usa os botões do pack (texto branco; o realce verde vem da textura).

## v28 — HUD de sobrevivência
- `survival.inc`: vida (10 corações) e fome (10 coxas) só em sobrevivência. Dano de queda (`ceil(distância-3)`, água/voo zeram), regeneração com fome >= 9 coxas (1 meio-coração a cada 4 s), exaustão por correr/pular/minerar que drena saturação e depois a fome, inanição com fome 0 (para em meio coração), 10 ticks de invulnerabilidade, flash vermelho, corações piscando/tremendo com pouca vida, tela "Você morreu!" e renascimento automático (3,5 s). Botão direito com comida na mão come (maçã 4, pão 5, bolacha 2, carne 3, bife 8, cenoura 3).
- Comandos: `/dano <n>` (n = meios-corações), `/fome <0-20>`, `/curar`.
- Paginador do criativo (`drawPager`): setas agora são pixel-art desenhadas (`uArrowBtn`), iguais nos dois lados, em vez do caractere `<` da fonte (que ficava fino e apagado no botão desativado).

## v29 — porco (sem IA)
- `pig.inc`: modelo exato do `ModelPig` do Java (cabeça 8x8x8 + focinho 4x3x1, corpo 10x16x8 girado 90°, 4 pernas 4x6x4, mesmos pivôs e UVs), com a textura lida do resource pack ativo (`entity/pig/pig_temperate.png`, ou `pig.png`; 64x64 ou 64x32). Sem pack, usa uma textura rosa simples. O porco fica parado: sem IA, sem animação, sem colisão.
- Comando `/porco [n]` cria n porcos (até 20) 2 blocos à frente, virados para você; `/porco limpar` remove todos.

## v30 — galinha, vaca, ovelha, drops e ovos de spawn
- `mobs.inc` (substitui `pig.inc`): porco, galinha, vaca e ovelha (pele + lã) com os modelos exatos do Java 1.8.9 e texturas do resource pack (`entity/<mob>/...`). Sem IA: ficam parados. Bater (clique esquerdo, alcance 3) tira vida (espada 4-7, machado 3-6, picareta 2-5, pá 1-4, mão 1; criativo mata na hora); ao morrer tombam e soltam os drops do 1.8.9: porco 1-3 costeleta; galinha 0-2 pena + frango; vaca 0-2 couro + 1-3 carne; ovelha 1 lã + 1-2 carneiro.
- Itens novos (ids 105-115): costeleta, frango e carneiro (crus/assados, comem como no Java), ovo e ovos de spawn (aba Diversos). Botão direito com um ovo de spawn cria o mob no bloco mirado.
- Comandos: `/porco`, `/galinha`, `/vaca`, `/ovelha [n]` e `/mobs limpar`.

## v31 — ciclo de dia e noite (igual ao Minecraft Java 1.8.9)
- `sky_cycle.inc` (novo): relógio do mundo `gWorldTime` (24000 ticks por dia, 20 ticks/s = 20 min; 0 = amanhecer, 6000 = meio-dia, 12000 = pôr do sol, 18000 = meia-noite), avançado 1x por tick no loop de ticks (para junto com o menu de pausa). Salvo em `saves/<mundo>/level.cfg` (`time=`).
- Fórmulas portadas do Minecraft: `calculateCelestialAngle` (ângulo celeste não linear), `getSkyColor`/`getFogColor` (fator `cos*2+0.5`, neblina nunca preta total), `calcSunriseSunsetColors` (faixa laranja no horizonte + neblina esquenta olhando para o sol), `getStarBrightness`, `getSunBrightness` + lightmap (`*0.95+0.05`, canais R/G `*0.65+0.35`: noite azulada), `getCloudColor`, `getMoonPhase` (8 fases, 1 por dia).
- Sol 60x60 e lua 40x40 a 100 blocos (`renderSky`), blend aditivo, 1500 estrelas geradas com `java.util.Random(10842)` (mesmo padrão do jogo original).
- Texturas: `environment/celestial/sun.png` (ou `environment/sun.png`) e `environment/moon_phases.png` 128x64 (ou as 8 de `environment/celestial/moon/`) do resource pack ativo. Se o pack não tiver, `placeholderSun` (32x32) e `placeholderMoon` (128x64, 4x2 fases) geram placeholders com a mesma resolução do Minecraft.
- Luz do mundo: a malha agora guarda a cor SEM luz (tingimento x sombreamento da face x AO) e, no alfa, os níveis de céu e bloco empacotados (`packLight`). Um shader GL 2.0 (`initSkyShader`, só a luz + neblina linear) aplica `0.06 + 0.94*0.8^(15-nível)*escala`, então o dia/noite muda a luz de todo o mundo sem refazer nenhuma malha; cavernas e luz de bloco (glowstone) não escurecem. Se o driver não tiver shaders, `gShaderOK = false`: luz assada como antes + escurecimento simples da tela (`drawNightOverlay`).
- Entidades (mobs, itens no chão, mão, jogadores) são escurecidas pela luz do local (`entLightAt`, `gEntDim` em `drawMesh`); partículas (`lightNear`) e nuvens (`gSky.cloud`) também.
- Comando `/time set <day|noon|sunset|night|midnight|sunrise|ticks>`, `/time add <n>`, `/time query`, `/time pause`, `/time resume`.
- Multiplayer: cada instância mantém o próprio relógio (ainda não sincroniza a hora pela rede).

## v32 — terreno novo, todos os blocos colocáveis, multiplayer sincronizado
- **Blocos de 16 bits** (`typedef uint16_t BT`): `Chunk::b`, luz e save usam 16 bits. Todo "cubo" do resource pack (opaco, vidro, gelo e blocos com `_top/_side/_bottom`, como tronco, abóbora, TNT, cacto) vira bloco colocável, minerável e que dropa a si mesmo. `pkblocks.inc`: faces por bloco, luz (vidro 0, glowstone/sea lantern 15...), dureza/ferramenta/nível por nome.
- **Save** `chunks16.dat`: paleta de nomes do pack + RLE de 16 bits (trocar de resource pack não embaralha os blocos; bloco ausente vira pedra). Saves antigos (`chunks256.dat`) ainda carregam.
- **Terreno** (`worldgen.inc`): cavernas 3D (salões + túneis, não furam o fundo do mar), veios de minério no estilo 1.12 (carvão, ferro, ouro, redstone, diamante, lápis, esmeralda em montanhas), blobs de granito/diorito/andesito/terra/cascalho, árvores no chunk inteiro (copas cruzam a borda sem cortes) com espaçamento mínimo, cactos, abóboras, melões, podzol (taiga), terra grossa (savana) e argila no fundo. Correção: o ruído do terreno agora refaz quando a seed muda (antes ficava preso à 1ª seed).
- **Multiplayer**: o host envia seed e hora no WELCOME e o convidado gera o mesmo mundo; edições de blocos do pack viajam como hash do nome do arquivo (cada PC pode ter ids diferentes); pacote de edição com id de 32 bits.

## v33 — blocos com modelo (nao-cubo) e vegetacao
- `models.inc`: le os modelos JSON do proprio resource pack (`models/block/torch|wall_torch|lever|lever_on|cross.json`, com `parent`, `rotation`, `rescale`, uv por face) e gera a malha deles no chunk. A bigorna nao existe no pack: usa o modelo vanilla embutido com `anvil.png`/`anvil_top.png` do pack.
- Ids fixos 900..963 (`XB_*` em `main.cpp`): tocha (em pe + 4 de parede), alavanca (12 orientacoes x ligada/desligada; botao direito alterna), bigorna (4 rotacoes), plantas (grama, samambaia, 12 flores, 5 altas de 2 blocos).
- Sem colisao (so a bigorna tem); o raio de mira acerta todos (`pickable`). Tocha emite luz 14 e nao bloqueia luz. Perdem suporte => caem e dropam. Grama/samambaia sao substituidas ao colocar bloco.
- Os itens sao os do inventario do pack (`block/torch`, `block/lever`, `block/anvil`, flores, `*_top` das plantas altas); colocar usa `xbPlace`.
- Vegetacao em `gen()` -> `xbVeg()`: mato por densidade de bioma e manchas de flores (20x20) por bioma.

## v34 — Etapa 1.1: camada de metadados por bloco
- `Chunk::m[]` (1 byte por voxel) guarda orientacao/estado/potencia, em paralelo a `Chunk::b[]`. `getM`, `setBM(x,y,z,id,meta)`, `setM` e `setB` (= `setBM` com meta 0, como o setBlock do Minecraft). Mudar so o meta refaz a malha (e a do vizinho na borda).
- Save `BL17`: igual ao `BL16` + RLE do meta por chunk. `BL16`/`chunks256` antigos carregam com meta 0. Se um bloco do pack some ao trocar de resource pack, vira pedra e o meta daquela posicao e descartado.
- Rede: `NP_BLOCKS` carrega 1 byte de meta por edicao (lote de 80 = 1363 B); `netEdits`/`netPend` guardam `id | meta << 16`; `netSendBlock(..., meta = 0)` e `netApplyEdit(..., meta = 0)`. **Protocolo mudou: host e convidado precisam da mesma versao.**
- Ainda NAO usado pelos blocos existentes (alavanca, agua e bigorna seguem codificadas em ids); a malha tambem ainda nao le o meta. Isso vem nas Etapas 1.2/1.3.
- `tests/run.sh`: checagem de sintaxe do jogo inteiro + testes headless (`tests/test_meta.cpp`) usando stubs de SDL/GL, para ambientes sem SDL2-dev.

## v35 — Etapa 1.2: formas de bloco, raycast por AABB, colisao real
- `blockshape.inc` (novo, incluido antes de `world.inc`): pool de formas (`gShapes`) = lista de ate 8 caixas locais **ou** uma funcao `ShapeFn` que olha vizinhos/meta (padrao para cerca, painel, escada...). Cada id tem forma de **colisao** (`gBlkColl`) e de **selecao** (`gBlkSel`) separadas (cerca: selecao 1,0 / colisao 1,5). Registrar bloco novo: `shapeAdd`/`shapeAddFn` + `blockShapeSet(id, coll, sel)`.
- `solid(id)` = tem colisao; `pickable(id)` = tem selecao; ambos leem a tabela (padrao identico ao anterior: provado para os 65.536 ids em `tests/test_shapes.cpp`). Tocha, alavanca (24 estados), bigorna (2 eixos) e plantas ganharam caixas proprias (valores do 1.12).
- `rayCast()` percorre as celulas e testa as caixas de selecao (slabs): devolve face, ponto, caixa e normal. `ray()` virou wrapper (`h` = bloco, `p` = celula de colocacao = bloco + normal da face) e guarda tudo em `gRay`. `rayCast(..., liquids=true)` mira agua (para o balde).
- `collBoxes()` (physics.inc) usa as caixas reais e varre uma celula abaixo (caixas > 1). `wireCube` -> `wireBox`: contorno da caixa realmente acertada.
- `struct AABB` saiu de physics.inc para blockshape.inc.

## v36 — Etapa 1.3: vizinhos, ticks, colocacao orientada, tronco com eixo
- `blockupdate.inc`: ganchos por id (`blockHooksSet`): `neighbor` (vizinho mudou), `added`, `tick`. `setBM` enfileira a celula e drena **na hora**, em largura (sem recursao; limite `UPD_LIMIT` contra laços entre dois blocos). `scheduleTick(x,y,z,delay)` + `runWorldTick()` (chamado no tick de 20 TPS; so em jogo local ou host). Tocha/alavanca/planta agora caem pelo gancho (`xbNeighbor`); `xbFixAround` foi removido. **Edicoes vindas da rede nao disparam ganchos** (`gNoBlockHooks`): quem editou ja transmite cada edicao da cascata. A geracao do mundo tambem nao dispara (escreve direto no chunk).
- `placement.inc`: `placeBlock(id, PlaceCtx, outPos)` e o unico caminho de colocacao de blocos do inventario: alvo = clicado + normal da face (ou o proprio mato/agua), recusa alvo ocupado (antes dava para colocar pedra sobre tocha/flor), meta (eixo pela face; `PlaceFn` por bloco via `blockPlaceSet` para orientacao por jogador/ponto), sobreposicao com o jogador pelas caixas do bloco **novo**. `PlaceCtx` ja traz ponto de impacto, yaw/pitch e sneak para laje/escada/alcapao/portas.
- Tronco com eixo (`gBlkAxis`; core `LOG` + pack por nome: `_log _wood _stem _hyphae basalt hay_block bone_block quartz_pillar purpur_pillar`): `meta & 3` = 0 Y, 1 X, 2 Z. O mesher usa topo nas pontas e lado nas demais faces, e gira o grao 90 graus nas faces cujo "v" nao acompanha o eixo.
- `ShapeFn` agora recebe `meta`; `blockBoxesId(..., meta)` permite a forma de um bloco ainda nao colocado.
- Limitacoes: ticks agendados nao sao salvos no disco; pistao/redstone/gravidade ainda nao usam estes ganchos (proximas etapas).

## v24 — mundo único, jogador salvo, biomas estilo 1.12.2
- Sem lista de mundos: "Um jogador" entra direto em `saves/mundo` (criado com seed aleatória na 1ª vez). Chunks (`chunks16.dat`), seed/hora/posição (`level.cfg`) e jogador (`player.dat`) são salvos ao sair e o jogador também a cada 5 s.
- `player.dat`: inventário, armadura, mão secundária, item preso ao mouse, vida e fome (blocos/itens do pack salvos pelo nome). No multiplayer o convidado salva só `player_guest.dat` (o mundo é do host; o host salva os chunks, incluindo edições dos convidados).
- `terrain112.inc`: biomas por camadas GenLayer do 1.12.2 (ilhas, zoom, mar profundo, praia, suavização) com 4 biomas de terra: planície, floresta, deserto e montanhas (+ oceano/praia). Relevo por bioma (base/variação do MC); cor da grama/folhas por bioma (média 3x3).
- Testes novos: `tests/test_biomes.cpp`, `tests/test_player_save.cpp`.

## Diversos: nomes em portugues, pilhas e tocha 3D
- `item_data.inc`: tabela `PK_INFO` (arquivo do pack -> nome em portugues + tamanho de pilha) dos itens da aba Diversos; `pkApplyItemData()` roda em `registerPackTab()` depois de montar as abas e tambem atualiza o texto de busca. Para traduzir/limitar outro item do pack, basta adicionar uma linha.
- `PkItem::stack` (padrao 64) alimenta `maxStack()` (main.cpp): baldes cheios, leite, livro e disco = 1; bola de neve, perola do Ender e balde vazio = 16; resto 64. Todo o inventario (criativo, shift-click, arrastar, drops, hotbar inicial) ja usa `maxStack`.
- `drawTorchIcon3D` (inventory_ui.inc): tocha e tocha de redstone no inventario/hotbar usam o modelo 3D do pack (`gXM`) na mesma projecao isometrica do cubo, em vez do sprite plano.

## v40
- `village_build.inc`: conteudo das pecas da vila portado do StructureVillagePieces 1.12.2 (poco, igreja, biblioteca, ferreiro, casas, jardim, cabana, campos, poste) com os blocos por bioma. Escadas/lajes viram blocos cheios; sem colheitas/aldeoes.

## HUD touch (F8) redesenhado
- `touch.inc`: toda a arte e as medidas na MESMA grade de pixel da GUI (1 unidade = `guiScaleFor` pixels, igual à hotbar). Botões: pausa, chat, F5, F1, F3, luz (F7) no topo; joystick, pular, agachar, trocar mão (F), soltar (Q), correr, inventário.
- F1 oculta o HUD; só o botão F1 permanece, com opacidade baixa (`gHideHud` compartilhado com a tecla F1). F5 alterna 1ª / 3ª atrás / 3ª na frente.
- Bedrock: toque em mob = bater (aldeão = interagir); segurar sobre mob = bater a cada 0,5 s; segurar sobre bloco = quebrar; toque em bloco = colocar/usar; segurar com comida (com fome) = comer em 1,6 s (anel enche); segurar com arco = puxar, soltar = atirar.
- Inventário no modo toque (`inventory_ui.inc`, `gInvTouch`): escala inteira máxima em que o painel cabe; tocar seleciona o slot (moldura 24x23 da hotbar), tocar noutro slot move a pilha (troca/junta), toque longo = metade, arrastar leva a pilha, tocar no resultado fabrica. Na paleta do criativo, arrastar na vertical rola a lista. Nenhum item fica "preso" ao cursor.
- Botões de baixo maiores (v26): pular 40, agachar 34, coluna lateral (F/Q/correr) 28 e inventário 30 unidades de GUI (antes 28/24/20/22), com ícones em 2x quando cabem; em telas estreitas (4:3) tudo encolhe até 80% para não encostar na hotbar. Os botões do topo ficaram iguais (20).
- Joystick flutuante (v26, estilo Roblox): a base nasce onde o dedo encosta na zona esquerda da tela (40% da largura, abaixo de 30% da altura; botões e hotbar têm prioridade) e acompanha o dedo se ele passar da borda. Base maior (até 88 unidades, ~36% da altura da GUI; antes até 64). Sem dedo, fica só uma dica apagada no canto. Um segundo dedo na zona esquerda vira "olhar". Tocar rápido na zona esquerda não coloca bloco (só no resto da tela).
