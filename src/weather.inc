// patchWx.1: clima (limpo/chuva/tempestade; neve em bioma frio). Estado em biome_light.inc; sincronizado do host nos bits 2-3 do NP_TIME.
static bool fireBaseOk(int x, int y, int z);   // nether.inc
static bool wxSkyOpen(int x, int y, int z) { for (int yy = y + 1; yy <= 255 && yy <= y + 80; yy++) if (getB(x, yy, z) != AIR) return false; return true; }
static bool wxCold(double x, double z) { float t, h; climateAt((float)x, (float)z, &t, &h); return t < .25f; }
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
    if (gWeather == 2 && --gBoltCd <= 0) {   // raio: so visual (clarao + traco); cai perto do jogador
        gBoltCd = 200 + rand() % 600; gBoltX = px + (rand() % 81) - 40; gBoltZ = pz + (rand() % 81) - 40; gBoltT = 8; gFlash = 1.f; gBoltSeed = rand();
        int y = 255; while (y > 0 && getB((int)floor(gBoltX), y, (int)floor(gBoltZ)) == AIR) y--; gBoltY = y + 1;
        {   // patchWx.5: o raio causa dano (5) a quem esta a menos de 3 blocos e ateia fogo no ponto atingido (como o MC; so o host/solo coloca o fogo)
            const double ddx = px - gBoltX, ddz = pz - gBoltZ, ddy = py - gBoltY; if (ddx * ddx + ddz * ddz < 9.0 && ddy > -4.0 && ddy < 6.0 && !creative) survDamage(5, "foi atingido por um raio");
            const int bx = (int)floor(gBoltX), bz = (int)floor(gBoltZ), by = (int)gBoltY;
            if ((!netOn || netHost) && by > 1 && by < CH - 1 && gBoltCd % 3 != 0 && getB(bx, by, bz) == AIR && fireBaseOk(bx, by, bz)) { setBM(bx, by, bz, XB_FIRE, 0); netSendBlock(bx, by, bz, XB_FIRE); }
        }
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
        for (int q = 0; q < 3; q++) c[q] = std::min(1.f, (c[q] + (l - c[q]) * k * 1.2f) * (1.f - k * .55f) + gFlash * .55f);
    }
    gSkyScale *= 1.f - gRain * .22f - gThunder * .1f; gSkyScale = std::min(1.f, gSkyScale + gFlash * .4f);
}
static void wxDraw(double px, double py, double pz, float t) {
    if (gDim != 0 || (gRain < .02f && gBoltT <= 0)) return;
    glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    if (gBoltT > 0) {   // traco do raio (8 segmentos irregulares)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); glBegin(GL_QUADS); glColor4f(.85f, .88f, 1.f, std::min(1.f, gBoltT / 5.f));
        float x = (float)gBoltX, z = (float)gBoltZ, y = (float)gBoltY + 90.f; const float dy = (y - (float)gBoltY) / 8.f; srand(gBoltSeed);
        for (int i = 0; i < 8; i++) { const float nx = x + ((rand() % 100) - 50) * .04f, nz = z + ((rand() % 100) - 50) * .04f, w = .18f;
            glVertex3f(x - w, y, z); glVertex3f(x + w, y, z); glVertex3f(nx + w, y - dy, nz); glVertex3f(nx - w, y - dy, nz);
            glVertex3f(x, y, z - w); glVertex3f(x, y, z + w); glVertex3f(nx, y - dy, nz + w); glVertex3f(nx, y - dy, nz - w); x = nx; z = nz; y -= dy; }
        glEnd(); srand((unsigned)SDL_GetTicks()); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    if (gRain >= .02f) {
        static int top[17 * 17]; static int cX = 1 << 30, cZ = 0; static float cT = -9.f;
        const int cx = (int)floor(px), cz = (int)floor(pz), cy = (int)floor(py); const bool snow = wxCold(px, pz);
        if (t - cT > .5f || cx != cX || cz != cZ) {
            cT = t; cX = cx; cZ = cz;
            for (int dz = -8; dz <= 8; dz++) for (int dx = -8; dx <= 8; dx++) { int y = std::min(255, cy + 24); while (y > 0 && getB(cx + dx, y, cz + dz) == AIR) y--; top[(dz + 8) * 17 + dx + 8] = y + 1; }
        }
        glBegin(GL_QUADS);
        for (int dz = -8; dz <= 8; dz++) for (int dx = -8; dx <= 8; dx++) for (int k = 0; k < 2; k++) {
            const int id = (dz + 8) * 17 + dx + 8;
            auto hs = [&](float a) { const float v = sinf((cx + dx) * 12.9898f + (cz + dz) * 78.233f + k * 37.7f + a * 5.1f) * 43758.5453f; return v - floorf(v); };
            if (hs(1.f) > gRain * .55f) continue;
            const float ph = hs(2.f) + t * (snow ? .18f + .1f * hs(3.f) : 1.5f + .5f * hs(3.f)), f = ph - floorf(ph);
            const float yT = (float)cy + 14.f, yy = yT - f * 26.f; if (yy < (float)top[id]) continue;
            float x = (float)(cx + dx) + hs(4.f), z = (float)(cz + dz) + hs(5.f);
            if (snow) { x += sinf(t * .9f + hs(6.f) * 6.f) * .3f; const float s = .035f; glColor4f(1.f, 1.f, 1.f, .85f);
                glVertex3f(x - s, yy, z); glVertex3f(x + s, yy, z); glVertex3f(x + s, yy + s * 2, z); glVertex3f(x - s, yy + s * 2, z);
                glVertex3f(x, yy, z - s); glVertex3f(x, yy, z + s); glVertex3f(x, yy + s * 2, z + s); glVertex3f(x, yy + s * 2, z - s); }
            else { const float w = .012f, L = 1.1f; glColor4f(.62f, .72f, .95f, .38f);
                glVertex3f(x - w, yy, z); glVertex3f(x + w, yy, z); glVertex3f(x + w + .03f, yy + L, z); glVertex3f(x - w + .03f, yy + L, z);
                glVertex3f(x, yy, z - w); glVertex3f(x, yy, z + w); glVertex3f(x + .03f, yy + L, z + w); glVertex3f(x + .03f, yy + L, z - w); }
        }
        glEnd();
    }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
}
