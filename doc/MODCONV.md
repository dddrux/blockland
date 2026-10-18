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
Limites: o jogo ainda NAO le `receitas.txt`/`drops.txt`/`sons.txt` (so o conversor os gera); logica de codigo (onBlockActivated, TileEntity, GUI, rede) so e classificada, nao traduzida.
- patchMod.7: itens (modelo 3D de item e icone plano) viram props; `guns/*.json` -> `armas.json`.

## Texturas vanilla (patchMod.10)
`vanilla.py` resolve `minecraft:blocks/x`, `blocks/x` e `block/x` pelo zip `assets/*minecraft-assets*textures*.zip` (ou `--vanilla`). O jogo NAO resolve `ref` por nome, entao tudo que for vanilla deve ser embutido aqui; o que sobrar em `RELATORIO.md` ("textura so por nome") e realmente ausente. Fluxo para mod novo: `python3 conversor/modconv/modconv.py novo.jar --out mods` -> ver `RELATORIO.md` -> `mod.txt` ja sai pronto. Pilha de armas/armaduras: tabela `tok[]` em `src/props.inc`.
