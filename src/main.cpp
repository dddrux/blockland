// Blockland - clone voxel próprio, sem engine, inspirado no estilo do Minecraft PE antigo.
// Código próprio. Texturas de blocos e itens: SEMPRE do resource pack ativo (assets/resourcepacks/, padrão Ashen_16x.zip); sem texturas placeholder. C++17 + SDL2 + OpenGL 1.x.
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
// Versao do jogo: altere somente aqui (exibida no menu principal)
#define BLOCKLAND_VERSION "v1.1.0.124rc"
// ---- modo Raspberry Pi 3B (compile com -DBL_RPI ou ./build-rpi.sh) ----
#ifdef BL_RPI
#ifndef BL_WIN_W
#define BL_WIN_W 400   // patchRpiOpt.6: janela 400x250 (VC4: menos pixels = mais FPS)
#define BL_WIN_H 250
#endif
#define BL_WIN_MINW 400
#define BL_WIN_MINH 250
#define BL_SMALL_UI 1
#define BL_SMOOTH_FORCE(x) 0   // patchRpiOpt.1: sem luz suave no Pi
#define BL_RPI_FASTGRASS 1   // patchRpi.3: grama "Better Grass: Fast" do OptiFine (lado = textura do topo, 1 quad em vez de 2)
#define BL_RPI_MAXRD 5   // patchRpi.1: 6 -> 5 (padrao 3; 161 chunks/361 mil vertices davam 1-3 FPS no VC4)
extern "C" void glFinish(void);   // espera a GPU (menos atraso de entrada)
#else
#ifdef BL_XP
#define BL_WIN_W 854   // patchXP.1: janela inicial pequena (Chrome9/Atom: fillrate baixo); o minimo do layout e 854x480
#define BL_WIN_H 480
#else
#define BL_WIN_W 1280
#define BL_WIN_H 720
#endif
#define BL_WIN_MINW 854
#define BL_WIN_MINH 480
#define BL_SMALL_UI 0
#define BL_SMOOTH_FORCE(x) (gPerf ? 0 : (x))   // patchPerf.1: modo desempenho = sem luz suave
#define BL_RPI_FASTGRASS (gPerf ? 1 : 0)
#define BL_RPI_MAXRD 5
#define BL_RPI_FOG_START 0.25f
#endif
#ifdef BL_XP
#define BL_DEF_AA 0   // patchXP.1: padroes do build Windows XP (VIA Chrome9 + Atom): sem MSAA, sem aniso, sem bias de LOD
#define BL_DEF_ANISO 1
#define BL_DEF_SHARP 0.f
#else
#define BL_DEF_AA 2
#define BL_DEF_ANISO 4
#define BL_DEF_SHARP -0.5f
#endif
static const char* const GAME_VERSION_TEXT = "Blockland " BLOCKLAND_VERSION;

#include <SDL.h>
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
// patchPerf.1: "Modo desempenho" = as otimizacoes do Raspberry como opcao em qualquer plataforma. No Raspberry (-DBL_RPI) fica SEMPRE ligado e travado.
// gPerf = valor ativo NESTA sessao (lido de options.cfg ao iniciar, antes de montar atlas/malhas); gPerfNext = escolha salva para a proxima (a tela de Configuracoes pede reinicio).
#ifdef BL_RPI
#define BL_PERF_LOCK 1
static bool gPerf = true, gPerfNext = true;
#else
#define BL_PERF_LOCK 0
#ifdef BL_XP
static bool gPerf = true, gPerfNext = true;   // patchXP.1: build Windows XP = modo desempenho ligado de fabrica (perfmode=0 em options.cfg desliga)
#else
static bool gPerf = false, gPerfNext = false;
#endif
#endif
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
// patchGfx.1: so o modo desempenho / celular / Raspberry corta plantas e modelos distantes; PC normal desenha tudo ate a neblina
#if defined(__ANDROID__) || defined(BL_RPI)
#define BL_FARSKIP 1
#else
#define BL_FARSKIP (gPerf)
#endif
static int gAA = BL_DEF_AA, gAniso = BL_DEF_ANISO;   // patchPerf.6: padrao MSAA 2 (4x custava ~10+ ms em GPU fraca, 1366x768); options.cfg: aa=0|2|4|8 (MSAA, precisa reiniciar), aniso=1|2|4|8|16
static double gEntDist = 0.0;   // patchCfg.2 (P138): options.cfg entdist=<16..160> = distancia max. de desenho dos mobs (0/ausente = automatico: 80, ou 48 em modo desempenho)
static bool gSkin3D = true;   // patchSkin3D.1: camadas externas da skin em voxels 3D (opcao Graficos > Camadas 3D da skin; options.cfg skin3d=0|1)
static float gSharp = BL_DEF_SHARP;   // patchGfx.2: options.cfg sharp=<-2..1>: bias de LOD do atlas (negativo = textura mais nitida ao longe/de lado; 0 = antigo). So PC (GL desktop)
static void gfxLoadOpts() {
    FILE* f = fopen("options.cfg", "rb"); if (!f) return; char ln[384];
    while (fgets(ln, sizeof ln, f)) { int v; if (sscanf(ln, "aa=%d", &v) == 1) gAA = v == 2 || v == 4 || v == 8 ? v : 0; if (sscanf(ln, "aniso=%d", &v) == 1) gAniso = std::max(1, std::min(16, v)); float fv; if (sscanf(ln, "sharp=%f", &fv) == 1) gSharp = std::max(-2.f, std::min(1.f, fv)); if (sscanf(ln, "entdist=%f", &fv) == 1) gEntDist = fv <= 0.f ? 0.0 : (double)std::max(16.f, std::min(160.f, fv)); }
    fclose(f);
}
static bool gShowFps = false;   // patchCfg.7: opcao Video > Mostrar FPS (contador no canto, sem abrir o F3)
static bool gOptParticles = true;   // patchPerf.2: opcao Particulas (screens.inc, gfxTogglesSync)
static int gRodId = 0, gRodCastId = 0; static bool gBobOn = false;   // BUG-019: vara de pesca (fishing.inc); ids do item pack e flag de boia lancada
static void perfModeLoad() {
#ifndef BL_RPI
    FILE* f = fopen("options.cfg", "rb"); if (!f) return; char ln[384];
    while (fgets(ln, sizeof ln, f)) { int v; if (sscanf(ln, "perfmode=%d", &v) == 1) gPerf = gPerfNext = v != 0; }
    fclose(f);
#endif
}
#ifdef BL_RPI
// patchRpiOpt.7: o jogo SEMPRE enxerga 400x250; em janela maior/tela cheia o quadro e esticado (bilinear = suavizacao gratis) mantendo a proporcao
#define BL_RPI_FOG_START 0.25f   // inicio da neblina (fracao do alcance)
static int gLowRW = BL_WIN_W, gLowRH = BL_WIN_H, gBW = BL_WIN_W, gBH = BL_WIN_H; static bool gLowOn = false; static float gLowF = 1.f;   // patchRpi.3: gBW x gBH = resolucao interna base (~100 mil pixels, como 400x250) adaptada a proporcao da janela (16:9, 4:3, 16:10...)   // gLowF > 1: resolucao interna = 400x250 / gLowF (inventario)
static inline int blVW() { return std::max(160, (int)(gBW / gLowF)); }
static inline int blVH() { return std::max(100, (int)(gBH / gLowF)); }
static inline void blGetWinSize(SDL_Window* w, int* pw, int* ph) {
    int rw = 0, rh = 0; SDL_GetWindowSize(w, &rw, &rh); if (rw > 0 && rh > 0) {
        gLowRW = rw; gLowRH = rh;
        const float asp = std::max(1.f, std::min(2.4f, rw / (float)rh)); int bw = (int)(sqrtf(BL_WIN_W * (float)BL_WIN_H * asp) + .5f), bh = (int)(bw / asp + .5f);
        gBW = std::min(bw, 512); gBH = std::min(bh, 512);   // 16:10 = 400x250, 16:9 = 421x237, 4:3 = 365x274 (mesmo custo de pixels)
    }
    gLowOn = gLowRW != gBW || gLowRH != gBH || gLowF > 1.001f;
    if (pw) *pw = gLowOn ? blVW() : gLowRW; if (ph) *ph = gLowOn ? blVH() : gLowRH;
}
#define SDL_GetWindowSize(w, pw, ph) blGetWinSize(w, pw, ph)
static void blSwap(SDL_Window* w);
#define SDL_GL_SwapWindow(w) blSwap(w)
static inline void blScaleEvent(SDL_Event& e) {   // mouse em pixels da janela real -> pixels do jogo (400x250)
    if (!gLowOn) return;
    const float s = std::min(gLowRW / (float)gBW, gLowRH / (float)gBH); const int ox = (int)((gLowRW - gBW * s) * .5f), oy = (int)((gLowRH - gBH * s) * .5f);
    auto cl = [](int v, int hi) { return v < 0 ? 0 : v >= hi ? hi - 1 : v; };
    if (e.type == SDL_MOUSEMOTION) { e.motion.x = cl((int)((e.motion.x - ox) / s / gLowF), blVW()); e.motion.y = cl((int)((e.motion.y - oy) / s / gLowF), blVH()); }
    else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) { e.button.x = cl((int)((e.button.x - ox) / s / gLowF), blVW()); e.button.y = cl((int)((e.button.y - oy) / s / gLowF), blVH()); }
}
#endif
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
#include <set>
#include <array>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <filesystem>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <array>
#include <chrono>
#include <tuple>
#include <deque>
#include <csignal>
#ifdef __ANDROID__
#include "platform/android.inc"   // Android: camada GL->GLES (gl4es), assets do APK, HUD touch automatico
#else
#define BL_ANDROID_WINFLAGS 0
#endif
#ifdef BL_GLES_NATIVE
#include "platform/glesnative.inc"   // Android: OpenGL 1.x -> OpenGL ES NATIVO (sem gl4es); no PC nao e usado
#endif
#ifdef BL_VULKAN
#include "platform/vkgl.inc"   // OpenGL 1.x -> Vulkan (macros gl*/SDL_GL_* redirecionadas)
#endif
#include "render/renderer.inc"   // patchVk.3: interface rd* (pilha de matrizes) acima do OpenGL/vkgl
static void splashPoll();   // splash.inc: desenha um quadro do loading se ja passou ~16 ms (cargas pesadas chamam isso nos loops)
static inline bool splashActive();
static void splashNote(const char* path);   // splash.inc: mostra "loaded: arquivo" na tela de loading (so tem efeito durante o loading)
#include "core/logger.inc"
#include "core/discord.inc"   // patchDiscord.1: Rich Presence (IPC puro, thread propria; discord.cfg)
#include "core/vorbis.inc"   // patchAud.OGG: decodificador Ogg Vorbis (sem libs)
#include "game/audio.inc"   // sons: mixer SDL proprio, .ogg (preferido) e WAV/IMA-ADPCM em assets/sounds (patchRPI.11, patchAud.OGG)
// (logger.inc acima: logs no formato do Minecraft Java)
#include "core/jobpool.inc"   // patchChunks.1: pool de threads (malha/luz/geracao de chunks); define BL_TLS (antes do perf.inc)
#include "core/perf.inc"   // profiler de lag (logs [PERF]/[LAG])
#include "core/frustum.inc"   // culling por chunk (opcional, BLOCKLAND_CULL=1)

static const int CW = 16, CH = 256;
static double gEntCam[3] = {0, 0, 0};   // patchRpiOpt.6: camera do quadro (itens/mobs longe nao sao desenhados nem simulados no Pi)
static int RAD = 7;   // distancia de renderizacao em chunks (menu Video > Renderizacao; atualizado todo quadro de screens.inc/main_loop.inc)
static inline bool entFarFrom(double x, double z, double r) { const double dx = x - gEntCam[0], dz = z - gEntCam[2]; return dx * dx + dz * dz > r * r; }
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
#ifdef BL_RPI
static const int ATL = 64, ATLR0 = 123, ATLR = ATLR0 + 5;   // patchRpi.3: atlas 1024x2048 (potencia de 2 e <= 2048 = limite do VC4/Mesa; o de 512x2384 dava GL_INVALID_VALUE = tudo sem textura). Mesma capacidade de tiles
#else
static const int ATL = 32, ATLR0 = 144, ATLR = ATLR0 + 5;
#endif
//   /* ATLR0 = linhas de pack/entidades/modelos; +2 linhas = sprites de cama e estandarte */   // atlas runtime 512x2304 = 32 colunas x 144 linhas de tiles 16px
static const int ATW = ATL * 16;       // largura do atlas em pixels
// ---- itens do resource pack: TODA textura de block/ e item/ do pack vira um item de inventario (ids >= PK0, tiles >= PKTILE0) ----
// Sao itens so de inventario (nao colocaveis: os chunks guardam 1 byte por bloco). Registrados em loadPackItems() (asset_manager.inc).
static const int PK0 = 1016, PKTILE0 = 768;
// patchProp.1b: ids dos props (Decocraft etc.) numa faixa propria acima do resource pack: 32768..65535 (ate 32768 itens; o Decocraft usa 3119).
// Antes cabiam so ~136 ids (XB 900..1015). Ordem = ordem dos itens em props.bin de cada mod (ordem alfabetica de mods/), entao os ids sao estaveis.
static const int PROP0 = 32768;
static int gPropTotal = 0;
static inline bool isProp(int id) { return id >= PROP0 && id < PROP0 + gPropTotal; }
static const char* propName(int id);
struct PkItem { std::string file, name, fold; bool cube; int tile = -1; int ft[3] = {-1, -1, -1}; int fr = -1; int stack = 64; };   // fr = tile da FRENTE (blocos com orientacao: abobora entalhada, fornalha, dispenser...), -1 = sem frente   // ft = tiles topo/lado/base (cubos com faces diferentes)   // file = "block/xxx" ou "item/xxx"; cube = face opaca de bloco (icone em cubo)
static std::vector<PkItem> PK;
static inline bool isPk(int id) { return id >= PK0 && id < PK0 + (int)PK.size(); }
static std::vector<uint8_t> gPkLeaf, gPkLog, gPkTrans;   // gPkTrans: 1 = cubo translucido (vidro colorido/gelo/slime: malha com blend), 2 = vidro comum (alpha-test, sem faces internas entre iguais)   // folhas (1 tinta de bioma, 2 abeto fixo, 3 bétula fixo, 4 sem tinta) e troncos do pack, por indice de PK
static int gChiselItems = 0, gChiselTiles = 0;   // Chisel (chisel.cfg): itens no fim de PK e tiles unicos que ocupam (tiles deduplicados: != 1 por item)
static int gChiselIron = 0, gChiselDia = 0;   // ids dos cinzeis (0 = sem Chisel)
static inline int pkTileEnd() { return PKTILE0 + (int)PK.size() - gChiselItems + gChiselTiles; }   // 1o tile livre depois dos tiles do pack
static inline bool isPkBlk(int id) { return isPk(id) && PK[id - PK0].cube; }   // bloco colocável vindo do pack
static inline int leafKind(int b) { if (b == LEAVES) return 1; if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkLeaf.size() ? gPkLeaf[i] : 0; } return 0; }   // 0 = nao e folha
static inline int transKind(int b) { if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkTrans.size() ? gPkTrans[i] : 0; } return 0; }
static inline bool isLeafB(int b) { return leafKind(b) != 0; }
static inline bool isLogB(int b) { if (b == LOG) return true; if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkLog.size() && gPkLog[i]; } return false; }
static std::vector<uint8_t> gPkOpac, gPkEmit, gPkTint, gPkTool; static std::vector<int8_t> gPkLevel; static std::vector<float> gPkHard;
static std::unordered_map<std::string, int> gPkByFile; static std::unordered_map<uint32_t, int> gPkByHash;   // nome do arquivo / hash -> id
static void pkBuildProps();
static uint32_t netFnvPk(const std::string& s) { uint32_t h = 2166136261u; for (unsigned char ch : s) { h ^= ch; h *= 16777619u; } return h & 0x3FFFFFFFu; }   // hash do nome (rede)
static uint32_t netFnvMod(const std::string& s) { uint32_t h = 2166136261u; for (unsigned char ch : s) { h ^= ch; h *= 16777619u; } return h & 0x7FFFFFFFu; }   // BUG-046: hash estavel do mod (pasta/bloco), independe da ordem de carga
// ---- mods: ids 120..255 (itens: tile 192+(id-120); blocos: tiles 336+3*(id-120)+face) ----
static const int CTABLE = 104;   // mesa de trabalho: id livre entre as armaduras (103) e os mods (120)
static const int MOD0 = 120;
static std::string modName[256]; static std::string modKey[256]; static std::unordered_map<uint32_t, int> gModByHash; static bool modBlk[256]; static int modEmit[256]; static int modNext = MOD0;
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
static int propArmorSlot(int id); static int propArmorPts(int id); static GLuint propArmorTex(int id, int layer);   // propsys.inc (patchMod.13: armadura de mod)
static inline int armorSlotOf(int id) { return isArmor(id) ? (id - IT_A0) % 4 : (id >= PROP0 && id < PROP0 + gPropTotal ? propArmorSlot(id) : -1); }   // 0 capacete 1 peitoral 2 calca 3 botas; -1 = nao e armadura
static int gFlintSteel = 0;   // id do isqueiro do pack (pkblocks.inc); 0 = pack sem ele
static int gBktEmpty = 0, gBktWater = 0, gBktLava = 0, gBktMilk = 0;   // ids dos baldes (item/bucket, water_bucket, lava_bucket do pack); 0 = pack sem balde
static int propStackOf(int id);
static inline int maxStack(int id) { if (id >= PROP0 && id < PROP0 + gPropTotal) return propStackOf(id); if (id && id == gFlintSteel) return 1; if (id && (id == gBktWater || id == gBktLava || id == gBktMilk)) return 1; if (id && id == gBktEmpty) return 16; if (id == IT_EGG) return 16; if (isPk(id) && PK[id - PK0].stack != 64) return PK[id - PK0].stack; return (isTool(id) || isArmor(id)) ? 1 : 64; }
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
static const int XB0 = 900, XB_TORCH = 900, XB_LEVER = 905, XB_ANVIL = 930, XB_RTORCH = 934, XB_PLANT = 940, XB_SAPLING = 964, XB_RAIL = 970, XB_END = 1016;   // tocha 900..904, alavanca 905..928 (12 estados x desligada/ligada), bigorna 930..933, plantas 940..963, mudas 964..969, trilhos 970..973 (comum/energizado/detector/ativador; forma no metadado)
static const int XB_DUST = 974, XB_PIST = 975, XB_DISP = 977, XB_DROP = 978, XB_OBS = 979, XB_PLATE = 980, XB_BTN = 981, XB_HOOK = 982, XB_DAY = 983, XB_HOPPER = 984, XB_REPT = 985, XB_COMP = 986, XB_TRAP = 987, XB_GATE = 988, XB_DOOR = 989, XB_CHEST = 990;   // aba Redstone (redstone.inc): 974 poeira, 975/976 pistao/grudento, dispenser, dropper, observador, placa, botao, gancho, sensor, funil, repetidor, comparador, alcapao, portao, porta, bau; estado no meta
static const int XB_CARPET = 991, XB_FENCE = 992, XB_PANE = 993, XB_LADDER = 994, XB_WALL = 995, XB_CPANE = 996, XB_ENDROD = 997, XB_BED = 998, XB_SIGN = 999;   /* placa: dir | parede<<2 | madeira<<3 (0..11) */   // painel colorido (cor<<4), haste do End (meta = face), cama (dir | cabeca<<2 | cor<<3)   // escada de mao (meta = lado para onde aponta), muro de pedra (como a cerca)   // aba Decoracao (redstone.inc): carpete (meta = cor), cerca (conexoes 0-3 N/L/S/O + tipo<<4), barras/painel de vidro (idem)
static const int XB_STAIR = 1000, XB_SLAB = 1001, XB_CROP = 1002, XB_FARM = 1003;   // v40: escada (dir|metade<<2|material<<3), laje (metade 0 baixo/1 cima/2 dupla | material<<2), colheita (tipo<<3|idade), terra arada
static const int XB_DECOR = 1006;   // patchBlk.2: bloco decorativo por variante (meta & 3: 0 caldeirao, 1 bolo [bits 2..4 = pedacos comidos])
static const int XB_LILY = 1004, XB_VINE = 1005;   // patch5.63: vitoria-regia (sobre agua; meta 0) e vinha (meta = mascara: bit k = placa do lado k como a escada de mao [0 +Z, 1 -X, 2 -Z, 3 +X], bit 4 = presa por cima)
static inline bool climbBlk(int b) { return b == XB_LADDER || b == XB_VINE; }
static const int XB_DEADBUSH = 1009;   // patchFlor.1: arbusto morto (deadbush do 1.12); fora de isRs
static const int XB_PEONY = 1007;   // patchPlant.2: peonia (planta alta do 1.12): 1007 metade de baixo, 1008 metade de cima (ids livres; fora de isRs)
static const int XB_FIRE = 1014, XB_PORTAL = 1015;   // nether.inc: fogo animado e portal do Nether (meta bit0 = eixo)
static int gDim = 0;   // dimensao atual: 0 superficie, 1 nether
static inline bool dimSky() { return gDim == 0 || gDim == 3; }   // patchBlood.1: dimensoes COM ceu (superficie e Rubra); Nether/End nao
// patchBlood.1/patchSleep.1: Dimensao Rubra (blood.inc) e sono estilo Minecraft (sleep.inc). Estado aqui porque propsys/redstone/chat usam antes da definicao.
static int gBloodDreamPct = 25;   /* chance (%) de o sono virar pesadelo (/pesadelo <0-100>) */ static bool gBloodSafe = false;   /* acabou de voltar do pesadelo */
static int gSleepKind = 0;   /* 0 nada, 1 prop (Decocraft), 2 bloco de cama */ static int gSleepBx = 0, gSleepBy = 0, gSleepBz = 0, gSleepHx = 0, gSleepHz = 0; static bool gSleepRolled = false; static int gSleepShown = -1;
struct Phys; struct PropEnt;
static bool gS2Use = false;   // patchSleep.2: true enquanto um jogador EXTRA do split usa um bloco (rsUse)
static bool sleepExtraUse(int fx, int y, int fz, int hx, int hz, float yaw); static void sleepExtraCount(int& tot, int& sleeping, int& full);   // splitscreen.inc
static bool sleepTick(Phys& P, bool leave); static bool sleepCanStart(double x, double y, double z); static void sleepBegin(int kind); static bool sleepUseBlockBed(int x, int y, int z);
static void bloodTransformChunk(struct Chunk* c, int cx, int cz); static void bloodGrassCol(int ix, int iz, float* a, float* c); static void bloodWaterTint(float* w); static void bloodSkyHook(float* top, float* hor); static void bloodUwHook(float* uw);
static const char* gChunkFile = "chunks16.dat";
static void dimSwitch(int nd); static const std::unordered_map<int64_t, std::vector<uint16_t>>& dimSavedC(int d); static const std::unordered_map<int64_t, std::vector<uint8_t>>& dimSavedM(int d); static std::unordered_map<int64_t, struct Chunk*>& dimWorld(int d); static void netherReset(); static std::unordered_map<int64_t, struct Chunk*>& netherOtherW(); static void netherOnWorldLoad(const std::string& dir);
static const int XB_BANNER = 929, XB_WBANNER = 939;   // estandarte: em pe (meta = rotacao 0..15 | cor<<4) e na parede (meta = dir 0..3 | cor<<4); ids livres do intervalo XB
static inline bool isBanner(int b) { return b == XB_BANNER || b == XB_WBANNER; }
static inline bool isXB(int b) { return b >= XB0 && b < XB_END; }
static inline bool isRs(int b) { return b >= XB_DUST && b < XB_FIRE && (b < XB_PEONY || b > XB_DEADBUSH); }
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
#include "core/core_noise.inc"
#include "pack/assets_atlas.inc"
#include "world/biome_light.inc"
#include "world/terrain112.inc"
#include "world/blockshape.inc"
#include "world/world.inc"
#include "core/png.inc"
#include "core/gif.inc"   // patchCapa.2: decodificador GIF (capas animadas)
#include "pack/capes.inc"   // patchCapa.1: capas (prontas, PNG/GIF), compactacao de rede
#include "pack/mcpack.inc"
#include "pack/tilecache.inc"   // patchFast.3: cache de tiles decodificados
#include "pack/asset_manager.inc"
#include "pack/pkblocks.inc"
#include "render/sky_cycle.inc"
#include "game/physics.inc"
#include "render/models.inc"
#include "render/player_skin.inc"
#include "render/render_items.inc"
#include "ui/font.inc"
#include "game/crafting.inc"
#include "ui/inventory_ui.inc"
#include "game/survival.inc"
#include "game/effects.inc"   // patchPot.1: efeitos de status (pocoes)
#include "game/enchant.inc"     // patchEnch.1: encantamentos (tabela, nbt, /enchant)
#include "ui/pause_menu.inc"
#include "pack/item_data.inc"   // nomes em portugues e pilha dos itens do pack
static void ptbrApply();   // ptbr.inc (incluido depois de redstone.inc)
#include "pack/mods.inc"
#include "pack/props.inc"   // patchProp.1: leitor do formato BLP1 (mods convertidos pelo conversor)
#include "game/chisel_tool.inc"
#include "modules/modules.inc"   // registro de ganchos dos modulos (ver src/modules/README.md)
#include "game/drops.inc"
#include "render/player_render.inc"
#include "mobs/mobs.inc"
#include "world/boat.inc"
#include "world/minecart.inc"   // trilhos + carrinhos de mina
#include "ui/chat_debug.inc"
#include "world/mining_sky.inc"
#include "core/launcher.inc"   // perfil: nome + skin (usado por net.inc e screens.inc)
#include "platform/gamepad.inc"   // controles (SDL_GameController): mapeamento, cursor virtual nos menus (usado por bow.inc, screens.inc, main_loop.inc)
#include "net/net.inc"
#include "net/voice.inc"   // voice chat (patchVoice.1): mic SDL + u-law 8 kHz sobre o UDP do net.inc
#include "ui/playerlist.inc"   // lista de jogadores (Tab): cabeca, nick, ping
#include "mobs/bow.inc"   // arco e flecha
#include "mobs/egg.inc"   // ovo arremessavel (usa bow.inc: arrowGauss/arrowSegBox, mobs.inc)
#include "mobs/hostile.inc"   // mobs hostis: zumbi, esqueleto, creeper (IA, ataque, flechas, explosao, spawn)
#include "mobs/pvp.inc"   // PvP + morte de jogador (usa net.inc, mobs.inc, survival.inc)
#include "mobs/mobsync.inc"   // mobs na rede: host simula e transmite, convidado interpola; mira em jogadores remotos
#include "world/blockupdate.inc"
#include "world/placement.inc"
#include "pack/propsys.inc"   // patchProp.2: props (Decocraft) colocaveis: icone 3D, mundo, colisao, salvar
#include "world/vehicle.inc"   // patchVeic.1: veiculos do Vehicle Mod (usa props: corpo/rodas)
#include "world/crops.inc"   // plantar/crescer/colher (usa redstone? nao: so world/drops)
#include "world/redstone.inc"   // aba Redstone: modelos, formas, colocacao, uso
#include "world/falling.inc"    // areia e cascalho caem (entidade de bloco)
#include "game/craft_extra.inc"   // patchCraft.3: receitas que faltavam + qualquer madeira (usa gRsDrop/gXbDrop; chamado por packRecipes em chest.inc)
#include "mobs/fishing.inc"   // BUG-019: vara de pesca (boia, mordida, pesca; usa bow.inc/craft_extra.inc/drops.inc)
#include "world/chest.inc"      // bau: armazenamento por posicao (conteudo, salvar/carregar, soltar ao quebrar)
#include "world/logic.inc"      // patchLogic.1: lampada de redstone e bloco de notas
#include "world/jukebox.inc"    // patchJuke.1: toca-discos
#include "world/anvil.inc"      // bigorna (patchAnvil.1): reparar, combinar, custo em niveis
#include "world/furnace.inc"    // fornalha (patchFur.1): fundir, combustivel, tela, luz, salvar
#include "pack/modrules.inc"    // patchMod.15: receitas.txt / drops.txt dos mods convertidos (usa props, fidv, Recipe)
#include "world/potions.inc"    // patchBrew.1: pocoes (itens, tabela de efeitos, regras de preparo)
#include "world/heads.inc"      // patchHead.1: cabecas de mob colocaveis
#include "world/potion_fx.inc"   // patchPot.1: beber pocoes
#include "world/brewing.inc"    // patchBrew.1: cervejaria (bloco, tela, tempo, salvar, rede)
#include "pack/ptbr.inc"       // nomes pt-BR dos itens do criativo
#include "platform/touch.inc"
#include "world/sign.inc"      // patchSign.1: texto das placas
#include "ui/menu.inc"
#include "net/netlan.inc"   // LAN/ZeroTier: descoberta, lista de servidores, hospedar/entrar (usa net.inc)
static void leafGfxApply(bool fast);   // patchGfx.2 (definida em compass.inc)
#include "core/keybinds.inc"   // patchKeys.1: teclas configuraveis (usado por screens.inc e keymap.inc)
#include "ui/screens.inc"
#include "ui/capes_ui.inc"   // patchCapa.1: botao/selecao da capa
#include "render/weather.inc"   // patchWx.1: clima
#include "ui/splash.inc"   // tela de loading estilo Mojang (inicio do jogo)
#include "modules/api.inc"   // patchMod.1: mini-APIs estaveis para modulos (api::blk/chat/hud/item/mob/cmd/cfg)
#include "modules/modules_list.inc"   // modulos ativos: entram DEPOIS de toda a API do jogo e ANTES do loop principal
#include "world/compass.inc"   // bussola: agulha aponta para o spawn (troca o quadro do tile)
#include "game/elytra.inc"   // elitra + foguetes (voo planado, desgaste, dano de parede)
#include "world/nether.inc"   // Nether: dimensao, terreno, fogo (isqueiro) animado, portal
#include "world/blood.inc"   // patchBlood.1: Dimensao Rubra (gDim 3): terreno, cores, ceu, entrada/saida (so dormindo)
#include "world/sleep.inc"   // patchSleep.1: dormir igual ao Minecraft (todos os jogadores) + pesadelo
#include "world/structures.inc"   // patchStruct.1: templo do deserto, iglu, cabana da bruxa
#include "world/stronghold.inc"   // patchStronghold.1: fortaleza (stronghold) 1.12.2
#include "world/mineshaft.inc"   // patchStruct.3: mina abandonada 1.12.2
#include "ui/splitscreen.inc"   // split screen local: jogador 2 (fisica, acoes, inventario) e layout dos viewports
#include "world/endgate.inc"   // patchEndGate.1: gateways do End (1.12.2)
#include "world/dragon.inc"   // patchEnd.1: Ender Dragon + ovo
#include "world/endportal.inc"   // patchEndPortal.1: portal do End (galaxia, camadas aditivas com as texturas do pack)
#include "world/map.inc"   // BUG-048: mapa
#include "core/keymap.inc"   // BUG-015: keys.cfg
#include "core/control.inc"   // patchCtl.1: --control DIR (print/estado/comandos por arquivo)
#include "render/gpu_chunks.inc"   // patchOrg.4: partes de main_loop.inc
#include "game/use_items.inc"
#include "render/sky_panorama.inc"
#include "render/shadows.inc"   // sombras do sol (F7)
#include "render/godrays.inc"   // god rays no nascer/por do sol (F9)
#include "main_loop.inc"
