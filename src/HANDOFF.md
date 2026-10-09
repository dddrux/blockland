# HANDOFF — Blockland (correcao profunda em etapas)

Documento para QUALQUER pessoa/sessao continuar o trabalho. Atualizado ao fim de cada entrega.
Estado: **Etapa 1 + Etapa 2 partes 1 (frente), 2 (folhas como bloco), 3 (correcao do tremor + folhas misturadas) e 4 (mudas colocaveis).**

## 0. Como verificar (SEM SDL2-dev, sem janela)
- `tests/run.sh` : checa sintaxe/tipos do jogo inteiro com stubs (`tests/stubs/SDL.h`, `SDL_opengl.h` so DECLARAM a API) e roda `tests/test_*.cpp`.
- Cada teste inclui o `src/main.cpp` REAL (`#define main game_main`) e linka com `-Wl,--unresolved-symbols=ignore-all` (so `SDL_GL_GetProcAddress` e definido no teste). Chamar SDL/GL sem querer = crash = alarme.
- `test_pack.cpp` carrega o resource pack REAL (`applyMcPack(); loadPackItems();`, roda headless) e valida ids/tiles/malha. Sem o zip => SKIP.
- Placar atual: test_meta 38, test_shapes 291, test_update 76, test_pack 54, test_shapes 315 (cresceu: itera os 6 ids novos de muda) = **483 verificacoes, 0 falhas**. Hash do mundo gerado (seed 777, chunks -3..3) = `65237e15ef1d26bc`, IDENTICO ao original (a geracao ainda nao foi tocada; ao mexer na Etapa 5 ele vai mudar de proposito).
- TESTE DE MUTACAO: depois de escrever testes, quebre o codigo de proposito (sed numa copia) e confirme que o teste falha. Ja pegou bugs reais (flags de eixo grudadas ao trocar de pack; `old != v` no fluidOnly).
- LIMITE: nada foi executado com SDL/OpenGL reais. Compile no seu ambiente (`./build.sh`) e olhe visualmente. Itens "so visual" estao marcados abaixo.
- Sonda do pack real: veja `/tmp/packprobe.cpp` no historico da sessao: `applyMcPack(); loadPackItems();` e imprimir `gPkByFile`/`PK[...]`.

## 1. Arquitetura nova (APIs para reutilizar)
| Arquivo | O que e |
|---|---|
| `world.inc` `Chunk::m[]`, `getM`, `setBM(x,y,z,id,meta)`, `setM`, `setB`(=meta 0) | Metadados por voxel (1 byte). Mudar so o meta refaz a malha. |
| `menu.inc` | Save `BL17` = BL16 + RLE do meta; BL16/chunks256 antigos carregam com meta 0. |
| `net.inc` | `NP_BLOCKS` leva 1 byte de meta; `netEdits/netPend` = `id \| meta<<16` (`netPackEdit/netEditId/netEditMeta`). **Protocolo mudou.** Edicao RECEBIDA roda com `gNoBlockHooks=true` (sem cascata). |
| `blockshape.inc` | `AABB`, pool de formas `gShapes`, `gBlkColl[id]`/`gBlkSel[id]` (colisao/selecao SEPARADAS), `shapeAdd`, `shapeAddFn(ShapeFn)`, `blockShapeSet(id,coll,sel)`, `blockBoxesId(id,x,y,z,sel,out,meta=-1)`, `rayCast()` (face, ponto, caixa), `gBlkAxis[]`, `gBlkFacing[]`. `solid()/pickable()` leem a tabela. |
| `world.inc` `ray()` | wrapper: `h`=bloco, `p`=celula de colocacao (=h+normal); detalhes em `gRay`. |
| `physics.inc` `collBoxes` | usa as caixas reais; varre 1 celula abaixo (cerca 1,5). |
| `blockupdate.inc` | `blockHooksSet(id,{neighbor,added,tick})`, `scheduleTick`, `runWorldTick()` (chamado no tick de 20TPS; so local/host). Fila drenada na hora, em largura, limite `UPD_LIMIT`. Tocha/alavanca/planta caem por aqui (`xbNeighbor`). Ticks NAO sao salvos. |
| `placement.inc` | `placeBlock(id, PlaceCtx, out)`: alvo, substituivel (`blockReplaceable`), meta (eixo `gBlkAxis`, frente `gBlkFacing`, `PlaceFn` por bloco via `blockPlaceSet`), sobreposicao pela forma do bloco NOVO, `setBM`+`netSendBlock`. |
| `pkblocks.inc` `pkBuildProps` | tabela `FD[]` de blocos com frente; regra por nome para eixo (`_log _wood _stem _hyphae basalt hay_block bone_block quartz_pillar purpur_pillar`). Zera flags do range do pack a cada carga. |
| `world.inc` mesher | eixo: topo nas pontas, lado no resto, grao girado (`rotUV`); frente: so a face horizontal que aponta para `meta&3` usa `pkFront(id)`. |

Convencoes importantes:
- Faces do RAYCAST/placement (`RayHit.face`, `FACE_N`, `PlaceCtx.face`): 0 -X, 1 +X, 2 -Y, 3 +Y, 4 -Z, 5 +Z.
- Faces do MESHER (`D[]`, `f`): 0 +X, 1 -X, 2 +Y, 3 -Y, 4 +Z, 5 -Z. (Sao DIFERENTES, cuidado.)
- Direcao horizontal (meta de frente): 0 N(-Z), 1 L(+X), 2 S(+Z), 3 O(-X). `facingFromYaw(yaw)` = para onde o jogador OLHA; frente do bloco = oposto (`+2`).
- Eixo (meta&3): 0 Y, 1 X, 2 Z (3 = como Y).
- yaw 0 olha para -Z.
- `blkTile(id,f)`: f 0 topo, 1 lado, 2 base. `iconTile` igual.
- Tocha de parede k=0..3: suporte em -X,-Z,+X,+Z. Alavanca st=0..11: 0..3 parede (suporte +Z,-X,-Z,+X), 4..7 chao, 8..11 teto; +12 = ligada. Bigorna k par = eixo longo em Z.
- IDs: 0-31 base (+NB), 40-47 agua, 900-963 `XB_*` (modelos), 1000+ `PK` (pack, 3062 no Ashen_16x). `isPk/isPkBlk/isXB/isBlk`.

## 2. Fatos descobertos sobre o resource pack (Ashen_16x) — evitam retrabalho
- O pack e PARCIAL: muitos blocos (dispenser, dropper, piston, bau, cacto, TNT, vidro...) NAO tem `models/block/*.json`; so texturas. Nao dependa de modelos; use convencao de nome + tabela.
- `pkIsCube` exige textura 100% opaca (ou vidro/gelo) e sem sufixo `_top/_front/...`. **Por isso folhas, mudas, flores etc. (com alpha) NAO viram bloco** — raiz do bug "folhas nao sao blocos".
- Texturas usadas pelos blocos centrais (`oak_log`, `oak_leaves`...) entram em `gPackUsed` e NAO aparecem em `PK` (ex.: `gPkByFile["block/oak_log"]` nao existe).
- Variantes aleatorias (`oak_log1..12`, `crafting_table1..8`) sao sorteio por posicao do pack, nao blocos distintos (filtradas por `isdigit` no fim do nome).
- `carved_pumpkin.png`/`jack_o_lantern.png` sao so o rosto; topo/lado/base = `pumpkin_top/side/bottom` (corrigido). `dispenser_front`/`dropper_front` so tem a frente; lados/topo = `furnace_side/top` (corrigido).

## 3. Status dos 53 itens do pedido original
Legenda: OK = feito+testado headless | PARC = parcial | VIS = falta conferir visualmente | TODO = nao iniciado

1 Analise: OK (README + este arquivo). 2 Modelo real em raycast/colisao/selecao: PARC (infra OK; tocha/alavanca/bigorna/plantas registradas; falta registrar cerca, painel, porta, alcapao, trilho, escada, laje, cacto, redstone, cama, bau...). 3 Topo/lado/base: PARC (PK ok; eixo e frente ok; VIS). **4 Abobora/melancia: OK dados+malha+colocacao+icone (VIS: conferir mao/inventario).** 5 Arvores: PARC (troncos com eixo OK; **folhas como bloco: TODO**; arvores usam so LOG/LEAVES core). 6 Vegetacao/flicker: TODO. 7 Saplings: TODO. 8 Cacto: TODO. 9 Infra nao-cubica: PARC (shapes OK; falta modelo/UV/colocacao por bloco). 10 Barras de ferro: TODO. 11 Vidros: TODO. 12 Slime: TODO. 13-17 Redstone/pistao/dispenser logica/repetidor/alavanca: TODO (ganchos+ticks prontos; dispenser ja e bloco com frente 4 direcoes; falta 6-dir, GUI, logica). 18 Alcapao: TODO. 19 Portas: TODO. 20 TNT: TODO. 21 Teia: TODO. 22 Gravidade: TODO (usar gancho neighbor+scheduleTick). 23 Liquidos/baldes: TODO (rayCast(...,liquids=true) ja existe; agua usa ids 40-47). 24 Bau: TODO. 25 Cama: TODO. 26 Trilhos/carrinhos: TODO. 27 Barco: TODO. 28 Elytra: TODO. 29 Cavalo: TODO. 30 Animacao de comer: TODO. 31 Bolo: TODO. 32 Toca-discos: TODO. 33 Modelos de personagens: TODO. 34 Sombra 16x16: TODO. 35 UTF-8/acentos: TODO. 36 Itens 2D vs 3D: TODO. 37 Stack: TODO. 38 Pocoes: TODO. 39 Mesa de encantamento: TODO. 40 Girassol: TODO (caixa de selecao de planta alta ja registrada, ver duvida). 41 Cercas: TODO. 42 Geracao de mundo: TODO. 43 Assets: em andamento (usar sempre o pack). 44 Selecao/raycast: OK para blocos existentes (resto vem com cada bloco). 45 Colocacao: OK infra (falta regras por bloco: escada, laje, porta, trilho...). 46 Testes: em andamento (440). 47-53: regras de trabalho.

## 3b. Etapa 2 parte 2 (feito)
- **Folhas = bloco**: `pkIsCube` aceita `*_leaves`; `pkBuildProps` preenche `gPkLeaf` (1 tinta bioma, 2 abeto fixo, 3 betula fixo, 4 sem tinta) e `gPkLog`. Helpers em main.cpp: `leafKind/isLeafB/isLogB` (cobrem o LEAVES/LOG core E o pack). Usados no mesher (face, tinta `tm==3` cor fixa), luz (opac 1), decaimento (`leafSupported`), quebra/colocacao/rede (`keepLeaves`).
- **Arvores**: `treeSpecies(sp)` troca `gTreeLog/gTreeLeaf` por especie (abeto, betula, selva, acacia) se o pack tiver tronco E folha; senao carvalho core (mundo sem pack inalterado). Hash do mundo MUDA com o pack (so ids de arvore), conferir.
- **Drops**: folha do pack solta 5% da muda da propria especie; folha core: 5% muda de carvalho, 3% maca.
- **Rachaduras**: `initCrack` usa `block/destroy_stage_0..9` do pack (fallback procedural).
- Falta (parte 2 continuacao): partLook (particulas) ainda so tinge LEAVES core; tinta fixa de abeto/betula e aproximada (0,38/0,60/0,38 e 0,50/0,65/0,33); mudas ainda nao sao colocaveis (item 7); cacto, vidro, cerca etc. seguem TODO.
- VIS: conferir folhas de abeto/betula/selva/acacia na tela e as rachaduras novas.

## 3c. Etapa 2 partes 3 e 4 (feito)
- **Tremor da vegetacao (causa)**: o atlas era `GL_NEAREST` SEM mipmap; de longe cada pixel amostra 1 texel de uma textura 16x16 com furos de alpha, e o texel muda a cada movimento da camera => folhas/mato/flores piscam. Correcao: `uploadAtlasMips()` (assets_atlas.inc) gera 4 niveis POR TILE (cor ponderada pelo alfa, alfa = cobertura; 16px => cabe no tile, sem vazamento), MIN `GL_NEAREST_MIPMAP_LINEAR`, MAG `GL_NEAREST`. Alpha test do mundo 0,02 -> 0,1 (como o MC). NAO foi z-fighting (folha-folha ja resolvido) nem depth test.
- **CORRECAO DA CORRECAO (3d)**: o mipmap acima NAO funcionava no jogo: `drawIcon` (inventory_ui.inc, chamado todo quadro pelo hotbar) religava o atlas e setava `MIN_FILTER=GL_NEAREST`, desligando os mipmaps. Removido; o filtro agora so e definido em `uploadAtlasMips()`. Teste P8 (test_pack) varre o codigo e falha se alguem setar MIN_FILTER logo apos bind do atlas fora de assets_atlas/main_loop. REGRA: nunca mexer no filtro do atlas fora do upload.
- **Folhas misturadas**: antes, ao encostar duas especies, so UMA face ficava no plano (a do bloco de menor coordenada), entao a textura de uma especie aparecia "dentro" da outra. Agora, especies diferentes desenham cada uma a sua face, afastadas 0,003 (`leafMix`, world.inc); mesma especie continua com 1 face.
- **Mudas**: `XB_SAPLING`=964..969 (carvalho, abeto, betula, selva, acacia, carvalho escuro), `XB_END`=970. Modelo cross do pack, sem tinta, sem colisao, selecao de planta, solo = grama/terra/terra grossa/podzol (`xbSoilOk`), nao dentro d'agua, caem sem solo (gancho de vizinhos), dropam o proprio item. Folhas dropam a muda da especie (5%).
- **Falta nas mudas**: CRESCER em arvore (precisa de tick aleatorio/`scheduleTick` + `placeTree` com especie forcada; ticks nao sao salvos); regra de luz; muda de acacia/carvalho escuro 2x2.
- VIS: conferir se o pisca sumiu (se ainda houver, tente `ATLAS_MIPS` 3 ou alpha test 0,2) e se a mira acerta a muda.

## 4. Duvidas / aproximacoes a conferir
- Caixa de selecao das plantas ALTAS (girassol/lilas/roseira): usei a mesma das flores (0,3..0,7 x 0..0,6 por metade) por memoria do `BlockBush` do 1.12; incerto. Sem deslocamento aleatorio XZ do 1.12.
- Icone 3D da frente: face ESQUERDA do cubo (inventory_ui.inc) e a face -Z na mao/chao (render_items.inc). So visual: conferir se parece com o Minecraft.
- Camera 3a pessoa (`raySolidDistance`) ainda recorta por celula inteira.
- Ticks agendados nao persistem no save; ganchos nao rodam na rede (convidado recebe edicoes prontas).

## 5. Proximos passos sugeridos (ordem)
Etapa 2 (continuacao): (a) **folhas como bloco**: ver `pkIsCube` — aceitar `*_leaves` (alpha) como cubo "folhagem" (transparente em `gPkOpac`, tinta de bioma, `LEAVES` core fica para carvalho); usar nas arvores; decaimento por tipo. (b) **cacto** (`cactus_top/side/bottom`; forma 1/16 recuada via `shapeAdd`, dano ao encostar, so sobre areia/cacto). (c) **vidro**: pipeline translucido separado (mesh em passe com blend, depth mask off, ordenacao), `gPkOpac` ja zera vidro. (d) **cerca/painel/barras**: `shapeAddFn` (poste + bracos por vizinho) + malha dinamica por vizinhos (precisa ler `getB` dos vizinhos no mesher, como `gb`) + `blockHooks.neighbor` so se o estado for em meta. (e) **porta/alcapao/escada/laje**: `PlaceFn` usando `PlaceCtx.fy/yaw`. (f) **slime** (coll cubo, quique em `physics.inc`), **teia** (sem colisao, lentidao), **TNT** (ignicao por gancho, entidade, explosao).
Etapa 3: gravidade (areia/cascalho como entidade `FallingBlock`), liquidos com niveis em meta (migrar ids 40-47), redstone (sinal 0-15 em meta + ticks), pistao, dispenser (GUI + inventario em mapa por posicao, salvar!), carrinho/trilho, barco, elytra, cavalo, comer, bolo, toca-discos.
Etapa 4: UTF-8 na fonte (`font.inc`), 2D vs 3D de itens, tabela de stack, pocoes/brewing, encantamento, bau (precisa de armazenamento por bloco: ainda NAO existe).
Etapa 5: geracao (ruido, cavernas, biomas, arvores, minerios). Hash do mundo vai mudar.


## Atualizacao: menu de mundos + seeds + rios (sessao asd.zip)
- **Singleplayer** agora abre "Selecionar Mundo" (screens.inc, pagina `mundos`, lista estilo 6): Jogar (ou duplo clique), Criar Novo Mundo, Editar (renomear), Excluir (confirmacao), Recriar (mesma seed/modo/nome), Cancelar. Sem mundos => vai direto p/ "Criar Novo Mundo". Botoes sem destino chamam `pageAction(id)`.
- **Criar Novo Mundo**: nome + modo (Sobrevivencia/Criativo) + seed (em "Mais Opcoes"). `createWorld()` (menu.inc) grava `saves/<pasta ASCII unica>/level.cfg` e entra no mundo. `parseSeed()`: vazio = aleatoria; numero = o proprio; texto = `String.hashCode()` do Java. `/seed` mostra a seed no chat.
- `saveCurWorld` agora PRESERVA o nome do mundo (antes gravava sempre "Mundo").
- **Terreno**: rios sinuosos em `terrain112.inc` (`riverFactor`/`carveRiver`, `RIVER_W` ajusta a largura). Hash do mundo mudou de proposito. Teste novo: `tests/test_rivers.cpp` (rios, emendas entre chunks, determinismo, seeds, gerenciamento de mundos).
- Nao verificado com SDL/GL real: compile (`./build.sh`) e confira visualmente a tela de mundos e os rios.

## Atualizacao 2: menu de mundos corrigido, sem corte por campo de visao, mipmaps de folhas/mato
- Lista de mundos (e de servidores): a moldura de selecao agora tem 36 de altura (= passo da linha, como o Java); antes 40 e cobria a linha seguinte.
- `main_loop.inc drawChunks`: REMOVIDO o corte de chunks fora do cone de visao (ao girar rapido eles "ressurgiam"). Resta so o corte por distancia (alem da nevoa).
- `assets_atlas.inc`: mipmaps com **cobertura de alfa preservada** (`tileCoverage0` / `mipCoverage`): por tile com furos e por nivel, o alfa e escalado (busca binaria) para manter a fracao de texels que passam no alpha test (0,1). Folhas nao engordam e mato fino nao some ao longe. Tiles sem furos (blocos solidos) nao sao alterados. Teste: `tests/test_mipcov.cpp`.
- Nao verificado com SDL/GL real: confira visualmente a vegetacao a distancia e o giro rapido da camera.

## Atualizacao 3: cacto
- Causa: a textura do cacto do pack tem borda transparente (corpo 14x14 + espinhos) e o jogo o desenhava como cubo cheio => cantos furados (via-se o interior preto e o ceu).
- Agora (modelo do pack, `cactus.json`): as 4 faces laterais sao planos RECUADOS 1/16 cobrindo 0..1 no outro eixo (`world.inc`, push -0,0625 quando `b == gCactusId`); topo/base cheios. Colisao 1/16..15/16 em XZ e 15/16 de altura; selecao ate o topo (`pkBuildProps`, ids de forma criados uma vez). No pack o cacto e ancorado em `block/cactus_top` (sem cactus.png): `gCactusId` vem de `gPkByFile["block/cactus"]`.
- Teste: secao "cacto" em `tests/test_pack.cpp` (4 planos recuados, topo, caixas de colisao/selecao).
- Nao verificado com SDL/GL real. As variantes do blockstate (cactus1..3, top1..3) nao sao usadas: o jogo usa so o modelo base.

## Atualizacao: cacto fantasma, faces soltas do pack, modulos
- Cacto: `gPkOpac[cacto]=0` (pkblocks.inc) -> a areia embaixo desenha o topo (antes: anel vazado de 1/16). Cactos empilhados nao desenham topo/base coplanares (world.inc).
- Faces soltas (`cactus_side`, `furnace_front`, `tnt_side`...): `pkFaceStem/pkIsFaceOnly/pkRealBlock` (mods.inc). O criativo redireciona para o bloco real; aba "Resource pack", busca e `/give` ignoram faces soltas. Itens colocaveis por modelo (xbItemBlock) nao sao afetados.
- Modulos: `src/modules/` (ver README la). Ganchos ligados em main_loop.inc e chat_debug.inc. Teste: tests/test_modules.cpp.
- Placar: test_pack 171, test_modules 10, demais iguais. Nada rodado com SDL/GL real.

## Atualizacao: mao segura o item de pe (1a pessoa)
- `player_render.inc drawHand`: com item na mao, braco + item formam um grupo preso ao PUNHO (pivo). Ferramentas/armas (`heldDiagonal`) giram 45 graus para ficar de pe; itens planos e blocos tem poses proprias (`HP_TOOL/HP_ITEM/HP_BLOCK`, valores ajustaveis). Mao vazia: pose antiga. 3a pessoa: `displayXf` inalterado. `drawHeldRaw` (render_items.inc) desenha sem displayXf.
- Pose ajustada no pre-visualizador em software `tools/handprev.cpp` (roda o drawHand real com um GL minimo). Nao validado no OpenGL real: ajuste as constantes olhando o jogo.

## Passada A (geracao identica ao 1.12.2) + correcao multiplayer
- Cavernas e ravinas: `worldgen.inc` agora e porte exato de MapGenBase/MapGenCaves/MapGenRavine (vizinhanca 17x17, semente por chunk, MathHelper.sin/cos por tabela, digBlock, lava em y<10). A faixa de lava fixa y<=10 foi removida. Hash do mundo MUDOU de proposito.
- Bug da coleta de drop em multiplayer: `NP_DROPSET` ganhou 1 byte `picker` (255 = sem animacao, ex.: juncao de pilhas). Quem recebe cnt<=0 com picker remoto cria `PickFx{who=picker}`; `drawPickups` voa o item ate a posicao interpolada do jogador remoto (`netRemotePos`). **Protocolo mudou**: todos os jogadores precisam da mesma versao.
- Proximas passadas (ordem): B) GenLayer completo (rio, taiga, pantano, selva, savana, mesa, gelo + base/var de cada bioma); C) BiomeDecorator na ordem do MC (lagos, masmorras, minerios WorldGenMinable com semente de populacao, areia/argila/cascalho, arvores por bioma, grama/flores, cactos, abobora); D) structures (minas, vilas); E) neve/gelo.

## Passada B (biomas = GenLayer completo do 1.12.2)
- `terrain112.inc`: pilha completa (Island, FuzzyZoom, AddIsland, RemoveTooMuchOcean, AddSnow, Edge x3, Mushroom, DeepOcean, RiverInit, Biome, BiomeEdge, Hills, River, Smooth, RareBiome, Shore, RiverMix, VoronoiZoom). Ids internos = ids de bioma do MC (0..39, 129+ mutacoes); `vbGame()` converte para os 11 biomas do jogo; `vbBV()` da base/variacao de relevo por bioma do MC.
- Rios agora sao o bioma Rio (base -0.5): o antigo `carveRiver`/`riverFactor` foi removido. Terreno usa genBiomes 1:4 (camada `mix`) e Voronoi (`vor`) por coluna.
- Pendente (passada C): superficie por bioma exata (mesa, swamp, taiga/podzol, oceano gravel/dirt, gelo/neve), BiomeDecorator na ordem do MC. Mesa/jungla/savana usam so os tipos de bloco ja existentes.
- Verificacao feita: compila isolado (harness com stubs) e distribuicao de biomas plausivel (rios ~4,5%, mutacoes 1-2%). NAO comparado com um mundo real do MC.

## Passada C (superficie por bioma + populacao)
- `terrain112.inc`: superficie (generateBiomeTerrain) agora por bioma do MC: deserto/praia = areia, praia de pedra = pedra, mega-taiga = podzol/terra grossa, savana mutada, montanhas (cascalho/pedra por ruido), cogumelo = micelio, mesa = areia vermelha + faixas de terracota (banda simplificada), gelo em fundo frio. Oceano/rio agora tem fundo de terra/cascalho como o MC (o antigo sorteio por hash de areia/cascalho saiu).
- `worldgen.inc` `populateChunk`: semente de populacao identica ao MC (`nextLong()/2*2+1`), veios `WorldGenMinable` exatos (ordem dirt, gravel, granito, diorito, andesito, carvao, ferro, ouro, redstone, diamante, lapis; todos substituem qualquer pedra), discos de areia x3 / argila / cascalho (so em agua), esmeralda em montanhas. Chunk C aplica as populacoes de P em {C-1,C}^2 recortadas em C. Alturas dos vizinhos vem de `gFloor` (limpo em `resetWorldData`).
- NAO portado: lagos, masmorras (consomem sorteios do MC, entao as posicoes dos veios nao batem 1:1, so a distribuicao), arvores/plantas por bioma (continuam com o sistema antigo), pantano (nenúfar/agua), neve/gelo em camadas, estruturas.
- Verificacao: compila isolado; 49 chunks: ~197 carvao, ~118 ferro, ~2,8 diamante por chunk (ordem de grandeza do MC).
- Proxima passada D: BiomeDecorator completo (arvores por bioma com as classes de arvore do MC, grama alta/flores, cogumelos, cana, abobora, cacto, nascentes de agua/lava), depois lagos+masmorras, neve/gelo, estruturas.

## Passada: seed exata do MC 1.12.2 (64 bits) + ruido de superficie
- `WORLD_SEED` agora e `int64_t` (long com sinal, como o MC). Arquivos: core_noise.inc, main_loop.inc, menu.inc (SaveInfo/WInfo/level.cfg `seed=%lld`, saves antigos continuam lendo), net.inc (**protocolo mudou**: WELCOME leva 8 bytes de seed; host e convidado precisam da mesma versao), terrain112.inc (`gBioTileSeed` 64 bits).
- `parseSeed()` identico ao MC (testado contra o Java real em 20 casos): vazio ou `0` = aleatoria; `Long.parseLong` (aceita `+`/`-`); estouro ou texto = `String.hashCode()` sobre UTF-16 (acentos/emoji ok); NAO tira espacos (como o MC). `randomSeed()` agora 64 bits.
- `SimplexImp::add` (ruido de superficie): z externo, x interno (indice z*16+x), como `NoiseGeneratorSimplex.add` do 1.12.2. Antes estava transposto. `-DSURF_X_MAJOR` volta ao layout antigo para comparar. Corte de altura `(float)(l1-29)/3.0f` agora em float como o MC.
- DUVIDAS abertas (nao comparadas com um MC real): o `xSc /= 1.5` e a progressao escala/amplitude em `SurfNoise::region` (NoiseGeneratorPerlin.getRegion). Se a espessura de terra/podzol/pedra nao bater com o MC, e o primeiro suspeito.
- Hash do mundo MUDA de proposito (superficie). Proximo: BiomeMesa exato (faixas/pilares), pantano (ruido por GRASS_COLOR_NOISE e nenufar), depois lagos+masmorras (consomem o RNG de populacao, necessarios para os veios baterem 1:1), arvores por bioma.

## Atualizacao: aba REDSTONE com modelo + colocacao (src/redstone.inc)
- 17 ids XB novos (974..990, `XB_END`=991): poeira, pistao/grudento, dispenser, dropper, observador (dir 6 vias), placa (4 tipos), botao (2), gancho, sensor de luz, funil, repetidor, comparador, alcapao (7), portao (6), porta (7, 2 blocos), bau armadilhado. Estado TODO no meta (layout no topo de redstone.inc).
- Malha sob demanda por (id, meta) com cache (`rsQuads`); modelos do pack quando existem (porta/alcapao/portao/funil/gancho), resto por caixas (`rsBx`). Formas por meta (`rsShape`), suporte (`rsSupported`), drop por variante (`rsDropItem`, usa `gRsLastMeta` guardado pelo setBM), colocacao (`rsPlace`), uso (`rsUse`: abre/fecha porta, alcapao, portao; atraso do repetidor; modo do comparador).
- Itens: `rsMapItems()` (chamado ao fim de `loadCreativeCfg`) liga cada item da aba ao bloco+variante por NOME (ex. "Oak Pressure Plate") ou por arquivo.
- Fora de escopo (sem logica): sinal de redstone, pistao empurrando, dispenser/funil com inventario, bau com armazenamento, botao/placa acionando. Nota/TNT/lampada/bloco de redstone seguem como cubos.
- VIS (conferir na tela): orientacao da textura do repetidor/comparador; dobradica da porta (`_hinge`); pistao/observador voltados ao jogador; icone plano de placas/botoes.
- Teste novo: `tests/test_redstone.cpp` (360 verificacoes, 0 falhas). `test_shapes` agora ignora ids da aba (formas proprias). Falha antiga e nao relacionada: `test_shapes` `gapBad` (xbGap desatualizado desde a tocha de redstone).

### Correcoes da aba Redstone (v0.2.2)
- Repetidor/comparador agora tem as tochas (repetidor: fixa + movel conforme o atraso; comparador: 2 de tras + 1 da frente, alta no modo subtrair). Tochas acesas quando `lig`.
- Porta: o lado da lamina estava errado (`kk=(f+1)&3`, antes +3). Selecao/colisao de porta e alcapao agora vem dos LIMITES DA PROPRIA MALHA (`rsShape` casos 13/15): nao divergem do modelo.
- Icones: placa, botao, portao, alcapao, sensor e bau armadilhado usam a malha do bloco (`rsIconQuads` -> `drawTorchIcon3D`); funil usa `item/hopper`.
- Nomes em pt-BR (`rsNames`, roda depois de `pkApplyItemData`). test_redstone: 460 verificacoes.


## Atualizacao: navegador proprio do Monitor (PIE Web)
- Desktop deixou de usar Chrome/CDP (o Chrome 153 abria e navegava, mas nao entregava NENHUM quadro: ver logs/monitor-browser.log). `cdp.inc` foi removido; o motor novo esta em `src/modules/monitor/web/` (README la).
- `monitor.inc`: bloco desktop agora so inclui `web/web.inc`. GUI nova: voltar/avancar/recarregar-parar, barra de progresso, status (URL sob o mouse/titulo/zoom), barra de rolagem, zoom (Ctrl +/-/0, Ctrl+roda), Alt+setas.
- Padrao desktop: `about:home` + busca DuckDuckGo html (funciona sem JS). Android continua com Google/WebView.
- LIMITE: sem JavaScript. NAO testado em Windows (WinHTTP) nem com HTTPS real (sandbox sem rede): testado com libcurl contra servidor local.

## Estandartes com nome de bigorna + BAU funcional (3 partes) — PARTE 1 FEITA
- **Bug dos nomes**: `gXbVar` (item -> variante) e compartilhado por bigorna (dano 0..2) e estandarte (cor 0..15); `rsNames()` (redstone.inc) nomeava TODO item de `gXbVar` como "Bigorna...". Agora olha o bloco do item em `gXbItem`: `XB_ANVIL` => Bigorna/levemente/muito danificada; `XB_BANNER` => "Estandarte <cor>" (tabela `BN[16]` em pt-BR). Teste: `test_banner.cpp` (nome das 16 cores + bigornas continuam certas). Mutacao conferida.
- **Parte 1 do bau = armazenamento** (`src/chest.inc`, incluido depois de `redstone.inc`): `gChests` mapa (x,y,z) -> `ChestInv` de 27 slots. API: `chestAt(x,y,z,create)` (so bau tipo 0/1; Ender 2 e shulker 3..18 ficam p/ Parte 3), `chestAdd` (empilha, devolve o resto: servira p/ funil), `chestCount`, `chestOnRemoved`. `setBM` (world.inc) chama `chestOnRemoved` quando o ID do bau SAI (quebrar/explodir/substituir; girar/abrir so muda meta e nao solta nada); so local/host solta os itens (convidado nao tem o conteudo). Salvar/carregar: `saves/<mundo>/chests.dat` (texto; item do pack por nome `@block/...`, core por `#id`), ligado em `saveCurWorld`, `resetWorldData` e carga do mundo (main_loop.inc). Teste: `tests/test_chest.cpp` (44 verificacoes) + mutacao conferida.
- **Parte 2 FEITA (tela)**: clique direito no bau abre `gChest` (inventory_ui.inc: slots `SL_CHEST+k`, 27 ou 54; layout/slots do GuiChest do Java; painel montado com recortes da `crafting_table.png` do Rethoughted GUI). Clique/direito/shift (bau->mochila de tras p/ frente; mochila->bau de cima p/ baixo)/tecla 1-9/arrastar/duplo clique. Agachado com item na mao coloca bloco; bloco cheio em cima impede abrir (vale p/ as duas metades). Fecha sozinho se o bau some ou o jogador passa de 8 blocos.
- **Parte 3 FEITA (duplo, tampa, rede)**: bau duplo = tipos 19..22 no meta (19 normal-esq, 20 normal-dir, 21/22 armadilhado; esq/dir vistos de frente), `CH_RV[dir]` = lado direito. Colocacao como `BlockChest` 1.12.2 (`chestPlaceMeta`: so 1 vizinho do mesmo tipo e nao duplo; junta se o vizinho esta no eixo perpendicular a frente; a parceira assume a frente nova). Metade superior = menor X/Z (InventoryLargeChest). Quebrar uma metade solta so o conteudo dela e a outra vira simples. Modelo: `entChest(type, dir, o, half, open)` com `chest/{normal,trapped}_{left,right}.png` do pack (sem elas cai no bau simples). Tampa aberta = bit 7 do meta (vai pela rede como bloco; sem animacao suave, abre/fecha de uma vez). Rede: NET_VER 5, `NP_CHEST` (27 slots por nome-hash) e `NP_CHESTREQ`; o HOST e dono do conteudo, o convidado pede ao abrir, manda o estado a cada mudanca e o host repassa. Salvo em `chests.dat` (as duas metades), tambem ao salvar com a tela aberta. Teste: `tests/test_chest2.cpp` (49).
- **Fora de escopo ainda**: Ender (por jogador), caixas de shulker, sinal de redstone do armadilhado, sons e tampa animada, funil/dispenser com inventario.
- Nada rodado com SDL/GL real; so compila (`-fsyntax-only` OK) e testes headless.

## Barco no multiplayer + animacao (boat.inc / net.inc)
- **Autoridade**: quem monta simula o bote e manda `NP_BOAT` todo tick; sem ninguem a bordo o HOST simula (keepalive de 1 s, e `netSyncPeer` manda os botes a quem entra). Os demais so enfileiram estados (`BoatEnt::q`, max 2/tick como os jogadores remotos) e interpolam (`boatRemoteTick`). `boatAuth(b)` decide.
- Pacotes novos: `NP_BOAT` (estado), `NP_BOATDEL` (quebrou), `NP_BOATHIT` (golpe de quem nao e dono: o dono aplica `boatApplyHit`). **NET_VER 5 -> 6** (protocolo mudou). IDs de bote = `(netSelf<<20)|contador` (host mantem ids pequenos, saves antigos funcionam).
- Montaria: 1 jogador por bote (`rider` = id de rede, -1 nenhum). Disputa simultanea: menor id de rede vence. Se o montador sai do jogo, o host zera `rider`.
- Jogador remoto montado: `netDraw` o prende ao assento (`boatRiderBegin`), pose sentada vanilla (`Pose.sit`), bracos remando no ritmo dos remos (`Pose.row`), cabeca presa a 105 graus.
- Animacao (so visual, `boatVisTick/boatVisOff`, igual em todos os clientes): balanço nas ondas (bob 1,6 cm + arfagem/rolagem de ~1 grau), casco inclina para fora na curva (max 6 graus), proa sobe ao acelerar. O passageiro (local e remoto) acompanha o mesmo balanço.
- Testes: `tests/test_boat_net.cpp` (20 verificacoes). `test_player_save` ja segfaultava no projeto original (nao relacionado).
- LIMITE: nada rodou com SDL/OpenGL reais; conferir visualmente (sinal da rolagem, altura do passageiro). Animais a bordo continuam locais (mobs nao sao sincronizados na rede).
