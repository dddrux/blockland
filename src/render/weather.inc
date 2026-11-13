// patchWx.1: clima (limpo/chuva/tempestade; neve em bioma frio). Estado em biome_light.inc; sincronizado do host nos bits 2-3 do NP_TIME.
static bool fireBaseOk(int x, int y, int z);   // nether.inc
static bool wxSkyOpen(int x, int y, int z) { for (int yy = y + 1; yy <= 255 && yy <= y + 80; yy++) if (getB(x, yy, z) != AIR) return false; return true; }
static bool wxCold(double x, double z) { if (gDim == 3) return false; float t, h; climateAt((float)x, (float)z, &t, &h); return t < .25f; }
// patchWx.5/.7: o raio causa dano (5) a quem esta a menos de 3 blocos (jogador e mobs) e ateia fogo no ponto atingido (como o MC; so o host/solo mexe em mobs e coloca o fogo)
static int gThT = -1; static float gThVol = 1.f;   // patchWx.8: trovao atrasado pela distancia
static void wxBoltStrike(double px, double py, double pz) {
    { const double dd0 = hypot(px - gBoltX, pz - gBoltZ); gThVol = (float)std::max(.3, std::min(1.5, 1.6 - dd0 / 80.0)); gThT = (int)std::min(50.0, dd0 / 5.0); }
    const double ddx = px - gBoltX, ddz = pz - gBoltZ, ddy = py - gBoltY; if (ddx * ddx + ddz * ddz < 9.0 && ddy > -4.0 && ddy < 6.0 && !creative) survDamage(5, "foi atingido por um raio");
    if (!netOn || netHost) for (auto& m : MOBS) {   // patchWx.7: mobs tambem (o host simula os mobs)
        if (m.dead) continue; const double mx = m.x - gBoltX, mz = m.z - gBoltZ, my = m.y - gBoltY; if (mx * mx + mz * mz >= 9.0 || my < -4.0 || my > 6.0) continue;
        mobHurt(m, 5, (float)mx, (float)mz, false);
    }
    const int bx = (int)floor(gBoltX), bz = (int)floor(gBoltZ), by = (int)gBoltY;
    if ((!netOn || netHost) && by > 1 && by < CH - 1 && gBoltCd % 3 != 0 && getB(bx, by, bz) == AIR && fireBaseOk(bx, by, bz)) { setBM(bx, by, bz, XB_FIRE, 0); netSendBlock(bx, by, bz, XB_FIRE); }
}
static bool gBoltPend = false; static float gBoltPx = 0, gBoltPy = 0, gBoltPz = 0; static int gBoltPSeed = 0;   // patchNet.1: raio recebido do host (aplicado no proximo wxTick)
static void wxBoltRecv(NBuf& b, int sender) { const float x = b.get<float>(), y = b.get<float>(), z = b.get<float>(); const int sd = b.get<int32_t>(); if (netHost || sender != 0) return; gBoltPend = true; gBoltPx = x; gBoltPy = y; gBoltPz = z; gBoltPSeed = sd; }
static SndBuf* gRainBuf = nullptr;
static void wxRainSound(double px, double py, double pz) {   // chuva: toca uma faixa de rain1-8 atras da outra; mais baixa debaixo de teto; neve nao faz barulho de chuva
    if (!gSndOn || !dimSky() || gRain < .05f || wxCold(px, pz)) { gRainBuf = nullptr; return; }
    for (const SndVoice& v : gSndV) if (v.b && v.b == gRainBuf && !v.music && v.pos < v.b->frames - .25 * v.b->rate) return;   // ainda tocando (emenda 0,25 s antes do fim)
    static int roofT = 0; static bool open = true; if (--roofT <= 0) { roofT = 10; open = wxSkyOpen((int)floor(px), (int)floor(py + 1.6), (int)floor(pz)); }
    const SndEvt* e = sndEvtGet("weather.rain"); if (!e) return; const std::string& n = e->f[rand() % e->f.size()];
    SndBuf* bf = sndGet(n); if (!bf) return; gRainBuf = bf; const float vol = e->vol * std::min(1.f, gRain * 1.2f) * (open ? 1.f : .3f);
    sndPlayBuf(bf, vol, vol, 1.f, false);
}
// patchRain.2: respingo como a ParticleRain do 1.12.2: sprite de particle/particles.png (indices 19-22), nasce a +0,1 do topo, pula (vy .1-.3), gravidade .06, atrito .98, no chao 50% some, vida 8/(.2-1) ticks, tamanho .1-.2
struct WxDrop { float x, y, z, px, py, pz, vx, vy, vz, sz, gy, br; int age, max, spr; };
static std::vector<WxDrop> gWxDrops; static float gWxTickT = 0.f; static bool gWxTickNew = false;
static void wxSplashTick(double px, double py, double pz) {
    if (!dimSky() || gPerf || !gOptParticles) { gWxDrops.clear(); return; }
    auto R = []() { return rand() / (float)RAND_MAX; };
    gWxTickNew = true;
    for (size_t i = 0; i < gWxDrops.size();) {
        WxDrop& d = gWxDrops[i]; d.px = d.x; d.py = d.y; d.pz = d.z;
        if (d.age++ >= d.max) { gWxDrops[i] = gWxDrops.back(); gWxDrops.pop_back(); continue; }
        d.vy -= .06f; d.x += d.vx; d.y += d.vy; d.z += d.vz; d.vx *= .98f; d.vy *= .98f; d.vz *= .98f;
        if (d.y < d.gy) {   // tocou o chao: 50% some, o resto desliza freando
            d.y = d.gy; d.vy = 0.f; if (R() < .5f) { gWxDrops[i] = gWxDrops.back(); gWxDrops.pop_back(); continue; } d.vx *= .7f; d.vz *= .7f; }
        i++;
    }
    if (gRain < .05f || wxCold(px, pz) || gWxDrops.size() > 1500) return;
    const int cx = (int)floor(px), cz = (int)floor(pz), cy = (int)floor(py), n = (int)(45.f * gRain * gRain);
    for (int k = 0; k < n; k++) {
        const int X = cx + rand() % 10 - rand() % 10, Z = cz + rand() % 10 - rand() % 10;
        int y = std::min(255, cy + 24); while (y > 0 && getB(X, y, Z) == AIR) y--;
        const int top = y + 1; if (top > cy + 10 || top <= cy - 10) continue;
        const bool wat = isWater(getB(X, y, Z)); const float gy = (float)top + (wat ? -.12f : 0.f);
        WxDrop d; d.x = d.px = X + R(); d.z = d.pz = Z + R(); d.y = d.py = gy + .1f; d.gy = gy;
        float mx = (R() * 2.f - 1.f) * .4f, my = (R() * 2.f - 1.f) * .4f, mz = (R() * 2.f - 1.f) * .4f;
        const float f = (R() + R() + 1.f) * .15f, l = std::max(1e-4f, sqrtf(mx * mx + my * my + mz * mz));
        d.vx = mx / l * f * .4f * .3f; d.vz = mz / l * f * .4f * .3f; d.vy = R() * .2f + .1f;
        d.sz = .1f * (R() * .5f + .5f) * 2.f; d.max = (int)(8.f / (R() * .8f + .2f)); d.age = 0; d.spr = 19 + rand() % 4;
        d.br = std::max(.25f, lightNear(X, y + 1, Z)); gWxDrops.push_back(d);
    }
}
static void wxSplashDraw(double px, double py, double pz, float t) {
    if (gWxDrops.empty()) return;
    static GLuint tex = 0;
    if (!tex) {
        std::vector<uint8_t> pxl; int w = 0, h = 0;
        if (packRGBA("particle/particles.png", pxl, w, h) && w >= 16 && h >= 16) tex = upTex(pxl.data(), w, h);
        else { w = h = 128; pxl.assign(w * h * 4, 0); for (int s = 19; s < 23; s++) for (int y = 2; y < 6; y++) for (int x = 3; x < 5; x++) { uint8_t* q = &pxl[(((s / 16) * 8 + y) * w + (s % 16) * 8 + x) * 4]; q[0] = 90; q[1] = 130; q[2] = 255; q[3] = 255; } tex = upTex(pxl.data(), w, h); }
    }
    if (gWxTickNew) { gWxTickNew = false; gWxTickT = t; }   // instante do ultimo tick, visto pelo desenho
    const float fr = std::max(0.f, std::min(1.f, (t - gWxTickT) * 20.f));
    rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .1f); glBegin(GL_QUADS);
    for (const WxDrop& d : gWxDrops) {
        const float x = d.px + (d.x - d.px) * fr, y = d.py + (d.y - d.py) * fr, z = d.pz + (d.z - d.pz) * fr;
        const float ex = x - (float)px, ez = z - (float)pz, l = sqrtf(ex * ex + ez * ez), rx = l > .01f ? -ez / l : 1.f, rz = l > .01f ? ex / l : 0.f, s = d.sz * .5f;   // gota voltada para a camera (vertical)
        const float u0 = (d.spr % 16) / 16.f, v0 = (d.spr / 16) / 16.f, du = 1.f / 16.f;
        if (gDim == 3) glColor3f(d.br, d.br * .06f, d.br * .08f); else glColor3f(d.br, d.br, d.br);
        glTexCoord2f(u0, v0 + du); glVertex3f(x - rx * s, y, z - rz * s);
        glTexCoord2f(u0 + du, v0 + du); glVertex3f(x + rx * s, y, z + rz * s);
        glTexCoord2f(u0 + du, v0); glVertex3f(x + rx * s, y + d.sz, z + rz * s);
        glTexCoord2f(u0, v0); glVertex3f(x - rx * s, y + d.sz, z - rz * s);
    }
    glEnd(); rdDisable(GL_ALPHA_TEST);
}
static void wxTick(double px, double py, double pz) {
    wxSplashTick(px, py, pz);
    if (!dimSky()) { gRain = std::max(0.f, gRain - .02f); gThunder = std::max(0.f, gThunder - .02f); gFlash = 0; return; }
    const int wxW = gDim == 3 ? 2 : gWeather;   // patchBlood.1: Rubra = tempestade de sangue constante (so visual; nao mexe no gWeather da rede)
    if ((!netOn || netHost) && gDim == 0) {
        if (--gWxLeft <= 0) {
            if (gWeather == 0) { gWeather = (rand() % 4 == 0) ? 2 : 1; gWxLeft = 12000 + rand() % 12000; } else { gWeather = 0; gWxLeft = 12000 + rand() % 168000; }
            if (netOn) netSendTime(false);
        }
    }
    gRain += ((wxW > 0 ? 1.f : 0.f) - gRain) * .01f; if (fabsf(gRain - (wxW > 0 ? 1.f : 0.f)) < .004f) gRain = wxW > 0 ? 1.f : 0.f;
    gThunder += ((wxW == 2 ? 1.f : 0.f) - gThunder) * .01f;
    gFlash = std::max(0.f, gFlash - .06f); if (gBoltT > 0) gBoltT -= 1;
    wxRainSound(px, py, pz); if (gThT >= 0 && --gThT < 0) sndEvtUi("weather.thunder", gThVol);
    if (gBoltPend) { gBoltPend = false; gBoltX = gBoltPx; gBoltY = gBoltPy; gBoltZ = gBoltPz; gBoltSeed = gBoltPSeed; gBoltT = 8; gFlash = 1.f; wxBoltStrike(px, py, pz); }   // convidado: raio do host
    if (gDim == 3 && --gBoltCd <= 0) {   // patchBlood.1: raios da Rubra: locais (cada cliente sorteia o seu perto de si; sem rede)
        gBoltCd = 80 + rand() % 260; gBoltX = px + (rand() % 61) - 30; gBoltZ = pz + (rand() % 61) - 30; gBoltT = 8; gFlash = 1.f; gBoltSeed = rand();
        int y = 255; while (y > 0 && getB((int)floor(gBoltX), y, (int)floor(gBoltZ)) == AIR) y--; gBoltY = y + 1; wxBoltStrike(px, py, pz);
    }
    if (gDim == 0 && gWeather == 2 && (!netOn || netHost) && --gBoltCd <= 0) {   // raio: cai perto de um jogador aleatorio (host ou convidado no mundo); convidados recebem NP_BOLT
        double ax = px, az = pz; if (netOn) { std::vector<std::pair<double, double>> pl; pl.push_back({px, pz}); for (const auto& kv : remotes) if (kv.second.init && kv.second.dim == 0 && kv.second.deathT < 0) pl.push_back({kv.second.x, kv.second.z}); const auto& c = pl[rand() % pl.size()]; ax = c.first; az = c.second; }
        gBoltCd = 200 + rand() % 600; gBoltX = ax + (rand() % 81) - 40; gBoltZ = az + (rand() % 81) - 40; gBoltT = 8; gFlash = 1.f; gBoltSeed = rand();
        int y = 255; while (y > 0 && getB((int)floor(gBoltX), y, (int)floor(gBoltZ)) == AIR) y--; gBoltY = y + 1;
        wxBoltStrike(px, py, pz);
        if (netOn) { NBuf nb = netMk(NP_BOLT); nb.put<float>((float)gBoltX); nb.put<float>((float)gBoltY); nb.put<float>((float)gBoltZ); nb.put<int32_t>(gBoltSeed); netBroadcast(nb); }
    }
    if (gDim == 0 && gRain > .5f && (!netOn || netHost) && (gWorldTime % 20) == 0 && !wxCold(px, pz)) {   // chuva enche o caldeirao (so com ceu aberto)
        static int row = 0; const int z = (int)floor(pz) - 16 + row; row = (row + 1) % 33;   // varre 1 fileira (33 colunas) por vez; cada caldeirao e testado a cada ~33 s
        for (int x = (int)floor(px) - 16; x <= (int)floor(px) + 16; x++) for (int y = (int)floor(py) + 12; y >= (int)floor(py) - 12; y--) {
            if (getB(x, y, z) != XB_DECOR) continue; const int m = getM(x, y, z); if ((m & 3) != 0 || ((m >> 2) & 3) >= 3) continue;
            if (rand() % 2 || !wxSkyOpen(x, y, z)) continue;
            const int nm = (m & 3) | ((((m >> 2) & 3) + 1) << 2); setBM(x, y, z, XB_DECOR, nm); netSendBlock(x, y, z, XB_DECOR, nm);
        }
    }
}
static void wxSkyApply(float* top, float* hor, bool colors = true) {   // escurece/acinzenta o ceu e a neblina; clarao do raio (colors=false: a cor ja vem da formula do 1.12, so luz do ceu e clarao)
    const float k = gRain * .45f + gThunder * .2f; if (k <= 0.f && gFlash <= 0.f) return;
    for (float* c : {top, hor}) { if (!colors) { for (int q = 0; q < 3; q++) c[q] = std::min(1.f, c[q] + gFlash * .8f); continue; }
        const float l = (c[0] + c[1] + c[2]) * .3333f * .75f;
        for (int q = 0; q < 3; q++) c[q] = std::min(1.f, (c[q] + (l - c[q]) * k * 1.2f) * (1.f - k * .55f) + gFlash * .8f);
    }
    gSkyScale *= 1.f - gRain * .22f - gThunder * .1f; gSkyScale = std::min(1.f, gSkyScale + gFlash * .6f);
}
static void wxDraw(double px, double py, double pz, float t) {
    if (!dimSky() || (gRain < .02f && gBoltT <= 0)) return;
    rdDisable(GL_TEXTURE_2D); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdDepthMask(GL_FALSE);
    if (gBoltT > 0) {   // patchWx.10: raio estilo Minecraft: caminho aleatorio fino em 40 passos, 3 camadas de brilho (largas e fracas -> finas e brancas) e ramos laterais
        rdBlendFunc(GL_SRC_ALPHA, GL_ONE); const float fa = std::min(1.f, gBoltT / 5.f);
        auto seg = [&](float x, float y, float z, float nx, float ny, float nz, float w) {
            glVertex3f(x - w, y, z); glVertex3f(x + w, y, z); glVertex3f(nx + w, ny, nz); glVertex3f(nx - w, ny, nz);
            glVertex3f(x, y, z - w); glVertex3f(x, y, z + w); glVertex3f(nx, ny, nz + w); glVertex3f(nx, ny, nz - w); };
        unsigned long long rs = 0x9E3779B97F4A7C15ULL ^ (unsigned long long)(unsigned)gBoltSeed * 2654435761ULL;
        auto rnd = [&]() { rs = rs * 6364136223846793005ULL + 1442695040888963407ULL; return (float)((rs >> 33) & 0xFFFF) / 65535.f - .5f; };   // -.5..+.5, deterministico por raio (host = convidado)
        const int N = 40; float PX[N + 1], PY[N + 1], PZ[N + 1]; const float top = (float)gBoltY + 100.f, dy = (top - (float)gBoltY) / N;
        PX[0] = (float)gBoltX; PY[0] = top; PZ[0] = (float)gBoltZ;
        for (int k = 1; k <= N; k++) { PX[k] = PX[k - 1] + rnd() * 3.2f; PZ[k] = PZ[k - 1] + rnd() * 3.2f; PY[k] = top - dy * k; }
        const float ex = (float)gBoltX - PX[N], ez = (float)gBoltZ - PZ[N];   // o fim acerta exatamente o ponto de impacto
        for (int k = 1; k <= N; k++) { const float f = (float)k / N; PX[k] += ex * f; PZ[k] += ez * f; }
        struct Br { float x[9], y[9], z[9]; }; Br br[4]; int nb = 0;
        for (int k = 8; k < N - 4 && nb < 4; k += 6 + (int)((rnd() + .5f) * 8)) {   // ramos: 8 passos para o lado e para baixo
            Br& b = br[nb++]; b.x[0] = PX[k]; b.y[0] = PY[k]; b.z[0] = PZ[k]; const float sx = rnd() * 4.f, sz = rnd() * 4.f;
            for (int q = 1; q < 9; q++) { b.x[q] = b.x[q - 1] + sx * .6f + rnd() * 1.6f; b.z[q] = b.z[q - 1] + sz * .6f + rnd() * 1.6f; b.y[q] = b.y[q - 1] - dy * .9f; } }
        static const float W[3] = {.55f, .28f, .11f}, A[3] = {.18f, .35f, 1.f};
        glBegin(GL_QUADS);
        for (int L = 0; L < 3; L++) { if (gDim == 3) glColor4f(1.f, L == 2 ? .85f : .12f, L == 2 ? .85f : .15f, A[L] * fa); else glColor4f(L == 2 ? 1.f : .72f, L == 2 ? 1.f : .74f, 1.f, A[L] * fa);
            for (int k = 0; k < N; k++) seg(PX[k], PY[k], PZ[k], PX[k + 1], PY[k + 1], PZ[k + 1], W[L]);
            for (int r = 0; r < nb; r++) for (int q = 0; q < 8; q++) seg(br[r].x[q], br[r].y[q], br[r].z[q], br[r].x[q + 1], br[r].y[q + 1], br[r].z[q + 1], W[L] * .55f); }
        glEnd(); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    if (gRain >= .02f) {   // patchWx.11: chuva/neve como o renderRainSnow do 1.12.2: uma faixa de 1 bloco por coluna (raio 10) com environment/rain.png e snow.png rolando, alfa cai com a distancia
        constexpr int R = 10, NN = 2 * R + 1; static int top[NN * NN]; static int cX = 1 << 30, cZ = 0; static float cT = -9.f;
        static GLuint rainT = 0, snowT = 0;
        if (!rainT) {   // texturas do pack (repetem em V); sem pack: listras procedurais
            auto mk = [&](const char* rel, bool snowTex) { std::vector<uint8_t> px; int w = 0, h = 0; GLuint g;
                if (packRGBA(rel, px, w, h) && w >= 1 && h >= 1) g = upTex(px.data(), w, h);
                else { w = h = 32; px.assign(w * h * 4, 0); for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { const bool on = snowTex ? ((x * 7 + y * 13) % 29 == 0) : ((x % 8 == 3) && ((y + x * 5) % 14 < 9)); uint8_t* q = &px[(y * w + x) * 4]; q[0] = q[1] = q[2] = 255; q[3] = on ? (snowTex ? 200 : 150) : 0; } g = upTex(px.data(), w, h); }
                glBindTexture(GL_TEXTURE_2D, g); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); return g; };
            rainT = mk("environment/rain.png", false); snowT = mk("environment/snow.png", true);
        }
        const int cx = (int)floor(px), cz = (int)floor(pz), cy = (int)floor(py); const bool snow = wxCold(px, pz);
        if (t - cT > .5f || cx != cX || cz != cZ) {
            cT = t; cX = cx; cZ = cz;
            for (int dz = -R; dz <= R; dz++) for (int dx = -R; dx <= R; dx++) { int y = std::min(255, cy + 24); while (y > 0 && getB(cx + dx, y, cz + dz) == AIR) y--; top[(dz + R) * NN + dx + R] = y + 1; }
        }
        const float inten = gRain * (.8f + .2f * gThunder), tk = t * 20.f; const float lum = std::max(.3f, std::min(1.f, gSkyScale));
        auto hsh = [&](int x, int z, float a) { const float v = sinf(x * 12.9898f + z * 78.233f + a * 5.1f) * 43758.5453f; return v - floorf(v); };
        rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, snow ? snowT : rainT);
        glBegin(GL_QUADS);
        for (int dz = -R; dz <= R; dz++) for (int dx = -R; dx <= R; dx++) {
            const int X = cx + dx, Z = cz + dz, rainY = top[(dz + R) * NN + dx + R];
            const int y0 = std::max(cy - R, rainY), y1 = std::max(cy + R, rainY); if (y0 == y1) continue;
            const float ex = X + .5f - (float)px, ez = Z + .5f - (float)pz, d = sqrtf(ex * ex + ez * ez), f = d / R; if (f >= 1.f) continue;
            const float rx = d > .01f ? -ez / d : 1.f, rz = d > .01f ? ex / d : 0.f, hw = .5f;   // faixa de 1 bloco voltada para a camera
            const unsigned ux = (unsigned)X, uz = (unsigned)Z; const unsigned hh = (ux * ux * 3121u + ux * 45238971u + uz * uz * 418711u + uz * 13761u) & 31u;
            const float rn = hsh(X, Z, 1.f), cxm = X + .5f, czm = Z + .5f;
            float u0 = 0.f, vo, a, col = lum;
            if (snow) {
                vo = -((float)((int)tk & 511) + fmodf(tk, 1.f)) / 512.f * (1.f + rn) + hsh(X, Z, 4.f); u0 = hsh(X, Z, 2.f) + sinf(t * .6f + hsh(X, Z, 3.f) * 6.f) * .05f;
                a = ((1.f - f * f) * .3f + .5f) * inten; col = std::min(1.f, lum * .85f + .15f);
            } else {
                vo = ((float)(((int)tk + (int)hh) & 31) + fmodf(tk, 1.f)) / 32.f * (3.f + rn); a = ((1.f - f * f) * .5f + .5f) * inten;
            }
            if (gDim == 3) glColor4f(col, col * .06f, col * .08f, std::min(1.f, a)); else glColor4f(col, col, col, std::min(1.f, a));   // chuva de sangue
            const float v0 = y0 * .25f + vo, v1 = y1 * .25f + vo;
            glTexCoord2f(u0, v0); glVertex3f(cxm - rx * hw, (float)y0, czm - rz * hw);
            glTexCoord2f(u0 + 1.f, v0); glVertex3f(cxm + rx * hw, (float)y0, czm + rz * hw);
            glTexCoord2f(u0 + 1.f, v1); glVertex3f(cxm + rx * hw, (float)y1, czm + rz * hw);
            glTexCoord2f(u0, v1); glVertex3f(cxm - rx * hw, (float)y1, czm - rz * hw);
        }
        glEnd();
        if (!snow) wxSplashDraw(px, py, pz, t);
    }
    rdDepthMask(GL_TRUE); rdDisable(GL_BLEND); rdEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
}
