# Conversor de mundo Minecraft 1.12.2 -> Blockland (patchWorld.1)
`python3 conversor/mundo/mc2bl.py <pasta_do_mundo> saves/<NOME> [--pack zip_do_pack] [--relatorio rel.txt]`
- Le `region/` e `DIM-1/region/` (Anvil), usa o registro de blocos do `level.dat` (FML) para dar nome a TODOS os ids, inclusive de mods.
- Escreve `chunks16.dat` e `chunks16n.dat` (BL18, paleta com nomes CRUS do pack Default-Java-1.12.2), `level.cfg` (nome, seed, modo, posicao, hora) e `dim.txt`. `conversao.txt` lista bloco | convertido | virou pedra | virou ar.
- Mods: blocos de Quark/Chisel/BOP/Rustic/IC2/Techguns viram o bloco mais parecido que o pack tem (marmore = diorito polido, ardosia = argila cinza, basalto = concreto cinza...); plantas viram plantas; luzes invisiveis viram ar. Tabela `FAM` e `mod()` em `mapping2.py`.
- Nao converte: entidades, baus/itens (tile entities), biomas (o jogo gera pelo seed), placas, estandartes, alavancas, funis, pistoes, fios de redstone (viram ar).
- Teste: `python3 tests/test_mapping2.py`.
