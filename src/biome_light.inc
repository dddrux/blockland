// ---------- clima/bioma (cor da grama e das folhas) + opacidade de luz ----------
static inline float clampf2(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
// 0 = transparente à luz, 15 = bloqueia totalmente (como o Minecraft: água 3~2, folhas 1)
static inline int lightOpac(int b) { if (b == AIR) return 0; if (isWater(b)) return 2; if (isLava(b)) return 1; if (isLeafB(b)) return 1; if (isXB(b)) return 0; if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkOpac.size() ? gPkOpac[i] : 15; } return 15; }
static inline int lightEmit(int b) { if (isLava(b)) return 15; if (b == XB_ENDROD) return 14; if (b >= XB_TORCH && b < XB_TORCH + 5) return 14; if (b >= XB_RTORCH && b < XB_RTORCH + 5) return 7; if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkEmit.size() ? gPkEmit[i] : 0; } return b == GLOWSTONE ? 15 : (b >= MOD0 && b < 256 ? modEmit[b] : 0); }
// Tabelas de consulta (como o Starlight/Phosphor): a luz le opacidade/emissao por indice, sem a cadeia de ifs por celula. Refeitas sozinhas quando o pack de blocos muda.
// Cor da luz de bloco por tipo de emissor (indice 0..7): 0 tocha, 1 lava/magma, 2 redstone, 3 end rod, 4 glowstone, 5 lanterna do mar, 6 abobora/shroomlight, 7 froglight
static const float TINTC[8][3] = {{1.f, .93f, .78f}, {1.f, .50f, .18f}, {1.f, .30f, .25f}, {.92f, .96f, 1.f}, {1.f, .84f, .48f}, {.55f, .90f, 1.f}, {1.f, .68f, .30f}, {.75f, 1.f, .65f}};
static inline int lightTint(int b) { if (isLava(b)) return 1; if (b >= XB_RTORCH && b < XB_RTORCH + 5) return 2; if (b == XB_ENDROD) return 3; if (b == GLOWSTONE) return 4; if (b >= PK0) { const size_t i = (size_t)(b - PK0); return i < gPkTint.size() ? gPkTint[i] : 0; } return 0; }
static uint8_t gOpacLut[65536], gEmitLut[65536], gTintLut[65536]; static size_t gLutPk = (size_t)-1, gLutPe = (size_t)-1;
static void lightLutRefresh() {
    if (gLutPk == gPkOpac.size() && gLutPe == gPkEmit.size()) return;
    for (int i = 0; i < 65536; i++) { gOpacLut[i] = (uint8_t)lightOpac(i); gEmitLut[i] = (uint8_t)lightEmit(i); gTintLut[i] = (uint8_t)lightTint(i); }
    gLutPk = gPkOpac.size(); gLutPe = gPkEmit.size();
}
static inline float ltab(float l) { return 0.06f + 0.94f * powf(0.8f, 15.f - l); }   // curva exponencial de brilho
// ---- ciclo de dia e noite (igual ao Minecraft Java 1.8.9; lógica em sky_cycle.inc) ----
static long long gWorldTime = 0;      // ticks do mundo: 24000 por dia (20 ticks/s = 20 min); 0 = amanhecer, 6000 = meio-dia, 12000 = pôr do sol, 18000 = meia-noite
static bool gDayCycle = true;         // false = tempo parado (/time pause)
static float gSkyScale = 1.f;         // brilho da luz do céu (lightmap do MC: sunBrightness*0.95+0.05 -> 1.0 de dia, 0.24 de noite)
static float gSkyRB = 1.f;            // tom do céu nos canais R/G (sunBrightness*0.65+0.35): a noite fica azulada
static bool gShaderOK = false;        // shader da luz do mundo disponível (senão a luz do céu fica "assada" na malha, sem ciclo)
static float gEntDim[3] = {1.f, 1.f, 1.f};   // multiplicador de cor das entidades (mobs, itens, mão), definido por entidade a partir da luz do local
struct SkyState { float angle = 0.f, skyF = 1.f, sunB = 1.f, starB = 0.f, cloud = 1.f, sunrise[4] = {0, 0, 0, 0}; bool hasSunrise = false; int moonPhase = 0; };
static SkyState gSky;
// brilho da luz do céu (nível 0..15) já com o ciclo: o piso 0,06 é constante (cavernas não ficam mais escuras à noite)
// empacota (céu, bloco) no alfa do vértice: níveis em quartos (média de 4 células), ((céu*4)*64 + bloco*4)/4096 (<1, exato em float). bloco 15.75 = emissivo (branco puro)
// croma da luz de bloco: 3 bits por canal (R,G,B em 0..7) => codigo 0..511, empacotado acima dos 12 bits de luz (21 bits no total, exato em float)
static inline int chromaCode(float r, float g, float b) { auto q = [](float v) { const int i = (int)lroundf(v * 7.f); return i < 0 ? 0 : i > 7 ? 7 : i; }; return (q(r) * 8 + q(g)) * 8 + q(b); }
static inline void ccRGB(int cc, float* o) { o[0] = (cc >> 6) / 7.f; o[1] = ((cc >> 3) & 7) / 7.f; o[2] = (cc & 7) / 7.f; }
static const int CC_DEF = (7 * 8 + 7) * 8 + 5;   // tocha (1, .93, .78)
static inline float packLight(float sk, float bl, int cc = CC_DEF) { return ((float)cc * 4096.f + (float)(int)lroundf(sk * 4.f) * 64.f + (float)(int)lroundf(bl * 4.f)) / 2097152.f; }
static inline float ltabSky(float l) { return 0.06f + 0.94f * powf(0.8f, 15.f - l) * gSkyScale; }

static void climateAt(float wx, float wz, float* t, float* h) {   // temperatura e umidade em 0..1 (manchas grandes e suaves)
    float a = vnoise(wx * .006f + 310.f, wz * .006f + 57.f) * .65f + vnoise(wx * .015f + 90.f, wz * .015f + 410.f) * .35f;
    float b = vnoise(wx * .006f - 220.f, wz * .006f + 130.f) * .65f + vnoise(wx * .015f + 40.f, wz * .015f - 180.f) * .35f;
    *t = clampf2((a - .5f) * 2.4f + .5f, 0.f, 1.f); *h = clampf2((b - .5f) * 2.4f + .5f, 0.f, 1.f);
}
// ---------- colormaps (como o Minecraft): x = temperatura, y = umidade; a textura é cinza e é multiplicada pela cor ----------
static const int CMS = 64;
static float cmGrass[CMS][CMS][3], cmFoliage[CMS][CMS][3];
static void cmDefault(float (*cm)[CMS][3], const unsigned c[4]) {   // c: frio-seco, quente-seco, frio-úmido, quente-úmido
    for (int y = 0; y < CMS; y++) for (int x = 0; x < CMS; x++) {
        float fx = x / (float)(CMS - 1), fy = y / (float)(CMS - 1);
        for (int k = 0; k < 3; k++) {
            int sh = 16 - 8 * k;
            float a = ((c[0] >> sh) & 255) * (1 - fx) + ((c[1] >> sh) & 255) * fx, b = ((c[2] >> sh) & 255) * (1 - fx) + ((c[3] >> sh) & 255) * fx;
            cm[y][x][k] = (a * (1 - fy) + b * fy) / 255.f;
        }
    }
}
static void cmSample(float (*cm)[CMS][3], float t, float h, float* o) {
    float fx = clampf2(t, 0.f, 1.f) * (CMS - 1), fy = clampf2(h, 0.f, 1.f) * (CMS - 1);
    int x0 = (int)fx, y0 = (int)fy, x1 = std::min(x0 + 1, CMS - 1), y1 = std::min(y0 + 1, CMS - 1); float ax = fx - x0, ay = fy - y0;
    for (int k = 0; k < 3; k++)
        o[k] = (cm[y0][x0][k] * (1 - ax) + cm[y0][x1][k] * ax) * (1 - ay) + (cm[y1][x0][k] * (1 - ax) + cm[y1][x1][k] * ax) * ay;
}

// ---------- biomas ----------
enum Biome { B_OCEAN, B_BEACH, B_PLAINS, B_FOREST, B_DESERT, B_SAVANNA, B_JUNGLE, B_SWAMP, B_TAIGA, B_TUNDRA, B_MOUNTAINS, NBIOME };
static const char* BIOME_NAME[NBIOME] = {"Oceano", "Praia", "Planície", "Floresta", "Deserto", "Savana", "Selva", "Pântano", "Taiga", "Tundra", "Montanhas"};
static float mountainAt(float wx, float wz) {   // 0..1, montanhas onde > 0.5 (~12% do mapa)
    float m = vnoise(wx * .0045f + 700.f, wz * .0045f - 300.f) * .7f + vnoise(wx * .011f - 50.f, wz * .011f + 900.f) * .3f;
    return clampf2((m - .64f) / .07f, 0.f, 1.f);
}
static int biomeFrom(float t, float h, float m) {
    if (m > .5f) return B_MOUNTAINS;
    if (t < .1f) return B_TUNDRA;
    if (t < .38f) return h > .4f ? B_TAIGA : B_PLAINS;
    if (t > .7f) return h < .35f ? B_DESERT : h < .6f ? B_SAVANNA : B_JUNGLE;
    return h > .82f ? B_SWAMP : h > .5f ? B_FOREST : B_PLAINS;
}
