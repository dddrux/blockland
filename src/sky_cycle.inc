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

// ---------- estado do céu por quadro ----------
// base: cores de dia do jogo (zênite e horizonte); aqui aplicamos os fatores de hora do dia do Minecraft.
// lookX = componente X do vetor de olhar (a neblina esquenta na direção do sol no nascer/pôr, como o EntityRenderer.updateFogColor).
static void updateSky(float pt, float lookX, const float* baseTop, const float* baseHor, float* top, float* hor) {
    const float ang = celestialAngle(gWorldTime, pt), c = cosf(ang * 6.2831853f);
    gSky.angle = ang;
    gSky.skyF = clampf2(c * 2.f + .5f, 0.f, 1.f);                                   // World.getSkyColor: fator do zênite
    const float fog = gSky.skyF * .94f + .06f;                                       // World.getFogColor: nunca preto total
    const float sb = 1.f - clampf2(1.f - (c * 2.f + .2f), 0.f, 1.f);
    gSky.sunB = sb * .8f + .2f;                                                      // World.getSunBrightness (0.2 .. 1.0)
    gSkyScale = gSky.sunB * .95f + .05f;                                             // lightmap: brilho do céu
    gSkyRB = gSky.sunB * .65f + .35f;                                                // lightmap: tom azulado da noite
    const float sc = clampf2(1.f - (c * 2.f + .25f), 0.f, 1.f);                      // RenderGlobal.getStarBrightness
    gSky.starB = sc * sc * .5f;
    gSky.cloud = gSky.skyF * .9f + .1f;                                              // World.getCloudColor
    gSky.moonPhase = moonPhaseOf(gWorldTime);
    // faixa do nascer/pôr do sol (calcSunriseSunsetColors)
    gSky.hasSunrise = false;
    if (c >= -.4f && c <= .4f) {
        const float f3 = c / .4f * .5f + .5f;
        float f4 = 1.f - (1.f - sinf(f3 * 3.14159265f)) * .99f; f4 *= f4;
        gSky.sunrise[0] = f3 * .3f + .7f; gSky.sunrise[1] = f3 * f3 * .7f + .2f; gSky.sunrise[2] = .2f; gSky.sunrise[3] = f4;
        gSky.hasSunrise = true;
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
    const float hh = fmodf((float)(((gWorldTime % DAY_TICKS) + DAY_TICKS) % DAY_TICKS) / 1000.f + pt / 1000.f + 6.f, 24.f);
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
static void drawSkyFull(const float* top, const float* hor) {
    drawSkyDome(top, hor);
    if (!celReady) initCelestial();
    if (!starList) buildStars();
    glDisable(GL_FOG); glDisable(GL_DEPTH_TEST); glDisable(GL_ALPHA_TEST); glDepthMask(GL_FALSE); glEnable(GL_BLEND);
    if (gSky.hasSunrise) {   // faixa colorida no horizonte do lado do sol
        glDisable(GL_TEXTURE_2D); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPushMatrix();
        glRotatef(90.f, 1, 0, 0);
        glRotatef(sinf(gSky.angle * 6.2831853f) < 0.f ? 180.f : 0.f, 0, 0, 1);
        glRotatef(90.f, 0, 0, 1);
        const float* sr = gSky.sunrise;
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(sr[0], sr[1], sr[2], sr[3]); glVertex3f(0.f, 100.f, 0.f);
        glColor4f(sr[0], sr[1], sr[2], 0.f);
        for (int l = 0; l <= 16; l++) {
            const float a = l * 6.2831853f / 16.f, s = sinf(a), c = cosf(a);
            glVertex3f(s * 120.f, c * 120.f, -c * 40.f * sr[3]);
        }
        glEnd();
        glPopMatrix();
    }
    {   // cinturao de Venus: faixa rosada/violeta no horizonte oposto ao sol, logo apos o por / antes do nascer (sombra da Terra)
        const float sy2 = cosf(gSky.angle * 6.2831853f), sx2 = -sinf(gSky.angle * 6.2831853f);
        const float bt = clampf2((.06f - sy2) / .16f, 0.f, 1.f) * clampf2((sy2 + .28f) / .2f, 0.f, 1.f);
        if (bt > .01f) {
            glDisable(GL_TEXTURE_2D); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            const float asx = sx2 > 0.f ? -1.f : 1.f;
            static const float HY[4] = {-8.f, 10.f, 34.f, 76.f}, HC[4][4] = {{.95f, .50f, .50f, .60f}, {.92f, .52f, .64f, .52f}, {.50f, .40f, .74f, .28f}, {.30f, .28f, .60f, 0.f}};
            for (int k = 0; k < 3; k++) {
                glBegin(GL_QUAD_STRIP);
                for (int l = 0; l <= 40; l++) {
                    const float a = l * 6.2831853f / 40.f, sa = sinf(a), ca = cosf(a), w = powf(std::max(sa * asx, 0.f), 1.5f) * bt;
                    for (int q = 0; q < 2; q++) { const int r = k + q; glColor4f(HC[r][0], HC[r][1], HC[r][2], HC[r][3] * w); glVertex3f(sa * 120.f, HY[r], ca * 120.f); }
                }
                glEnd();
            }
        }
    }
    glEnable(GL_TEXTURE_2D); glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // sol e lua somam à cor do céu (preto = transparente)
    glPushMatrix();
    glRotatef(-90.f, 0, 1, 0);
    glRotatef(gSky.angle * 360.f, 1, 0, 0);
    glColor4f(1.f, 1.f, 1.f, 1.f);
    float sc3[3]; const float sunY = cosf(gSky.angle * 6.2831853f); sunTransmit(sunY, sc3);
    const float low = clampf2(1.f - sunY / .55f, 0.f, 1.f), hvis = clampf2((sunY + .16f) / .18f, 0.f, 1.f);
    if (hvis > .01f) {   // halo atmosferico (aproximacao de Mie): aneis aditivos com a cor real do sol; forte no nascer/por
        glDisable(GL_TEXTURE_2D);
        const float hb = (.10f + .62f * low * low) * hvis; static const float RR[5] = {0.f, 16.f, 48.f, 110.f, 240.f}, AA[5] = {1.f, .55f, .20f, .06f, 0.f};
        for (int k = 0; k < 4; k++) {
            glBegin(GL_QUAD_STRIP);
            for (int l = 0; l <= 32; l++) {
                const float a = l * 6.2831853f / 32.f, sa = sinf(a), ca = cosf(a);
                glColor4f(sc3[0], sc3[1], sc3[2], AA[k] * hb); glVertex3f(sa * RR[k], 100.f, ca * RR[k]);
                glColor4f(sc3[0], sc3[1], sc3[2], AA[k + 1] * hb); glVertex3f(sa * RR[k + 1], 100.f, ca * RR[k + 1]);
            }
            glEnd();
        }
        glEnable(GL_TEXTURE_2D);
    }
    glColor4f(1.f - .8f * (1.f - sc3[0]), 1.f - .8f * (1.f - sc3[1]), 1.f - .8f * (1.f - sc3[2]), 1.f);
    const float S = 30.f * (1.f + .22f * low);   // sol: 60x60 a 100 blocos (maior perto do horizonte)
    glBindTexture(GL_TEXTURE_2D, sunTex);
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f); glVertex3f(-S, 100.f, -S); glTexCoord2f(1.f, 0.f); glVertex3f(S, 100.f, -S);
    glTexCoord2f(1.f, 1.f); glVertex3f(S, 100.f, S);   glTexCoord2f(0.f, 1.f); glVertex3f(-S, 100.f, S);
    glEnd();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    const float M = 20.f;   // lua: 40x40 a 100 blocos, no lado oposto
    const int ph = gSky.moonPhase, i2 = ph % 4, k2 = ph / 4 % 2;
    const float u0 = i2 / 4.f, v0 = k2 / 2.f, u1 = (i2 + 1) / 4.f, v1 = (k2 + 1) / 2.f;
    if (gSky.starB > .001f) {   // halo suave azulado em volta da lua (aditivo)
        const float hb = gSky.starB; glDisable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLE_FAN); glColor4f(.55f * hb, .70f * hb, 1.f * hb, .34f * hb); glVertex3f(0.f, -100.f, 0.f);
        glColor4f(.35f, .5f, .9f, 0.f); for (int l = 0; l <= 28; l++) { const float a = l * 6.2831853f / 28.f; glVertex3f(sinf(a) * 80.f, -100.f, cosf(a) * 80.f); }
        glEnd(); glEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
    }
    glBindTexture(GL_TEXTURE_2D, moonTex);
    glBegin(GL_QUADS);
    glTexCoord2f(u1, v1); glVertex3f(-M, -100.f, M); glTexCoord2f(u0, v1); glVertex3f(M, -100.f, M);
    glTexCoord2f(u0, v0); glVertex3f(M, -100.f, -M); glTexCoord2f(u1, v0); glVertex3f(-M, -100.f, -M);
    glEnd();
    if (gSky.starB > .001f) {
        glDisable(GL_TEXTURE_2D);
        glColor4f(gSky.starB, gSky.starB, gSky.starB, gSky.starB);
        glCallList(starList);
    }
    glPopMatrix();
    glColor4f(1.f, 1.f, 1.f, 1.f); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D);
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
    gEntSk = (float)sk;
    const float v = .94f * powf(.8f, 15.f - (float)sk) * gSkyScale, sf = .06f;
    const float bf = bl > 0 ? ltab((float)bl) : 0.f;
    o[0] = std::max(sf + v * gSkyRB, bf); o[1] = std::max(sf + v * gSkyRB, bf * .93f); o[2] = std::max(sf + v, bf * .78f);
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
struct ShCas { int n = 0; float e = 0; GLuint fbo = 0, tex = 0; float ctr[3] = {0, 0, 0}; Uint32 last = 0; float ang = -9.f; float mat[16] = {0}; GLint uM = -1, uT = -1; };   // um mapa de sombras: n texels, meia-largura e (blocos)
static ShCas gCasT[3] = {{2048, 10.f}, {2048, 28.f}, {2048, 80.f}};   // terreno em 3 cascatas: perto = muito nitido (~102 texels/bloco), medio ~32, longe ~10
static ShCas gCasE[2] = {{1024, 5.f}, {1024, 24.f}};                 // jogador/mobs em 2 cascatas
static GLint uTimeL = -1, uShOn = -1, uShSun = -1, uShStr = -1, uShEOn = -1; static bool gShadowEnt = false;
static GLuint gGodProg = 0; static GLint uGdSun = -1, uGdCol = -1, uGdI = -1, uGdAsp = -1;   static GLuint gFlProg = 0; static GLint uFlSun = -1, uFlCol = -1, uFlI = -1, uFlAsp = -1;   // god rays (godrays.inc)
static float gSunDir[3] = {0.f, 1.f, 0.f}, gSunStr = 0.f, gViewM[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}, gCamW[3] = {0, 0, 0}, gEntSkUnused = 0.f; static bool gShadowAct = false; static GLint uEnt = -1, uInvV = -1, uEntSky = -1, uCamW = -1;   // gViewM = vista do mundo (para entidades acharem a posicao no mundo)
static bool gShaderPackOn = false, gRfPass = false;
static GLuint gRfTex = 0, gRfDep = 0, gRfFbo = 0; static int gRfW = 0, gRfH = 0, gRfSW = 1, gRfSH = 1; static bool gRfOn = false; static GLint uRfOn = -1, uScrL = -1;   // reflexao planar da agua (estilo HL2)   // pacote de shader escolhido em Opcoes > Shaders (!= "Nao"): liga agua realista, god rays e lens flare
static GLuint gCloudTex = 0; static float gCloudOff = 0.f, gSunOcc = 0.f; static GLint uCloudOnL = -1, uCloudOffL = -1, uFlOcc = -1;   // sombras das nuvens (mascara na unidade 7); gSunOcc = fracao do sol tapada por nuvem (0..1)
static GLuint skyProg = 0; static GLint uFancy = -1, uMode = -1, uSkyScale = -1, uSkyRB = -1;
static PFNGLUSEPROGRAMPROC pUseProgram = nullptr; static PFNGLUNIFORM1FPROC pUniform1f = nullptr;
static void initSkyShader() {
#ifdef BL_VULKAN
    gShaderOK = true; logInfo("Luz do ciclo dia/noite: shader Vulkan"); return;   // o shader do mundo ja faz parte do pipeline Vulkan (src/vk/blockland.vert)
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
        "uniform float uMode; uniform float uSkyScale; uniform float uSkyRB;\n"
        "varying vec4 vCol; varying float vFog;\n"
        "vec3 chroma(float c) { float r = floor(c / 64.0); float g = floor((c - r * 64.0) / 8.0); float b = c - r * 64.0 - g * 8.0; return vec3(r, g, b) / 7.0; }\n"
        "void main() {\n"
        "  gl_Position = ftransform();\n"
        "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "  vec4 e = gl_ModelViewMatrix * gl_Vertex;\n"
        "  vFog = abs(e.z);\n"
        "  float v = floor(gl_Color.a * 2097152.0 + 0.5);\n"
        "  float cc = floor(v / 4096.0); v -= cc * 4096.0; if (cc > 511.5) { cc = 509.0; v = 4096.0; }\n"
        "  float sk4 = floor(v / 64.0);\n"
        "  float sk = sk4 * 0.25;\n"
        "  float bl = (v - sk4 * 64.0) * 0.25;\n"
        "  if (uMode > 0.5) sk = min(15.0, sk + 2.0);\n"
        "  float sv = 0.94 * pow(0.8, 15.0 - sk) * uSkyScale;\n"
        "  float bf = 0.0;\n"
        "  if (bl > 0.01) bf = 0.06 + 0.94 * pow(0.8, 15.0 - bl);\n"
        "  vec3 bc = bf * chroma(cc);\n"
        "  vec3 L = vec3(max(0.06 + sv * uSkyRB, bc.r), max(0.06 + sv * uSkyRB, bc.g), max(0.06 + sv, bc.b));\n"
        "  if (bl > 15.5) L = vec3(1.0);\n"
        "  vCol = vec4(gl_Color.rgb * L, uMode > 0.5 ? 0.62 : 1.0);\n"
        "}\n";
    static const char* FS =
        "#version 120\n"
        "uniform sampler2D uTex;\n"
        "varying vec4 vCol; varying float vFog;\n"
        "void main() {\n"
        "  vec4 c = texture2D(uTex, gl_TexCoord[0].xy) * vCol;\n"
        "  if (c.a < 0.1) discard;\n"   // o alpha test fixo nao vale com shader proprio (GLES): sem isso, partes transparentes ficam pretas
        "  float f = clamp((gl_Fog.end - vFog) / (gl_Fog.end - gl_Fog.start), 0.0, 1.0);\n"
        "  c.rgb = mix(gl_Fog.color.rgb, c.rgb, f);\n"
        "  gl_FragColor = c;\n"
        "}\n";
    static const char* VSH =
        "#version 120\n"
        "uniform float uMode; uniform float uSkyScale; uniform float uSkyRB; uniform float uEnt; uniform mat4 uInvV; uniform float uEntSky;\n"
        "varying vec4 vCol; varying float vFog; varying vec3 vWorld; varying float vSky;\n"
        "vec3 chroma(float c) { float r = floor(c / 64.0); float g = floor((c - r * 64.0) / 8.0); float b = c - r * 64.0 - g * 8.0; return vec3(r, g, b) / 7.0; }\n"
        "void main() {\n"
        "  gl_Position = ftransform();\n"
        "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "  vec4 e = gl_ModelViewMatrix * gl_Vertex;\n"
        "  vWorld = uEnt > 0.5 ? (uInvV * e).xyz : gl_Vertex.xyz;\n"
        "  gl_ClipVertex = e;\n"
        "  vFog = abs(e.z);\n"
        "  if (uEnt > 0.5) { vSky = uEntSky; vCol = gl_Color; return; }\n"
        "  float v = floor(gl_Color.a * 2097152.0 + 0.5);\n"
        "  float cc = floor(v / 4096.0); v -= cc * 4096.0; if (cc > 511.5) { cc = 509.0; v = 4096.0; }\n"
        "  float sk4 = floor(v / 64.0);\n"
        "  float sk = sk4 * 0.25;\n"
        "  float bl = (v - sk4 * 64.0) * 0.25;\n"
        "  vSky = sk;\n"
        "  if (uMode > 0.5) sk = min(15.0, sk + 2.0);\n"
        "  float sv = 0.94 * pow(0.8, 15.0 - sk) * uSkyScale;\n"
        "  float bf = 0.0;\n"
        "  if (bl > 0.01) bf = 0.06 + 0.94 * pow(0.8, 15.0 - bl);\n"
        "  vec3 bc = bf * chroma(cc);\n"
        "  vec3 L = vec3(max(0.06 + sv * uSkyRB, bc.r), max(0.06 + sv * uSkyRB, bc.g), max(0.06 + sv, bc.b));\n"
        "  if (bl > 15.5) L = vec3(1.0);\n"
        "  vCol = vec4(gl_Color.rgb * L, uMode > 0.5 ? 0.62 : 1.0);\n"
        "}\n";
    std::string FSHs =   // com sombras em cascatas: gerado porque cada cascata precisa do seu proprio sampler (GLSL 1.20 nao deixa passar sampler de forma portavel)
        "#version 120\n"
        "#define SHTYPE sampler2DShadow\n#define SHS(n, u, z) shadow2D(n, vec3(u, z)).r\n"   // @SHCOMPAT@ (trocado na versao compat)
        "uniform sampler2D uTex; uniform SHTYPE uShT0, uShT1, uShT2, uShE0, uShE1;\n"
        "uniform mat4 uMT0, uMT1, uMT2, uME0, uME1; uniform vec2 uTT0, uTT1, uTT2, uTE0, uTE1;\n"   // vec2 = (1/texels, tamanho do texel em blocos)
        "uniform sampler2D uCloud; uniform float uCloudOn; uniform float uCloudOff; uniform float uFancy; uniform sampler2D uRfl; uniform float uRfOn; uniform vec2 uScr; uniform float uTime; uniform float uShadowOn; uniform float uSunStr; uniform float uEntOn; uniform vec3 uSunDir; uniform vec3 uCamW; uniform float uEnt; uniform float uMode;\n"
        "varying vec4 vCol; varying float vFog; varying vec3 vWorld; varying float vSky;\n"
        "float ign(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }\n"   // ruido por pixel: gira o filtro (penumbra macia em vez de serrilhado)
        "vec2 vogel(float i, float n, float rot) { float a = i * 2.39996323 + rot; return vec2(cos(a), sin(a)) * sqrt((i + 0.5) / n); }\n"
        "float valid(vec3 p) { float e = min(min(p.x, 1.0 - p.x), min(p.y, 1.0 - p.y)); return (p.z < 0.0 || p.z > 1.0) ? 0.0 : clamp(e * 10.0, 0.0, 1.0); }\n"   // 1 = dentro; cai a 0 na borda (mistura entre cascatas)
        "vec3 prj(mat4 M, vec3 wp, vec3 n, float tw, float slope) { return (M * vec4(wp + n * ((0.7 + slope) * tw), 1.0)).xyz; }\n";   // deslocamento pela normal proporcional ao texel (contra acne sem "descolar" a sombra)
    for (const char* s : {"T0", "T1", "T2", "E0", "E1"}) {
        const std::string n = s;   // 4 amostras; se todas iguais (interior de area clara/escura) para; senao +8 em disco de Vogel
        FSHs += "float pcf" + n + "(vec3 p, float z, float rad, float rot) {\n  float s = 0.0;\n"
                "  for (int i = 0; i < 4; i++) { float a = rot + float(i) * 1.5707963; s += SHS(uSh" + n + ", p.xy + vec2(cos(a), sin(a)) * rad, z); }\n"
                "  if (s < 0.01 || s > 3.99) return s * 0.25;\n"
                "  for (int i = 0; i < 8; i++) s += SHS(uSh" + n + ", p.xy + vogel(float(i), 8.0, rot) * rad * 0.9, z);\n  return s / 12.0;\n}\n";
    }
    FSHs +=
        "float cloudCov(vec3 wp) {\n"
        "  if (uCloudOn < 0.5 || uSunDir.y < 0.06) return 0.0;\n"
        "  float t = (130.0 - wp.y) / uSunDir.y; if (t < 0.0) return 0.0;\n"
        "  vec2 c = (wp.xz + uSunDir.xz * t + vec2(uCloudOff, 0.0)) * (1.0 / 3072.0);\n"
        "  float r = (0.9 + t * 0.0016) / 256.0;\n"
        "  float s = texture2D(uCloud, c).r * 0.36 + texture2D(uCloud, c + vec2(r, r * 0.4)).r * 0.16 + texture2D(uCloud, c + vec2(-r * 0.4, r)).r * 0.16 + texture2D(uCloud, c + vec2(-r, -r * 0.4)).r * 0.16 + texture2D(uCloud, c + vec2(r * 0.4, -r)).r * 0.16;\n"
        "  return smoothstep(0.12, 0.88, s) * smoothstep(0.06, 0.2, uSunDir.y);\n"
        "}\n"
        "float casT(vec3 wp, vec3 n, float slope, float rot) {\n"   // terreno: usa a cascata mais nitida que cobre o pixel; mistura suave na borda
        "  float bias = 0.00015 + 0.0002 * slope;\n"
        "  vec3 p0 = prj(uMT0, wp, n, uTT0.y, slope); float v0 = valid(p0);\n"
        "  if (v0 >= 1.0) return pcfT0(p0, p0.z - bias, uTT0.x * 1.25, rot);\n"
        "  vec3 p1 = prj(uMT1, wp, n, uTT1.y, slope); float v1 = valid(p1); float l;\n"
        "  if (v1 >= 1.0) l = pcfT1(p1, p1.z - bias, uTT1.x * 1.9, rot);\n"
        "  else {\n"
        "    vec3 p2 = prj(uMT2, wp, n, uTT2.y, slope); float v2 = valid(p2);\n"
        "    l = v2 > 0.0 ? mix(1.0, pcfT2(p2, p2.z - bias, uTT2.x * 1.6, rot), v2) : 1.0;\n"   // fora da ultima cascata: sem sombra
        "    if (v1 > 0.0) l = mix(l, pcfT1(p1, p1.z - bias, uTT1.x * 1.9, rot), v1);\n"
        "  }\n"
        "  return v0 > 0.0 ? mix(l, pcfT0(p0, p0.z - bias, uTT0.x * 1.25, rot), v0) : l;\n"
        "}\n"
        "float casE(vec3 wp, vec3 n, float slope, float rot) {\n"   // jogador/mobs
        "  float bias = 0.00004 + 0.00006 * slope;\n"
        "  vec3 q0 = prj(uME0, wp, n, uTE0.y, slope); float w0 = valid(q0);\n"
        "  if (w0 >= 1.0) return pcfE0(q0, q0.z - bias, uTE0.x * 1.4, rot);\n"
        "  vec3 q1 = prj(uME1, wp, n, uTE1.y, slope); float w1 = valid(q1);\n"
        "  float l = w1 > 0.0 ? mix(1.0, pcfE1(q1, q1.z - bias, uTE1.x * 1.5, rot), w1) : 1.0;\n"
        "  return w0 > 0.0 ? mix(l, pcfE0(q0, q0.z - bias, uTE0.x * 1.4, rot), w0) : l;\n"
        "}\n"
        "void main() {\n"
        "  vec4 c = texture2D(uTex, gl_TexCoord[0].xy) * vCol;\n"
        "  if (c.a < 0.1) discard;\n"   // o alpha test fixo nao vale com shader proprio (GLES): sem isso, partes transparentes ficam pretas
        "  float elv = clamp(uSunDir.y, 0.0, 1.0); float am = min(1.0 / (elv + 0.50572 * pow(6.08 + degrees(asin(elv)), -1.6364)), 18.0);\n"
        "  vec3 scol = exp(-vec3(0.04, 0.095, 0.22) * am); scol /= max(max(scol.r, scol.g), scol.b); float glowK = clamp(uSunDir.y * 4.0 + 0.55, 0.0, 1.0);\n"
        "  if (uShadowOn > 0.5 && vFog < 120.0 && vSky > 7.5) {\n"
        "    vec3 n = normalize(cross(dFdx(vWorld), dFdy(vWorld)));\n"   // normal da face pela derivada da posicao (aponta para a camera)
        "    float ndl = dot(n, uSunDir);\n"
        "    float direct = 0.0;\n"
        "    if (ndl > 0.02) {\n"
        "      float slope = min(sqrt(max(0.0, 1.0 - ndl * ndl)) / max(ndl, 0.15), 3.0);\n"
        "      float rot = ign(gl_FragCoord.xy) * 6.2831853;\n"
        "      float lit = casT(vWorld, n, slope, rot);\n"
        "      if (uEntOn > 0.5) lit = min(lit, casE(vWorld, n, slope, rot));\n"
        "      direct = lit * smoothstep(0.02, 0.32, ndl);\n"   // terminador suave: faces rasantes ao sol escurecem aos poucos
        "    }\n"
        "    float k = uSunStr * clamp((vSky - 8.0) / 6.0, 0.0, 1.0) * (1.0 - smoothstep(80.0, 120.0, vFog));\n"   // so onde ha luz do ceu; some suave com a distancia
        "    float warm = 1.0 - smoothstep(0.12, 0.6, uSunDir.y);\n"   // sol baixo = luz dourada
        "    vec3 shTint = vec3(0.50, 0.58, 0.80);\n"   // sombra azulada (luz do ceu)
        "    vec3 sunTint = mix(vec3(1.03, 1.01, 0.96), mix(vec3(1.0), scol, 0.5) * vec3(1.2, 1.1, 1.0), warm);\n"
        "    c.rgb *= mix(vec3(1.0), shTint, k * (1.0 - direct));\n"
        "    float cl = cloudCov(vWorld) * smoothstep(0.02, 0.3, ndl);\n"
        "    c.rgb *= mix(vec3(1.0), sunTint, k * direct * 0.7 * (1.0 - cl));\n"
        "    c.rgb *= mix(vec3(1.0), vec3(0.72, 0.79, 0.94), k * cl * (0.3 + 0.7 * direct) * 0.85);\n"
        "  }\n"
        "  vec3 vd = normalize(vWorld - uCamW);\n"   // direcao do olho ao pixel (mundo)
        "  float sunUp = clamp(uSunDir.y, -0.2, 1.0); float dayK = clamp(sunUp * 4.0 + 0.3, 0.0, 1.0);\n"
        "  float sd = max(dot(vd, uSunDir), 0.0);\n"
        "  if (uMode > 0.5 && uFancy > 0.5) {\n"   // agua: ondas (normal perturbada, animada), fresnel de Schlick, reflexo do ceu e brilho suave do sol
        "    vec2 w = vWorld.xz; float t = uTime;\n"
        "    vec2 g = vec2(cos(w.x * 1.7 + t * 1.1 + w.y * 0.9), cos(w.y * 2.1 - t * 0.9 + w.x * 0.6)) * 0.07;\n"
        "    g += vec2(cos(w.x * 3.9 - t * 1.7 + w.y * 2.3), cos(w.y * 4.4 + t * 1.5 - w.x * 1.9)) * 0.04;\n"
        "    g *= 1.0 - smoothstep(16.0, 56.0, vFog);\n"   // some com a distancia (evita cintilar/aliasing longe)
        "    vec3 nw = normalize(vec3(-g.x, 1.0, -g.y));\n"
        "    float fr = 0.05 + 0.95 * pow(1.0 - clamp(dot(-vd, nw), 0.0, 1.0), 4.0);\n"
        "    vec3 rf = reflect(vd, nw); rf.y = abs(rf.y);\n"
        "    vec3 zen = gl_Fog.color.rgb * vec3(0.55, 0.75, 1.15);\n"
        "    vec3 refl = mix(gl_Fog.color.rgb, zen, clamp(rf.y * 1.6, 0.0, 1.0));\n"
        "    if (uRfOn > 0.5) { vec2 ruv = clamp(gl_FragCoord.xy / uScr + g * 0.12, 0.003, 0.997); refl = mix(refl, texture2D(uRfl, ruv).rgb, 0.93); }\n"
        "    float sd2 = max(dot(rf, uSunDir), 0.0);\n"
        "    float sp = pow(sd2, 380.0) * 1.4 + pow(sd2, 36.0) * 0.16; sp = sp / (1.0 + sp);\n"   // curva suave: sem disco estourado
        "    float sunV = smoothstep(-0.02, 0.12, uSunDir.y);\n"
        "    vec3 wb = mix(vec3(0.06, 0.27, 0.36) * (0.3 + 0.7 * dayK), c.rgb * vec3(0.8, 1.1, 1.2), 0.3); c.rgb = mix(wb, refl, clamp(fr * 1.3 + 0.08, 0.0, 1.0)) + scol * vec3(1.0, 0.92, 0.8) * sp * sunV * (0.35 + 0.65 * fr);\n"
        "    c.rgb *= 0.94 + 0.12 * (g.x + g.y) * 8.0;\n"   // leve variacao de brilho nas cristas das ondas
        "    c.a = clamp(c.a + fr * 0.6 + sp * 0.3, 0.0, 0.96);\n"
        "  }\n"
        "  float lum = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"   // vibrance (cores baixas ganham saturacao) + curva S leve: mais profundidade
        "  c.rgb = mix(vec3(lum), c.rgb, 0.93 + 0.2 * (1.0 - clamp(lum * 1.5, 0.0, 1.0)));\n"
        "  c.rgb = mix(c.rgb, c.rgb * c.rgb * (3.0 - 2.0 * c.rgb), 0.26);\n"
        "  c.rgb *= mix(vec3(1.0), vec3(1.04, 1.0, 0.94), dayK * 0.6);\n"
        "  c.rgb = mix(c.rgb, c.rgb * vec3(0.80, 0.92, 1.25), (1.0 - dayK) * 0.55);\n"
        "  float f = clamp((gl_Fog.end - vFog) / (gl_Fog.end - gl_Fog.start), 0.0, 1.0);\n"
        "  float hz = (1.0 - exp(-vFog * 0.011)) * 0.27 * dayK;\n"   // perspectiva atmosferica: o longe esfria e some no ceu
        "  c.rgb = mix(c.rgb, gl_Fog.color.rgb * vec3(0.92, 0.97, 1.05), hz);\n"
        "  vec3 fogC = gl_Fog.color.rgb + scol * (pow(sd, 5.0) * 0.5 + pow(sd, 36.0) * 0.55) * (1.0 - f) * glowK * (1.0 - smoothstep(0.35, 0.8, uSunDir.y) * 0.6);\n"   // luz do sol espalhada na neblina, do lado do sol
        "  c.rgb = mix(fogC, c.rgb, f);\n"
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
        "  vec2 dv = p - s; float ds = length(dv); float ang = atan(dv.y, dv.x);\n"
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
        "  vec3 haze = uCol * (exp(-ds * 7.0) * 0.30 + exp(-ds * 2.2) * 0.12 + exp(-ds * 26.0) * 0.20) * bl;\n"
        "  float rim = exp(-pow((ds - 0.17) / 0.07, 2.0)) * 4.0 * uOcc * (1.0 - uOcc);\n"
        "  vec3 halo = mix(uCol, vec3(1.0), 0.6) * rim * 0.14 * uI;\n"
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
    {   // celular: shader ENXUTO (sem sombras em cascata/PCF e sem ondas de agua). Mesmo desligado por uniform, esse codigo infla registradores na Mali e derruba o FPS (~2 FPS medido).
        auto cut = [&](const char* from, const char* to) { const size_t a = FSHs.find(from); if (a == std::string::npos) return; const size_t b = FSHs.find(to, a); if (b == std::string::npos) return; FSHs.erase(a, b - a); };
        cut("  if (uShadowOn > 0.5 && vFog", "  vec3 vd = normalize(vWorld - uCamW);");
        cut("  if (uMode > 0.5 && uFancy > 0.5) {", "  float lum = dot(c.rgb");
        cut("float ign(vec2 p)", "void main() {");
    }
#endif
    std::string FSc = FSHs; {   // versao compat: sampler2D comum + comparacao manual (GLES2/gl4es nao tem sampler2DShadow)
        const size_t a = FSc.find("#define SHTYPE"), b = FSc.find("uniform sampler2D uTex");
        if (a != std::string::npos && b != std::string::npos) FSc.replace(a, b - a, "#define SHTYPE sampler2D\n#define SHS(n, u, z) step(z, texture2D(n, u).r)\n");
    }
#ifdef __ANDROID__
    gShCompat = true;
#endif
    if (pUniform3f && pUniformM4 && pUniform2f) {
        skyProg = tryProg(VSH, (gShCompat ? FSc : FSHs).c_str());
        if (!skyProg && !gShCompat) { gShCompat = true; logWarn("Sombras: shader com sampler2DShadow falhou; usando modo de compatibilidade"); skyProg = tryProg(VSH, FSc.c_str()); }
        if (skyProg && gShCompat) logInfo("Sombras: modo de compatibilidade (profundidade comparada no shader)");
    }   // versao com sombras; se falhar, cai na versao simples (o dia/noite nunca depende das sombras)
    gShadowShader = skyProg != 0;
    if (!skyProg) skyProg = tryProg(VS, FS);
    if (!skyProg) return;
    uMode = getUniformLoc(skyProg, "uMode"); uFancy = getUniformLoc(skyProg, "uFancy"); uSkyScale = getUniformLoc(skyProg, "uSkyScale"); uSkyRB = getUniformLoc(skyProg, "uSkyRB");
    pUseProgram(skyProg); uniform1i(getUniformLoc(skyProg, "uTex"), 0);
    if (gShadowShader) {
        uShOn = getUniformLoc(skyProg, "uShadowOn"); uShSun = getUniformLoc(skyProg, "uSunDir"); uShStr = getUniformLoc(skyProg, "uSunStr");
        uShEOn = getUniformLoc(skyProg, "uEntOn"); uEnt = getUniformLoc(skyProg, "uEnt"); uInvV = getUniformLoc(skyProg, "uInvV"); uEntSky = getUniformLoc(skyProg, "uEntSky"); uCamW = getUniformLoc(skyProg, "uCamW"); uTimeL = getUniformLoc(skyProg, "uTime"); uRfOn = getUniformLoc(skyProg, "uRfOn"); uScrL = getUniformLoc(skyProg, "uScr"); uniform1i(getUniformLoc(skyProg, "uRfl"), 6); uniform1i(getUniformLoc(skyProg, "uCloud"), 7); uCloudOnL = getUniformLoc(skyProg, "uCloudOn"); uCloudOffL = getUniformLoc(skyProg, "uCloudOff");
        char nm[16];
        for (int i = 0; i < 3; i++) { snprintf(nm, sizeof nm, "uMT%d", i); gCasT[i].uM = getUniformLoc(skyProg, nm); snprintf(nm, sizeof nm, "uTT%d", i); gCasT[i].uT = getUniformLoc(skyProg, nm); snprintf(nm, sizeof nm, "uShT%d", i); uniform1i(getUniformLoc(skyProg, nm), 1 + i); }
        for (int i = 0; i < 2; i++) { snprintf(nm, sizeof nm, "uME%d", i); gCasE[i].uM = getUniformLoc(skyProg, nm); snprintf(nm, sizeof nm, "uTE%d", i); gCasE[i].uT = getUniformLoc(skyProg, nm); snprintf(nm, sizeof nm, "uShE%d", i); uniform1i(getUniformLoc(skyProg, nm), 4 + i); }
    }
    if (pUniform2f && pUniform3f) {   // god rays (opcional: se nao compilar, so nao aparecem)
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
    if (!gShaderOK) return;
#ifdef BL_VULKAN
    vkS.sky[0] = 1.f; vkS.sky[1] = mode; vkS.sky[2] = sc; vkS.sky[3] = rb;
    { const float th = gSky.angle * 6.2831853f; vkS.env[0] = -sinf(th); vkS.env[1] = cosf(th); vkS.env[2] = 0.f; vkS.env[3] = (float)SDL_GetTicks() * 0.001f; vkS.cam[0] = gCamW[0]; vkS.cam[1] = gCamW[1]; vkS.cam[2] = gCamW[2]; }
    return;
#endif
    pUseProgram(skyProg); pUniform1f(uMode, mode); pUniform1f(uFancy, gShaderPackOn ? 1.f : 0.f); pUniform1f(uSkyScale, sc); pUniform1f(uSkyRB, rb);
    if (gShadowShader) {
        const float th = gSky.angle * 6.2831853f; pUniform3f(uShSun, -sinf(th), cosf(th), 0.f); pUniform3f(uCamW, gCamW[0], gCamW[1], gCamW[2]); pUniform1f(uEnt, 0.f); pUniform1f(uRfOn, gRfOn && gShaderPackOn ? 1.f : 0.f); pUniform2f(uScrL, (float)gRfSW, (float)gRfSH); pUniform1f(uTimeL, (float)fmod(SDL_GetTicks() * 0.001, 6283.0));   // sol/camera: sempre (neblina e agua usam)
        const bool on = gShadowDraw && mode < .5f; pUniform1f(uShOn, on ? 1.f : 0.f);
        if (on) {
            pUniform3f(uShSun, gSunDir[0], gSunDir[1], gSunDir[2]); pUniform1f(uShStr, gSunStr); pUniform1f(uShEOn, gShadowEnt ? 1.f : 0.f); pUniform1f(uCloudOnL, gCloudTex ? 1.f : 0.f); pUniform1f(uCloudOffL, gCloudOff);
            for (const ShCas& c : gCasT) { pUniformM4(c.uM, 1, GL_FALSE, c.mat); pUniform2f(c.uT, 1.f / c.n, 2.f * c.e / c.n); }
            if (gShadowEnt) for (const ShCas& c : gCasE) { pUniformM4(c.uM, 1, GL_FALSE, c.mat); pUniform2f(c.uT, 1.f / c.n, 2.f * c.e / c.n); }
        }
    }
}
static void skyShaderOff() {
#ifdef BL_VULKAN
    vkS.sky[0] = 0.f; return;
#endif
    if (gShaderOK) pUseProgram(0); }
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
    glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor4f(.02f, .03f, .10f, a);
    glBegin(GL_QUADS); glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, 1); glVertex2f(-1, 1); glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glColor4f(1.f, 1.f, 1.f, 1.f); glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D);
}
