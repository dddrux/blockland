// ================= ciclo de dia e noite (portado do Minecraft Java 1.8.9) =================
// - Relógio do mundo: 24000 ticks por dia, 20 ticks/s (20 min reais). 0 = amanhecer, 6000 = meio-dia, 12000 = pôr do sol, 18000 = meia-noite.
// - Ângulo celeste = WorldProvider.calculateCelestialAngle; cor do céu/neblina = World.getSkyColor / getFogColor;
//   faixa do nascer/pôr do sol = calcSunriseSunsetColors; estrelas = RenderGlobal.renderStars (Random(10842), 1500 quadrados);
//   fases da lua = getMoonPhase (8 fases, 1 por dia); luz do céu = lightmap do EntityRenderer (getSunBrightness).
// - Sol (32x32) e lua (moon_phases.png 128x64, 4x2 fases) vêm do resource pack ativo; sem eles, placeholders com a mesma resolução.
#include <cstdint>

static void drawSkyDome(const float* top, const float* hor);   // definido em main_loop.inc
static const long long DAY_TICKS = 24000;

// ---------- relógio / ângulo celeste ----------
static float celestialAngle(long long t, float pt) {   // 0 = meio-dia, 0.5 = meia-noite
    int i = (int)(t % DAY_TICKS); if (i < 0) i += (int)DAY_TICKS;
    float f = ((float)i + pt) / 24000.f - .25f;
    if (f < 0.f) f += 1.f;
    if (f > 1.f) f -= 1.f;
    const float f1 = f;
    f = 1.f - (float)((cos((double)f * 3.14159265358979323846) + 1.0) / 2.0);
    return f1 + (f - f1) / 3.f;
}
static int moonPhaseOf(long long t) { long long d = t / DAY_TICKS; return (int)(((d % 8) + 8) % 8); }
static std::string timeString(long long t) {   // "HH:MM" como o relógio do MC (tick 0 = 06:00)
    long long m = ((t % DAY_TICKS) + DAY_TICKS) % DAY_TICKS;
    const int hh = (int)((m / 1000 + 6) % 24), mm = (int)((m % 1000) * 60 / 1000);
    char b[16]; snprintf(b, sizeof b, "%02d:%02d", hh, mm); return b;
}

// ---------- patchAmb.1: ambientacao do Minecraft Java 1.12.2 (traduzido do bytecode do client.jar) ----------
// World.getSkyColor: HSV por temperatura do bioma (Biome.getSkyColorByTemp) x fator do dia; WorldProvider.getFogColor: (.7529412,.84705883,1.0) x fator;
// EntityRenderer.updateFogColor: neblina puxada para a cor do ceu por f4 = 1-(.25+.75*dist/32)^.25, chuva/trovao escurecem, y<32 escurece (getVoidFogYFactor);
// EntityRenderer.setupFog: inicio = 75% e fim = 100% do alcance (fogStartD em screens.inc).
static bool gAmbJava = true; static float gAmbTemp = .8f, gAmbY = 80.f; static int gAmbRD = 7;   // temperatura do bioma da camera, altura do olho, distancia em chunks (render.inc, todo quadro)
static float ambBiomeTemp(int b) {   // Biome.getTemperature dos biomas do 1.12.2 equivalentes
    switch (b) { case B_OCEAN: return .5f; case B_BEACH: return .8f; case B_PLAINS: return .8f; case B_FOREST: return .7f; case B_DESERT: return 2.f; case B_SAVANNA: return 1.2f;
                 case B_JUNGLE: return .95f; case B_SWAMP: return .8f; case B_TAIGA: return .25f; case B_TUNDRA: return 0.f; case B_MOUNTAINS: return .2f; default: return .8f; }
}
static void ambHsv(float h, float s, float v, float* o) {   // MathHelper.hsvToRGB
    const int i = (int)(h * 6.f) % 6; const float f = h * 6.f - floorf(h * 6.f), p = v * (1.f - s), q = v * (1.f - f * s), t = v * (1.f - (1.f - f) * s);
    switch (i) { case 0: o[0] = v; o[1] = t; o[2] = p; break; case 1: o[0] = q; o[1] = v; o[2] = p; break; case 2: o[0] = p; o[1] = v; o[2] = t; break;
                 case 3: o[0] = p; o[1] = q; o[2] = v; break; case 4: o[0] = t; o[1] = p; o[2] = v; break; default: o[0] = v; o[1] = p; o[2] = q; break; }
}

// ---------- estado do céu por quadro ----------
// base: cores de dia do jogo (zênite e horizonte); aqui aplicamos os fatores de hora do dia do Minecraft.
// lookX = componente X do vetor de olhar (a neblina esquenta na direção do sol no nascer/pôr, como o EntityRenderer.updateFogColor).
static void updateSky(float pt, float lookX, const float* baseTop, const float* baseHor, float* top, float* hor) {
    const float ang = celestialAngle(skyTime(), pt), c = cosf(ang * 6.2831853f);
    gSky.angle = ang;
    gSky.skyF = clampf2(c * 2.f + .5f, 0.f, 1.f);                                   // World.getSkyColor: fator do zênite
    const float fog = gSky.skyF * .94f + .06f;                                       // World.getFogColor: nunca preto total
    const float sb = 1.f - clampf2(1.f - (c * 2.f + .2f), 0.f, 1.f);
    gSky.sunB = sb * .8f + .2f;                                                      // World.getSunBrightness (0.2 .. 1.0)
    gSkyScale = gSky.sunB * .95f + .05f;                                             // lightmap: brilho do céu
    gSkyRB = gSky.sunB * .65f + .35f;                                                // lightmap: tom azulado da noite
    const float sc = clampf2(1.f - (c * 2.f + .25f), 0.f, 1.f);                      // RenderGlobal.getStarBrightness
    gSky.starB = sc * sc * .5f;
    gSky.cloud = gSky.skyF * .9f + .1f;
    {   // patchAmb.2: World.getCloudColour do 1.12 (chuva: cinza .62; tempestade: mais escuro; azul um pouco mais claro)
        const float f = clampf2(c * 2.f + .5f, 0.f, 1.f); float cc[3] = {1.f, 1.f, 1.f};
        if (gRain > 0.f) { const float m = 1.f - gRain * .95f, l = .6f; for (int k = 0; k < 3; k++) cc[k] = cc[k] * m + l * (1.f - m); }
        cc[0] *= f * .9f + .1f; cc[1] *= f * .9f + .1f; cc[2] *= f * .85f + .15f;
        if (gThunder > 0.f) { const float l = (cc[0] * .3f + cc[1] * .59f + cc[2] * .11f) * .2f, m = 1.f - gThunder * .95f; for (int k = 0; k < 3; k++) cc[k] = cc[k] * m + l * (1.f - m); }
        for (int k = 0; k < 3; k++) gSky.cloudC[k] = cc[k];
    }                                              // World.getCloudColor
    gSky.moonPhase = moonPhaseOf(skyTime());
    // faixa do nascer/pôr do sol (calcSunriseSunsetColors)
    gSky.hasSunrise = false;
    if (c >= -.4f && c <= .4f) {
        const float f3 = c / .4f * .5f + .5f;
        float f4 = 1.f - (1.f - sinf(f3 * 3.14159265f)) * .99f; f4 *= f4;
        gSky.sunrise[0] = f3 * .3f + .7f; gSky.sunrise[1] = f3 * f3 * .7f + .2f; gSky.sunrise[2] = .2f; gSky.sunrise[3] = f4;
        if (!gPerf) gSky.hasSunrise = true;   // patchRpiOpt.5: modo desempenho sem faixa/tinta de nascer e por do sol
    }
    if (gAmbJava) {   // patchAmb.1: formulas do 1.12.2 no lugar da paleta por hora
        const float f = clampf2(c * 2.f + .5f, 0.f, 1.f), tp = clampf2(gAmbTemp / 3.f, -1.f, 1.f);
        float sk[3]; ambHsv(.62222224f - tp * .05f, .5f + tp * .1f, 1.f, sk);
        for (int k = 0; k < 3; k++) sk[k] *= f;
        if (gRain > 0.f) { const float l = (sk[0] * .3f + sk[1] * .59f + sk[2] * .11f) * .6f, m = 1.f - gRain * .75f; for (int k = 0; k < 3; k++) sk[k] = sk[k] * m + l * (1.f - m); }
        if (gThunder > 0.f) { const float l = (sk[0] * .3f + sk[1] * .59f + sk[2] * .11f) * .2f, m = 1.f - gThunder * .75f; for (int k = 0; k < 3; k++) sk[k] = sk[k] * m + l * (1.f - m); }
        float fg[3] = {.7529412f * (f * .94f + .06f), .84705883f * (f * .94f + .06f), 1.f * (f * .91f + .09f)};
        if (gSky.hasSunrise) { const float d0 = sinf(ang * 6.2831853f) > 0.f ? -1.f : 1.f; float f5 = lookX * d0; if (f5 < 0.f) f5 = 0.f; f5 *= gSky.sunrise[3]; for (int k = 0; k < 3; k++) fg[k] = fg[k] * (1.f - f5) + gSky.sunrise[k] * f5; }
        const float f4 = 1.f - powf(.25f + .75f * (float)gAmbRD / 32.f, .25f);
        for (int k = 0; k < 3; k++) fg[k] += (sk[k] - fg[k]) * f4;
        if (gRain > 0.f) { fg[0] *= 1.f - gRain * .5f; fg[1] *= 1.f - gRain * .5f; fg[2] *= 1.f - gRain * .4f; }
        if (gThunder > 0.f) { const float m = 1.f - gThunder * .5f; for (int k = 0; k < 3; k++) fg[k] *= m; }
        { float vy = gAmbY * (1.f / 32.f); if (vy < 1.f) { vy = std::max(0.f, vy); vy *= vy; for (int k = 0; k < 3; k++) fg[k] *= vy; } }   // getVoidFogYFactor: cavernas escurecem a neblina
        for (int k = 0; k < 3; k++) { top[k] = sk[k]; hor[k] = fg[k]; }
        hor[3] = 1.f; return;
    }
    // paleta por hora (0-24h, tick 0 = 06:00): zênite e horizonte/neblina. Interpolada com smoothstep; barata (so CPU, 1x por quadro).
    // meio-dia = cores base do jogo; as demais escalam pela razão baseTop/baseHor, então mods/packs que mudam a base continuam valendo.
    struct SkyKey { float h, t[3], o[3]; };
    static const SkyKey K[] = {
        { 0.f, {.020f,.030f,.090f}, {.050f,.070f,.150f}},   // meia-noite
        { 4.f, {.030f,.040f,.120f}, {.070f,.080f,.180f}},   // madrugada
        { 5.f, {.090f,.110f,.260f}, {.300f,.250f,.420f}},   // pré-aurora (roxo)
        { 6.f, {.280f,.340f,.600f}, {.980f,.600f,.380f}},   // nascer do sol (laranja)
        { 7.f, {.340f,.540f,.900f}, {.960f,.800f,.680f}},   // manhã dourada
        { 9.f, {.290f,.520f,.930f}, {.760f,.880f,1.00f}},   // manhã clara
        {12.f, {.270f,.500f,.900f}, {.740f,.860f,1.00f}},   // meio-dia
        {15.f, {.270f,.490f,.880f}, {.760f,.860f,.980f}},   // tarde
        {17.f, {.320f,.440f,.800f}, {.920f,.800f,.700f}},   // fim de tarde (quente)
        {18.f, {.300f,.280f,.560f}, {1.00f,.500f,.240f}},   // pôr do sol (laranja/vermelho)
        {18.8f,{.140f,.130f,.340f}, {.620f,.300f,.380f}},   // crepúsculo (rosa/violeta)
        {19.6f,{.050f,.060f,.180f}, {.160f,.140f,.290f}},   // azul hora
        {21.f, {.025f,.035f,.100f}, {.060f,.080f,.160f}},   // noite
        {24.f, {.020f,.030f,.090f}, {.050f,.070f,.150f}},
    };
    const float hh = fmodf((float)(((skyTime() % DAY_TICKS) + DAY_TICKS) % DAY_TICKS) / 1000.f + pt / 1000.f + 6.f, 24.f);
    int ki = 0; while (ki < 12 && hh >= K[ki + 1].h) ki++;
    float ft = clampf2((hh - K[ki].h) / (K[ki + 1].h - K[ki].h), 0.f, 1.f); ft = ft * ft * (3.f - 2.f * ft);
    static const float DT[3] = {.27f, .50f, .90f}, DH[3] = {.74f, .86f, 1.f};
    for (int k = 0; k < 3; k++) {
        top[k] = (K[ki].t[k] + (K[ki + 1].t[k] - K[ki].t[k]) * ft) * (baseTop[k] / DT[k]);
        hor[k] = (K[ki].o[k] + (K[ki + 1].o[k] - K[ki].o[k]) * ft) * (baseHor[k] / DH[k]);
    }
    (void)fog;
    hor[3] = 1.f;
    if (gSky.hasSunrise) {   // olhando para o lado do sol, a neblina (e o horizonte) puxa para a cor do nascer/pôr
        const float d0 = sinf(ang * 6.2831853f) > 0.f ? -1.f : 1.f;
        float f5 = lookX * d0; if (f5 < 0.f) f5 = 0.f;
        f5 *= gSky.sunrise[3];
        for (int k = 0; k < 3; k++) hor[k] = hor[k] * (1.f - f5) + gSky.sunrise[k] * f5;
    }
}

// ---------- texturas do sol e da lua (pack ou placeholder) ----------
static GLuint sunTex = 0, moonTex = 0; static bool celReady = false, sunPh = false, moonPh = false;
static uint32_t phHash(int x, int y) { uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }
static void placeholderSun(uint8_t* o) {   // 32x32: sol quadrado pixelado (igual ao tamanho do sun.png do MC)
    memset(o, 0, 32 * 32 * 4);
    for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) {
        const int dx = abs(2 * x - 31), dy = abs(2 * y - 31), d = std::max(dx, dy);   // distância "quadrada" ao centro (0..31)
        uint8_t* p = o + (y * 32 + x) * 4;
        if (d <= 12) { p[0] = 255; p[1] = 255; p[2] = 220; p[3] = 255; }
        else if (d <= 20) { p[0] = 255; p[1] = 232; p[2] = 110; p[3] = 255; }
        else if (d <= 24) { p[0] = 255; p[1] = 190; p[2] = 70; p[3] = 255; }
    }
}
static void placeholderMoon(uint8_t* o) {   // 128x64: 8 fases (4x2 de 32x32), mesma ordem do moon_phases.png (cheia, minguante...)
    memset(o, 0, 128 * 64 * 4);
    static const float T[8] = {-1.f, -.5f, 0.f, .5f, 1.f, .5f, 0.f, -.5f};    // posição do terminador (-1 = tudo aceso, 1 = apagada)
    static const bool waxing[8] = {false, false, false, false, false, true, true, true};
    for (int ph = 0; ph < 8; ph++) {
        const int ox = (ph % 4) * 32, oy = (ph / 4) * 32;
        for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) {
            const float nx = (x - 15.5f) / 10.f, ny = (y - 15.5f) / 10.f;
            if (fabsf(nx) > 1.f || fabsf(ny) > 1.f) continue;   // lua quadrada de 20x20, como a do MC
            const float r = sqrtf(std::max(0.f, 1.f - ny * ny));
            const bool lit = waxing[ph] ? (nx > T[ph] * r) : (-nx > T[ph] * r);
            const int cr = (int)(phHash(x + ph * 7, y) % 5);   // "crateras" pixeladas
            uint8_t* p = o + ((oy + y) * 128 + ox + x) * 4;
            const int v = lit ? 232 - cr * 9 : 22 + cr * 2;
            p[0] = (uint8_t)v; p[1] = (uint8_t)v; p[2] = (uint8_t)(lit ? v - 8 : v + 8); p[3] = 255;
        }
    }
}
static GLuint upTex(const uint8_t* px, int w, int h) {
    GLuint t = 0; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    return t;
}
static void initCelestial() {
    celReady = true;
    std::vector<uint8_t> px; int w = 0, h = 0;
    // sol: nomes novos (1.13+: environment/celestial/sun.png) e antigos (environment/sun.png)
    if ((packRGBA("environment/celestial/sun.png", px, w, h) || packRGBA("environment/sun.png", px, w, h)) && w >= 1 && h >= 1) { sunTex = upTex(px.data(), w, h); }
    else { uint8_t ph[32 * 32 * 4]; placeholderSun(ph); sunTex = upTex(ph, 32, 32); sunPh = true; }
    // lua: moon_phases.png (4x2) ou as 8 texturas separadas de environment/celestial/moon/ (1.21.4+), montadas na mesma grade
    if (packRGBA("environment/moon_phases.png", px, w, h) && w >= 4 && h >= 2) { moonTex = upTex(px.data(), w, h); return; }
    static const char* N[8] = {"full_moon", "waning_gibbous", "third_quarter", "waning_crescent", "new_moon", "waxing_crescent", "first_quarter", "waxing_gibbous"};
    std::vector<uint8_t> tiles[8]; int tw = 0, th = 0; bool all = true;
    for (int i = 0; i < 8 && all; i++) {
        int a, b; if (!packRGBA(std::string("environment/celestial/moon/") + N[i] + ".png", tiles[i], a, b) || a != b) { all = false; break; }
        if (i == 0) { tw = a; th = b; } else if (a != tw || b != th) all = false;
    }
    if (all && tw > 0) {
        std::vector<uint8_t> atlas((size_t)tw * 4 * th * 2 * 4, 0);
        for (int i = 0; i < 8; i++) for (int y = 0; y < th; y++) memcpy(&atlas[(((size_t)((i / 4) * th + y)) * tw * 4 + (i % 4) * tw) * 4], &tiles[i][(size_t)y * tw * 4], (size_t)tw * 4);
        moonTex = upTex(atlas.data(), tw * 4, th * 2);
    } else { uint8_t ph[128 * 64 * 4]; placeholderMoon(ph); moonTex = upTex(ph, 128, 64); moonPh = true; }
}

// ---------- estrelas (RenderGlobal.renderStars: java.util.Random(10842), 1500 quadrados) ----------
struct JRand {   // java.util.Random
    uint64_t s; explicit JRand(int64_t seed) { s = ((uint64_t)seed ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1); }
    int next(int bits) { s = (s * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1); return (int)(int64_t)(s >> (48 - bits)); }
    float nextFloat() { return next(24) / (float)(1 << 24); }
    double nextDouble() { return (double)(((int64_t)next(26) << 27) + next(27)) * (1.0 / 9007199254740992.0); }
};
static GLuint starList = 0;
static void buildStars() {
    starList = glGenLists(1); glNewList(starList, GL_COMPILE); glBegin(GL_QUADS);
    JRand rnd(10842);
    for (int i = 0; i < 1500; i++) {
        double d0 = rnd.nextFloat() * 2.f - 1.f, d1 = rnd.nextFloat() * 2.f - 1.f, d2 = rnd.nextFloat() * 2.f - 1.f;
        const double d3 = .15f + rnd.nextFloat() * .1f;
        double d4 = d0 * d0 + d1 * d1 + d2 * d2;
        if (d4 < 1.0 && d4 > .01) {
            d4 = 1.0 / sqrt(d4); d0 *= d4; d1 *= d4; d2 *= d4;
            const double d5 = d0 * 100.0, d6 = d1 * 100.0, d7 = d2 * 100.0;
            const double d8 = atan2(d0, d2), d9 = sin(d8), d10 = cos(d8);
            const double d11 = atan2(sqrt(d0 * d0 + d2 * d2), d1), d12 = sin(d11), d13 = cos(d11);
            const double d14 = rnd.nextDouble() * 3.14159265358979323846 * 2.0, d15 = sin(d14), d16 = cos(d14);
            for (int j = 0; j < 4; j++) {
                const double d17 = 0.0, d18 = (double)((j & 2) - 1) * d3, d19 = (double)(((j + 1) & 2) - 1) * d3;
                const double d20 = d18 * d16 - d19 * d15, d21 = d19 * d16 + d18 * d15;
                const double d22 = d20 * d12 + d17 * d13, d23 = d17 * d12 - d20 * d13;
                const double d24 = d23 * d9 - d21 * d10, d25 = d21 * d9 + d23 * d10;
                glVertex3d(d5 + d24, d6 + d22, d7 + d25);
            }
        }
    }
    glEnd(); glEndList();
}

// cor da luz do sol depois de atravessar a atmosfera: massa de ar (Kasten-Young) x dispersao de Rayleigh (mais forte no azul), como nos shaders de atmosfera (BSL/SEUS/Nishita). Normalizada.
static void sunTransmit(float y, float* o) {
    const float e = clampf2(y, 0.f, 1.f), el = asinf(e) * 57.29578f;
    float am = 1.f / (e + .50572f * powf(6.08f + el, -1.6364f)); if (am > 18.f) am = 18.f;
    o[0] = expf(-.04f * am); o[1] = expf(-.095f * am); o[2] = expf(-.22f * am);
    const float m = std::max(o[0], std::max(o[1], o[2])); o[0] /= m; o[1] /= m; o[2] /= m;
}
// ---------- desenho: domo + faixa do nascer/pôr + sol + lua + estrelas (RenderGlobal.renderSky) ----------
// Usa a matriz atual (só a rotação da câmera). Deixa o teste de profundidade desligado, como o drawSkyDome.
static float cloudSunOcc(const double* cam, const float* sd);   // mining_sky.inc
static float gSkyCamW[3] = {0.f, 0.f, 0.f};   // camera do quadro anterior (gCamW so e declarada depois): oclusao do sol pelas nuvens
static bool gOptSky = true, gOptSunMoon = true, gOptStars = true; static float gCloudH = 0.f;   // patchOpt.3: Detalhes > Ceu / Sol e Lua / Estrelas / Altura das nuvens (screens.inc: gfxTogglesSync)
static void drawSkyFull(const float* top, const float* hor) {
    if (gOptSky) drawSkyDome(top, hor);   // patchRpiOpt.7: Ceu OFF so tira o domo (cor de fundo/neblina); sol, lua e estrelas continuam com as proprias opcoes
    if (!celReady) initCelestial();
    if (!starList) buildStars();
    rdDisable(GL_FOG); rdDisable(GL_DEPTH_TEST); rdDisable(GL_ALPHA_TEST); rdDepthMask(GL_FALSE); rdEnable(GL_BLEND);
    if (gOptSky && gSky.hasSunrise) {   // faixa colorida no horizonte do lado do sol
        rdDisable(GL_TEXTURE_2D); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        rdPushMatrix();
        rdRotatef(90.f, 1, 0, 0);
        rdRotatef(sinf(gSky.angle * 6.2831853f) < 0.f ? 180.f : 0.f, 0, 0, 1);
        rdRotatef(90.f, 0, 0, 1);
        const float* sr = gSky.sunrise;
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(sr[0], sr[1], sr[2], sr[3]); glVertex3f(0.f, 100.f, 0.f);
        glColor4f(sr[0], sr[1], sr[2], 0.f);
        for (int l = 0; l <= 16; l++) {
            const float a = l * 6.2831853f / 16.f, s = sinf(a), c = cosf(a);
            glVertex3f(s * 120.f, c * 120.f, -c * 40.f * sr[3]);
        }
        glEnd();
        if (BL_RPI_FASTGRASS == 0) {   // patchRpi.7: no Pi, sem as camadas extras/halos em modo imediato (centenas de vertices por quadro)
            // patchLight.1: 2 camadas extras no nascer/por: faixa larga rosada-violeta (alto) + brasa dourada estreita rente ao horizonte
            const float a3 = sr[3];
            glBegin(GL_TRIANGLE_FAN); glColor4f(.86f, .38f, .58f, .30f * a3); glVertex3f(0.f, 100.f, 0.f); glColor4f(.55f, .30f, .72f, 0.f);
            for (int l = 0; l <= 16; l++) { const float a = l * 6.2831853f / 16.f, s = sinf(a), c = cosf(a); glVertex3f(s * 190.f, c * 190.f, -c * 70.f * a3); }
            glEnd();
            glBegin(GL_TRIANGLE_FAN); glColor4f(1.f, .78f, .38f, .55f * a3); glVertex3f(0.f, 100.f, 0.f); glColor4f(1.f, .55f, .2f, 0.f);
            for (int l = 0; l <= 16; l++) { const float a = l * 6.2831853f / 16.f, s = sinf(a), c = cosf(a); glVertex3f(s * 62.f, c * 62.f, -c * 16.f * a3); }
            glEnd();
        }
        rdPopMatrix();
    }
    {   // cinturao de Venus: faixa rosada/violeta no horizonte oposto ao sol, logo apos o por / antes do nascer (sombra da Terra)
        const float sy2 = cosf(gSky.angle * 6.2831853f), sx2 = -sinf(gSky.angle * 6.2831853f);
        const float bt = clampf2((.06f - sy2) / .16f, 0.f, 1.f) * clampf2((sy2 + .28f) / .2f, 0.f, 1.f);
        if (BL_RPI_FASTGRASS == 0 && gOptSky && !gAmbJava && bt > .01f) {   /* patchJava.4: o 1.12 nao tem o cinturao de Venus */
            rdDisable(GL_TEXTURE_2D); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            const float asx = sx2 > 0.f ? -1.f : 1.f;
            static const float HY[5] = {-30.f, -8.f, 10.f, 34.f, 76.f}, HC[5][4] = {{.95f, .50f, .50f, 0.f}, {.95f, .50f, .50f, .60f}, {.92f, .52f, .64f, .52f}, {.50f, .40f, .74f, .28f}, {.30f, .28f, .60f, 0.f}};   // patchSkyFix.1: linha roxa no horizonte = a faixa acabava seca em y=-8 (alfa .6); agora desce ate -30 com alfa 0
            for (int k = 0; k < 4; k++) {
                glBegin(GL_QUAD_STRIP);
                for (int l = 0; l <= 40; l++) {
                    const float a = l * 6.2831853f / 40.f, sa = sinf(a), ca = cosf(a), w = powf(std::max(sa * asx, 0.f), 1.5f) * bt;
                    for (int q = 0; q < 2; q++) { const int r = k + q; glColor4f(HC[r][0], HC[r][1], HC[r][2], HC[r][3] * w); glVertex3f(sa * 120.f, HY[r], ca * 120.f); }
                }
                glEnd();
            }
        }
    }
    rdEnable(GL_TEXTURE_2D); rdBlendFunc(GL_SRC_ALPHA, GL_ONE);   // sol e lua somam à cor do céu (preto = transparente)
    rdPushMatrix();
    rdRotatef(-90.f, 0, 1, 0);
    rdRotatef(gSky.angle * 360.f, 1, 0, 0);
    glColor4f(1.f, 1.f, 1.f, 1.f);
    float sc3[3]; const float sunY = cosf(gSky.angle * 6.2831853f); sunTransmit(sunY, sc3);
    // patchLight.2: o sol (disco + halo aditivos) aparecia por cima/atraves das nuvens como um circulo brilhante; agora escurece com a fracao do sol tapada pela camada de nuvens (suavizado)
    static float sOcc = 0.f; { const double cm[3] = {gSkyCamW[0], gSkyCamW[1], gSkyCamW[2]}; const float sdv[3] = {-sinf(gSky.angle * 6.2831853f), sunY, 0.f}; const float tg = cloudSunOcc(cm, sdv); sOcc += (tg - sOcc) * .08f; }
    const float sunVis = 1.f - .93f * clampf2(sOcc, 0.f, 1.f);
    const float low = clampf2(1.f - sunY / .55f, 0.f, 1.f), hvis = clampf2((sunY + .16f) / .18f, 0.f, 1.f);
    if (BL_RPI_FASTGRASS == 0 && gOptSky && hvis > .01f) {   // halo atmosferico (aproximacao de Mie): aneis aditivos com a cor real do sol; forte no nascer/por
        rdDisable(GL_TEXTURE_2D);
        const float hb = (.10f + .62f * low * low) * hvis * sunVis; static const float RR[5] = {0.f, 16.f, 48.f, 110.f, 240.f}, AA[5] = {1.f, .55f, .20f, .06f, 0.f};
        for (int k = 0; k < 4; k++) {
            glBegin(GL_QUAD_STRIP);
            for (int l = 0; l <= 32; l++) {
                const float a = l * 6.2831853f / 32.f, sa = sinf(a), ca = cosf(a);
                glColor4f(sc3[0], sc3[1], sc3[2], AA[k] * hb); glVertex3f(sa * RR[k], 100.f, ca * RR[k]);
                glColor4f(sc3[0], sc3[1], sc3[2], AA[k + 1] * hb); glVertex3f(sa * RR[k + 1], 100.f, ca * RR[k + 1]);
            }
            glEnd();
        }
        rdEnable(GL_TEXTURE_2D);
    }
    glColor4f(1.f - .8f * (1.f - sc3[0]), 1.f - .8f * (1.f - sc3[1]), 1.f - .8f * (1.f - sc3[2]), sunVis);
    const float S = 30.f * (gAmbJava ? 1.f : 1.f + .22f * low);   /* patchJava.4: Java = sol fixo 30 */    // sol: 60x60 a 100 blocos (maior perto do horizonte)
    if (!gOptSky) { rdEnable(GL_TEXTURE_2D); rdBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR); glColor4f(sc3[0] * sunVis, sc3[1] * sunVis, sc3[2] * sunVis, 1.f); }   // patchRpi.5: Ceu OFF + Sol/Lua ON: sem halos (viravam mancha) e mistura "screen" (o aditivo estourava no fundo claro)
    glBindTexture(GL_TEXTURE_2D, sunTex);
    if (gOptSunMoon) { glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f); glVertex3f(-S, 100.f, -S); glTexCoord2f(1.f, 0.f); glVertex3f(S, 100.f, -S);
    glTexCoord2f(1.f, 1.f); glVertex3f(S, 100.f, S);   glTexCoord2f(0.f, 1.f); glVertex3f(-S, 100.f, S);
    glEnd(); }
    glColor4f(1.f, 1.f, 1.f, 1.f);
    const float M = 20.f;   // lua: 40x40 a 100 blocos, no lado oposto
    const int ph = gSky.moonPhase, i2 = ph % 4, k2 = ph / 4 % 2;
    const float eu = .5f / 128.f, ev = .5f / 64.f, u0 = i2 / 4.f + eu, v0 = k2 / 2.f + ev, u1 = (i2 + 1) / 4.f - eu, v1 = (k2 + 1) / 2.f - ev;   // patchSky.7: meio texel para dentro: sem sangrar a linha da fase vizinha embaixo da lua
    if (BL_RPI_FASTGRASS == 0 && gOptSky && gOptSunMoon && gSky.starB > .001f) {   // halo suave azulado em volta da lua (aditivo)
        static const float PFh[8] = {1.f, .75f, .5f, .25f, 0.f, .25f, .5f, .75f}; const float hb = gSky.starB * (.45f + .55f * PFh[gSky.moonPhase & 7]); rdDisable(GL_TEXTURE_2D);   // patchLight.1: halo proporcional a fase
        glBegin(GL_TRIANGLE_FAN); glColor4f(.55f * hb, .70f * hb, 1.f * hb, .34f * hb); glVertex3f(0.f, -100.f, 0.f);
        glColor4f(.35f, .5f, .9f, 0.f); for (int l = 0; l <= 28; l++) { const float a = l * 6.2831853f / 28.f; glVertex3f(sinf(a) * 80.f, -100.f, cosf(a) * 80.f); }
        glEnd(); rdEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
    }
    glBindTexture(GL_TEXTURE_2D, moonTex); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   // patchSkyFix.1: lua nitida como no MC (LINEAR deixava o disco borrado/estourado)
    { const float mv = .62f + .38f * clampf2(gSky.starB * 3.f, 0.f, 1.f); glColor4f(1.f, 1.f, 1.f, mv); }   // no crepusculo a lua aditiva estourava em branco sobre o ceu rosa   // patchSky.11: o pack tem nucleo 8x8 + brilho azul suave no tile 32x32; NEAREST mostrava o brilho em degraus (faixa azul seca ao lado da lua)
    if (gOptSunMoon) { glBegin(GL_QUADS);
    glTexCoord2f(u1, v1); glVertex3f(-M, -100.f, M); glTexCoord2f(u0, v1); glVertex3f(M, -100.f, M);
    glTexCoord2f(u0, v0); glVertex3f(M, -100.f, -M); glTexCoord2f(u1, v0); glVertex3f(-M, -100.f, -M);
    glEnd(); }
    if (gOptStars && gSky.starB > .001f) {
        rdDisable(GL_TEXTURE_2D);
        glColor4f(gSky.starB, gSky.starB, gSky.starB, gSky.starB);
        glCallList(starList);
    }
    rdPopMatrix();
    glColor4f(1.f, 1.f, 1.f, 1.f); rdDepthMask(GL_TRUE); rdDisable(GL_BLEND); rdEnable(GL_TEXTURE_2D);
}

// ---------- luz das entidades (mobs, itens no chão, mão): lê a luz do chunk (céu + bloco) na posição ----------
static float gEntSk = 15.f;   // luz do ceu (0-15) da ultima entidade consultada
static void entLightAt(double x, double y, double z, float* o) {
    int sk = 15, bl = 0;
    const int wx = (int)floor(x), wy = (int)floor(y), wz = (int)floor(z);
    gEntSk = 15.f;
    if (wy >= 0 && wy < CH) {
        const int cx = fdiv(wx, CW), cz = fdiv(wz, CW); Chunk* c = getC(cx, cz);
        if (c && c->lc) { const int v = c->lc[idx(wx - cx * CW, wy, wz - cz * CW)]; sk = v >> 4; bl = v & 15; }
    }
    for (int i = 0; i < 4; i++) if (gDyn[i][3] > 0.f) {   // luz dinamica (item na mao / no chao) tambem ilumina entidades e a mao
        const float dx = (float)x - gDyn[i][0], dy = (float)y - gDyn[i][1], dz = (float)z - gDyn[i][2], l = gDyn[i][3] - sqrtf(dx * dx + dy * dy + dz * dz);
        if (l > (float)bl) bl = (int)l;
    }
    gEntSk = (float)sk;
    const float sv0 = (float)sk / 15.f, v = .96f * (sv0 / (4.f - 3.f * sv0)) * gSkyScale, sf = .03f;   // patchLuz.1: tabela de brilho do 1.12 (WorldProvider.generateLightBrightnessTable) e piso .03 do lightmap
    float bf = bl > 0 ? ltabB((float)bl) : 0.f; if (gDim == 2) bf = std::max(bf, .60f); else if (gDim == 1) bf = std::max(bf, ltabB((float)NETHER_AMB));   /* BUG-032: Nether: mesmo piso de luz de bloco do mundo (jogador/mobs nao ficam mais escuros que o chao) */   // patchLight.4: no End personagens/mobs nao ficam escuros (piso de brilho; sem ceu) (VIS: valor)
    const float wg = (bf * .6f + .4f) * .6f + .4f, wb = bf * bf * .6f + .4f;   // luz de bloco quente que esfria (lightmap do MC)
    o[0] = std::min(1.f, sf + v * gSkyRB + bf); o[1] = std::min(1.f, sf + v * gSkyRB + bf * wg * .93f); o[2] = std::min(1.f, sf + v + bf * wb * .78f);
    if (gDim == 2) { gEntSk = 15.f; for (int i = 0; i < 3; i++) o[i] = std::max(o[i], .92f); }   // patchFix.25: End = luz ambiente neutra (branca) para jogador, mobs, itens e mao; antes piso .60 tingia de laranja/escuro (R .66 G .48 B .35) e o shader de entidade usava ceu 0
    // patchWater.1: removida a tinta azul por celula de agua (azulava barco/mobs so por encostar na agua; a passada translucida da agua ja tinge o que fica submerso)
}
static inline void entDimReset() { gEntDim[0] = gEntDim[1] = gEntDim[2] = 1.f; }

// ---------- shader da luz do mundo (GL 2.0, sintaxe 1.20) ----------
// A malha guarda, por vértice, a cor SEM luz (tingimento do bioma x sombreamento da face x oclusão ambiente) e, no alfa, os níveis de
// luz do céu e de bloco empacotados ((céu*4)*64 + bloco*4)/4096. O shader aplica o ciclo: céu = 0.06 + 0.94*0.8^(15-nível)*escala.
// Com isso o dia/noite muda a luz de TODO o mundo sem refazer nenhuma malha.
#ifndef APIENTRY
#define APIENTRY
#endif
// ---- sombras (shadow mapping, estilo shaderpack do OptiFine; F7 liga/desliga; logica em shadows.inc) ----
typedef void (APIENTRY* FnU3f)(GLint, GLfloat, GLfloat, GLfloat); typedef void (APIENTRY* FnUM4)(GLint, GLsizei, GLboolean, const GLfloat*);
static FnU3f pUniform3f = nullptr; typedef void (APIENTRY* FnU2f)(GLint, GLfloat, GLfloat); static FnU2f pUniform2f = nullptr; static FnUM4 pUniformM4 = nullptr;
static bool gShCompat = false;   // compat (GLES/gl4es): sombras comparadas no shader (sampler2D + step) em vez de sampler2DShadow
static bool gShadowShader = false, gShadowDraw = false, gFxOn = false;   // shader com sombras compilou / sombras ativas neste quadro (so na passada dos solidos); gFxOn = sombras ligadas e prontas (usado por god rays)
static int gShQ = 2; static GLint uShQL = -1, uSunColL = -1; static float gSunCol[3] = {1.f, 1.f, 1.f};   // gShQ: qualidade das sombras no celular (2 normal, 1 leve, 0 minimo; cai sozinha se o FPS ficar baixo)
static GLint uTimeL = -1, uShOn = -1, uShSun = -1, uShStr = -1;
static GLuint gGodProg = 0; static GLint uGdSun = -1, uGdCol = -1, uGdI = -1, uGdAsp = -1;   static GLuint gFlProg = 0; static GLint uFlSun = -1, uFlCol = -1, uFlI = -1, uFlAsp = -1;   // god rays (godrays.inc)
static float gSunDir[3] = {0.f, 1.f, 0.f}, gSunStr = 0.f, gViewM[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}, gCamW[3] = {0, 0, 0}, gEntSkUnused = 0.f; static bool gShadowAct = false; static GLint uEnt = -1, uInvV = -1, uEntSky = -1, uCamW = -1;   // gViewM = vista do mundo (para entidades acharem a posicao no mundo)
static bool gShaderPackOn = false, gRfPass = false;
static GLuint gRfTex = 0, gRfDep = 0, gRfFbo = 0; static int gRfW = 0, gRfH = 0, gRfSW = 1, gRfSH = 1; static bool gRfOn = false; static GLint uRfOn = -1, uScrL = -1, uRfYL = -1; static float gRfWy = 0.f;   // reflexao planar da agua (estilo HL2)   // pacote de shader escolhido em Opcoes > Shaders (!= "Nao"): liga agua realista, god rays e lens flare
static float gEntSh[12][5]; static int gEntShN = 0; static bool gEntShOn = false; static GLint uEsNL = -1, uEsAL[12], uEsBL[12];   // patchShadow.4: capsulas verticais (x,y,z,raio,altura) das entidades que projetam sombra
static GLuint gCloudTex = 0; static float gCloudOff = 0.f, gSunOcc = 0.f; static GLint uCloudOnL = -1, uCloudOffL = -1, uFlOcc = -1;   // sombras das nuvens (mascara na unidade 7); gSunOcc = fracao do sol tapada por nuvem (0..1)
// luz dinamica: ate 4 fontes (x,y,z,nivel) em coordenadas de mundo; o nivel cai 1 por bloco de distancia (como mods Dynamic Lights)
static GLint uDynL[4] = {-1, -1, -1, -1};
typedef void (APIENTRY* FnU4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat); static FnU4f pUniform4f = nullptr;
static bool gXpShader = true;   // patchXP.1: options.cfg xpshader=0|1 (so no build XP; 0 = nunca usa GLSL)
static GLuint gSkyProgOpq = 0; static GLint uModeO = -2, uScaleO = -1, uRBO = -1; static float gPiU[3] = {0, 1, 1}; static bool gPiOn = false;   // patchRpiPerf.3
static GLuint skyProg = 0; static GLint uNethL = -1, uFancy = -1, uMode = -1, uSkyScale = -1, uSkyRB = -1;
static PFNGLUSEPROGRAMPROC pUseProgram = nullptr; static PFNGLUNIFORM1FPROC pUniform1f = nullptr;
static GLuint gSkyProgSh = 0, gSkyProgLean = 0; static std::function<void(GLuint)> gSkyBind; static GLint uHsPL = -1; static float gHsOx = 0.f, gHsOz = 0.f;
static const int HS_N = 256, HS_M = 48;   // patchShadow.3: mapa de alturas do sol: area HS_N x HS_N em volta do jogador (+HS_M de margem em X, de onde vem os raios)
static void skySelectProg(bool sh) {   // celular: sombras ligadas = programa de sombras; desligadas = programa enxuto (custo igual ao de antes)
    const GLuint want = (sh && gSkyProgSh) ? gSkyProgSh : (gSkyProgLean ? gSkyProgLean : gSkyProgSh);
    if (!want || want == skyProg || !gSkyBind) return; gSkyBind(want); pUseProgram(0);
}
static void initSkyShader() {
#ifdef BL_GLES_NATIVE
    gShaderOK = true; logInfo("Luz do ciclo dia/noite: shader GLES nativo (variantes)"); return;   // src/glesnative.inc faz a luz no vertex shader
#endif
#ifdef BL_VULKAN
    if (BL_VKON) { gShaderOK = true; logInfo("Luz do ciclo dia/noite: shader Vulkan"); return; }   // o shader do mundo ja faz parte do pipeline Vulkan (src/vk/blockland.vert)
#endif
    auto createShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
    auto shaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
    auto compileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
    auto getShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
    auto getShaderLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
    auto createProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
    auto attachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
    auto linkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
    auto getProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
    auto getUniformLoc = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
    auto uniform1i = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
    pUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
    pUniform1f = (PFNGLUNIFORM1FPROC)SDL_GL_GetProcAddress("glUniform1f");
    if (!createShader || !shaderSource || !compileShader || !getShaderiv || !createProgram || !attachShader || !linkProgram || !getProgramiv || !getUniformLoc || !uniform1i || !pUseProgram || !pUniform1f) {
        fprintf(stderr, "ciclo dia/noite: sem shaders (OpenGL 2.0); usando escurecimento simples\n"); return;
    }
    static const char* VS =
        "#version 120\n"
        "uniform float uMode; uniform float uSkyScale; uniform float uSkyRB; uniform vec4 uDyn0, uDyn1, uDyn2, uDyn3;\n"
        "float dynL(vec4 d, vec3 p) { vec3 q = p - d.xyz; float r2 = dot(q, q); return r2 < d.w * d.w ? d.w - sqrt(r2) : 0.0; }\n"
        "centroid varying vec2 vUVc; varying vec4 vCol; varying float vFog;\n"
        "vec3 chroma(float c) { float r = floor(c / 64.0); float g = floor((c - r * 64.0) / 8.0); float b = c - r * 64.0 - g * 8.0; return vec3(r, g, b) / 7.0; }\n"
        "void main() {\n"
        "  gl_Position = ftransform();\n"
        "  gl_TexCoord[0] = gl_MultiTexCoord0; vUVc = gl_MultiTexCoord0.xy;\n"
        "  vec4 e = gl_ModelViewMatrix * gl_Vertex;\n"
        "  vFog = length(e.xyz);\n"
        "  float v = floor(gl_Color.a * 2097152.0 + 0.5);\n"
        "  float cc = floor(v / 4096.0); v -= cc * 4096.0; if (cc > 511.5) { cc = 509.0; v = 4096.0; }\n"
        "  float sk4 = floor(v / 64.0);\n"
        "  float sk = sk4 * 0.25;\n"
        "  float bl = (v - sk4 * 64.0) * 0.25;\n"
        "  if (uDyn0.w > 0.0) { float dl = dynL(uDyn0, gl_Vertex.xyz); if (uDyn1.w > 0.0) dl = max(dl, dynL(uDyn1, gl_Vertex.xyz)); if (uDyn2.w > 0.0) { dl = max(dl, dynL(uDyn2, gl_Vertex.xyz)); dl = max(dl, dynL(uDyn3, gl_Vertex.xyz)); } if (dl > bl) { bl = dl; cc = 509.0; } }\n"
        "  if (uMode > 0.5) sk = min(15.0, sk + 2.0);\n"
        "  float sv = 0.96 * ((sk / 15.0) / (4.0 - 3.0 * sk / 15.0)) * uSkyScale;\n"
        "  float bf = 0.0;\n"
        "  if (bl > 0.01) { float vb = min(bl / 15.0, 1.0); bf = 1.5 * vb / (4.0 - 3.0 * vb); }\n"
        "  vec3 bc = chroma(cc) * vec3(bf, bf * ((bf * 0.6 + 0.4) * 0.6 + 0.4), bf * (bf * bf * 0.6 + 0.4));\n"
        "  vec3 L = min(vec3(1.0), vec3(0.03 + sv * uSkyRB + bc.r, 0.03 + sv * uSkyRB + bc.g, 0.03 + sv + bc.b));\n"
        "  if (bl > 15.5) L = vec3(1.0);\n"
        "  vCol = vec4(gl_Color.rgb * L, uMode > 0.5 ? 0.62 : 1.0);\n"
        "}\n";
    static const char* FS =
        "#version 120\n"
        "uniform sampler2D uTex;\n"
        "centroid varying vec2 vUVc; varying vec4 vCol; varying float vFog;\n"
        "void main() {\n"
        "  vec4 c = texture2D(uTex, vUVc, clamp((vFog * 0.0625 - 8.0) * 0.25, 0.0, 3.0)) * vCol;\n"
        "  if (c.a < mix(0.3, 0.1, smoothstep(6.0, 56.0, vFog))) discard;\n"   // o alpha test fixo nao vale com shader proprio (GLES): sem isso, partes transparentes ficam pretas
        "  float f = clamp((gl_Fog.end - vFog) / max(gl_Fog.end - gl_Fog.start, 0.001), 0.0, 1.0); f = mix(f, f * f * (3.0 - 2.0 * f), 0.55);\n"
        "  c.rgb = mix(gl_Fog.color.rgb, c.rgb, f);\n"
        "  gl_FragColor = c;\n"
        "}\n";
    static const char* VSH =
        "#version 120\n"
        "uniform float uMode; uniform float uSkyScale; uniform float uSkyRB; uniform float uEnt; uniform mat4 uInvV; uniform float uEntSky;\n"
        "uniform vec4 uDyn0, uDyn1, uDyn2, uDyn3;\n"
        "float dynL(vec4 d, vec3 p) { vec3 q = p - d.xyz; float r2 = dot(q, q); return r2 < d.w * d.w ? d.w - sqrt(r2) : 0.0; }\n"
        "centroid varying vec2 vUVc; varying vec4 vCol; varying float vFog; varying vec3 vWorld; varying float vSky; varying vec4 vWarm;\n"
        "vec3 chroma(float c) { float r = floor(c / 64.0); float g = floor((c - r * 64.0) / 8.0); float b = c - r * 64.0 - g * 8.0; return vec3(r, g, b) / 7.0; }\n"
        "void main() {\n"
        "  gl_Position = ftransform();\n"
        "  gl_TexCoord[0] = gl_MultiTexCoord0; vUVc = gl_MultiTexCoord0.xy;\n"
        "  vec4 e = gl_ModelViewMatrix * gl_Vertex;\n"
        "  vWorld = uEnt > 0.5 ? (uInvV * e).xyz : gl_Vertex.xyz;\n"
        "  gl_ClipVertex = e;\n"
        "  vFog = length(e.xyz);\n"
        "  if (uEnt > 0.5) { vSky = uEntSky; vCol = gl_Color; vWarm = vec4(0.0); return; }\n"
        "  float v = floor(gl_Color.a * 2097152.0 + 0.5);\n"
        "  float cc = floor(v / 4096.0); v -= cc * 4096.0; if (cc > 511.5) { cc = 509.0; v = 4096.0; }\n"
        "  float sk4 = floor(v / 64.0);\n"
        "  float sk = sk4 * 0.25;\n"
        "  float bl = (v - sk4 * 64.0) * 0.25;\n"
        "  if (uDyn0.w > 0.0) { float dl = dynL(uDyn0, vWorld); if (uDyn1.w > 0.0) dl = max(dl, dynL(uDyn1, vWorld)); if (uDyn2.w > 0.0) { dl = max(dl, dynL(uDyn2, vWorld)); dl = max(dl, dynL(uDyn3, vWorld)); } if (dl > bl) { bl = dl; cc = 509.0; } }\n"
        "  vSky = sk;\n"
        "  if (uMode > 0.5) sk = min(15.0, sk + 2.0);\n"
        "  float sv = 0.96 * ((sk / 15.0) / (4.0 - 3.0 * sk / 15.0)) * uSkyScale;\n"
        "  float bf = 0.0;\n"
        "  if (bl > 0.01) { float vb = min(bl / 15.0, 1.0); bf = 1.5 * vb / (4.0 - 3.0 * vb); }\n"
        "  vec3 bc = chroma(cc) * vec3(bf, bf * ((bf * 0.6 + 0.4) * 0.6 + 0.4), bf * (bf * bf * 0.6 + 0.4));\n"
        "  vec3 L = min(vec3(1.0), vec3(0.03 + sv * uSkyRB + bc.r, 0.03 + sv * uSkyRB + bc.g, 0.03 + sv + bc.b));\n"
        "  if (bl > 15.5) L = vec3(1.0);\n"
        "  vWarm = vec4(chroma(cc), bl > 0.01 ? clamp(bf / (bf + sv * 0.6 + 0.08), 0.0, 1.0) : 0.0);\n"   // patchLight.3: cor e peso da luz de bloco (tocha/lava) para o brilho por pixel
        "  vCol = vec4(gl_Color.rgb * L, uMode > 0.5 ? 0.62 : 1.0);\n"
        "}\n";
    std::string FSHs =   // com sombras em cascatas: gerado porque cada cascata precisa do seu proprio sampler (GLSL 1.20 nao deixa passar sampler de forma portavel)
        "#version 120\n"
        "uniform sampler2D uTex; uniform sampler2D uHs; uniform sampler2D uWd; uniform vec4 uHsP; uniform vec4 uEsA[12]; uniform vec4 uEsB[12]; uniform float uEsN;\n"
        "uniform vec3 uSunCol; uniform sampler2D uCloud; uniform float uCloudOn; uniform float uCloudOff; uniform float uFancy; uniform sampler2D uRfl; uniform float uRfOn; uniform float uRfY; uniform vec2 uScr; uniform float uTime; uniform float uShadowOn; uniform float uSunStr; uniform float uEntOn; uniform vec3 uSunDir; uniform vec3 uCamW; uniform float uEnt; uniform float uMode;\n"
        "centroid varying vec2 vUVc; varying vec4 vCol; varying float vFog; varying vec3 vWorld; varying float vSky; varying vec4 vWarm;\n"
        "uniform float uNether;\n"
        "vec3 netherGrade(vec3 c, vec3 p, float t) {\n"
        "  if (uNether > 1.5) return c * vec3(0.97, 0.92, 1.06);\n"
        "  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
        "  float em = smoothstep(0.38, 0.85, lum) * clamp((c.r - c.b) * 3.0, 0.0, 1.0);\n"   // brilho de lava/glowstone/fogo: pixels quentes e claros
        "  float fl = 0.5 + 0.5 * sin(t * 2.3 + p.x * 0.9 + p.z * 1.1 + p.y * 0.6);\n"
        "  c += vec3(1.0, 0.42, 0.08) * em * (0.30 + 0.30 * fl); c += vec3(0.10, 0.025, 0.0) * (0.5 + 0.5 * fl) * (1.0 - smoothstep(0.0, 0.3, lum));\n"
        "  { float eb = fract(sin(dot(floor(p.xz * 6.0 + p.y * 3.0) + floor(t * 1.5), vec2(12.9898, 78.233))) * 43758.5453); c += vec3(1.0, 0.35, 0.05) * step(0.993, eb) * clamp((c.r - c.b) * 2.0, 0.0, 1.0) * 0.55; }\n"
        "  c *= mix(vec3(1.35, 0.78, 0.62), vec3(1.10, 0.96, 0.88), smoothstep(0.04, 0.45, lum));\n"   // sombras avermelhadas, luzes alaranjadas
        "  return c;\n"
        "}\n"

        "float ign(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }\n"   // ruido por pixel: gira o filtro (penumbra macia em vez de serrilhado)
        "vec2 rotv(vec2 v, vec2 cs) { return vec2(v.x * cs.x - v.y * cs.y, v.x * cs.y + v.y * cs.x); }\n"   // patchShader.3: gira por (cos,sin) ja calculados (1 sincos por pixel em vez de 12 por cascata)
        "vec2 vogel(float i, float n, float rot) { float a = i * 2.39996323 + rot; return vec2(cos(a), sin(a)) * sqrt((i + 0.5) / n); }\n"
        "";
    FSHs +=
        "float cloudCov(vec3 wp) {\n"
        "  if (uCloudOn < 0.5 || uSunDir.y < 0.06) return 0.0;\n"
        "  float t = (130.0 - wp.y) / uSunDir.y; if (t < 0.0) return 0.0;\n"
        "  vec2 c = (wp.xz + uSunDir.xz * t + vec2(uCloudOff, 0.0)) * (1.0 / 3072.0);\n"
        "  float r = (0.9 + t * 0.0016) / 256.0;\n"
        "  float s = texture2D(uCloud, c).r * 0.36 + texture2D(uCloud, c + vec2(r, r * 0.4)).r * 0.16 + texture2D(uCloud, c + vec2(-r * 0.4, r)).r * 0.16 + texture2D(uCloud, c + vec2(-r, -r * 0.4)).r * 0.16 + texture2D(uCloud, c + vec2(r * 0.4, -r)).r * 0.16;\n"
        "  return smoothstep(0.12, 0.88, s) * smoothstep(0.06, 0.2, uSunDir.y);\n"
        "}\n"
        "// ---- tons do dia: cor do ambiente em sombra, do sol, do topo/base das faces, por elevacao do sol (e) ----\n"
        "vec3 todShade(float e) {\n"   // noite azul-profundo > crepusculo violeta > hora dourada lilas > manha/tarde > meio-dia azul-frio
        "  vec3 r = mix(vec3(0.60, 0.68, 1.00), vec3(0.66, 0.56, 0.84), smoothstep(-0.10, -0.01, e));\n"
        "  r = mix(r, vec3(0.62, 0.58, 0.82), smoothstep(-0.01, 0.12, e));\n"
        "  r = mix(r, vec3(0.54, 0.63, 0.86), smoothstep(0.12, 0.35, e));\n"
        "  return mix(r, vec3(0.49, 0.59, 0.85), smoothstep(0.35, 0.75, e));\n"
        "}\n"
        "vec3 todSkyTop(float e) { vec3 r = mix(vec3(1.07, 0.96, 1.03), vec3(0.95, 0.99, 1.07), smoothstep(0.08, 0.50, e)); return mix(vec3(0.93, 0.98, 1.10), r, smoothstep(-0.08, 0.02, e)); }\n"
        "vec3 todGround(float e) { return mix(vec3(1.09, 0.93, 0.80), vec3(1.04, 0.97, 0.89), smoothstep(0.05, 0.55, e)); }\n"
        "vec3 todSunTint(vec3 sd, vec3 sc, float nightL) {\n"
        "  float e = sd.y; float warm = 1.0 - smoothstep(0.12, 0.6, e);\n"
        "  vec3 low = mix(vec3(1.0), sc, 0.5) * vec3(1.2, 1.1, 1.0);\n"
        "  float eve = smoothstep(0.08, -0.08, sd.x);\n"   // tarde (sol do lado -x): laranja; manha: rosado e mais frio
        "  low *= mix(vec3(0.98, 0.97, 1.04), vec3(1.07, 0.96, 0.86), eve);\n"
        "  vec3 t = mix(vec3(1.04, 1.02, 0.97), low, warm); t *= 1.0 + 0.06 * smoothstep(0.55, 0.95, e);\n"
        "  return mix(t, vec3(0.88, 1.02, 1.38), nightL);\n"
        "}\n"
        "// sombra das entidades: capsulas verticais (A = x,y_base,z,raio; B.x = altura) contra o raio do pixel ate a luz L; penumbra cresce com a distancia\n"
        "float entSh(vec3 q, vec3 L) {\n"
        "  float occ = 0.0; float den = max(1.0 - L.y * L.y, 0.02);\n"
        "  for (int i = 0; i < 12; i++) {\n"
        "    if (float(i) >= uEsN) break;\n"
        "    vec4 a = uEsA[i]; float h = uEsB[i].x; vec3 w = q - a.xyz; float d = dot(L, w);\n"
        "    float tc = clamp((w.y - L.y * d) / den, 0.0, h); float sc = max(L.y * tc - d, 0.0);\n"
        "    tc = clamp(w.y + sc * L.y, 0.0, h); sc = max(L.y * tc - d, 0.0); tc = clamp(w.y + sc * L.y, 0.0, h);\n"
        "    float dist = length(q + L * sc - (a.xyz + vec3(0.0, tc, 0.0)));\n"
        "    float pen = 0.06 + 0.07 * sc;\n"
        "    occ = max(occ, 1.0 - smoothstep(a.w - pen, a.w + pen, dist));\n"
        "  }\n"
        "  return occ;\n"
        "}\n"
        "float wdDepth(vec2 xz) {\n"   // profundidade da agua (blocos), interpolada: o raso da margem vira gradiente suave
        "  vec2 f = xz - uHsP.xy - 0.5; vec2 i0 = floor(f); vec2 w = f - i0; float inv = uHsP.z; vec2 u0 = (i0 + 0.5) * inv;\n"
        "  vec4 a = vec4(texture2D(uWd, u0).g, texture2D(uWd, u0 + vec2(inv, 0.0)).g, texture2D(uWd, u0 + vec2(0.0, inv)).g, texture2D(uWd, u0 + vec2(inv, inv)).g) * 255.0;\n"
        "  return mix(mix(a.x, a.y, w.x), mix(a.z, a.w, w.x), w.y);\n"
        "}\n"
        "// mapa de alturas do sol (RGBA8, 1 texel = 1 coluna): R+G = altura do raio de sol sobre a coluna (R inteiro, G fracao), B = topo da propria coluna, A = distancia do bloqueador (blocos)\n"
        "float hsLit1(vec3 q) {\n"
        "  vec2 f = q.xz - uHsP.xy - 0.5; vec2 i0 = floor(f); vec2 w = f - i0; float inv = uHsP.z; vec2 u0 = (i0 + 0.5) * inv;\n"
        "  vec4 t00 = texture2D(uHs, u0); vec4 t10 = texture2D(uHs, u0 + vec2(inv, 0.0)); vec4 t01 = texture2D(uHs, u0 + vec2(0.0, inv)); vec4 t11 = texture2D(uHs, u0 + vec2(inv, inv));\n"
        "  vec4 s0 = vec4(floor(t00.r * 255.0 + 0.5) + t00.g, floor(t10.r * 255.0 + 0.5) + t10.g, floor(t01.r * 255.0 + 0.5) + t01.g, floor(t11.r * 255.0 + 0.5) + t11.g);\n"
        "  float S = mix(mix(s0.x, s0.y, w.x), mix(s0.z, s0.w, w.x), w.y);\n"
        "  float D = mix(mix(t00.a, t10.a, w.x), mix(t01.a, t11.a, w.x), w.y) * 255.0;\n"
        "  vec4 tn = mix(mix(t00, t10, step(0.5, w.x)), mix(t01, t11, step(0.5, w.x)), step(0.5, w.y));\n"
        "  float own = floor(tn.b * 255.0 + 0.5);\n"
        "  float pen = 0.30 + 0.085 * D;\n"   // penumbra: nitida junto do objeto, macia longe dele
        "  return smoothstep(0.0, 1.0, clamp((q.y - S) / pen * 0.5 + 0.5, 0.0, 1.0)) * step(own, q.y + 0.15);\n"
        "}\n"
        "float hsSolid(vec3 q) {\n"   // patchSombra.8: iluminacao considerando so solidos (canal B de uWd)
        "  vec2 f = q.xz - uHsP.xy - 0.5; vec2 i0 = floor(f); vec2 w = f - i0; float inv = uHsP.z; vec2 u0 = (i0 + 0.5) * inv;\n"
        "  vec4 s4 = vec4(texture2D(uWd, u0).b, texture2D(uWd, u0 + vec2(inv, 0.0)).b, texture2D(uWd, u0 + vec2(0.0, inv)).b, texture2D(uWd, u0 + vec2(inv, inv)).b) * 255.0;\n"
        "  float S = mix(mix(s4.x, s4.y, w.x), mix(s4.z, s4.w, w.x), w.y);\n"
        "  return smoothstep(0.0, 1.0, clamp((q.y - S) / 0.6 * 0.5 + 0.5, 0.0, 1.0));\n"
        "}\n"
        "float hsLit(vec3 q) {\n"   // patchSombra.5: PCF perto (3 amostras ao longo de Z, deslocadas por ruido por pixel): borda da sombra sem degraus de bloco; longe 1 amostra
        "  if (vFog > 32.0) { float a0 = hsLit1(q); return 1.0 - max(1.0 - hsSolid(q), (1.0 - a0) * 0.62); }\n"
        "  float j = ign(gl_FragCoord.xy) - 0.5; float dz = 0.5 + 0.35 * (1.0 - smoothstep(8.0, 32.0, vFog));\n"
        "  float a = hsLit1(q + vec3(0.0, 0.0, j * 0.5)) * 0.5 + hsLit1(q + vec3(0.0, 0.0, dz + j * 0.5)) * 0.25 + hsLit1(q - vec3(0.0, 0.0, dz - j * 0.5)) * 0.25;\n"
        "  return 1.0 - max(1.0 - hsSolid(q), (1.0 - a) * 0.62);\n"   // sombra de solido 100%, de folha 62%
        "}\n"
        "float hsAO(vec3 q) {\n"   // patchSombra.7: oclusao ambiente barata pelo mapa de alturas (4 leituras): vales, pe de arvore e quinas escurecem so o ambiente
        "  float inv = uHsP.z; vec2 uv = (q.xz - uHsP.xy) * inv; float o = 2.0 * inv;\n"
        "  vec4 hh = vec4(texture2D(uHs, uv + vec2(o, 0.0)).b, texture2D(uHs, uv - vec2(o, 0.0)).b, texture2D(uHs, uv + vec2(0.0, o)).b, texture2D(uHs, uv - vec2(0.0, o)).b) * 255.0;\n"
        "  float own = texture2D(uHs, uv).b * 255.0; vec4 oc = clamp((hh - (q.y + 0.5)) * 0.25, 0.0, 1.0);\n"
        "  return mix(1.0, 1.0 - 0.11 * (oc.x + oc.y + oc.z + oc.w), step(own - 0.6, q.y));\n"
        "}\n"
        "void main() {\n"
        "  vec4 c = texture2D(uTex, vUVc, clamp((vFog * 0.0625 - 8.0) * 0.25, 0.0, 3.0)) * vCol;\n"
        "  if (c.a < mix(0.3, 0.1, smoothstep(6.0, 56.0, vFog))) discard;\n"
        "  if (uMode > 0.5 && uFancy < 0.5) c.a = min(1.0, c.a / 0.62);\n"   // patchJava.1: agua do Java = so o alfa da textura (0,70); o 0,62 do vertice deixava ~0,43 (lavada, cinza)
        "  vec3 gnv = cross(dFdx(vWorld), dFdy(vWorld)); float gnl = dot(gnv, gnv); vec3 gnn = gnl > 1e-12 ? gnv * inversesqrt(gnl) : vec3(0.0, 1.0, 0.0);\n"   // patchShader.3: normal da face calculada 1x (antes 3x)   // o alpha test fixo nao vale com shader proprio (GLES): sem isso, partes transparentes ficam pretas
        "  \n"
        "  vec3 scol = uSunCol; float glowK = clamp(uSunDir.y * 4.0 + 0.55, 0.0, 1.0) * (1.0 - min(uNether, 1.0));\n"
        "  float cc0 = 0.0;\n"
        "  if (uShadowOn > 0.5 && vFog < 90.0 && vSky > 7.5) {\n"
        "    vec3 n = gnn;\n"   // normal da face pela derivada da posicao (aponta para a camera)
        "    float nightL = 1.0 - smoothstep(-0.10, 0.0, uSunDir.y); vec3 Ld = uSunDir.y >= 0.0 ? uSunDir : -uSunDir;\n"   // patchLight.1: de noite a luz direta e a LUA (direcao oposta ao sol)
        "    float ndl = dot(n, Ld);\n"
        "    float direct = 0.0;\n"
        "    if (ndl > 0.02) {\n"
        "      float slope = min(sqrt(max(0.0, 1.0 - ndl * ndl)) / max(ndl, 0.15), 3.0);\n"
        "      float lit = hsLit(vWorld + n * (0.5 + 0.12 * slope));\n"
        "      if (uEsN > 0.5) lit *= 1.0 - 0.82 * entSh(vWorld + n * 0.45, Ld);\n"
        "      direct = lit * smoothstep(0.02, 0.32, ndl);\n"   // terminador suave: faces rasantes ao sol escurecem aos poucos
        "    }\n"
        "    float k = uSunStr * clamp((vSky - 8.0) / 6.0, 0.0, 1.0) * (1.0 - smoothstep(48.0, 90.0, vFog));\n"   // so onde ha luz do ceu; some suave com a distancia
        "    float warm = 1.0 - smoothstep(0.12, 0.6, uSunDir.y);\n"   // sol baixo = luz dourada
        "    vec3 shTint = todShade(uSunDir.y);\n"   // sombra azulada (luz do ceu)
        "    vec3 sunTint = todSunTint(uSunDir, scol, nightL);\n"   // luar prateado-azulado
        "    c.rgb *= mix(vec3(1.0), shTint * vec3(0.93, 0.94, 0.97), k * (1.0 - direct));\n"   // patchSombra.5: sombra um pouco mais funda (mais contraste de luz e sombra)
        "    c.rgb *= mix(vec3(1.0), vec3(1.07, 0.97, 0.86), warm * k * direct * smoothstep(0.02, 0.2, ndl) * (1.0 - smoothstep(0.2, 0.6, ndl)));\n"   // luz dourada na borda do terminador com o sol baixo
        "    cc0 = k > 0.001 ? cloudCov(vWorld) : 0.0; float cl = cc0 * smoothstep(0.02, 0.3, ndl);\n"   // patchSombra.7: nuvem so e lida onde ha luz do ceu (cc0 reaproveitado nas folhas)
        "    if (uMode < 0.5 && vFog < 40.0 && k > 0.01) c.rgb *= mix(1.0, hsAO(vWorld + n * 0.5), k * (0.55 + 0.45 * (1.0 - direct)));\n"
        "    c.rgb *= mix(vec3(1.0), sunTint, k * direct * mix(0.7, 2.6, nightL) * (1.0 - cl));\n"
        "    c.rgb *= mix(vec3(1.0), vec3(0.72, 0.79, 0.94), k * cl * (0.3 + 0.7 * direct) * 0.85);\n"
        "  }\n"
        "  vec3 vd = normalize(vWorld - uCamW);\n"   // direcao do olho ao pixel (mundo)
        "  float sunUp = clamp(uSunDir.y, -0.2, 1.0); float dayK = clamp(sunUp * 4.0 + 0.3, 0.0, 1.0);\n"
        "  float sd = max(dot(vd, uSunDir), 0.0); float s2 = sd * sd; float s4 = s2 * s2; float s8 = s4 * s4; float s32 = s8 * s8; s32 *= s32;\n"
        "  if (uMode < 0.5 && uFancy > 0.5 && uNether < 0.5 && uShadowOn > 0.5 && vFog < 90.0) {\n"   // patchSombra.6: folhas/grama a contraluz brilham verde-dourado (translucidez), so com sol alto o bastante e luz do ceu
        "    float fol = smoothstep(0.03, 0.14, c.g - max(c.r, c.b));\n"
        "    float kb = uSunStr * clamp((vSky - 8.0) / 6.0, 0.0, 1.0) * (1.0 - smoothstep(48.0, 90.0, vFog)) * smoothstep(0.0, 0.25, uSunDir.y);\n"
        "    c.rgb += vec3(0.12, 0.19, 0.03) * fol * kb * (s8 * 0.9 + s4 * 0.12) * (1.0 - cc0 * 0.6);\n"
        "  }\n"
        "  if (uMode > 0.5 && uFancy > 0.5) {\n"   // agua: ondas (normal perturbada, animada), fresnel de Schlick, reflexo do ceu e brilho suave do sol
        "    vec2 w = vWorld.xz; float t = uTime;\n"
        "    float lodK = 1.0 - smoothstep(20.0, 85.0, vFog);\n"
        "    vec2 g = vec2(0.0);\n"
        "    { vec2 d1 = vec2(0.80, 0.60); g += d1 * cos(dot(w, d1) * 1.30 + t * 1.15) * 0.060;\n"
        "      vec2 d2 = vec2(-0.55, 0.83); g += d2 * cos(dot(w, d2) * 2.10 - t * 0.95) * 0.045;\n"
        "      vec2 d3 = vec2(0.20, -0.98); g += d3 * cos(dot(w, d3) * 3.70 + t * 1.60) * 0.030;\n"
        "      vec2 d4 = vec2(-0.93, -0.37); g += d4 * cos(dot(w, d4) * 6.30 - t * 2.20) * 0.018 * lodK; }\n"   // 4 ondas direcionais (a de alta frequencia some com a distancia: sem cintilar)
        "    vec2 gf = vec2(sin(w.x * 11.0 + t * 3.1 + sin(w.y * 7.0 + t * 1.3)), cos(w.y * 12.0 - t * 2.7 + sin(w.x * 8.0 - t))) * 0.020 * lodK;\n"   // ondulacao fina: so para o brilho do sol
        "    g *= 1.0 - 0.55 * smoothstep(40.0, 120.0, vFog);\n"
        "    vec3 nw = normalize(vec3(-g.x, 1.0, -g.y));\n"
        "    vec3 gn = gnv; float sideF = dot(gn, gn) > 1e-12 ? 1.0 - smoothstep(0.35, 0.7, abs(normalize(gn).y)) : 0.0;\n"
        "    if (sideF > 0.0) { gn = dot(gn, vd) > 0.0 ? -gn : gn; nw = normalize(mix(nw, normalize(gn) + vec3(g.x, 0.0, g.y) * 0.6, sideF)); }\n"
        "    bool below = vd.y > 0.02 && sideF < 0.5;\n"   // superficie vista de baixo d'agua
        "    float ct = clamp(abs(dot(vd, nw)), 0.0, 1.0);\n"
        "    float fr = 0.03 + 0.97 * pow(1.0 - ct, 4.5);\n"   // Fresnel de Schlick (F0 da agua = 0,02)
        "    vec3 rf = reflect(vd, nw); rf.y = mix(abs(rf.y), max(rf.y, 0.04), sideF);\n"
        "    float sunV = smoothstep(-0.02, 0.12, uSunDir.y);\n"
        "    float dpt = wdDepth(vWorld.xz); float fm = 0.0;\n"
        "    vec3 zen = gl_Fog.color.rgb * vec3(0.55, 0.75, 1.15);\n"
        "    vec3 refl = mix(gl_Fog.color.rgb, zen, clamp(rf.y * 1.6, 0.0, 1.0));\n"
        "    refl += scol * pow(max(dot(rf, uSunDir), 0.0), 4.0) * 0.16 * clamp(uSunStr, 0.0, 1.0) * sunV;\n"   // brilho do sol no ceu refletido (alaranjado ao por do sol)
        "    if (uRfOn > 0.5 && sideF < 0.5 && !below) {\n"
        "      vec2 suv = gl_FragCoord.xy / uScr; vec2 off = g * (0.16 / (1.0 + vFog * 0.035)); vec2 ruv = clamp(suv + off, 0.004, 0.996);\n"   // distorcao proporcional a ondulacao e menor com a distancia
        "      float br = 0.0012 + 0.0028 * smoothstep(6.0, 70.0, vFog);\n"   // desfoque cresce com a distancia
        "      vec3 rc = texture2D(uRfl, ruv).rgb * 0.36 + (texture2D(uRfl, ruv + vec2(br, 0.0)).rgb + texture2D(uRfl, ruv - vec2(br, 0.0)).rgb + texture2D(uRfl, ruv + vec2(0.0, br)).rgb + texture2D(uRfl, ruv - vec2(0.0, br)).rgb) * 0.16;\n"
        "      vec2 eq = suv + off; float edge = smoothstep(0.0, 0.06, eq.y) * smoothstep(1.0, 0.94, eq.y) * smoothstep(0.0, 0.04, eq.x) * smoothstep(1.0, 0.96, eq.x);\n"
        "      float onPlane = 1.0 - smoothstep(0.15, 0.6, abs(vWorld.y - uRfY));\n"   // lago de outra altura: o plano do reflexo nao vale, fica so o ceu
        "      refl = mix(refl, rc, 0.95 * edge * onPlane); }\n"
        "    float sd2 = max(dot(rf, uSunDir), 0.0);\n"
        "    vec3 rfs = reflect(vd, normalize(vec3(-(g.x + gf.x), 1.0, -(g.y + gf.y)))); float sdf = max(dot(rfs, uSunDir), 0.0);\n"
        "    float sp = pow(sd2, 380.0) * 1.2 + pow(sd2, 36.0) * 0.16 + pow(sdf, 900.0) * 1.6 * lodK; sp = sp / (1.0 + sp);\n"   // disco suave + glitter das ondas finas
        "    vec3 shal = vec3(0.12, 0.52, 0.55) * (0.35 + 0.65 * dayK); vec3 deep = vec3(0.03, 0.15, 0.30) * (0.25 + 0.75 * dayK); vec3 wb = mix(mix(shal, deep, smoothstep(0.4, 7.0, dpt)), c.rgb * vec3(0.8, 1.1, 1.2), 0.25);\n"
        "    wb *= mix(vec3(1.0), scol * 1.05, 0.35 * sunV);\n"   // a agua pega o tom do sol (dourada ao amanhecer/entardecer)
        "    wb += vec3(0.02, 0.10, 0.09) * clamp(pow(max(dot(vd, uSunDir), 0.0), 3.0) * (0.5 + 3.0 * (g.x + g.y)), 0.0, 1.0) * smoothstep(0.0, 0.5, uSunDir.y);\n"   // brilho de translucidez nas cristas, olhando contra o sol
        "    if (below) {\n"   // janela de Snell: dentro de 48,6 graus da vertical se ve o ceu; fora, reflexao interna total (espelho escuro)
        "      float sinI = sqrt(max(0.0, 1.0 - ct * ct)); float tir = smoothstep(0.72, 0.78, sinI);\n"
        "      vec3 skyW = mix(gl_Fog.color.rgb, zen, 0.6) * 1.15; vec3 deepW = gl_Fog.color.rgb * vec3(0.55, 0.75, 0.9) * (0.6 + 0.4 * lodK);\n"
        "      c.rgb = mix(skyW + scol * pow(max(dot(vd, uSunDir), 0.0), 40.0) * 0.5 * sunV, deepW, tir) * (0.85 + 0.15 * dayK);\n"
        "      fr = tir; sp = 0.0; c.a = 0.5;\n"
        "    } else {\n"
        "      c.rgb = mix(wb, refl, clamp(fr * 1.3 + 0.08, 0.0, 1.0)) + scol * vec3(1.0, 0.92, 0.8) * sp * sunV * (0.35 + 0.65 * fr);\n"
        "      c.rgb *= 0.94 + 0.12 * (g.x + g.y) * 8.0;\n"
        "      fm = (1.0 - smoothstep(0.05, 0.9, dpt)) * smoothstep(0.30, 0.65, 0.5 + 0.5 * sin(w.x * 5.0 + t * 1.6 + sin(w.y * 4.0 - t)) * sin(w.y * 4.6 - t * 1.3 + w.x * 1.7)) * lodK;\n"   // espuma na margem (so onde a agua e rasa)
        "      c.rgb = mix(c.rgb, vec3(0.92, 0.97, 1.0) * (0.55 + 0.45 * dayK), fm * 0.75);\n"
        "    }\n"
        "    { float mN = smoothstep(0.02, -0.08, uSunDir.y) * clamp(uSunStr * 2.5, 0.0, 1.0); float md = max(dot(rf, -uSunDir), 0.0); c.rgb += vec3(0.62, 0.78, 1.0) * (pow(md, 220.0) * 0.9 + pow(md, 26.0) * 0.07) * mN * (0.3 + 0.7 * fr); }\n"   // leve variacao de brilho nas cristas das ondas
        "    c.a = clamp(c.a * mix(0.6, 1.1, smoothstep(0.3, 5.0, dpt)) + fr * 0.6 + sp * 0.3 + fm * 0.5, 0.0, 0.96);\n"
        "  }\n"
        "  if (uFancy > 0.5 && uMode < 0.5 && uNether < 0.5 && vFog < 400.0) {\n"   // patchShader.1: luz ambiente hemisferica - topo recebe tom frio do ceu, base/lados recebem rebote quente do chao (so cor; o brilho por face do jogo nao muda)
        "    vec3 hn = gnn; float up = hn.y * 0.5 + 0.5;\n"
        "    float skyK = clamp((vSky - 4.0) / 10.0, 0.0, 1.0) * dayK;\n"
        "    c.rgb *= 1.0 + 0.12 * (dot(hn, uSunDir) - 0.3) * skyK * clamp(uSunStr * 2.0, 0.0, 1.0);\n"
        "    c.rgb *= mix(vec3(1.0), mix(todGround(uSunDir.y), todSkyTop(uSunDir.y), up), skyK * 0.6 * (1.0 - smoothstep(90.0, 400.0, vFog)));\n"
        "  }\n"
        "  if (vWarm.a > 0.02) { vec3 cl = floor(vWorld); float fk = 0.5 + 0.5 * sin(uTime * 8.0 + cl.x * 1.7 + cl.z * 2.3 + cl.y * 0.9) * sin(uTime * 4.7 + cl.x * 0.6 + cl.z * 0.4);\n"
        "    float wk = vWarm.a * (uNether > 0.5 ? 1.35 : 1.0); c.rgb *= 1.0 - 0.07 * wk * fk; c.rgb += c.rgb * vWarm.rgb * wk * (0.30 + 0.10 * fk); }\n"   // patchLight.3: tremulo de chama por celula + brilho quente na cor da fonte
        "  float lum = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"   // vibrance (cores baixas ganham saturacao) + curva S leve: mais profundidade
        "  if (uFancy > 0.5) c.rgb = mix(vec3(lum), c.rgb, 0.93 + 0.2 * (1.0 - clamp(lum * 1.5, 0.0, 1.0)));\n"
        "  c.rgb = mix(c.rgb, c.rgb * c.rgb * (3.0 - 2.0 * c.rgb), 0.26 * step(0.5, uFancy));\n"
        "  if (uNether > 0.5) c.rgb = netherGrade(c.rgb, vWorld, uTime); else if (uFancy > 0.5) {\n"
        "  c.rgb *= mix(vec3(1.0), vec3(1.04, 1.0, 0.94), dayK * 0.6);\n"
        "  c.rgb = mix(c.rgb, c.rgb * vec3(0.80, 0.92, 1.25), (1.0 - dayK) * 0.55);\n"
        "  }\n"
        "  c.rgb += vWarm.rgb * vWarm.a * 0.05 * (1.0 - exp(-vFog * 0.08));\n"
        "  { float ovw = 1.0 - min(uNether, 1.0); float sl = clamp((vSky - 4.0) / 10.0, 0.0, 1.0) * dayK * ovw * step(0.5, uFancy);\n"
        "    float gh = (1.0 - smoothstep(0.0, 0.45, abs(uSunDir.y))) * step(-0.05, uSunDir.y); c.rgb *= mix(vec3(1.0), vec3(1.12, 0.93, 0.76), gh * 0.5 * sl);\n"
        "    float lg = c.g - max(c.r, c.b); c.rgb = mix(vec3(dot(c.rgb, vec3(0.299, 0.587, 0.114))), c.rgb, 1.0 + 0.22 * clamp(lg * 4.0, 0.0, 1.0) * sl);\n"
        "    float lm = dot(c.rgb, vec3(0.299, 0.587, 0.114)); c.rgb *= mix(mix(vec3(0.93, 0.97, 1.06), vec3(1.0), smoothstep(0.12, 0.5, lm)), vec3(1.0), 1.0 - sl);\n"
        "    vec3 tm = (c.rgb * 1.06 * (2.51 * c.rgb * 1.06 + 0.03)) / (c.rgb * 1.06 * (2.43 * c.rgb * 1.06 + 0.59) + 0.14); c.rgb = mix(c.rgb, tm, 0.28 * step(0.5, uFancy)); }\n"
        "  float sty = step(0.5, uFancy); float fd = vFog * (1.0 - 0.5 * smoothstep(70.0, 150.0, vWorld.y) * sty);\n"
        "  float f = clamp((gl_Fog.end - fd) / (gl_Fog.end - gl_Fog.start), 0.0, 1.0); f = mix(f, f * f * (3.0 - 2.0 * f), 0.8 * sty); f = pow(f, mix(1.0, 1.3, sty)); if (abs(gl_Fog.density - 1.0) > 0.01) f = clamp(exp(-gl_Fog.density * fd), 0.0, 1.0);\n"
        "  float hz = sty * (1.0 - exp(-fd * 1.6 / max(gl_Fog.end, 24.0))) * 0.34 * dayK * (1.0 - min(uNether, 1.0)) * (1.0 + 0.9 * clamp((74.0 - vWorld.y) / 40.0, 0.0, 1.0));\n"   // perspectiva atmosferica: o longe esfria e some no ceu
        "  c.rgb = mix(c.rgb, gl_Fog.color.rgb * mix(vec3(1.0), vec3(0.92, 0.97, 1.08), dayK), hz);   // perspectiva aerea: longe esfria para o azul (Rayleigh)\n"
        "  vec3 fogC = gl_Fog.color.rgb + scol * (s4 * sd * 0.5 + s32 * s4 * 0.55) * (1.0 - f) * glowK * sty * (1.0 - smoothstep(0.35, 0.8, uSunDir.y) * 0.6);\n"   // luz do sol espalhada na neblina, do lado do sol
        "  if (uNether > 0.5 && uNether < 1.5) { float nz = sin(vWorld.x * 0.09 + uTime * 0.12) * sin(vWorld.z * 0.11 - uTime * 0.09) * sin(vWorld.y * 0.08 + uTime * 0.15) * 0.5 + 0.5;\n"
        "    fogC = gl_Fog.color.rgb * (0.75 + 0.7 * nz) + vec3(0.10, 0.025, 0.0) * nz; c.rgb = mix(c.rgb, fogC, (1.0 - exp(-vFog * 0.012)) * 0.22); }\n"   // neblina de calor: densidade ondulante
        "  c.rgb = mix(fogC, c.rgb, f);\n"
        "  if (!(dot(c.rgb, c.rgb) < 1.0e6)) c.rgb = gl_Fog.color.rgb;\n"   // patchShader.2: NaN/inf (normal nula, divisao) virava tela branca
        "  c.rgb = clamp(c.rgb, 0.0, 1.0);\n"
        "  c.rgb += (ign(gl_FragCoord.xy + vec2(uTime * 7.0, 0.0)) - 0.5) * (1.0 / 255.0);\n"   // patchShader.1: dither de 1 nivel - some o degrade em faixas (neblina, ceu, sombras suaves)
        "  gl_FragColor = c;\n"
        "}\n";
    static const char* GVS = "#version 120\nvoid main() { gl_Position = gl_Vertex; gl_TexCoord[0] = gl_MultiTexCoord0; }\n";
    static const char* GFS =   // god rays: marcha em direcao ao sol e conta quanto do caminho e ceu aberto (profundidade = 1); terreno/folhas/nuvens bloqueiam
        "#version 120\n"
        "uniform sampler2D uDepth; uniform vec2 uSun; uniform vec3 uCol; uniform float uI; uniform float uAsp;\n"
        "float ign(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }\n"
        "void main() {\n"
        "  vec2 uv = gl_TexCoord[0].xy; vec2 d = uSun - uv;\n"
        "  float r = length(d * vec2(uAsp, 1.0));\n"
        "  if (r > 0.95) { gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0); return; }\n"
        "  vec2 stp = d * (0.9 / 22.0); vec2 p = uv + stp * (0.5 + 0.5 * ign(gl_FragCoord.xy));\n"   // ruido no inicio: troca faixas por granulado fino
        "  float acc = 0.0, w = 1.0, ws = 0.0;\n"
        "  for (int i = 0; i < 22; i++) {\n"
        "    p += stp;\n"
        "    float inside = step(0.0, p.x) * step(p.x, 1.0) * step(0.0, p.y) * step(p.y, 1.0);\n"
        "    float sky = max(step(0.99999, texture2D(uDepth, clamp(p, 0.0, 1.0)).r), 1.0 - inside);\n"
        "    acc += sky * w; ws += w; w *= 0.93;\n"
        "  }\n"
        "  acc /= ws;\n"
        "  float sk0 = step(0.99999, texture2D(uDepth, uv).r);\n"
        "  float fall = pow(clamp(1.0 - r * 1.05, 0.0, 1.0), 2.6);\n"   // mais forte perto do sol
        "  float k = acc * acc * acc * fall * uI * 1.5 * mix(0.15, 1.0, sk0 * smoothstep(0.0, 0.5, acc));\n"   // silhuetas (arvores/montanhas) ficam mais escuras que o ceu: contraste dos raios
        "  k = k / (1.0 + k); gl_FragColor = vec4(uCol * k, 1.0);\n"
        "}\n";
    static const char* FFS =   // lens flare + escurecimento ao olhar o sol (blend GL_ONE, GL_SRC_ALPHA: dst*alfa + cor); sol tapado por terreno/folhas = sem flare
        "#version 120\n"
        "uniform sampler2D uDepth; uniform vec2 uSun; uniform vec3 uCol; uniform float uI; uniform float uAsp; uniform float uOcc;\n"
        "float ring(float d, float r, float w) { float x = (d - r) / w; return exp(-x * x); }\n"
        "float disc(float d, float r) { return 1.0 - smoothstep(r * 0.6, r, d); }\n"
        "vec3 hue(float h) { return clamp(abs(fract(h + vec3(0.0, 0.6667, 0.3333)) * 6.0 - 3.0) - 1.0, 0.0, 1.0); }\n"
        "void main() {\n"
        "  vec2 uv = gl_TexCoord[0].xy; vec2 k = vec2(uAsp, 1.0);\n"
        "  vec2 p = (uv - 0.5) * k; vec2 s = (uSun - 0.5) * k;\n"
        "  float vis = 0.0;\n"
        "  for (int i = 0; i < 6; i++) {\n"
        "    float a = float(i) * 2.39996; float rr = 0.018 * sqrt((float(i) + 0.5) / 6.0);\n"
        "    vec2 q = uSun + vec2(cos(a), sin(a)) * rr / k;\n"
        "    float inside = step(0.0, q.x) * step(q.x, 1.0) * step(0.0, q.y) * step(q.y, 1.0);\n"
        "    vis += max(step(0.99999, texture2D(uDepth, clamp(q, 0.0, 1.0)).r), 1.0 - inside);\n"
        "  }\n"
        "  vis /= 12.0;\n"
        "  float I = uI * vis * (1.0 - uOcc);\n"
        "  float visN = clamp(vis * 2.0, 0.0, 1.0); float skyPx = step(0.99999, texture2D(uDepth, uv).r);\n"   // patchShader.3: sol atras de bloco = sem anel/halo; o anel nao desenha sobre blocos em primeiro plano
        "  vec2 dv = p - s; float ds = length(dv);\n"
        "  float core = exp(-ds * 40.0) * 1.3 + exp(-ds * 10.0) * 0.5 + exp(-ds * 2.8) * 0.20;\n"
        "  vec3 col = mix(uCol, vec3(1.0), 0.55) * core;\n"
        "  float fade = 1.0 - smoothstep(0.35, 1.3, length(s));\n"
        "  vec3 g = vec3(0.0); float d;\n"
        "  d = length(p - s * 0.65); g += vec3(1.0, 0.70, 0.40) * disc(d, 0.09) * 0.10;\n"
        "  d = length(p - s * 0.25); g += vec3(0.45, 0.95, 0.55) * disc(d, 0.17) * 0.08 + vec3(0.9, 0.7, 0.3) * ring(d, 0.17, 0.012) * 0.10;\n"
        "  d = length(p + s * 0.20); g += vec3(0.55, 0.65, 1.0) * disc(d, 0.06) * 0.16;\n"
        "  d = length(p + s * 0.70); g += hue(d * 2.2) * ring(d, 0.30, 0.03) * 0.22 + vec3(0.9, 0.55, 0.3) * disc(d, 0.30) * 0.035;\n"
        "  d = length(p + s * 1.30); g += vec3(0.95, 0.6, 0.35) * ring(d, 0.12, 0.010) * 0.22 + vec3(0.5, 0.8, 0.5) * disc(d, 0.12) * 0.05;\n"
        "  float dark = I * (0.26 + 0.34 * smoothstep(0.10, 0.9, length(p)));\n"
        "  float bl = uI * uOcc * (1.0 - 0.55 * uOcc);\n"
        "  vec3 haze = uCol * (exp(-ds * 7.0) * 0.30 + exp(-ds * 2.2) * 0.12 + exp(-ds * 26.0) * 0.20) * bl * visN;\n"
        "  float rim = exp(-((ds - 0.17) / 0.07) * ((ds - 0.17) / 0.07)) * 4.0 * uOcc * (1.0 - uOcc);\n"
        "  vec3 halo = mix(uCol, vec3(1.0), 0.6) * rim * 0.14 * uI * visN * mix(0.12, 1.0, skyPx);\n"
        "  gl_FragColor = vec4((col + g * fade) * I + haze + halo, 1.0 - dark);\n"
        "}\n";
    auto build = [&](GLenum type, const char* src) -> GLuint {
        GLuint s = createShader(type); shaderSource(s, 1, &src, nullptr); compileShader(s);
        GLint ok = 0; getShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[1024] = {0}; if (getShaderLog) getShaderLog(s, sizeof log - 1, nullptr, log); fprintf(stderr, "shader do ciclo dia/noite: %s\n", log); return 0; }
        return s;
    };
    auto tryProg = [&](const char* v, const char* f) -> GLuint {
        const GLuint a = build(GL_VERTEX_SHADER, v), b = build(GL_FRAGMENT_SHADER, f); if (!a || !b) return 0;
        const GLuint p = createProgram(); attachShader(p, a); attachShader(p, b); linkProgram(p);
        GLint ok = 0; getProgramiv(p, GL_LINK_STATUS, &ok); if (!ok) { fprintf(stderr, "shader do ciclo dia/noite: falha ao linkar\n"); return 0; } return p;
    };
    pUniform3f = (FnU3f)SDL_GL_GetProcAddress("glUniform3f"); pUniform2f = (FnU2f)SDL_GL_GetProcAddress("glUniform2f"); pUniformM4 = (FnUM4)SDL_GL_GetProcAddress("glUniformMatrix4fv");
#ifdef __ANDROID__
    // celular: shader de SOMBRAS por mapa de alturas (4 leituras de 1 textura 128x128 por pixel perto, ate 54 blocos). So e usado com as sombras LIGADAS;
    // desligadas, roda o shader enxuto (abaixo), igual ao de antes: registradores a mais na Mali derrubam o FPS mesmo com o codigo desligado por uniform.
    static const char* FSm =
        "#version 120\n"
        "uniform sampler2D uTex;\n"
        "uniform float uFancy; uniform float uMode; uniform float uTime; uniform float uShadowOn; uniform float uSunStr;\n"
        "uniform vec3 uSunDir; uniform vec3 uSunCol; uniform vec3 uCamW;\n"
        "centroid varying vec2 vUVc; varying vec4 vCol; varying float vFog; varying vec3 vWorld; varying float vSky; varying vec4 vWarm;\n"
        "uniform float uNether;\n"
        "vec3 netherGrade(vec3 c, vec3 p, float t) {\n"
        "  if (uNether > 1.5) return c * vec3(0.97, 0.92, 1.06);\n"
        "  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
        "  float em = smoothstep(0.38, 0.85, lum) * clamp((c.r - c.b) * 3.0, 0.0, 1.0);\n"   // brilho de lava/glowstone/fogo: pixels quentes e claros
        "  float fl = 0.5 + 0.5 * sin(t * 2.3 + p.x * 0.9 + p.z * 1.1 + p.y * 0.6);\n"
        "  c += vec3(1.0, 0.42, 0.08) * em * (0.30 + 0.30 * fl); c += vec3(0.10, 0.025, 0.0) * (0.5 + 0.5 * fl) * (1.0 - smoothstep(0.0, 0.3, lum));\n"
        "  { float eb = fract(sin(dot(floor(p.xz * 6.0 + p.y * 3.0) + floor(t * 1.5), vec2(12.9898, 78.233))) * 43758.5453); c += vec3(1.0, 0.35, 0.05) * step(0.993, eb) * clamp((c.r - c.b) * 2.0, 0.0, 1.0) * 0.55; }\n"
        "  c *= mix(vec3(1.35, 0.78, 0.62), vec3(1.10, 0.96, 0.88), smoothstep(0.04, 0.45, lum));\n"   // sombras avermelhadas, luzes alaranjadas
        "  return c;\n"
        "}\n"

        "float ign(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }\n"
        "void main() {\n"
        "  vec4 c = texture2D(uTex, vUVc, clamp((vFog * 0.0625 - 8.0) * 0.25, 0.0, 3.0)) * vCol;\n"
        "  if (c.a < mix(0.3, 0.1, smoothstep(6.0, 56.0, vFog))) discard;\n"
        "  if (uMode > 0.5 && uFancy < 0.5) c.a = min(1.0, c.a / 0.62);\n"   // patchJava.1: agua do Java = so o alfa da textura (0,70); o 0,62 do vertice deixava ~0,43 (lavada, cinza)
        "  float dayK = clamp(clamp(uSunDir.y, -0.2, 1.0) * 4.0 + 0.3, 0.0, 1.0); float glowK = clamp(uSunDir.y * 4.0 + 0.55, 0.0, 1.0) * (1.0 - min(uNether, 1.0));\n"
        "  vec3 vd = normalize(vWorld - uCamW);\n"
        "  vec3 n = vec3(0.0, 1.0, 0.0);\n"
        "  if (uShadowOn > 0.5) { n = cross(dFdx(vWorld), dFdy(vWorld)); n *= inversesqrt(max(dot(n, n), 1e-12)); }\n"   // derivadas fora de ramo por pixel
        "  if (uShadowOn > 0.5 && vSky > 7.5) {\n"
        "    float dist = length(vWorld.xz - uCamW.xz);\n"
        "    if (dist < 54.0) {\n"
        "      float nightL = 1.0 - smoothstep(-0.10, 0.0, uSunDir.y); float ndl = dot(n, uSunDir.y >= 0.0 ? uSunDir : -uSunDir); float direct = 0.0;\n"
        "      if (ndl > 0.02) direct = smoothstep(0.02, 0.32, ndl);\n"
        "      float k = uSunStr * clamp((vSky - 8.0) / 6.0, 0.0, 1.0) * (1.0 - smoothstep(36.0, 52.0, dist));\n"
        "      float warm = 1.0 - smoothstep(0.12, 0.6, uSunDir.y);\n"
        "      vec3 sunTint = mix(mix(vec3(1.03, 1.01, 0.96), mix(vec3(1.0), uSunCol, 0.5) * vec3(1.2, 1.1, 1.0), warm), vec3(0.88, 1.02, 1.38), nightL);\n"
        "      c.rgb *= mix(vec3(1.0), mix(vec3(0.50, 0.58, 0.80), vec3(0.64, 0.73, 1.0), nightL), k * (1.0 - direct));\n"
        "      c.rgb *= mix(vec3(1.0), sunTint, k * direct * mix(0.7, 2.6, nightL));\n"
        "    }\n"
        "  }\n"
        "  if (uMode > 0.5 && uFancy > 0.5) {\n"   // agua leve: ondas, fresnel, reflexo do ceu e brilho do sol (sem reflexo planar)
        "    vec2 w = vWorld.xz; float t = uTime;\n"
        "    vec2 g = vec2(cos(w.x * 1.7 + t * 1.1 + w.y * 0.9), cos(w.y * 2.1 - t * 0.9 + w.x * 0.6)) * 0.07;\n"
        "    g *= 1.0 - smoothstep(16.0, 48.0, vFog);\n"
        "    vec3 nw = normalize(vec3(-g.x, 1.0, -g.y));\n"
        "    vec3 gn = cross(dFdx(vWorld), dFdy(vWorld)); float sideF = dot(gn, gn) > 1e-12 ? 1.0 - smoothstep(0.35, 0.7, abs(normalize(gn).y)) : 0.0;\n"
        "    if (sideF > 0.0) { gn = dot(gn, vd) > 0.0 ? -gn : gn; nw = normalize(mix(nw, normalize(gn) + vec3(g.x, 0.0, g.y) * 0.6, sideF)); }\n"
        "    float q = 1.0 - clamp(dot(-vd, nw), 0.0, 1.0); q *= q; float fr = 0.05 + 0.95 * q * q;\n"
        "    vec3 rf = reflect(vd, nw); rf.y = mix(abs(rf.y), max(rf.y, 0.04), sideF);\n"
        "    vec3 refl = mix(gl_Fog.color.rgb, gl_Fog.color.rgb * vec3(0.55, 0.75, 1.15), clamp(rf.y * 1.6, 0.0, 1.0));\n"
        "    float sd2 = max(dot(rf, uSunDir), 0.0); float sp = pow(sd2, 300.0) * 1.4 + pow(sd2, 36.0) * 0.16; sp = sp / (1.0 + sp);\n"
        "    float sunV = smoothstep(-0.02, 0.12, uSunDir.y);\n"
        "    vec3 wb = mix(vec3(0.06, 0.27, 0.36) * (0.3 + 0.7 * dayK), c.rgb * vec3(0.8, 1.1, 1.2), 0.3);\n"
        "    c.rgb = mix(wb, refl, clamp(fr * 1.3 + 0.08, 0.0, 1.0)) + uSunCol * vec3(1.0, 0.92, 0.8) * sp * sunV * (0.35 + 0.65 * fr);\n"
        "    c.rgb *= 0.94 + 0.12 * (g.x + g.y) * 8.0;\n"
        "    c.a = clamp(c.a + fr * 0.6 + sp * 0.3, 0.0, 0.96);\n"
        "  }\n"
        "  if (vWarm.a > 0.02) { vec3 cl = floor(vWorld); float fk = 0.5 + 0.5 * sin(uTime * 8.0 + cl.x * 1.7 + cl.z * 2.3 + cl.y * 0.9) * sin(uTime * 4.7 + cl.x * 0.6 + cl.z * 0.4);\n"
        "    float wk = vWarm.a * (uNether > 0.5 ? 1.35 : 1.0); c.rgb *= 1.0 - 0.07 * wk * fk; c.rgb += c.rgb * vWarm.rgb * wk * (0.30 + 0.10 * fk); }\n"   // patchLight.3: tremulo de chama por celula + brilho quente na cor da fonte
        "  float lum = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
        "  if (uFancy > 0.5) c.rgb = mix(vec3(lum), c.rgb, 0.93 + 0.2 * (1.0 - clamp(lum * 1.5, 0.0, 1.0)));\n"
        "  c.rgb = mix(c.rgb, c.rgb * c.rgb * (3.0 - 2.0 * c.rgb), 0.26 * step(0.5, uFancy));\n"
        "  if (uNether > 0.5) c.rgb = netherGrade(c.rgb, vWorld, uTime); else if (uFancy > 0.5) {\n"
        "  c.rgb *= mix(vec3(1.0), vec3(1.04, 1.0, 0.94), dayK * 0.6);\n"
        "  c.rgb = mix(c.rgb, c.rgb * vec3(0.80, 0.92, 1.25), (1.0 - dayK) * 0.55);\n"
        "  }\n"
        "  c.rgb += vWarm.rgb * vWarm.a * 0.05 * (1.0 - exp(-vFog * 0.08));\n"
        "  { float ovw = 1.0 - min(uNether, 1.0); float sl = clamp((vSky - 4.0) / 10.0, 0.0, 1.0) * dayK * ovw * step(0.5, uFancy);\n"
        "    float gh = (1.0 - smoothstep(0.0, 0.45, abs(uSunDir.y))) * step(-0.05, uSunDir.y); c.rgb *= mix(vec3(1.0), vec3(1.12, 0.93, 0.76), gh * 0.5 * sl);\n"
        "    float lg = c.g - max(c.r, c.b); c.rgb = mix(vec3(dot(c.rgb, vec3(0.299, 0.587, 0.114))), c.rgb, 1.0 + 0.22 * clamp(lg * 4.0, 0.0, 1.0) * sl);\n"
        "    float lm = dot(c.rgb, vec3(0.299, 0.587, 0.114)); c.rgb *= mix(mix(vec3(0.93, 0.97, 1.06), vec3(1.0), smoothstep(0.12, 0.5, lm)), vec3(1.0), 1.0 - sl);\n"
        "    vec3 tm = (c.rgb * 1.06 * (2.51 * c.rgb * 1.06 + 0.03)) / (c.rgb * 1.06 * (2.43 * c.rgb * 1.06 + 0.59) + 0.14); c.rgb = mix(c.rgb, tm, 0.28 * step(0.5, uFancy)); }\n"
        "  float sty = step(0.5, uFancy); float fd = vFog * (1.0 - 0.5 * smoothstep(70.0, 150.0, vWorld.y) * sty);\n"
        "  float f = clamp((gl_Fog.end - fd) / (gl_Fog.end - gl_Fog.start), 0.0, 1.0); f = mix(f, f * f * (3.0 - 2.0 * f), 0.8 * sty); f = pow(f, mix(1.0, 1.3, sty)); if (abs(gl_Fog.density - 1.0) > 0.01) f = clamp(exp(-gl_Fog.density * fd), 0.0, 1.0);\n"
        "  float hz = sty * (1.0 - exp(-fd * 0.016)) * 0.34 * dayK * (1.0 - min(uNether, 1.0)) * (1.0 + 0.9 * clamp((74.0 - vWorld.y) / 40.0, 0.0, 1.0));\n"
        "  c.rgb = mix(c.rgb, gl_Fog.color.rgb * mix(vec3(1.0), vec3(0.92, 0.97, 1.08), dayK), hz);   // perspectiva aerea: longe esfria para o azul (Rayleigh)\n"
        "  float sd = max(dot(vd, uSunDir), 0.0); float s2 = sd * sd; float s4 = s2 * s2; float s8 = s4 * s4; float s32 = s8 * s8; s32 *= s32;\n"   // pow(sd,5) e pow(sd,36) por multiplicacao
        "  vec3 fogC = gl_Fog.color.rgb + uSunCol * (s4 * sd * 0.5 + s32 * s4 * 0.55) * (1.0 - f) * glowK * sty * (1.0 - smoothstep(0.35, 0.8, uSunDir.y) * 0.6);\n"
        "  if (uNether > 0.5 && uNether < 1.5) { float nz = sin(vWorld.x * 0.09 + uTime * 0.12) * sin(vWorld.z * 0.11 - uTime * 0.09) * sin(vWorld.y * 0.08 + uTime * 0.15) * 0.5 + 0.5;\n"
        "    fogC = gl_Fog.color.rgb * (0.75 + 0.7 * nz) + vec3(0.10, 0.025, 0.0) * nz; c.rgb = mix(c.rgb, fogC, (1.0 - exp(-vFog * 0.012)) * 0.22); }\n"   // neblina de calor: densidade ondulante
        "  c.rgb = mix(fogC, c.rgb, f);\n"
        "  gl_FragColor = c;\n"
        "}\n";
#endif
#ifdef __ANDROID__
    {   // celular: shader ENXUTO (sem sombras em cascata/PCF e sem ondas de agua). Mesmo desligado por uniform, esse codigo infla registradores na Mali e derruba o FPS (~2 FPS medido).
        auto cut = [&](const char* from, const char* to) { const size_t a = FSHs.find(from); if (a == std::string::npos) return; const size_t b = FSHs.find(to, a); if (b == std::string::npos) return; FSHs.erase(a, b - a); };
        cut("  if (uShadowOn > 0.5 && vFog", "  vec3 vd = normalize(vWorld - uCamW);");
        cut("  if (uMode > 0.5 && uFancy > 0.5) {", "  float lum = dot(c.rgb");
        cut("float ign(vec2 p)", "void main() {");
    }
#endif
    std::string FSc = FSHs;
#ifdef __ANDROID__
    gShCompat = true;
#endif

#ifdef BL_RPI
    // patchRpi.1: shader ENXUTO para o VC4 (Raspberry Pi 3B). O de sombras (sampler2DShadow em cascatas) compilava no Mesa mas devolvia sombra total (tela preta) e era pesado demais; este
    // so faz luz de ceu/bloco + neblina: sem luzes dinamicas, sem pow(), sem bias de mipmap, 1 textura. 361 mil vertices/quadro => cada instrucao do vertex shader conta.
    static const char* VSPi =
        "#version 120\n"
        "uniform float uMode; uniform float uSkyScale; uniform float uSkyRB;\n"
        "centroid varying vec2 vUVc; varying vec4 vCol; varying float vFog;\n"
        "void main() {\n"
        "  gl_Position = ftransform();\n"
        "  gl_TexCoord[0] = gl_MultiTexCoord0; vUVc = gl_MultiTexCoord0.xy;\n"
        "  vFog = length((gl_ModelViewMatrix * gl_Vertex).xyz);\n"
        "  float v = floor(gl_Color.a * 2097152.0 + 0.5);\n"
        "  float cc = floor(v / 4096.0); v -= cc * 4096.0; if (cc > 511.5) { cc = 509.0; v = 4096.0; }\n"
        "  float sk4 = floor(v / 64.0);\n"
        "  float sk = sk4 * 0.25;\n"
        "  float bl = (v - sk4 * 64.0) * 0.25;\n"
        "  if (uMode > 0.5) sk = min(15.0, sk + 2.0);\n"
        "  float sv = 0.96 * ((sk / 15.0) / (4.0 - 3.0 * sk / 15.0)) * uSkyScale;\n"
        "  float vb = min(bl * 0.0666667, 1.0); float bf = 1.5 * vb / (4.0 - 3.0 * vb);\n"
        "  float r = floor(cc / 64.0); float g = floor((cc - r * 64.0) / 8.0); float b = cc - r * 64.0 - g * 8.0;\n"
        "  vec3 L = min(vec3(1.0), vec3(0.03 + sv * uSkyRB + r * 0.142857 * bf, 0.03 + sv * uSkyRB + g * 0.142857 * bf, 0.03 + sv + b * 0.142857 * bf));\n"
        "  if (bl > 15.5) L = vec3(1.0);\n"
        "  vCol = vec4(gl_Color.rgb * L, uMode > 0.5 ? 0.62 : 1.0);\n"
        "}\n";
    static const char* FSPi =
        "#version 120\n"
        "uniform sampler2D uTex;\n"
        "centroid varying vec2 vUVc; varying vec4 vCol; varying float vFog;\n"
        "void main() {\n"
        "  vec4 c = texture2D(uTex, vUVc) * vCol;\n"
        "  if (c.a < 0.3) discard;\n"
        "  c.rgb = mix(gl_Fog.color.rgb, c.rgb, clamp((gl_Fog.end - vFog) * gl_Fog.scale, 0.0, 1.0));\n"
        "  gl_FragColor = c;\n"
        "}\n";
    static const char* FSPiO =   // patchRpiPerf.3: igual ao FSPi, mas SEM discard (terreno opaco: o VC4 mantem o early-Z)
        "#version 120\n"
        "uniform sampler2D uTex;\n"
        "centroid varying vec2 vUVc; varying vec4 vCol; varying float vFog;\n"
        "void main() {\n"
        "  vec4 c = texture2D(uTex, vUVc) * vCol;\n"
        "  c.rgb = mix(gl_Fog.color.rgb, c.rgb, clamp((gl_Fog.end - vFog) * gl_Fog.scale, 0.0, 1.0));\n"
        "  gl_FragColor = vec4(c.rgb, 1.0);\n"
        "}\n";
    skyProg = tryProg(VSPi, FSPi); gShadowShader = false; gSkyProgOpq = skyProg ? tryProg(VSPi, FSPiO) : 0;
    logInfo("Raspberry: shader enxuto (sem sombras) %s", skyProg ? "ok" : "FALHOU (usa escurecimento simples)");
    if (!skyProg) return;
#elif defined(BL_XP)
    // patchXP.1: Windows XP + VIA Chrome9 (GLSL 1.10). Mesmo shader enxuto do Raspberry (luz de ceu/bloco + neblina, 1 textura, sem sombras), mas SEM "centroid" e SEM #version 120.
    // Se o driver recusar (ou xpshader=0 em options.cfg), skyProg fica 0 e o jogo usa o escurecimento simples de funcoes fixas. Sem programa "opaco" separado (gSkyProgOpq = 0).
    static const char* VSXp =
        "#version 110\n"
        "uniform float uMode; uniform float uSkyScale; uniform float uSkyRB;\n"
        "varying vec2 vUVc; varying vec4 vCol; varying float vFog;\n"
        "void main() {\n"
        "  gl_Position = ftransform();\n"
        "  gl_TexCoord[0] = gl_MultiTexCoord0; vUVc = gl_MultiTexCoord0.xy;\n"
        "  vFog = length((gl_ModelViewMatrix * gl_Vertex).xyz);\n"
        "  float v = floor(gl_Color.a * 2097152.0 + 0.5);\n"
        "  float cc = floor(v / 4096.0); v -= cc * 4096.0; if (cc > 511.5) { cc = 509.0; v = 4096.0; }\n"
        "  float sk4 = floor(v / 64.0);\n"
        "  float sk = sk4 * 0.25;\n"
        "  float bl = (v - sk4 * 64.0) * 0.25;\n"
        "  if (uMode > 0.5) sk = min(15.0, sk + 2.0);\n"
        "  float sv = 0.96 * ((sk / 15.0) / (4.0 - 3.0 * sk / 15.0)) * uSkyScale;\n"
        "  float vb = min(bl * 0.0666667, 1.0); float bf = 1.5 * vb / (4.0 - 3.0 * vb);\n"
        "  float r = floor(cc / 64.0); float g = floor((cc - r * 64.0) / 8.0); float b = cc - r * 64.0 - g * 8.0;\n"
        "  vec3 L = min(vec3(1.0), vec3(0.03 + sv * uSkyRB + r * 0.142857 * bf, 0.03 + sv * uSkyRB + g * 0.142857 * bf, 0.03 + sv + b * 0.142857 * bf));\n"
        "  if (bl > 15.5) L = vec3(1.0);\n"
        "  vCol = vec4(gl_Color.rgb * L, uMode > 0.5 ? 0.62 : 1.0);\n"
        "}\n";
    static const char* FSXp =
        "#version 110\n"
        "uniform sampler2D uTex;\n"
        "varying vec2 vUVc; varying vec4 vCol; varying float vFog;\n"
        "void main() {\n"
        "  vec4 c = texture2D(uTex, vUVc) * vCol;\n"
        "  if (c.a < 0.3) discard;\n"
        "  c.rgb = mix(gl_Fog.color.rgb, c.rgb, clamp((gl_Fog.end - vFog) * gl_Fog.scale, 0.0, 1.0));\n"
        "  gl_FragColor = c;\n"
        "}\n";
    { FILE* xf = fopen("options.cfg", "rb"); if (xf) { char xl[384]; int xv; while (fgets(xl, sizeof xl, xf)) if (sscanf(xl, "xpshader=%d", &xv) == 1) gXpShader = xv != 0; fclose(xf); } }
    gShadowShader = false; skyProg = gXpShader ? tryProg(VSXp, FSXp) : 0;
    logInfo("Windows XP: shader enxuto (sem sombras) %s", !gXpShader ? "DESLIGADO (xpshader=0): escurecimento simples" : skyProg ? "ok" : "FALHOU (usa escurecimento simples)");
    if (!skyProg) return;
#else
    if (pUniform3f && pUniformM4 && pUniform2f) {
#ifdef __ANDROID__
        gSkyProgLean = tryProg(VSH, FSc.c_str()); gSkyProgSh = tryProg(VSH, FSm); skyProg = gSkyProgLean ? gSkyProgLean : gSkyProgSh;   // padrao: enxuto; o de sombras so com sombras ligadas
        logInfo("Shaders de celular: enxuto %s, sombras %s", gSkyProgLean ? "ok" : "FALHOU", gSkyProgSh ? "ok" : "FALHOU");
#else
        skyProg = tryProg(VSH, (gShCompat ? FSc : FSHs).c_str());
#endif
        if (!skyProg && !gShCompat) { gShCompat = true; logWarn("Sombras: shader com sampler2DShadow falhou; usando modo de compatibilidade"); skyProg = tryProg(VSH, FSc.c_str()); }
        if (skyProg && gShCompat) logInfo("Sombras: modo de compatibilidade (profundidade comparada no shader)");
    }   // versao com sombras; se falhar, cai na versao simples (o dia/noite nunca depende das sombras)
#ifdef __ANDROID__
    gShadowShader = gSkyProgSh != 0;
#else
    gShadowShader = skyProg != 0;
#endif
    if (!skyProg) skyProg = tryProg(VS, FS);
#endif   // BL_RPI
    if (!skyProg) return;
    gSkyBind = [=](GLuint p) {   // resolve os uniformes do programa p (chamado de novo quando troca enxuto <-> sombras)
        skyProg = p;
    uNethL = getUniformLoc(skyProg, "uNether"); uMode = getUniformLoc(skyProg, "uMode"); uFancy = getUniformLoc(skyProg, "uFancy"); uSkyScale = getUniformLoc(skyProg, "uSkyScale"); uSkyRB = getUniformLoc(skyProg, "uSkyRB"); pUniform4f = (FnU4f)SDL_GL_GetProcAddress("glUniform4f"); for (int i = 0; i < 4; i++) uDynL[i] = getUniformLoc(skyProg, (std::string("uDyn") + (char)('0' + i)).c_str());
    pUseProgram(skyProg); uniform1i(getUniformLoc(skyProg, "uTex"), 0);
    if (gShadowShader) {
        uShOn = getUniformLoc(skyProg, "uShadowOn"); uShSun = getUniformLoc(skyProg, "uSunDir"); uShStr = getUniformLoc(skyProg, "uSunStr");
        uEnt = getUniformLoc(skyProg, "uEnt"); uInvV = getUniformLoc(skyProg, "uInvV"); uEntSky = getUniformLoc(skyProg, "uEntSky"); uCamW = getUniformLoc(skyProg, "uCamW"); uTimeL = getUniformLoc(skyProg, "uTime"); uRfOn = getUniformLoc(skyProg, "uRfOn"); uRfYL = getUniformLoc(skyProg, "uRfY"); uEsNL = getUniformLoc(skyProg, "uEsN"); for (int k = 0; k < 12; k++) { char nb[24]; snprintf(nb, sizeof nb, "uEsA[%d]", k); uEsAL[k] = getUniformLoc(skyProg, nb); snprintf(nb, sizeof nb, "uEsB[%d]", k); uEsBL[k] = getUniformLoc(skyProg, nb); } uScrL = getUniformLoc(skyProg, "uScr"); uniform1i(getUniformLoc(skyProg, "uRfl"), 6); uniform1i(getUniformLoc(skyProg, "uCloud"), 7); uCloudOnL = getUniformLoc(skyProg, "uCloudOn"); uCloudOffL = getUniformLoc(skyProg, "uCloudOff");
        uHsPL = getUniformLoc(skyProg, "uHsP"); uShQL = getUniformLoc(skyProg, "uShQ"); uSunColL = getUniformLoc(skyProg, "uSunCol");
        uniform1i(getUniformLoc(skyProg, "uHs"), 1); uniform1i(getUniformLoc(skyProg, "uWd"), 2);   // patchShadow.3: mapa de alturas na unidade 1
        char nm[16];
    }
    };
    gSkyBind(skyProg);
    if (pUniform2f && pUniform3f
#ifdef BL_XP
        && false   // patchXP.1: sem godrays/flare no XP (nao compila os shaders de tela cheia)
#endif
        ) {   // god rays (opcional: se nao compilar, so nao aparecem)
        gGodProg = tryProg(GVS, GFS);
        if (gGodProg) { pUseProgram(gGodProg); uniform1i(getUniformLoc(gGodProg, "uDepth"), 0); uGdSun = getUniformLoc(gGodProg, "uSun"); uGdCol = getUniformLoc(gGodProg, "uCol"); uGdI = getUniformLoc(gGodProg, "uI"); uGdAsp = getUniformLoc(gGodProg, "uAsp"); }
        gFlProg = tryProg(GVS, FFS);   // lens flare (opcional, mesmo vertex shader de tela cheia)
        if (gFlProg) { pUseProgram(gFlProg); uniform1i(getUniformLoc(gFlProg, "uDepth"), 0); uFlSun = getUniformLoc(gFlProg, "uSun"); uFlCol = getUniformLoc(gFlProg, "uCol"); uFlI = getUniformLoc(gFlProg, "uI"); uFlAsp = getUniformLoc(gFlProg, "uAsp"); uFlOcc = getUniformLoc(gFlProg, "uOcc"); }
    }
    pUseProgram(0);
    gShaderOK = true;
}
// liga o shader (mode 0 = sólidos, 1 = água); sc/rb = brilho do céu (use 1,1 para "meio-dia", ex.: panorama do título)
static void skyShaderOn(float mode, float sc, float rb) {
    if (gFxNV > 0.f) { sc = std::max(sc, 1.f) + gFxNV * 24.f; rb = rb + (1.f - rb) * gFxNV; }   // patchPot.3: Visao Noturna: escala o brilho do ceu para o nivel de luz 0 tambem ficar claro (o shader satura em 1)
    if (!gShaderOK) return;
#if defined(BL_VULKAN) || defined(BL_GLES_NATIVE)
    if (BL_VKORGN) {
    vkS.sky[0] = 1.f; vkS.sky[1] = mode; vkS.sky[2] = sc; vkS.sky[3] = rb;
    { const float th = gSky.angle * 6.2831853f; vkS.env[0] = -sinf(th); vkS.env[1] = cosf(th); vkS.env[2] = (float)(gDim == 3 ? 0 : gDim); vkS.env[3] = (float)SDL_GetTicks() * 0.001f; vkS.cam[0] = gCamW[0]; vkS.cam[1] = gCamW[1]; vkS.cam[2] = gCamW[2]; }
    return; }
#endif
    gPiU[0] = mode; gPiU[1] = sc; gPiU[2] = rb; gPiOn = true;
    pUseProgram(skyProg); if (uNethL >= 0) pUniform1f(uNethL, (gDim == 1 && !gNetherFx) ? 2.f : (float)(gDim == 3 ? 0 : gDim));   /* patchGfx.1: Nether sem shader = visual neutro (2) */ pUniform1f(uMode, mode); pUniform1f(uFancy, gShaderPackOn ? 1.f : 0.f); pUniform1f(uSkyScale, sc); pUniform1f(uSkyRB, rb);
    if (pUniform4f) { static float last[4][4]; static GLuint lastProg = 0;   // so reenvia as luzes quando mudam (ou o programa e outro)
        const bool all = lastProg != skyProg; lastProg = skyProg;
        for (int i = 0; i < 4; i++) if (uDynL[i] >= 0 && (all || memcmp(last[i], gDyn[i], sizeof last[i]))) { pUniform4f(uDynL[i], gDyn[i][0], gDyn[i][1], gDyn[i][2], gDyn[i][3]); memcpy(last[i], gDyn[i], sizeof last[i]); } }
    if (gShadowShader) {
        const float th = gSky.angle * 6.2831853f;
        {   // cor do sol pela massa de ar (extincao atmosferica): calculada 1x aqui, nao por pixel
            const float el = std::max(0.f, std::min(1.f, cosf(th))); const float am = std::min(1.f / (el + 0.50572f * powf(6.08f + asinf(el) * 57.29578f, -1.6364f)), 18.f);
            float r = expf(-0.04f * am), g = expf(-0.095f * am), b = expf(-0.22f * am); const float m = std::max(r, std::max(g, b)); gSunCol[0] = r / m; gSunCol[1] = g / m; gSunCol[2] = b / m;
            if (uSunColL >= 0) pUniform3f(uSunColL, gSunCol[0], gSunCol[1], gSunCol[2]); if (uShQL >= 0) pUniform1f(uShQL, (float)gShQ);
        }
        pUniform3f(uShSun, -sinf(th), cosf(th), 0.f); pUniform3f(uCamW, gCamW[0], gCamW[1], gCamW[2]); pUniform1f(uEnt, 0.f); pUniform1f(uRfOn, gRfOn && gShaderPackOn ? 1.f : 0.f); if (uRfYL >= 0) pUniform1f(uRfYL, gRfWy); pUniform2f(uScrL, (float)gRfSW, (float)gRfSH); pUniform1f(uTimeL, (float)fmod(SDL_GetTicks() * 0.001, 6283.0));   // sol/camera: sempre (neblina e agua usam)
        const bool on = gShadowDraw && mode < .5f; pUniform1f(uShOn, on ? 1.f : 0.f);
        if (on) {
            pUniform3f(uShSun, gSunDir[0], gSunDir[1], gSunDir[2]); pUniform1f(uShStr, gSunStr); pUniform1f(uCloudOnL, gCloudTex ? 1.f : 0.f); pUniform1f(uCloudOffL, gCloudOff); if (uHsPL >= 0 && pUniform4f) pUniform4f(uHsPL, gHsOx, gHsOz, 1.f / (float)HS_N, 0.f); if (uEsNL >= 0) { pUniform1f(uEsNL, gEntShOn ? (float)gEntShN : 0.f); for (int k = 0; k < gEntShN && gEntShOn; k++) { if (uEsAL[k] >= 0) pUniform4f(uEsAL[k], gEntSh[k][0], gEntSh[k][1], gEntSh[k][2], gEntSh[k][3]); if (uEsBL[k] >= 0) pUniform4f(uEsBL[k], gEntSh[k][4], 0.f, 0.f, 0.f); } }
        }
    }
}
static void skyShaderOff() {
#if defined(BL_VULKAN) || defined(BL_GLES_NATIVE)
    if (BL_VKORGN) { vkS.sky[0] = 0.f; return; }
#endif
    gPiOn = false;
    if (gShaderOK) pUseProgram(0); }
#ifdef BL_RPI
// patchRpiPerf.3: alterna entre o programa SEM discard (terreno opaco) e o normal (recortados/agua). So vale entre skyShaderOn e skyShaderOff.
static void rpiSolidProg(bool opq) {
    if (!gPiOn || !gSkyProgOpq || !skyProg || !gShaderOK) return;
    if (opq) {
        pUseProgram(gSkyProgOpq);
        static PFNGLGETUNIFORMLOCATIONPROC getUniformLoc = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation"); static PFNGLUNIFORM1IPROC uniform1i = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
        if (uModeO == -2) { uModeO = getUniformLoc(gSkyProgOpq, "uMode"); uScaleO = getUniformLoc(gSkyProgOpq, "uSkyScale"); uRBO = getUniformLoc(gSkyProgOpq, "uSkyRB"); uniform1i(getUniformLoc(gSkyProgOpq, "uTex"), 0); }
        if (uModeO >= 0) pUniform1f(uModeO, gPiU[0]); if (uScaleO >= 0) pUniform1f(uScaleO, gPiU[1]); if (uRBO >= 0) pUniform1f(uRBO, gPiU[2]);
    } else pUseProgram(skyProg);
}
#else
static inline void rpiSolidProg(bool) {}
#endif
// entidades (jogador/mobs) com o shader do mundo: recebem sombra do terreno e das outras entidades. Chamar DEPOIS de entLightAt (usa gEntSk); desligar com skyShaderOff().
// gViewM = vista do mundo (rigida): inversa = R^T e -R^T*t. Retorna true se ligou.
static bool entShaderOn() {
    if (!gShaderOK || !gShadowShader || !gShadowAct || uEnt < 0) return false;
    gShadowDraw = true; skyShaderOn(0.f, gSkyScale, gSkyRB); gShadowDraw = false;
    const float* M = gViewM; float I[16] = {M[0], M[4], M[8], 0, M[1], M[5], M[9], 0, M[2], M[6], M[10], 0, 0, 0, 0, 1};
    I[12] = -(M[0] * M[12] + M[1] * M[13] + M[2] * M[14]); I[13] = -(M[4] * M[12] + M[5] * M[13] + M[6] * M[14]); I[14] = -(M[8] * M[12] + M[9] * M[13] + M[10] * M[14]);
    pUniformM4(uInvV, 1, GL_FALSE, I); pUniform1f(uEnt, 1.f); pUniform1f(uEntSky, gEntSk);
    return true;
}

// sem shader: escurece a cena inteira com um quadro azul-escuro translúcido (aproximação; cavernas também escurecem)
static void drawNightOverlay() {
    if (gShaderOK) return;
    const float a = (1.f - gSkyScale) * .72f; if (a < .01f) return;
    rdDisable(GL_TEXTURE_2D); rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    rdMatrixMode(GL_PROJECTION); rdPushMatrix(); rdLoadIdentity(); rdMatrixMode(GL_MODELVIEW); rdPushMatrix(); rdLoadIdentity();
    glColor4f(.02f, .03f, .10f, a);
    glBegin(GL_QUADS); glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, 1); glVertex2f(-1, 1); glEnd();
    rdPopMatrix(); rdMatrixMode(GL_PROJECTION); rdPopMatrix(); rdMatrixMode(GL_MODELVIEW);
    glColor4f(1.f, 1.f, 1.f, 1.f); rdDisable(GL_BLEND); rdEnable(GL_TEXTURE_2D);
}
