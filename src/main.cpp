// Blockland - clone voxel próprio, sem engine, inspirado no estilo do Minecraft PE antigo.
// Código próprio. Texturas de blocos e itens: SEMPRE do resource pack ativo (assets/resourcepacks/, padrão Ashen_16x.zip); sem texturas placeholder. C++17 + SDL2 + OpenGL 1.x.
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
// Versao do jogo: altere somente aqui (exibida no menu principal)
#define BLOCKLAND_VERSION "v0.1.0"
static const char* const GAME_VERSION_TEXT = "Blockland " BLOCKLAND_VERSION;

#include <SDL.h>
#include <SDL_opengl.h>
#ifdef _WIN32
#undef near   // windef.h define "near"/"far" como macros vazias (quebram identificadores)
#undef far
#endif
#include <algorithm>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846   // mingw em -std=c++17 estrito nao define M_PI
#endif
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <ctime>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <filesystem>
#include <vector>
#include <functional>
#include <memory>
#include <tuple>
#include <deque>
#include <csignal>
#ifdef __ANDROID__
#include "android.inc"   // Android: camada GL->GLES (gl4es), assets do APK, HUD touch automatico
#else
#define BL_ANDROID_WINFLAGS 0
#endif
#ifdef BL_VULKAN
#include "vkgl.inc"   // OpenGL 1.x -> Vulkan (macros gl*/SDL_GL_* redirecionadas)
#endif
static void splashPoll();   // splash.inc: desenha um quadro do loading se ja passou ~16 ms (cargas pesadas chamam isso nos loops)
#include "logger.inc"
// (logger.inc acima: logs no formato do Minecraft Java)
#include "perf.inc"   // profiler de lag (logs [PERF]/[LAG])
#include "frustum.inc"   // culling por chunk (opcional, BLOCKLAND_CULL=1)

static const int CW = 16, CH = 256;
static int RAD = 7;   // distancia de renderizacao em chunks (menu Video > Renderizacao; atualizado todo quadro de screens.inc/main_loop.inc)
typedef uint16_t BT;   // bloco: 16 bits (ids 0..65535); ids >= PK0 = blocos do resource pack
#include <array>
enum { AIR, GRASS, DIRT, STONE, SAND, LOG, LEAVES, PLANKS, COBBLE, WATER, GRAVEL, BRICKS, BEDROCK, COALORE, DIAMONDORE,
       GOLDORE, IRONORE, GOLDBLK, IRONBLK, DIAMBLK, COALBLK, SANDSTONE, STONEBRICK, MOSSY, CLAY, SNOWBLK, OBSIDIAN, GLOWSTONE, BOOKSHELF, WOOL, REDSTONEORE, LAPISORE, NB = 32 };
// itens (ids >= 32): não são blocos
enum { IT_STICK = 32, IT_COAL, IT_IRON, IT_DIAMOND, IT_APPLE, IT_WPICK, IT_SPICK, IT_WSWORD, IT_END };
// itens novos (ids 48+; 41..47 sao agua corrente). Misc empilhaveis -> ferramentas -> armaduras
enum { IT_GOLD = 48, IT_REDSTONE, IT_FLINT, IT_FEATHER, IT_BONE, IT_PAPER, IT_BOOK, IT_STRING, IT_LEATHER, IT_LAPIS, IT_BREAD, IT_COOKIE, IT_BEEF, IT_STEAK, IT_CARROT, IT_GAPPLE, IT_ARROW,
       IT_T0,   // = 65: primeira ferramenta nova
       IT_IPICK = IT_T0, IT_GPICK, IT_DPICK, IT_SSWORD, IT_ISWORD, IT_GSWORD, IT_DSWORD,
       IT_WAXE, IT_SAXE, IT_IAXE, IT_GAXE, IT_DAXE, IT_WSHOVEL, IT_SSHOVEL, IT_ISHOVEL, IT_GSHOVEL, IT_DSHOVEL,
       IT_WHOE, IT_SHOE, IT_IHOE, IT_GHOE, IT_DHOE, IT_BOW, IT_T1,
       IT_A0 = IT_T1,   // armaduras: couro, ferro, ouro, diamante x (capacete, peitoral, calca, botas)
       IT_A1 = IT_A0 + 16, IT_ALLEND = IT_A1 };
// itens de mobs (ids 105..115 livres: tiles 177..187): carnes, ovo e ovos de spawn
enum { IT_PORK = 105, IT_CPORK, IT_CHICKEN, IT_CCHICKEN, IT_MUTTON, IT_CMUTTON, IT_EGG, IT_EGG_PIG, IT_EGG_CHICKEN, IT_EGG_COW, IT_EGG_SHEEP };
static const int ATL = 32, ATLR0 = 144, ATLR = ATLR0 + 5;   /* ATLR0 = linhas de pack/entidades/modelos; +2 linhas = sprites de cama e estandarte */   // atlas runtime 512x2304 = 32 colunas x 144 linhas de tiles 16px
static const int ATW = ATL * 16;       // largura do atlas em pixels
// ---- itens do resource pack: TODA textura de block/ e item/ do pack vira um item de inventario (ids >= PK0, tiles >= PKTILE0) ----
// Sao itens so de inventario (nao colocaveis: os chunks guardam 1 byte por bloco). Registrados em loadPackItems() (asset_manager.inc).
static const int PK0 = 1000, PKTILE0 = 768;
struct PkItem { std::string file, name, fold; bool cube; int tile = -1; int ft[3] = {-1, -1, -1}; int fr = -1; int stack = 64; };   // fr = tile da FRENTE (blocos com orientacao: abobora entalhada, fornalha, dispenser...), -1 = sem frente   // ft = tiles topo/lado/base (cubos com faces diferentes)   // file = "block/xxx" ou "item/xxx"; cube = face opaca de bloco (icone em cubo)
static std::vector<PkItem> PK;
static inline bool isPk(int id) { return id >= PK0 && id < PK0 + (int)PK.size(); }
static std::vector<uint8_t> gPkLeaf, gPkLog, gPkTrans;   // gPkTrans: 1 = cubo translucido (vidro colorido/gelo/slime: malha com blend), 2 = vidro comum (alpha-test, sem faces internas entre iguais)   // folhas (1 tinta de bioma, 2 abeto fixo, 3 bétula fixo, 4 sem tinta) e troncos do pack, por indice de PK
static inline bool isPkBlk(int id) { return isPk(id) && PK[id - PK0].cube; }   // bloco colocável vindo do pack
static inline int leafKind(int b) { if (b == LEAVES) return 1; if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkLeaf.size() ? gPkLeaf[i] : 0; } return 0; }   // 0 = nao e folha
static inline int transKind(int b) { if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkTrans.size() ? gPkTrans[i] : 0; } return 0; }
static inline bool isLeafB(int b) { return leafKind(b) != 0; }
static inline bool isLogB(int b) { if (b == LOG) return true; if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkLog.size() && gPkLog[i]; } return false; }
static std::vector<uint8_t> gPkOpac, gPkEmit, gPkTint, gPkTool; static std::vector<int8_t> gPkLevel; static std::vector<float> gPkHard;
static std::unordered_map<std::string, int> gPkByFile; static std::unordered_map<uint32_t, int> gPkByHash;   // nome do arquivo / hash -> id
static void pkBuildProps();
static uint32_t netFnvPk(const std::string& s) { uint32_t h = 2166136261u; for (unsigned char ch : s) { h ^= ch; h *= 16777619u; } return h & 0x3FFFFFFFu; }   // hash do nome (rede)
// ---- mods: ids 120..255 (itens: tile 192+(id-120); blocos: tiles 336+3*(id-120)+face) ----
static const int CTABLE = 104;   // mesa de trabalho: id livre entre as armaduras (103) e os mods (120)
static const int MOD0 = 120;
static std::string modName[256]; static bool modBlk[256]; static int modEmit[256]; static int modNext = MOD0;
struct ModDef { std::string name; std::vector<int> ids; };
static std::vector<ModDef> MODS;
static const int TILE_GRASS_OVER = 328, TILE_GRASS_BASE = 332, TILE_ICON_GTOP = 329, TILE_ICON_GSIDE = 330, TILE_ICON_LEAF = 331;   // tiles livres entre itens e blocos de mods
static int ICONTILE[256][3];   // tiles usados em ícones/mão (grama e folhas já tingidas)
static inline bool isBlk(int id) { return id < 32 || id == CTABLE || (id >= MOD0 && id < 256 && modBlk[id]) || isPkBlk(id); }
static inline bool isCubeIcon(int id) { return isBlk(id) || (isPk(id) && PK[id - PK0].cube); }   // desenhado como cubo (inventario/mao/chao)
static inline int pkFront(int id) { return isPk(id) ? PK[id - PK0].fr : -1; }   // tile da face frontal (ou -1)
static inline int iconTile(int id, int f) { return isPk(id) ? (PK[id - PK0].ft[f] >= 0 ? PK[id - PK0].ft[f] : PK[id - PK0].tile) : ICONTILE[id][f]; }
static inline bool isTool(int id) { return (id >= IT_WPICK && id < IT_END) || (id >= IT_T0 && id < IT_T1); }
static inline bool isArmor(int id) { return id >= IT_A0 && id < IT_A1; }
static int gBktEmpty = 0, gBktWater = 0, gBktLava = 0;   // ids dos baldes (item/bucket, water_bucket, lava_bucket do pack); 0 = pack sem balde
static inline int maxStack(int id) { if (id && (id == gBktWater || id == gBktLava)) return 1; if (id && id == gBktEmpty) return 16; if (isPk(id) && PK[id - PK0].stack != 64) return PK[id - PK0].stack; return (isTool(id) || isArmor(id)) ? 1 : 64; }
static const int SEA = 63;
static bool netOn = false;   // true quando há outro jogador conectado
static std::string netName();
static void netSendChat(const std::string& s);
static constexpr int WATER_FALL = 40;     // agua caindo (coluna cheia); 41..47 = fluxo; WATER = fonte
static constexpr int WATER_FLOW_MIN = 41; // 41..47 = fluxo decrescente (47 = vizinho da fonte)
static inline bool isWater(int b) { return b == WATER || (b >= WATER_FALL && b <= WATER_FLOW_MIN + 6); }
static inline int waterLevel(int b) { return (b == WATER || b == WATER_FALL) ? 8 : (isWater(b) ? b - WATER_FLOW_MIN + 1 : 0); }
// "meta" do Minecraft: 0 = fonte, 1..7 = fluxo (maior = mais fraco), 8 = caindo; -1 = nao e agua
static inline int liqMeta(int b) { return b == WATER ? 0 : b == WATER_FALL ? 8 : isWater(b) ? 48 - b : -1; }
static inline int metaBlock(int m) { return m <= 0 ? WATER : m >= 8 ? WATER_FALL : 48 - m; }
// lava (mesma ideia da agua): 48 = fonte, 49 = caindo, 50..56 = fluxo (id = 57 - meta). No overworld o fluxo perde 2 niveis por bloco e e lento (1,5 s)
static constexpr int LAVA = 48, LAVA_FALL = 49, LAVA_FLOW_MIN = 50;
static inline bool isLava(int b) { return b >= LAVA && b <= LAVA_FLOW_MIN + 6; }
static inline bool isLiquid(int b) { return isWater(b) || isLava(b); }
static inline int lavaMeta(int b) { return b == LAVA ? 0 : b == LAVA_FALL ? 8 : isLava(b) ? 57 - b : -1; }
static inline int lavaBlock(int m) { return m <= 0 ? LAVA : m >= 8 ? LAVA_FALL : 57 - m; }
static int gLavaTile = 0;   // tile do atlas com a textura da lava (models.inc/xbInit)
// ---- blocos com modelo (nao-cubo): ids fixos 900..963 (models.inc) ----
static const int XB0 = 900, XB_TORCH = 900, XB_LEVER = 905, XB_ANVIL = 930, XB_RTORCH = 934, XB_PLANT = 940, XB_SAPLING = 964, XB_RAIL = 970, XB_END = 1000;   // tocha 900..904, alavanca 905..928 (12 estados x desligada/ligada), bigorna 930..933, plantas 940..963, mudas 964..969, trilhos 970..973 (comum/energizado/detector/ativador; forma no metadado)
static const int XB_DUST = 974, XB_PIST = 975, XB_DISP = 977, XB_DROP = 978, XB_OBS = 979, XB_PLATE = 980, XB_BTN = 981, XB_HOOK = 982, XB_DAY = 983, XB_HOPPER = 984, XB_REPT = 985, XB_COMP = 986, XB_TRAP = 987, XB_GATE = 988, XB_DOOR = 989, XB_CHEST = 990;   // aba Redstone (redstone.inc): 974 poeira, 975/976 pistao/grudento, dispenser, dropper, observador, placa, botao, gancho, sensor, funil, repetidor, comparador, alcapao, portao, porta, bau; estado no meta
static const int XB_CARPET = 991, XB_FENCE = 992, XB_PANE = 993, XB_LADDER = 994, XB_WALL = 995, XB_CPANE = 996, XB_ENDROD = 997, XB_BED = 998, XB_SIGN = 999;   /* placa: dir | parede<<2 | madeira<<3 (0..11) */   // painel colorido (cor<<4), haste do End (meta = face), cama (dir | cabeca<<2 | cor<<3)   // escada de mao (meta = lado para onde aponta), muro de pedra (como a cerca)   // aba Decoracao (redstone.inc): carpete (meta = cor), cerca (conexoes 0-3 N/L/S/O + tipo<<4), barras/painel de vidro (idem)
static const int XB_BANNER = 929, XB_WBANNER = 939;   // estandarte: em pe (meta = rotacao 0..15 | cor<<4) e na parede (meta = dir 0..3 | cor<<4); ids livres do intervalo XB
static inline bool isBanner(int b) { return b == XB_BANNER || b == XB_WBANNER; }
static inline bool isXB(int b) { return b >= XB0 && b < XB_END; }
static inline bool isRs(int b) { return b >= XB_DUST && b < XB_END; }
static int gRsLastMeta = 0;   // meta do ultimo bloco da aba Redstone removido por setBM (para o drop saber a variante)
static bool rsSupported(int b, int x, int y, int z); static int rsDropItem(int x, int y, int z, int b); static float rsHard(int b); static int rsTool(int b);
static void rsDustFix(int x, int y, int z); static void rsConnFix(int x, int y, int z, int b); static void rsMapItems(); static bool rsUse(int x, int y, int z); static int rsItemVar(int item); static int rsTile(int b); static void rsInit(); static void rsNames();
static inline bool xbSolid(int b) { return b >= XB_ANVIL && b < XB_ANVIL + 4; }   // so a bigorna tem colisao
static int xbTile(int b);
// ---- formas dos blocos (blockshape.inc): indice no pool de formas, 0 = vazio, 1 = cubo cheio. Preenchidas na inicializacao estatica de blockshape.inc ----
static uint8_t gBlkColl[65536], gBlkSel[65536];   // forma de COLISAO e de SELECAO (raycast/contorno) de cada id
static inline bool solid(int b) { return (unsigned)b < 65536u && gBlkColl[b] != 0; }      // tem colisao (qualquer forma, nao so cubo cheio)
static inline bool pickable(int b) { return (unsigned)b < 65536u && gBlkSel[b] != 0; }    // o raio de mira acerta (cubo, tocha, alavanca, planta, ...)
// tiles por bloco: topo, lado, base (índice no atlas 4x4)
static int TILES[256][3] = {{0,0,0},{0,1,2},{3,4,5},{6,7,8},{9,10,11},{12,13,14},{15,16,17},{18,19,20},{21,22,23},{24,25,26},{27,28,29},{30,31,32},{33,34,35},{36,37,38},{39,40,41},
    // blocos novos: 3 tiles cada a partir do tile 64 (topo, lado, base)
    {64,65,66},{67,68,69},{70,71,72},{73,74,75},{76,77,78},{79,80,81},{82,83,84},{85,86,87},{88,89,90},{91,92,93},{94,95,96},{97,98,99},{100,101,102},{103,104,105},{106,107,108},{109,110,111},{112,113,114}};
static inline int blkTile(int b, int f) { return b >= 256 ? (isXB(b) ? xbTile(b) : isPk(b) ? (PK[b - PK0].ft[f] >= 0 ? PK[b - PK0].ft[f] : PK[b - PK0].tile) : 0) : TILES[b][f]; }   // tile do bloco (inclui blocos do pack)
static const struct CTableTiles { CTableTiles() { TILES[CTABLE][0] = 115; TILES[CTABLE][1] = 116; TILES[CTABLE][2] = 117; } } ctableTiles;   // tiles livres do atlas (115..117)
#include "core_noise.inc"
#include "assets_atlas.inc"
#include "biome_light.inc"
#include "terrain112.inc"
#include "blockshape.inc"
#include "world.inc"
#include "png.inc"
#include "mcpack.inc"
#include "asset_manager.inc"
#include "pkblocks.inc"
#include "sky_cycle.inc"
#include "physics.inc"
#include "models.inc"
#include "player_skin.inc"
#include "render_items.inc"
#include "font.inc"
#include "crafting.inc"
#include "inventory_ui.inc"
#include "survival.inc"
#include "pause_menu.inc"
#include "item_data.inc"   // nomes em portugues e pilha dos itens do pack
static void ptbrApply();   // ptbr.inc (incluido depois de redstone.inc)
#include "mods.inc"
#include "modules/modules.inc"   // registro de ganchos dos modulos (ver src/modules/README.md)
#include "drops.inc"
#include "player_render.inc"
#include "mobs.inc"
#include "boat.inc"
#include "minecart.inc"   // trilhos + carrinhos de mina
#include "chat_debug.inc"
#include "mining_sky.inc"
#include "launcher.inc"   // perfil: nome + skin (usado por net.inc e screens.inc)
#include "net.inc"
#include "bow.inc"   // arco e flecha
#include "pvp.inc"   // PvP + morte de jogador (usa net.inc, mobs.inc, survival.inc)
#include "blockupdate.inc"
#include "placement.inc"
#include "redstone.inc"   // aba Redstone: modelos, formas, colocacao, uso
#include "chest.inc"      // bau: armazenamento por posicao (conteudo, salvar/carregar, soltar ao quebrar)
#include "ptbr.inc"       // nomes pt-BR dos itens do criativo
#include "touch.inc"
#include "menu.inc"
#include "netlan.inc"   // LAN/ZeroTier: descoberta, lista de servidores, hospedar/entrar (usa net.inc)
#include "screens.inc"
#include "splash.inc"   // tela de loading estilo Mojang (inicio do jogo)
#include "modules/modules_list.inc"   // modulos ativos: entram DEPOIS de toda a API do jogo e ANTES do loop principal
#include "compass.inc"   // bussola: agulha aponta para o spawn (troca o quadro do tile)
#include "main_loop.inc"
