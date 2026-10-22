// patchWx.1: clima (limpo/chuva/tempestade; neve em bioma frio). Estado em biome_light.inc; sincronizado do host nos bits 2-3 do NP_TIME.
static bool fireBaseOk(int x, int y, int z);   // nether.inc
static bool wxSkyOpen(int x, int y, int z) { for (int yy = y + 1; yy <= 255 && yy <= y + 80; yy++) if (getB(x, yy, z) != AIR) return false; return true; }
static bool wxCold(double x, double z) { float t, h; climateAt((float)x, (float)z, &t, &h); return t < .25f; }
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
    if (!gSndOn || gDim != 0 || gRain < .05f || wxCold(px, pz)) { gRainBuf = nullptr; return; }
    for (const SndVoice& v : gSndV) if (v.b && v.b == gRainBuf && !v.music && v.pos < v.b->frames - .25 * v.b->rate) return;   // ainda tocando (emenda 0,25 s antes do fim)
    static int roofT = 0; static bool open = true; if (--roofT <= 0) { roofT = 10; open = wxSkyOpen((int)floor(px), (int)floor(py + 1.6), (int)floor(pz)); }
    const SndEvt* e = sndEvtGet("weather.rain"); if (!e) return; const std::string& n = e->f[rand() % e->f.size()];
    SndBuf* bf = sndGet(n); if (!bf) return; gRainBuf = bf; const float vol = e->vol * std::min(1.f, gRain * 1.2f) * (open ? 1.f : .3f);
    sndPlayBuf(bf, vol, vol, 1.f, false);
}
static void wxTick(double px, double py, double pz) {
    if (gDim != 0) { gRain = std::max(0.f, gRain - .02f); gThunder = std::max(0.f, gThunder - .02f); gFlash = 0; return; }
    if (!netOn || netHost) {
        if (--gWxLeft <= 0) {
            if (gWeather == 0) { gWeather = (rand() % 4 == 0) ? 2 : 1; gWxLeft = 12000 + rand() % 12000; } else { gWeather = 0; gWxLeft = 12000 + rand() % 168000; }
            if (netOn) netSendTime(false);
        }
    }
    gRain += ((gWeather > 0 ? 1.f : 0.f) - gRain) * .01f; if (fabsf(gRain - (gWeather > 0 ? 1.f : 0.f)) < .004f) gRain = gWeather > 0 ? 1.f : 0.f;
    gThunder += ((gWeather == 2 ? 1.f : 0.f) - gThunder) * .01f;
    gFlash = std::max(0.f, gFlash - .06f); if (gBoltT > 0) gBoltT -= 1;
    wxRainSound(px, py, pz); if (gThT >= 0 && --gThT < 0) sndEvtUi("weather.thunder", gThVol);
    if (gBoltPend) { gBoltPend = false; gBoltX = gBoltPx; gBoltY = gBoltPy; gBoltZ = gBoltPz; gBoltSeed = gBoltPSeed; gBoltT = 8; gFlash = 1.f; wxBoltStrike(px, py, pz); }   // convidado: raio do host
    if (gWeather == 2 && (!netOn || netHost) && --gBoltCd <= 0) {   // raio: cai perto de um jogador aleatorio (host ou convidado no mundo); convidados recebem NP_BOLT
        double ax = px, az = pz; if (netOn) { std::vector<std::pair<double, double>> pl; pl.push_back({px, pz}); for (const auto& kv : remotes) if (kv.second.init && kv.second.dim == 0 && kv.second.deathT < 0) pl.push_back({kv.second.x, kv.second.z}); const auto& c = pl[rand() % pl.size()]; ax = c.first; az = c.second; }
        gBoltCd = 200 + rand() % 600; gBoltX = ax + (rand() % 81) - 40; gBoltZ = az + (rand() % 81) - 40; gBoltT = 8; gFlash = 1.f; gBoltSeed = rand();
        int y = 255; while (y > 0 && getB((int)floor(gBoltX), y, (int)floor(gBoltZ)) == AIR) y--; gBoltY = y + 1;
        wxBoltStrike(px, py, pz);
        if (netOn) { NBuf nb = netMk(NP_BOLT); nb.put<float>((float)gBoltX); nb.put<float>((float)gBoltY); nb.put<float>((float)gBoltZ); nb.put<int32_t>(gBoltSeed); netBroadcast(nb); }
    }
    if (gRain > .5f && (!netOn || netHost) && (gWorldTime % 20) == 0 && !wxCold(px, pz)) {   // chuva enche o caldeirao (so com ceu aberto)
        static int row = 0; const int z = (int)floor(pz) - 16 + row; row = (row + 1) % 33;   // varre 1 fileira (33 colunas) por vez; cada caldeirao e testado a cada ~33 s
        for (int x = (int)floor(px) - 16; x <= (int)floor(px) + 16; x++) for (int y = (int)floor(py) + 12; y >= (int)floor(py) - 12; y--) {
            if (getB(x, y, z) != XB_DECOR) continue; const int m = getM(x, y, z); if ((m & 3) != 0 || ((m >> 2) & 3) >= 3) continue;
            if (rand() % 2 || !wxSkyOpen(x, y, z)) continue;
            const int nm = (m & 3) | ((((m >> 2) & 3) + 1) << 2); setBM(x, y, z, XB_DECOR, nm); netSendBlock(x, y, z, XB_DECOR, nm);
        }
    }
}
static void wxSkyApply(float* top, float* hor) {   // escurece/acinzenta o ceu e a neblina; clarao do raio
    const float k = gRain * .45f + gThunder * .2f; if (k <= 0.f && gFlash <= 0.f) return;
    for (float* c : {top, hor}) {
        const float l = (c[0] + c[1] + c[2]) * .3333f * .75f;
        for (int q = 0; q < 3; q++) c[q] = std::min(1.f, (c[q] + (l - c[q]) * k * 1.2f) * (1.f - k * .55f) + gFlash * .8f);
    }
    gSkyScale *= 1.f - gRain * .22f - gThunder * .1f; gSkyScale = std::min(1.f, gSkyScale + gFlash * .6f);
}
static void wxDraw(double px, double py, double pz, float t) {
    if (gDim != 0 || (gRain < .02f && gBoltT <= 0)) return;
    glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    if (gBoltT > 0) {   // patchWx.10: raio estilo Minecraft: caminho aleatorio fino em 40 passos, 3 camadas de brilho (largas e fracas -> finas e brancas) e ramos laterais
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); const float fa = std::min(1.f, gBoltT / 5.f);
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
        for (int L = 0; L < 3; L++) { glColor4f(L == 2 ? 1.f : .72f, L == 2 ? 1.f : .74f, 1.f, A[L] * fa);
            for (int k = 0; k < N; k++) seg(PX[k], PY[k], PZ[k], PX[k + 1], PY[k + 1], PZ[k + 1], W[L]);
            for (int r = 0; r < nb; r++) for (int q = 0; q < 8; q++) seg(br[r].x[q], br[r].y[q], br[r].z[q], br[r].x[q + 1], br[r].y[q + 1], br[r].z[q + 1], W[L] * .55f); }
        glEnd(); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    if (gRain >= .02f) {   // patchWx.8: chuva nova = faixas que acompanham a camera (cauda some), vento, mais densa, respingo no chao; neve com flocos que giram
        constexpr int R = 10, NN = 2 * R + 1; static int top[NN * NN]; static int cX = 1 << 30, cZ = 0; static float cT = -9.f;
        const int cx = (int)floor(px), cz = (int)floor(pz), cy = (int)floor(py); const bool snow = wxCold(px, pz);
        if (t - cT > .5f || cx != cX || cz != cZ) {
            cT = t; cX = cx; cZ = cz;
            for (int dz = -R; dz <= R; dz++) for (int dx = -R; dx <= R; dx++) { int y = std::min(255, cy + 24); while (y > 0 && getB(cx + dx, y, cz + dz) == AIR) y--; top[(dz + R) * NN + dx + R] = y + 1; }
        }
        const float inten = gRain * (.8f + .2f * gThunder), wind = .1f + .12f * gThunder, wsx = .8f * wind, wsz = .35f * wind;   // inclinacao por unidade de queda
        auto strip = [&](float x0, float z0, float x1, float z1, float y) { glVertex3f(x0, y, z0); glVertex3f(x1, y, z0); glVertex3f(x1, y, z1); glVertex3f(x0, y, z1); };
        glBegin(GL_QUADS);
        for (int dz = -R; dz <= R; dz++) for (int dx = -R; dx <= R; dx++) for (int k = 0; k < 3; k++) {
            const int id = (dz + R) * NN + dx + R;
            auto hs = [&](float a) { const float v = sinf((cx + dx) * 12.9898f + (cz + dz) * 78.233f + k * 37.7f + a * 5.1f) * 43758.5453f; return v - floorf(v); };
            if (hs(1.f) > inten * (snow ? .55f : .62f) * (k == 2 ? .6f : 1.f)) continue;
            const float gt = (float)top[id];
            float x = (float)(cx + dx) + hs(4.f), z = (float)(cz + dz) + hs(5.f);
            const float ex = x - (float)px, ez = z - (float)pz, d = sqrtf(ex * ex + ez * ez), fade = std::max(0.f, 1.f - d / (R + .5f));
            if (fade <= 0.f) continue;
            if (snow) {
                const float ph = hs(2.f) + t * (.18f + .1f * hs(3.f)), f = ph - floorf(ph); const float yT = (float)cy + 14.f, yy = yT - f * 26.f; if (yy < gt) continue;
                x += sinf(t * .9f + hs(6.f) * 6.f) * .3f; z += cosf(t * .7f + hs(7.f) * 6.f) * .2f; const float s = .03f + .02f * hs(8.f), a = .85f * std::min(1.f, fade * 1.6f);
                glColor4f(1.f, 1.f, 1.f, a);
                glVertex3f(x - s, yy, z); glVertex3f(x + s, yy, z); glVertex3f(x + s, yy + s * 2, z); glVertex3f(x - s, yy + s * 2, z);
                glVertex3f(x, yy, z - s); glVertex3f(x, yy, z + s); glVertex3f(x, yy + s * 2, z + s); glVertex3f(x, yy + s * 2, z - s);
                continue;
            }
            const float ph = hs(2.f) + t * (2.1f + .6f * hs(3.f)), f = ph - floorf(ph); const float yT = (float)cy + 16.f, yy = yT - f * 30.f;
            if (yy >= gt) {   // gota: faixa voltada para a camera, cabeca embaixo (clara) e cauda que some
                const float L = .8f + .7f * hs(9.f), rx = d > .01f ? -ez / d : 1.f, rz = d > .01f ? ex / d : 0.f, w = .014f, a = .5f * fade;
                const float tx = x + wsx * L, tz = z + wsz * L, ty = yy + L;
                glColor4f(.72f, .80f, 1.f, a); glVertex3f(x - rx * w, yy, z - rz * w); glVertex3f(x + rx * w, yy, z + rz * w);
                glColor4f(.72f, .80f, 1.f, 0.f); glVertex3f(tx + rx * w, ty, tz + rz * w); glVertex3f(tx - rx * w, ty, tz - rz * w);
            } else if (yy > gt - 1.2f) {   // respingo no chao/agua: anel que abre e some
                const float u = (gt - yy) / 1.2f, sz = .05f + .17f * u, th = .014f, y = gt + .03f; glColor4f(.85f, .9f, 1.f, .5f * (1.f - u) * fade);
                strip(x - sz, z + sz - th, x + sz, z + sz, y); strip(x - sz, z - sz, x + sz, z - sz + th, y); strip(x - sz, z - sz + th, x - sz + th, z + sz - th, y); strip(x + sz - th, z - sz + th, x + sz, z + sz - th, y);
            }
        }
        glEnd();
    }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
}
