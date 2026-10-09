# Blockland

> Um jogo voxel em C++ inspirado na liberdade de exploração, construção e sobrevivência dos clássicos sandbox de blocos.

**Blockland** é um projeto independente em C++17 que explora mecânicas de um sandbox voxel com renderização OpenGL, janela e entrada via SDL2, geração procedural de mundo, inventário, crafting, resource packs e multiplayer local por UDP.

O projeto busca aproximar algumas mecânicas e convenções de interface de versões clássicas do Minecraft Java — especialmente 1.8.9 e elementos do criativo 1.12.2 — como referência de comportamento. **Blockland não é um produto oficial da Mojang ou da Microsoft e não possui afiliação com essas empresas.**

> **Estado do projeto:** em desenvolvimento. Algumas opções de menu e sistemas podem estar incompletos; consulte a seção [Limitações conhecidas](#limitações-conhecidas).

---

## Índice

- [Destaques](#destaques)
- [Capturas de tela](#capturas-de-tela)
- [Requisitos](#requisitos)
- [Compilação](#compilação)
- [Como executar](#como-executar)
- [Controles](#controles)
- [Modos de jogo e sistemas](#modos-de-jogo-e-sistemas)
- [Multiplayer em rede local](#multiplayer-em-rede-local)
- [Resource packs, skins e interface](#resource-packs-skins-e-interface)
- [Comandos](#comandos)
- [Estrutura do projeto](#estrutura-do-projeto)
- [Testes](#testes)
- [Desenvolvimento e contribuição](#desenvolvimento-e-contribuição)
- [Problemas comuns](#problemas-comuns)
- [Limitações conhecidas](#limitações-conhecidas)
- [Licença e créditos](#licença-e-créditos)

## Destaques

- **Mundo voxel procedural:** chunks, terreno, cavernas e diferentes biomas.
- **Geração de estruturas:** árvores variadas, lagos, masmorras e vilas com conteúdo ainda simplificado em algumas peças.
- **Ciclo de dia e noite:** dia de 24.000 ticks, equivalente a aproximadamente 20 minutos em tempo real, com sol, lua, estrelas, nuvens, céu e neblina.
- **Água dinâmica:** fontes, níveis de fluxo, quedas, fontes infinitas e correnteza que afeta o jogador.
- **Iluminação do mundo:** iluminação por voxel, luz do céu e de blocos emissores, suavização por vértice e oclusão ambiente.
- **Renderização de chunks:** malhas armazenadas em VBOs quando suportado, atualização de malha sob demanda e corte de geometria fora da visão.
- **Sobrevivência e criativo:** mineração, drops, hotbar, inventário, ferramentas, receitas e paleta de blocos.
- **Inventário inspirado no Minecraft Java:** armaduras, grade de crafting 2×2, mochila, hotbar, movimentação de pilhas e atalhos de teclado.
- **Criativo com categorias:** abas inspiradas na organização do Minecraft 1.12.2, busca e conteúdo adicional do resource pack.
- **Resource packs:** carregamento de texturas de blocos e itens a partir de packs compatíveis com o leitor implementado no projeto.
- **Skins:** suporte a textura de personagem.
- **Multiplayer experimental por UDP:** host e conexão por IP, com sincronização de jogadores e chat.
- **HUD para toque:** interface alternativa acionada por tecla, com controles de movimento, câmera, chat e inventário.
- **Ferramentas de depuração:** informações de FPS, tempo de quadro, chunks, posição, direção, bloco mirado e dados gráficos.
- **Código modular sem sistema de build complexo:** os arquivos `.inc` são incluídos por `src/main.cpp`; a compilação usa diretamente `g++`.

## Capturas de tela

Adicione imagens do jogo ao repositório, por exemplo, em `docs/screenshots/`, e substitua os caminhos abaixo:

```md
![Mundo e exploração](docs/screenshots/world.png)
![Inventário](docs/screenshots/inventory.png)
![Modo criativo](docs/screenshots/creative.png)
```

Imagens reais do projeto ajudam as pessoas a avaliar o estado atual sem precisar compilar o jogo.

## Requisitos

### Linux

- Compilador C++ com suporte a **C++17** (`g++` recomendado).
- **SDL2** e seus arquivos de desenvolvimento.
- **OpenGL** e os arquivos de desenvolvimento correspondentes.
- `bash`, `make` não é necessário para o script atual.
- `sdl2-config`, normalmente fornecido pelo pacote de desenvolvimento do SDL2.
- A pasta `assets/` junto ao executável.

Exemplo para distribuições baseadas em Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential libsdl2-dev libgl-dev
```

Em Arch Linux e derivados:

```bash
sudo pacman -S --needed base-devel sdl2
```

Em Fedora:

```bash
sudo dnf install gcc-c++ SDL2-devel mesa-libGL-devel
```

Os nomes dos pacotes podem variar entre distribuições e versões.

### Windows

O script contém um caminho para ambientes **MSYS2 MinGW64**, com `g++`, SDL2, `sdl2-config` e bibliotecas OpenGL/WinSock disponíveis. É necessário manter `SDL2.dll` e `assets/` junto ao executável distribuído.

## Compilação

Clone o repositório e entre na pasta do projeto:

```bash
git clone <https://github.com/druxpp/blockland.git>
cd Blockland
```

Compile:

```bash
chmod +x build.sh
./build.sh
```

No Linux, o script gera `./blockland`. Em um ambiente MSYS2/MinGW compatível, gera `blockland.exe`.

O script usa, em essência, `g++ -O2 -std=c++17`, SDL2 e OpenGL. Não é necessário CMake para o fluxo de compilação atual.

### Executar após compilar

```bash
./blockland
```

Confirme que `assets/` está no diretório esperado em relação ao executável. Se mover o binário para outro local, o carregamento de recursos poderá falhar.

## Como executar

O programa aceita opções de inicialização:

```bash
# Iniciar com a configuração padrão
./blockland

# Iniciar em modo solo, sem rede
./blockland --solo

# Conectar a um host por endereço IP
./blockland --join 192.168.1.20

# Usar uma porta específica
./blockland --port 47777
```

A porta padrão documentada para o multiplayer é **UDP 47777**. Para jogar entre computadores diferentes, ambos precisam estar na mesma rede ou possuir conectividade de rede apropriada, e o firewall do host deve permitir o tráfego UDP nessa porta.

## Controles

Os controles abaixo refletem o mapeamento documentado no projeto. Teclas e ações podem mudar durante o desenvolvimento.

| Tecla | Ação |
|---|---|
| `W` `A` `S` `D` | Mover-se |
| `Espaço` | Pular |
| `Shift` | Agachar |
| `Ctrl` | Correr |
| `E` | Abrir/fechar inventário |
| `F3` | Alternar informações de depuração do corpo |
| `F4` | Alternar entre criativo e sobrevivência |
| `F5` | Alternar perspectiva da câmera |
| `F6` | Alternar visualização dos braços |
| `F7` | Alternar sombras |
| `F8` | Ativar/desativar HUD para toque |
| `T` | Abrir chat |
| `Esc` | Fechar telas ou abrir o menu de pausa |
| `Q` | Soltar um item |
| `Ctrl` + `Q` | Soltar uma pilha de itens |
| `1`–`9` | Selecionar/trocar o slot da hotbar no inventário |
| Botão esquerdo | Minerar/atacar ou interagir com a interface |
| Botão direito | Colocar bloco ou interagir com blocos/estações |
| Roda do mouse | Navegar em listas/telas que oferecem rolagem |

### HUD para toque

Com o HUD para toque habilitado por `F8`, a interface oferece botões na tela para movimento, pulo, agachar, câmera, chat, pausa e inventário. Arrastar altera a direção da visão; toque rápido pode colocar blocos e manter pressionado pode minerar. O suporte depende da entrada e do dispositivo utilizados.

## Modos de jogo e sistemas

### Sobrevivência

- Mineração com tempo de quebra relacionado à dureza e à ferramenta.
- Drops de blocos e itens, coleta, empilhamento e desaparecimento após determinado tempo.
- Hotbar e inventário com movimentação de itens.
- Crafting 2×2 no inventário e 3×3 na mesa de trabalho.
- Ferramentas e receitas para os materiais implementados.
- Slots de armadura preparados para itens compatíveis; a cobertura completa de equipamentos pode variar.

### Criativo

- Paleta de blocos e itens.
- Abas por categoria, busca e rolagem.
- Organização baseada no criativo do Minecraft 1.12.2, com uma aba adicional para itens encontrados no resource pack.
- Voo criativo com dois toques na tecla de pulo, conforme a implementação atual.

### Mundo e geração procedural

O gerador inclui biomas como planície, floresta, deserto, savana, selva, pântano, taiga, tundra e montanhas, além de praias e oceanos determinados pela altura. A distribuição de vegetação e a superfície variam conforme o bioma.

Também existem implementações de árvores, lagos, masmorras e vilas. Nem todas as estruturas têm o mesmo nível de detalhe do jogo usado como referência.

### Água e física

O sistema de água implementa fontes e blocos em fluxo, quedas, atualização por eventos e correnteza. A física do jogador inclui colisões por caixa delimitadora, degraus e comportamento na água. Como o projeto continua em desenvolvimento, diferenças de comportamento em relação ao Minecraft original são possíveis.

### Dia e noite

O ciclo utiliza 24.000 ticks por dia. O horário é salvo no mundo e pode ser consultado ou alterado pelos comandos de tempo.

### Inventário e crafting

O inventário de sobrevivência segue uma disposição familiar: slots de armadura, prévia do personagem, crafting 2×2, resultado, mochila e hotbar. Há suporte a pegar/soltar pilhas, dividir pilhas e transferir itens com modificadores de teclado. O resultado depende das receitas implementadas no código.

## Multiplayer em rede local

O multiplayer é **experimental** e utiliza UDP.

### Teste no mesmo computador

Abra dois terminais no diretório do projeto:

```bash
# Terminal 1: inicia o host
./blockland

# Terminal 2: conecta ao host local
./blockland --join 127.0.0.1
```

Se o modo padrão não iniciar como host no seu build, use as opções de inicialização disponíveis no código ou `--solo` para jogar sem rede.

### Conectar outro computador

1. Inicie o jogo no computador que será o host.
2. Descubra o endereço IP local desse computador.
3. No outro computador, execute `./blockland --join IP_DO_HOST`.
4. Permita tráfego UDP na porta configurada no firewall, se necessário.

O protocolo não deve ser considerado seguro para uso em redes públicas. Não exponha o serviço diretamente à internet sem revisar autenticação, validação de pacotes e segurança do protocolo.

## Resource packs, skins e interface

A maior parte das texturas de blocos e itens é carregada de `assets/resourcepacks/`. O pack padrão incluído no projeto é `Ashen_16x.zip`. A compatibilidade depende dos formatos e recursos que o carregador reconhece; nem todo resource pack do Minecraft necessariamente funcionará.

Estrutura de recursos relevante:

```text
assets/
├── creative/       # Configuração das categorias do criativo
├── resourcepacks/  # Packs de texturas
├── skins/          # Skins de personagem
├── textures/
│   ├── colormap/   # Mapas de cores para grama e folhas
│   └── gui/        # Texturas da interface
└── ui/             # Configurações e documentação da interface
```

### Editar as categorias do criativo

A configuração principal fica em:

```text
assets/creative/mc-1.12.2.cfg
```

O gerador associado está em `tools/gen_creative_1122.py`. Para regenerar a configuração, execute:

```bash
python3 tools/gen_creative_1122.py
```

Faça uma cópia de segurança antes de regenerar arquivos modificados manualmente.

### Adicionar ou trocar um resource pack

Coloque o arquivo `.zip` em `assets/resourcepacks/` e confira `assets/resourcepacks/README.md` e `assets/resourcepacks/pack.cfg` para as convenções aceitas pela versão atual. Preserve a atribuição e os termos de licença do pack original.

## Comandos

Os comandos são digitados no chat (`T` ou `/`, conforme a tela ativa). Os comandos documentados incluem:

| Comando | Descrição |
|---|---|
| `/gamemode 0` ou `/gm 0` | Alternar para sobrevivência |
| `/gamemode 1` ou `/gm 1` | Alternar para criativo |
| `/give <item> [qtd]` | Adicionar itens ao inventário |
| `/tp x y z` | Teleportar para coordenadas |
| `/tp ~ ~ ~` | Usar coordenadas relativas com `~` |
| `/clear` | Limpar inventário |
| `/pos` | Mostrar posição |
| `/cls` | Limpar a área visível do chat |
| `/ajuda` | Exibir ajuda |
| `/locate <bioma\|vila>` | Procurar um bioma ou vila |
| `/time set day\|noon\|sunset\|night\|midnight\|sunrise\|<ticks>` | Definir o horário |
| `/time add <n>` | Avançar o horário |
| `/time query` | Consultar o horário |
| `/time pause` | Pausar o ciclo de tempo |
| `/time resume` | Retomar o ciclo de tempo |

A disponibilidade e a sintaxe exata podem variar conforme a versão compilada. Use `/ajuda` para conferir as opções reconhecidas pelo executável.

## Estrutura do projeto

```text
.
├── assets/             # Texturas, resource packs, skins e configurações
├── src/                # Código C++ e módulos incluídos
├── tests/              # Testes e fixtures
├── tools/              # Ferramentas auxiliares
├── build.sh            # Script de compilação
├── README.md           # Documentação principal
└── LEIAME.txt          # Resumo de execução e controles
```

O ponto de entrada é `src/main.cpp`. A implementação é dividida em arquivos `.inc` incluídos pelo arquivo principal, mantendo o fluxo de compilação simples.

Entre os módulos estão:

- `world.inc`, `worldgen.inc`, `blockupdate.inc`: mundo, geração e atualização de blocos.
- `physics.inc`: movimento, colisões e física do jogador.
- `main_loop.inc`: eventos e loop principal.
- `inventory_ui.inc`, `pause_menu.inc`: inventário e interface.
- `player_skin.inc`, `render_items.inc`: personagem, itens e renderização.
- `biome_light.inc`, `godrays.inc`, `mining_sky.inc`: iluminação, efeitos visuais e mineração.
- `netlan.inc`: comunicação em rede local.
- `mcpack.inc`, `asset_manager.inc`, `png.inc`: carregamento e gerenciamento de recursos.
- `ptbr.inc`, `font.inc`, `chat_debug.inc`: textos, fonte, chat e ferramentas de depuração.
- `chest.inc`, `village.inc`, `village_build.inc`: baús e estruturas do mundo.

Os nomes acima refletem a organização encontrada no código; consulte os arquivos e `src/README.md` para obter detalhes técnicos atualizados.

## Testes

O diretório `tests/` contém testes unitários e de integração para sistemas como física de líquidos, biomas, rede, inventário/baús, geração, atualização e outros módulos.

Execute a suíte fornecida:

```bash
chmod +x tests/run.sh
./tests/run.sh
```

Nem todos os testes necessariamente são executados em todas as plataformas. Se algum teste falhar, registre o sistema operacional, compilador, comando usado e a saída relevante ao abrir uma issue.

## Desenvolvimento e contribuição

Contribuições são bem-vindas. Para facilitar a revisão:

1. Abra uma issue descrevendo o problema ou a proposta antes de mudanças grandes.
2. Faça um fork e crie uma branch com nome claro.
3. Mantenha as alterações focadas e evite misturar refatorações amplas com correções de bugs.
4. Inclua passos reproduzíveis para bugs e, quando possível, um teste de regressão.
5. Compile e execute os testes relevantes localmente.
6. Atualize a documentação e os controles se o comportamento mudar.
7. Abra um pull request com resumo, motivação, como testar e limitações restantes.

### Relato de bugs

Inclua, quando possível:

- Sistema operacional e versão.
- GPU e driver gráfico.
- Versão do compilador.
- Comando de compilação e inicialização.
- Passos para reproduzir.
- Resultado esperado e resultado observado.
- Logs, capturas de tela ou gravações curtas.

Evite anexar mundos pessoais ou dados privados sem remover informações sensíveis.

## Problemas comuns

### `sdl2-config: command not found`

Instale o pacote de desenvolvimento do SDL2 para sua distribuição e confirme que `sdl2-config` está no `PATH`.

### Erros de cabeçalhos SDL2

Verifique se os pacotes de desenvolvimento do SDL2 estão instalados. Ter apenas a biblioteca de runtime pode não ser suficiente para compilar.

### Erros de linkagem do OpenGL

Instale os arquivos de desenvolvimento do OpenGL e confirme que o compilador consegue localizar a biblioteca apropriada para seu sistema.

### O jogo abre, mas não encontra texturas

Execute o jogo a partir da pasta do projeto e mantenha `assets/` no local esperado. Verifique também os nomes e caminhos do resource pack e os logs do carregador.

### O multiplayer não conecta

Confirme IP e porta, verifique se os dois computadores estão na mesma rede e permita UDP no firewall. Para testar localmente, use `127.0.0.1`.

### Problemas gráficos ou desempenho baixo

Atualize os drivers gráficos, confira se o sistema está usando a GPU desejada e use as informações do `F3` para registrar FPS e tempo de quadro. O comportamento varia entre drivers e hardware.

## Limitações conhecidas

- O projeto está em desenvolvimento e ainda não oferece paridade completa com Minecraft Java.
- O multiplayer é experimental; não é anunciado como seguro ou robusto para redes públicas.
- Algumas opções do menu de pausa são apenas placeholders.
- Algumas estruturas geradas, incluindo partes das vilas, têm conteúdo simplificado.
- A compatibilidade com resource packs é parcial e depende do formato utilizado.
- O conjunto de itens, receitas, armaduras, entidades e interações não cobre todo o conteúdo dos jogos usados como referência.
- Desempenho, física, renderização e comportamento podem variar conforme o hardware, os drivers e o estado do código.

Se encontrar uma limitação que não esteja listada, abra uma issue com os detalhes necessários para reproduzi-la.

## Licença e créditos

Antes de publicar o repositório, **adicione um arquivo `LICENSE`** e defina explicitamente os termos de uso, modificação e redistribuição do código. Não presuma que o código ou os recursos incluídos tenham todos a mesma licença.

- **Código:** informe aqui os autores e colaboradores do projeto.
- **SDL2:** biblioteca de janela, entrada e suporte multimídia; consulte a licença oficial do SDL.
- **OpenGL:** API gráfica utilizada para renderização; consulte a documentação e os termos aplicáveis à implementação usada.
- **Resource packs, skins e texturas de terceiros:** preserve os créditos, a licença e as condições de redistribuição dos autores originais. A inclusão de `Ashen_16x.zip` no repositório deve estar autorizada pela licença correspondente.
- **Minecraft:** nome e referências são usados para descrever inspirações de mecânicas e compatibilidade pretendida. Minecraft é propriedade de seus respectivos titulares. Este projeto é independente e não oficial.

Consulte os arquivos de licença incluídos nos recursos de terceiros e mantenha os avisos exigidos. Se a licença de algum recurso não permitir redistribuição, remova o arquivo do repositório e documente como obtê-lo legalmente.

---

<p align="center">
  Feito para aprender, experimentar e construir um mundo de blocos — um sistema de cada vez.
</p>
