# MAPA do codigo (gerado por tools/mapa.py - nao editar)
Jogo = 1 unidade de compilacao (main.cpp inclui os .inc na ordem). Achar algo: `tools/onde.sh <nome>` -> arquivo:linha; ler so o trecho (`sed -n X,Yp`).
Modulos novos: `tools/novo_modulo.sh <nome>` + APIs em modules/api.inc (ver modules/README.md).

| arquivo | linhas | o que faz |
|---|---|---|
| core/control.inc | 71 | patchCtl.1: --control DIR (print/estado/comandos por arquivo) |
| core/core_noise.inc | 26 | ruído |
| core/discord.inc | 75 | patchDiscord.1: Rich Presence (IPC puro, thread propria; discord.cfg) |
| core/frustum.inc | 34 | culling por chunk (opcional, BLOCKLAND_CULL=1) |
| core/gif.inc | 73 | patchCapa.2: decodificador GIF (capas animadas) |
| core/jobpool.inc | 48 | patchChunks.1: pool de threads (malha/luz/geracao de chunks); define BL_TLS (antes do perf.inc) |
| core/keybinds.inc | 80 | patchKeys.1: teclas configuraveis (usado por screens.inc e keymap.inc) |
| core/keymap.inc | 26 | BUG-015: keys.cfg |
| core/launcher.inc | 166 | perfil: nome + skin (usado por net.inc e screens.inc) |
| core/logger.inc | 228 | logger: sistema de logs no formato do Minecraft Java |
| core/perf.inc | 137 | profiler de lag (logs [PERF]/[LAG]) |
| core/png.inc | 156 | PNG (decoder mínimo, sem dependências) |
| core/vorbis.inc | 189 | patchAud.OGG: decodificador Ogg Vorbis (sem libs) |
| game/audio.inc | 465 | sons: mixer SDL proprio, .ogg (preferido) e WAV/IMA-ADPCM em assets/sounds (patchRPI.11, patchAud.OGG) |
| game/chisel_tool.inc | 22 | Cinzel (Chisel): botao direito num bloco do Chisel troca para a proxima variante do mesmo material (agachado:  |
| game/craft_extra.inc | 182 | patchCraft.3: receitas que faltavam + qualquer madeira (usa gRsDrop/gXbDrop; chamado por packRecipes em chest. |
| game/crafting.inc | 81 | receitas (Minecraft Java 1.8.9) para os itens que existem no Blockland |
| game/drops.inc | 187 | itens no chão (EntityItem do Minecraft 1.8.9) |
| game/effects.inc | 34 | patchPot.1: efeitos de status (pocoes) |
| game/elytra.inc | 80 | elitra + foguetes (voo planado, desgaste, dano de parede) |
| game/enchant.inc | 123 | patchEnch.1: encantamentos (tabela, nbt, /enchant) |
| game/physics.inc | 334 | física do jogador (port do Minecraft Java 1.8.9) |
| game/survival.inc | 275 | sobrevivência: vida, fome, dano, regeneração, morte |
| game/use_items.inc | 170 | usar item no mundo: tesoura, comida de mobs (amor), balde (movido de main_loop.inc em patchOrg.4) |
| loop/events.inc | 242 | eventos SDL (SDL_PollEvent), HUD touch, gamepad |
| loop/frame_begin.inc | 178 | inicio do quadro: teclado, lambdas de uso de item/quebrar, ajustes por quadro |
| loop/init.inc | 234 | inicializacao: SDL/janela/GL, pack, mods, modulos, atlas, estado do jogador (antes do laco) |
| loop/menus_timing.inc | 85 | menus/telas, carregamento, autosave, dt, repeticao de acao, animacoes |
| loop/render.inc | 759 | camera interpolada, mundo por viewport, mobs, HUD, tela (ate o fim do quadro) |
| loop/tick.inc | 164 | ticks fixos de 20/s (tickAcc): fisica, mobs, redstone, rede |
| main.cpp | 413 | Blockland - clone voxel próprio, sem engine, inspirado no estilo do Minecraft PE antigo. |
| main_loop.inc | 22 | main(): esqueleto. O corpo vive em fragmentos por fase (src/loop/*.inc), incluidos AQUI dentro da funcao: init |
| mobs/bow.inc | 235 | arco e flecha |
| mobs/egg.inc | 119 | ovo arremessavel (usa bow.inc: arrowGauss/arrowSegBox, mobs.inc) |
| mobs/fishing.inc | 101 | BUG-019: vara de pesca (boia, mordida, pesca; usa bow.inc/craft_extra.inc/drops.inc) |
| mobs/hostile.inc | 455 | mobs hostis: zumbi, esqueleto, creeper (IA, ataque, flechas, explosao, spawn) |
| mobs/mobs.inc | 1093 | mobs com IA básica: porco, galinha, vaca, ovelha |
| mobs/mobsync.inc | 102 | mobs na rede: host simula e transmite, convidado interpola; mira em jogadores remotos |
| mobs/pvp.inc | 84 | PvP + morte de jogador (usa net.inc, mobs.inc, survival.inc) |
| modules/api.inc | 68 | patchMod.1: mini-APIs estaveis para modulos (api::blk/chat/hud/item/mob/cmd/cfg) |
| modules/exemplo/exemplo.inc | 16 | modulo de exemplo: molde para criar o seu (usa as mini-APIs de src/modules/api.inc) |
| modules/itemphysic/itemphysic.inc | 17 | modulo itemphysic (port de ItemPhysic Lite 1.3.7, MC 1.12.2) |
| modules/jade/jade.inc | 122 | modulo jade (port de Jade 0.1.0 / Snownee, addon do Hwyla, MC 1.12.2) |
| modules/jvm/aot_bridge.inc | 33 | patchJVM.5: ponte do "pseudo Forge" (runtime AOT em src/modules/jvm/aot) com o jogo: rt::host() passa a ler/gr |
| modules/jvm/jvm_class.inc | 107 | jvm: parser de .class (Java 8 / Forge 1.12.2) |
| modules/jvm/jvm_coll.inc | 81 | jvm: java/util (ArrayList, HashMap, HashSet, Iterator, Arrays, Collections) |
| modules/jvm/jvm_exec.inc | 212 | jvm: interpretador de bytecode (todos os opcodes de Java 8, exceto jsr/ret e invokedynamic -> jvm_indy.inc) |
| modules/jvm/jvm_indy.inc | 44 | jvm: invokedynamic (lambdas e method refs via LambdaMetafactory) |
| modules/jvm/jvm_rt.inc | 123 | jvm: runtime Java minimo (java/lang/*) em C++ nativo |
| modules/jvm/jvm_vm.inc | 80 | jvm: VM (objetos, classes, resolucao). Interpretador em jvm_exec.inc, runtime Java em jvm_rt.inc |
| modules/jvm/jvm_zip.inc | 36 | jvm: leitor de .jar (zip) usando inflateZ de core/png.inc |
| modules/modules.inc | 48 | registro de ganchos dos modulos (ver src/modules/README.md) |
| modules/modules_list.inc | 13 | modulos ativos: entram DEPOIS de toda a API do jogo e ANTES do loop principal |
| modules/monitor/cdp.inc | 325 | Monitor Touch / desktop: Chrome/Edge headless controlado por DevTools Protocol (CDP) |
| modules/monitor/monitor.inc | 434 | modulo Monitor Touch |
| modules/monitor/web/css.inc | 189 | PIE Web: CSS (analise, seletores, cascata, herdanca) |
| modules/monitor/web/html.inc | 112 | PIE Web: analisador de HTML (tolerante, estilo HTML5 simplificado) |
| modules/monitor/web/http.inc | 107 | PIE Web: cliente HTTP(S) multiplataforma |
| modules/monitor/web/img.inc | 42 | PIE Web: imagens (PNG/JPEG/data:) e base64 |
| modules/monitor/web/jpeg.inc | 123 | JPEG: decodificador baseline (quadros de outros jogadores / imagens de paginas) + codificador (envia a tela ao |
| modules/monitor/web/layout.inc | 140 | PIE Web: estilos, layout (bloco, linha, tabela, flex/grid simples) e lista de desenho |
| modules/monitor/web/layout2.inc | 232 | PIE Web: implementacao do layout |
| modules/monitor/web/raster.inc | 37 | PIE Web: rasterizador por software (retangulos, texto 5x7, imagens) |
| modules/monitor/web/web.inc | 180 | PIE Web: navegador proprio do Monitor Touch (desktop: Windows / Linux / macOS) |
| modules/trashcans/trashcans.inc | 43 | modulo trashcans (port de Trash Cans 1.1.2 / SuperMartijn642, MC 1.12.2) |
| modules/treechop/treechop.inc | 49 | modulo treechop (port de HT's TreeChop 1.12.2, versao simplificada) |
| net/net.inc | 1092 | multiplayer (UDP, LAN / ZeroTier / IP direto; PC Windows, Linux e Android) |
| net/netlan.inc | 319 | LAN/ZeroTier: descoberta, lista de servidores, hospedar/entrar (usa net.inc) |
| net/voice.inc | 119 | voice chat (patchVoice.1): mic SDL + u-law 8 kHz sobre o UDP do net.inc |
| pack/asset_manager.inc | 515 | assets separados |
| pack/assets_atlas.inc | 128 | atlas em memoria (ATW x 16*ATLR) |
| pack/capes.inc | 177 | patchCapa.1: capas (prontas, PNG/GIF), compactacao de rede |
| pack/item_data.inc | 48 | nomes em portugues e pilha dos itens do pack |
| pack/mcpack.inc | 197 | ponte de resource packs do Minecraft |
| pack/modrules.inc | 176 | patchMod.15: receitas.txt / drops.txt dos mods convertidos (usa props, fidv, Recipe) |
| pack/mods.inc | 243 | mods: pasta mods/<nome>/mod.txt |
| pack/pk112.inc | 83 | nomes 1.12.2 -> nomes canonicos (1.13) do pack Default-Java-1.12.2 (patchTex.2) |
| pack/pkblocks.inc | 171 | blocos do resource pack colocáveis (v32) |
| pack/props.inc | 199 | patchProp.1: leitor do formato BLP1 (mods convertidos pelo conversor) |
| pack/propsys.inc | 710 | patchProp.2: props (Decocraft) colocaveis: icone 3D, mundo, colisao, salvar |
| pack/ptbr.inc | 729 | nomes pt-BR dos itens do criativo |
| pack/tilecache.inc | 26 | patchFast.3: cache de tiles decodificados |
| platform/android.inc | 99 | Android: camada GL->GLES (gl4es), assets do APK, HUD touch automatico |
| platform/gamepad.inc | 197 | controles (SDL_GameController): mapeamento, cursor virtual nos menus (usado por bow.inc, screens.inc, main_loo |
| platform/glesnative.inc | 506 | Android: OpenGL 1.x -> OpenGL ES NATIVO (sem gl4es); no PC nao e usado |
| platform/touch.inc | 242 | HUD touch estilo Minecraft PE (liga/desliga com F8) |
| platform/vk_frag.inc | 3724 |  |
| platform/vk_vert.inc | 1665 |  |
| platform/vkgl.inc | 795 | OpenGL 1.x -> Vulkan (macros gl*/SDL_GL_* redirecionadas) |
| render/entmodels.inc | 407 | modelos com textura de ENTIDADE 64x64 (bau, bau do Ender, cama) |
| render/godrays.inc | 97 | god rays no nascer/por do sol (F9) |
| render/gpu_chunks.inc | 119 | patchOrg.4: partes de main_loop.inc |
| render/lod.inc | 209 | patchLOD.1: LOD de terreno distante (incluido por world.inc antes de build()). |
| render/models.inc | 567 | blocos com modelo (nao-cubo): tocha, alavanca, bigorna, grama/flores |
| render/player_render.inc | 568 | desenho do personagem |
| render/player_skin.inc | 265 | skin + modelo do personagem |
| render/render_items.inc | 159 | malhas de blocos e itens (3D) |
| render/renderer.inc | 78 | patchVk.3: interface rd* (pilha de matrizes) acima do OpenGL/vkgl |
| render/shadows.inc | 212 | sombras do sol (F7) |
| render/sky_cycle.inc | 1076 | ciclo de dia e noite (portado do Minecraft Java 1.8.9) |
| render/sky_panorama.inc | 165 | ceu (domo) e panorama do menu (movido de main_loop.inc em patchOrg.4) |
| render/weather.inc | 195 | patchWx.1: clima |
| ui/capes_ui.inc | 43 | patchCapa.1: botao/selecao da capa |
| ui/chat_debug.inc | 657 | chat, comandos, F3 e consistência do inventário |
| ui/font.inc | 112 | fonte 5x7 (ASCII completo + acentos do português) |
| ui/inventory_ui.inc | 1173 | inventário (reimplementação do Minecraft Java 1.8.9) |
| ui/menu.inc | 539 | menus: titulo (1.21.5) + mundo unico (saves/mundo) com chunks, seed e jogador salvos |
| ui/pause_menu.inc | 102 | menu de pausa (GuiIngameMenu do Minecraft Java, em portugues) |
| ui/playerlist.inc | 104 | lista de jogadores (Tab): cabeca, nick, ping |
| ui/screens.inc | 1065 | telas do Minecraft 1.8.9 (Opções, Vídeo OptiFine, Controles, Idioma, Multijogador, Criar mundo, Mods...) |
| ui/splash.inc | 185 | tela de loading estilo Mojang (inicio do jogo) |
| ui/splitscreen.inc | 379 | split screen local: jogador 2 (fisica, acoes, inventario) e layout dos viewports |
| world/anvil.inc | 79 | bigorna (patchAnvil.1): reparar, combinar, custo em niveis |
| world/biome_light.inc | 93 | clima/bioma (cor da grama e das folhas) + opacidade de luz |
| world/blockshape.inc | 188 | formas dos blocos: colisao, selecao e raycast |
| world/blockupdate.inc | 153 | atualizacao de vizinhos e ticks de bloco |
| world/blood.inc | 87 | patchBlood.1: Dimensao Rubra (gDim 3): terreno, cores, ceu, entrada/saida (so dormindo) |
| world/boat.inc | 504 | bote: EntityBoat + ModelBoat + RenderBoat do Minecraft 1.12.2 |
| world/brewing.inc | 168 | patchBrew.1: cervejaria (bloco, tela, tempo, salvar, rede) |
| world/chest.inc | 427 | bau: armazenamento por posicao (conteudo, salvar/carregar, soltar ao quebrar) |
| world/chunksnap.inc | 63 | patchChunks.2: snapshot 3x3 de chunks para montar a malha FORA da thread principal. Incluido por world.inc log |
| world/compass.inc | 141 | bussola: agulha aponta para o spawn (troca o quadro do tile) |
| world/crops.inc | 148 | plantar/crescer/colher (usa redstone? nao: so world/drops) |
| world/dragon.inc | 493 | patchEnd.1: Ender Dragon + ovo |
| world/endgate.inc | 57 | patchEndGate.1: gateways do End (1.12.2) |
| world/endportal.inc | 63 | patchEndPortal.1: portal do End (galaxia, camadas aditivas com as texturas do pack) |
| world/falling.inc | 249 | areia e cascalho caem (entidade de bloco) |
| world/furnace.inc | 246 | fornalha (patchFur.1): fundir, combustivel, tela, luz, salvar |
| world/heads.inc | 28 | patchHead.1: cabecas de mob colocaveis |
| world/jukebox.inc | 71 | patchJuke.1: toca-discos |
| world/logic.inc | 100 | patchLogic.1: lampada de redstone e bloco de notas |
| world/map.inc | 52 | BUG-048: mapa |
| world/minecart.inc | 352 | trilhos + carrinhos de mina |
| world/mineshaft.inc | 138 | patchStruct.3: mina abandonada 1.12.2 |
| world/mining_sky.inc | 329 | mineração (tempo de quebra do 1.8.9), rachaduras, partículas e nuvens |
| world/nether.inc | 565 | Nether: dimensao, terreno, fogo (isqueiro) animado, portal |
| world/nether112.inc | 161 | Nether 1.12.2 (patchNether.3): ChunkGeneratorHell + MapGenCavesHell + decoracao, usando o ruido vanilla de ter |
| world/nether_fort.inc | 249 | Fortaleza do Nether 1.12.2 (patchNether.3): MapGenNetherBridge + StructureNetherBridgePieces, por pecas (lista |
| world/placement.inc | 57 | colocacao de blocos (ItemBlock.onItemUse) |
| world/potion_fx.inc | 77 | patchPot.1: beber pocoes |
| world/potions.inc | 122 | patchBrew.1: pocoes (itens, tabela de efeitos, regras de preparo) |
| world/redstone.inc | 1008 | aba Redstone: modelos, formas, colocacao, uso |
| world/sign.inc | 136 | patchSign.1: texto das placas |
| world/sleep.inc | 127 | patchSleep.1: dormir igual ao Minecraft (todos os jogadores) + pesadelo |
| world/stronghold.inc | 269 | patchStronghold.1: fortaleza (stronghold) 1.12.2 |
| world/structures.inc | 102 | patchStruct.1: templo do deserto, iglu, cabana da bruxa |
| world/terrain112.inc | 624 | terreno no estilo Minecraft Java 1.12.2 (ChunkGeneratorOverworld) |
| world/trees_mc.inc | 223 | árvores originais do Minecraft 1.12.2 (v33): WorldGenTrees, BigTree, Birch, Taiga1/2, Savanna, Shrub |
| world/vehicle.inc | 235 | patchVeic.1: veiculos do Vehicle Mod (usa props: corpo/rodas) |
| world/village.inc | 167 | vilas (v37): LOCALIZAÇÃO idêntica ao MapGenVillage 1.12.2 (espaçamento 32, semente 10387312, biomas); layout s |
| world/village_build.inc | 216 | conteudo das pecas da vila: port de StructureVillagePieces (1.12.2, v40) |
| world/world.inc | 1185 | mundo |
| world/worldgen.inc | 309 | decoração do terreno (v32): cavernas 3D, veios de minério, árvores sem cortes, cactos |
