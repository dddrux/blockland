# Conversor de mods 1.12.2 → Blockland (modconv)

Uso: `python3 conversor/modconv/modconv.py <mod.jar> [mais.jar ...] [--out mods]`
Saída em `mods/<mod>/`: `props.bin` (lido pelo jogo, `src/props.inc`), `props.json` (mesmo conteúdo, legível), `props_atlas_N.png`, `mod.txt` (`props props.bin`) e `RELATORIO.md` (lacunas).
Ferramentas: `preview.py <pasta_do_mod> <saida.png> [id ...]` (folha de contato para conferir à vista); testes: `tests/test_modconv.py` (jar sintético) e `tests/test_props.cpp` (mod real).

## Como ele cresce a cada mod enviado
1. `core.pick()` escolhe um handler específico (`handlers/<mod>.py`, `detect(jar)`) ou o `generic` (blockstates + modelos JSON + elements).
2. Tudo o que o conversor **não** soube traduzir vai para `RELATORIO.md` e acumula em `db/unknown.json` (parents de modelo desconhecidos, tileType, campos ignorados, texturas só por nome). `db/history.json` guarda cada execução.
3. No chat seguinte: envie o projeto com `db/` + o novo .jar. A lista de lacunas diz exatamente o que ensinar: um parent novo vira linha em `VANILLA` (generic.py); um mod com formato próprio vira `handlers/<mod>.py` (use `decocraft.py` de modelo). Handlers novos reaproveitam `pack.py` (atlas, quaternions, escrita BLP1).

## Formato BLP1 (`pack.py`)
Caixa = `[cx,cy,cz, hx,hy,hz, qw,qx,qy,qz, f0..f5]`, em blocos; x/z centrados no bloco, y=0 no chão; h = meio-tamanho. Face fi (ordem do `entFace`): 0=-X 1=+X 2=-Y 3=+Y 4=-Z 5=+Z; face = 0 (oculta) ou `[slot,u0,v0,u1,v1]` normalizada. Cantos por face em `CORNERS` (igual a `PROP_CORNERS` do `props.inc`). Item: id, nome pt-BR (+`en`), aba, modelo, `tex[]` (id de textura por slot; negativo = referência por nome, ex. vanilla), luz, `nocoll`, `beh`.

## Decocraft (handler `decocraft.py`)
Modelos CraftStudio dentro do `.cspack` (`.csmodel`, binário descoberto por engenharia reversa; ver `csmodel.py`): nó = pos, offset (rotacionado), escala, quaternio (w,x,y,z), tamanho em px (1 px = 1/16 bloco) e 6 origens UV [frente, trás, esquerda, base, direita, topo]. Frente = +Z. Variantes (2043) viram itens próprios (textura `textureOverride` quando o arquivo existe no jar, senão a embutida; `colorPrimary` multiplicado na textura). Resultado: 1076 modelos, 3119 itens, 39 018 caixas, 2688 texturas em 5 páginas.
Não convertido (vira `beh` no item, sem comportamento ainda): Chair 470, Bed 240, LinkedModel 136, SwitchableLamp 133, Container 94, Door 70, VariableRendering 51, ParticleEmitter 29, Faucet 9 etc. Texturas de `textureOverride` que não vêm no jar (245) usam a embutida. 230 faces (36 modelos) tinham UV além da textura e foram fixadas na borda.
- patchMod.1: o generic agora aplica rotacao x/y de variante e multipart sem `when`; parents extras: cross, slab, slab_top, carpet.

## Tradutor de acoes do Forge (`forge.py`, patchMod.4)
Roda depois de qualquer handler. Varre os `.class` do jar (tabela de constantes, sem decompilar) e classifica cada chamada `net/minecraft*`/`net/minecraftforge*` pela `TABLE` (convertido / aproximado / ignorado / sem equivalente). Para ensinar uma acao nova, acrescente uma linha na `TABLE`.
Saidas em `mods/<mod>/`: `forge_acoes.json` (cada chamada, quantas vezes, status, o que o Blockland faz), `receitas.txt` (`shaped <saida> <n> <linha|linha> <c=item;...>`, `shapeless <saida> <n> <a,b>`, `smelt <entrada> <saida> <xp>`; `ore:<nome>` = ore dict, `@N` = dado), `drops.txt` (`<bloco> -> <item> <min>-<max> [se <condicao>]`), `sons.txt` (`<evento> <ns:arquivo>`). Lacunas vao para `RELATORIO.md` e `db/unknown.json` (secoes `Forge: aproximado` e `Forge: sem equivalente`).
Limites: o jogo le `receitas.txt` e `drops.txt` (patchMod.15, `src/pack/modrules.inc`) mas AINDA NAO toca `sons.txt` (o conversor ja copia os .ogg para `mods/<mod>/sounds/<ns>/`); logica de codigo (onBlockActivated, TileEntity, GUI, rede) so e classificada, nao traduzida.
- patchMod.7: itens (modelo 3D de item e icone plano) viram props; `guns/*.json` -> `armas.json`.

## Texturas vanilla (patchMod.10)
`vanilla.py` resolve `minecraft:blocks/x`, `blocks/x` e `block/x` pelo zip `assets/*minecraft-assets*textures*.zip` (ou `--vanilla`). O jogo NAO resolve `ref` por nome, entao tudo que for vanilla deve ser embutido aqui; o que sobrar em `RELATORIO.md` ("textura so por nome") e realmente ausente. Fluxo para mod novo: `python3 conversor/modconv/modconv.py novo.jar --out mods` -> ver `RELATORIO.md` -> `mod.txt` ja sai pronto. Pilha de armas/armaduras: tabela `tok[]` em `src/props.inc`.

## Regras no jogo (patchMod.15, `src/pack/modrules.inc`)
Ao primeiro uso da mesa/fornalha/quebra de prop, `modRulesLoad()` le `receitas.txt` e `drops.txt` de cada pacote de props carregado. Nomes: `ns:nome` de mod -> prop `ns_nome`; `minecraft:x` -> id nativo ou do pack (`fidv`); `planks@N`/`log@N` -> madeira por especie; `ore:<nome>` -> tabela vanilla (ingotIron...) ou 1o prop cujo id contem todas as palavras do camelCase. Receita com ingrediente nao resolvido e descartada (contada em `Regras de mod:` no log). Fundicao: `gModSmelt` consultado por `furnResult`. Drop: `modPropDrop` em `propHit`. Teste: `tests/test_modrules.cpp`.

## Como o Forge 1.12.2 funciona (patches F1-F7)
O conversor agora segue o que o Forge faz de verdade, em vez de supor que "blockstate = item":
- **F1 `forgestate.py`**: blockstates `forge_marker 1` (variantes por propriedade, `defaults` no topo, `inventory`, `submodel`). Antes so a 1a chave era lida (persianas, Chisel). Chisel: 21 -> 162 blocos.
- **F2 `classinfo.py`**: le a tabela de constantes dos `.class` e acha o PAPEL de cada classe seguindo a heranca dentro do jar (Block, BlockContainer, TileEntity, IInventory, ItemBlock, ItemSword, ItemArmor, GuiContainer, IGuiHandler, eventos de HUD).
- **F3 `placers.py`**: o jogador carrega o ITEM (ItemBlock), o bloco so existe no mundo. Liga item->bloco (mesmo nome / modelo / prefixo) em `placers.txt` e tira do criativo blocos tecnicos (aberto/fechado, ligado/desligado, cima/baixo) em `tecnicos.txt`.
- **F4 `containers.py`**: bau = bloco cuja classe usa uma TileEntity com inventario (nao basta ter TileEntity: liquidificador/banheira nao sao baus). Marca `beh=Container`; `containers.txt`. cfm: 94 baus.
- **F5 `gear.py` + `armor.py`**: espadas/ferramentas/arcos por classe ou nome (`gear.txt`: tipo, nivel, dano, durabilidade); armaduras ganham dica de material vinda das classes.
- **F6 `gui.py`**: telas (GuiContainer/GuiScreen), ganchos de HUD e texturas de interface -> `gui/`, `gui_override/` (substitui interface vanilla), `gui.json`.
- **F7 `modinfo.py` + `db/mods/<modid>.json`**: mcmod.info + perfil por mod (beh por nome, hide, nomes, notas). Perfis: cfm, chisel, ironchest, storagedrawers. Mod novo = um .json.
Limites: o jogo ainda nao LE placers/containers/gear/gui (so o conversor gera); Container de prop tem 27 slots; GUI de mod nao executa codigo.
Teste: `python3 tests/test_forgeinfo.py`.
- **F8 (`forge.py`)**: receitas com `_factories.json` (tipo proprio herda shaped/shapeless do nome), `_constants.json` e `forge:ore_shaped/ore_shapeless`. cfm: 445 -> 450 receitas.
- **F9 (`objmodel.py`)**: modelo `.obj` do Forge vira uma caixa (bounding box dos vertices) com a textura da variante/.mtl; declarado como aproximacao.
- **F10-F11 (`forgemap.py`)**: `forge_mapa.json` = propriedades de estado por bloco, renderers por codigo (TESR), entidades, nomes do OreDictionary, fluidos e comidas.
- **F12 (`generic.py`)**: blockstates em subpastas (Chisel) ganham id proprio; modelo de blockstate sempre relativo a `models/block/` (`mossy/mossy`); pai abstrato `block/block`. Chisel: 162 -> 266 blocos.
- **F13 (`forgestate.multipart`)**: `multipart` com `when` entra pelo estado de repouso (false/0/none/north). cfm: 3604 -> 4050 caixas (portoes, janelas).
- **F14 (`lights.py`)**: luz = constante float de `setLightLevel` na classe do bloco -> `item.light` (+ `lights.txt`); variantes `_off` nao emitem. cfm: 27 itens.

- patchConv.1: `forgestate.pick` escolhe o estado de repouso (`rest_score`), mescla `defaults` do topo, `submodels()` vira caixas extras; `Jar.has_string` evita itens de textura legada (sem lang e sem registro no .class).

## Engenharia reversa do Forge (`forgeapi.py`, patchForge.1)
Catalogo gerado do `forge-1.12.2-*-universal.jar` (`python3 conversor/modconv/forgeapi.py <jar>` -> `db/forge_api.json.gz`): 1434 classes (super, interfaces, metodos, campos, anotacoes), 306 eventos, e do `deobfuscation_data` do jar as classes e nomes SRG do Minecraft 1.12.2 (o jar do Forge cita o Minecraft por nomes ofuscados: o parser traduz notch->SRG).
`forge.scan()` marca cada chamada em `forge_acoes.json` com `api`: `forge` (existe no Forge), `forge?` (ausente: mod de outra versao), `mc`/`mc?` (SRG existe/nao na 1.12.2), `externo`. Sem linha na TABLE a chamada e classificada pelo pacote (`CATS`).
Novas saidas: `forge_eventos.json` (cada evento com @SubscribeEvent: handlers, cancelavel, resultado, status) e `forge_mod.json` (@Mod: modid, version, dependencies). Para ensinar um evento: `event_status()`; um pacote: linha em `CATS`.

## JSON do Minecraft e condicoes do Forge (patchForge.2)
`jsonlenient.py` imita o Gson leniente do jogo (comentarios, nomes/valores sem aspas, aspas simples, `;`, `=`/`=>`); `core.Jar.json` tenta o `json.loads` e cai nele. Receitas: `conditions` (`forge:mod_loaded`, `minecraft:item_exists`, `forge:not|or|and|false`) sao avaliadas por `forge.cond_ok` com `forge.LOADED` (mod atual + ja convertidos); falsa = receita ignorada. O tipo vem da classe da fabrica em `_factories.json`.

## Nomes legiveis e drops por codigo (patchForge.3)
`srgnames.py`: nome MCP dos `func_/field_` (tabela curada, metodos conferidos pelo descritor) + csv do MCP opcional em `conversor/modconv/db/mcp/{methods,fields}.csv` (colunas `searge,name,...`; com ele cobre tudo). `forge_acoes.json` ganha `nome`; o RELATORIO mostra `getItem [func_77973_b]`; stat `srg_com_nome` = quantos SRG tem nome.
`dropscode.py`: le `getItemDropped`/`quantityDropped` do bytecode (a classe ou a superclasse dentro do jar) e grava em `drops.txt` linhas `bloco -> ns:item n-n por codigo`. Registro: `ldc "nome"` + `new Classe` + `putstatic Dono.CAMPO` liga campo -> nome. Casos: campo de item, `Item.getItemFromBlock(campo)`, `new ItemStack(bloco).getItem()`, `Items.X` vanilla (so os de `srgnames.VANILLA_ITEMS`, ou todos com o csv), null/AIR (nenhum). NAO vira regra (vai para o RELATORIO): drop que depende de blockstate (`getValue`), aleatorio, e classes com `harvestBlock`/`removedByPlayer`/`getDrops` (escrevem o proprio drop, ex.: caixote/presente do cfm). Mods que registram blocos por enum/lambda (ic2, chisel) ainda nao entram.
Correcao: `func_180651_a` e `damageDropped` (dado do item), nao resistencia; resistencia por metodo e `func_149638_a`. `breakBlock` e `func_180663_b`. Jogo: `modPropDrop` usa a chave de loot MAIS LONGA que termina o id do prop.
Teste: `python3 tests/test_forgedrops.py`.

## Receitas em .ini (patchIni.1, `iniconfig.py`)
IC2 2.8 nao tem `recipes/*.json`: craft em `assets/ic2/config/shaped_recipes.ini` e `shapeless_recipes.ini`, fundicao em `furnace.ini`, maquinas em `macerator.ini`, `compressor.ini`, etc. `iniconfig.extract` roda logo depois do `forge.translate`, resolve `ns:item#variante` / `cable#type:x,insulation:n` / `OreDict:x` contra o `props.json` e MESCLA em `receitas.txt` (sem duplicar); maquinas vao para `maquinas.txt` (`maquina|entrada*n|saida*n,...`, o jogo ainda nao le: nao ha bloco de maquina). IC2: 0 -> 229 receitas, 223 de maquina. patchIni.2: `ORE_ALIAS` troca ore dict sem item (itemRubber, circuitBasic, martelo, alicate...) por item real: receitas descartadas pelo jogo por ore nao resolvido 60 -> 5. Sem item no jogo (TileEntity `te`, fluido, bateria sem modelo) = lacuna no RELATORIO. Teste: `python3 tests/test_iniconfig.py`.

## Argumentos das acoes de clique (patchForge.4)
`bytecode.py` guarda, dentro de `onBlockActivated`, o ultimo `ldc` de texto antes de `new TextComponent*` (mensagem) e de `new SoundEvent` (nome do som) em `acoes_args` (comportamento.json). `mecanicas.apply` escreve `mods/<mod>/acoes.txt` (`item|mensagem|texto` com a chave ja resolvida no .lang pt-BR/en, `item|som|ns:nome`). Jogo: `modRulesLoad` -> `gPropActArg`; `propUse` chama `modPropActArgs` (mensagem no chat, alternando se houver varias; som se o .ogg existe). Limite: o Java escolhe a mensagem por estado do bloco; o jogo alterna. Teste: `python3 tests/test_forgeargs.py`.
