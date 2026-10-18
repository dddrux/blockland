// ================= ENDER DRAGON (patchEnd.1) =================
// Entidade propria (fora de MOBS): modelo ModelDragon simplificado (caixas, textura entity/enderdragon/dragon.png 256x256), 200 de vida,
// voa em circulo no End e mergulha no jogador (10 de dano). Cabeca = dano total; corpo/asas = 1/4+1. Ao morrer (10 s) surge o ovo do dragao sobre
// um pilar de bedrock em (0,topo,0). So o host/solo simula; nao ha sincronia de rede (VIS: nada visto rodando).
struct DragonEnt { bool on = false; double x = 0, y = 90, z = 60, px = 0, py = 90, pz = 60, vx = 0, vy = 0, vz = 0; float yaw = 0, pyaw = 0, hp = 200.f, wing = 0, pwing = 0, ang = 0; int dead = 0, hurtT = 0, atkCd = 0, phase = 0, phT = 0; };
static void crystalsTick(Phys& P); static bool crystalHit(const float* eye, const float* dir, float reach, int dmg); static int crystalsAlive(double x, double y, double z, double r);
static DragonEnt gDragon; static bool gDragonKilled = false; static GLuint gDragonTex = 0;
static void dragonWorldSave(const std::string& dir) { const std::string p = "saves/" + dir + "/dragon.dat"; if (gDragonKilled) { FILE* f = fopen(p.c_str(), "wb"); if (f) { fputs("1", f); fclose(f); } } else remove(p.c_str()); }
static void dragonWorldLoad(const std::string& dir) { FILE* f = fopen(("saves/" + dir + "/dragon.dat").c_str(), "rb"); gDragonKilled = f != nullptr; if (f) fclose(f); gDragon = DragonEnt(); }
static void dragonEggSpawn() {
    const int egg = pkFind("block/dragon_egg"); if (!egg) return; int top = 0;
    for (int y = CH - 2; y > 0; y--) { const int b = getB(0, y, 0); if (b != AIR && !isLiquid(b)) { top = y; break; } }
    if (!getC(0, 0)) return;
    for (int i = 1; i <= 3; i++) { setBM(0, top + i, 0, BEDROCK, 0); netSendBlock(0, top + i, 0, BEDROCK, 0); }
    setBM(0, top + 4, 0, egg, 0); netSendBlock(0, top + 4, 0, egg, 0);
}
static void dragonApplyDmg(int dmg) { DragonEnt& d = gDragon; if (!d.on || d.dead > 0) return; d.hp -= (float)dmg; d.hurtT = 10; if (d.hp <= 0.f) { d.dead = 1; d.vy = 0; d.vx = d.vz = 0; } }
// patchNet.1: o host manda o estado a cada tick (NP_DRAGON); convidado desenha e, ao bater, manda NP_DRAGONHIT; mordida em convidado = NP_DRAGONBITE
static void dragonNetSend() {
    if (!netOn || !netHost) return; static bool was = false; const DragonEnt& d = gDragon; if (!d.on && !was) return; was = d.on;
    NBuf b = netMk(NP_DRAGON); b.put<uint8_t>(d.on ? 1 : 0);
    if (d.on) { b.put<float>((float)d.x); b.put<float>((float)d.y); b.put<float>((float)d.z); b.put<float>(d.yaw); b.put<float>(d.wing); b.put<int16_t>((int16_t)d.hp); b.put<uint8_t>((uint8_t)std::min(255, d.dead)); b.put<uint8_t>((uint8_t)d.hurtT); }
    netBroadcast(b);
}
static void dragonNetRecv(int type, NBuf& b, int sender) {
    DragonEnt& d = gDragon;
    if (type == NP_DRAGON) {
        if (netHost || sender != 0) return; const int on = b.get<uint8_t>(); if (!on) { d.on = false; return; }
        const float x = b.get<float>(), y = b.get<float>(), z = b.get<float>(), yw = b.get<float>(), wg = b.get<float>(); const int hp = b.get<int16_t>(), dead = b.get<uint8_t>(), ht = b.get<uint8_t>();
        const bool was = d.on; d.px = was ? d.x : x; d.py = was ? d.y : y; d.pz = was ? d.z : z; d.pyaw = was ? d.yaw : yw; d.pwing = was ? d.wing : wg;
        d.x = x; d.y = y; d.z = z; d.yaw = yw; d.wing = wg; d.hp = (float)hp; d.dead = dead; d.hurtT = ht; d.on = true;
    } else if (type == NP_DRAGONHIT) {
        const int dmg = b.get<int16_t>(); if (netHost && sender != 0) dragonApplyDmg(std::max(0, std::min(40, dmg)));
    } else if (type == NP_DRAGONBITE) {
        const int tid = b.get<uint8_t>(); const float kx = b.get<float>(), kz = b.get<float>();
        if (!netHost && sender == 0 && tid == netSelf && gPvpP && !creative && !gDead && survDamage(10, "Foi morto pelo Ender Dragon")) { gPvpP->mx += kx; gPvpP->mz += kz; gPvpP->my += .6; }
    }
}
static void dragonTick(Phys& P) {
    DragonEnt& d = gDragon; crystalsTick(P);   // patchEnd.8: cristais tambem no convidado (estado = bloco de fogo, que ja sincroniza)
    if (gDim != 2) { if (!(netOn && !netHost)) d.on = false; return; }
    if (netOn && !netHost) { if (d.hurtT > 0 && d.on) d.hurtT--; return; }   // convidado: so desenha o que o host manda
    if (!d.on) { if (gDragonKilled) return; d = DragonEnt(); d.on = true; d.x = d.px = 0; d.z = d.pz = 70; d.y = d.py = 85; }
    d.px = d.x; d.py = d.y; d.pz = d.z; d.pyaw = d.yaw; d.pwing = d.wing;
    if (d.dead == 0 && d.hp < 200.f && (gWorldTime & 7) == 0 && crystalsAlive(d.x, d.y, d.z, 90.0)) d.hp = std::min(200.f, d.hp + 1.f);   // patchEnd.5: cristais curam 1 HP a cada 8 ticks
    if (d.hurtT > 0) d.hurtT--; if (d.atkCd > 0) d.atkCd--;
    if (d.dead > 0) {   // morte: afunda girando; ao fim solta o ovo
        d.dead++; d.vy -= .01; d.y += d.vy * .3; d.yaw += 4.f; d.wing += .3f;
        if (d.dead == 200) { dragonEggSpawn(); survAddXp(gDragonKilled ? 500 : 12000); gDragonKilled = true; d.on = false; }   // patchEnd.3: XP (12000 na 1a morte, 500 nas outras)
        dragonNetSend(); return;
    }
    d.wing += .55f; d.phT++;
    double TX = P.x, TY = P.y, TZ = P.z; int tid = -1; bool tOk = !gDead && gGm != 3 && !(creative && gGm == 1);   // tid -1 = host
    double pd = tOk ? sqrt((P.x - d.x) * (P.x - d.x) + (P.y + 1. - d.y) * (P.y + 1. - d.y) + (P.z - d.z) * (P.z - d.z)) : 1e9;
    if (netOn) for (const auto& kv : remotes) { const RemotePl& r = kv.second; if (!r.init || r.dim != 2 || r.gm == 3 || r.gm == 1 || r.deathT >= 0) continue;
        const double q = sqrt((r.x - d.x) * (r.x - d.x) + (r.y + 1. - d.y) * (r.y + 1. - d.y) + (r.z - d.z) * (r.z - d.z)); if (q < pd) { pd = q; TX = r.x; TY = r.y; TZ = r.z; tid = kv.first; tOk = true; } }
    double tx, ty, tz;
    if (d.phase == 0) {   // circula a ilha central
        d.ang += .02f; tx = cos(d.ang) * 55.; tz = sin(d.ang) * 55.; ty = 80. + 8. * sin(d.ang * 3.);
        if (d.phT > 300 && pd < 120. && tOk) { d.phase = 1; d.phT = 0; }
    } else {              // mergulho no jogador
        tx = TX; ty = TY + 1.; tz = TZ;
        if (d.phT > 140 || (pd < 6. && d.atkCd > 0)) { d.phase = 0; d.phT = 0; d.ang = atan2(d.z, d.x); }
    }
    const double dx = tx - d.x, dy = ty - d.y, dz = tz - d.z, dl = std::max(1., sqrt(dx * dx + dy * dy + dz * dz)), sp = d.phase ? 1.1 : .75;
    d.vx += (dx / dl * sp - d.vx) * .1; d.vy += (dy / dl * sp - d.vy) * .1; d.vz += (dz / dl * sp - d.vz) * .1;
    d.x += d.vx; d.y += d.vy; d.z += d.vz;
    if (fabs(d.vx) + fabs(d.vz) > .05) { const float ty2 = (float)(atan2(-d.vx, -d.vz) * 57.29578); float df = ty2 - d.yaw; while (df > 180.f) df -= 360.f; while (df < -180.f) df += 360.f; d.yaw += df * .15f; }
    // mordida/choque: cabeca a ~5 blocos a frente
    const double hx = d.x - sin(d.yaw * .01745329) * 5., hz = d.z - cos(d.yaw * .01745329) * 5.;
    if (d.atkCd == 0 && tOk) {   // mordida no alvo mais proximo; convidado leva por NP_DRAGONBITE
        const double qx = TX - hx, qy = (TY + 1.) - (d.y + 1.), qz = TZ - hz;
        if (qx * qx + qy * qy + qz * qz < 4.5 * 4.5) {
            d.atkCd = 40; const double l = std::max(.1, sqrt(qx * qx + qz * qz));
            if (tid < 0) { P.mx += qx / l * 1.4; P.mz += qz / l * 1.4; P.my += .6; survDamage(10, "Foi morto pelo Ender Dragon"); }
            else { NBuf nb = netMk(NP_DRAGONBITE); nb.put<uint8_t>((uint8_t)tid); nb.put<float>((float)(qx / l * 1.4)); nb.put<float>((float)(qz / l * 1.4)); netBroadcast(nb); }
        }
    }
    dragonNetSend();
}
// clique esquerdo: raio contra a cabeca (esfera r3) e o corpo (r4.5); devolve true se acertou
static bool dragonHit(const float* eye, const float* dir, int held, bool crit, float reach) {
    if (gDim == 2 && crystalHit(eye, dir, reach, mobWeaponDmg(held))) return true;   // patchEnd.5: bater no cristal o destroi
    DragonEnt& d = gDragon; if (!d.on || d.dead > 0 || gDim != 2) return false;
    const double hx = d.x - sin(d.yaw * .01745329) * 6., hz = d.z - cos(d.yaw * .01745329) * 6.;
    const double C[2][4] = {{hx, d.y + 1., hz, 3.}, {d.x, d.y + 1., d.z, 4.5}};
    int hitI = -1; double best = 1e9;
    for (int i = 0; i < 2; i++) {
        const double ox = C[i][0] - eye[0], oy = C[i][1] - eye[1], oz = C[i][2] - eye[2], t = ox * dir[0] + oy * dir[1] + oz * dir[2];
        if (t < 0) continue; const double px2 = ox - dir[0] * t, py2 = oy - dir[1] * t, pz2 = oz - dir[2] * t;
        if (px2 * px2 + py2 * py2 + pz2 * pz2 > C[i][3] * C[i][3]) continue;
        const double tt = t - C[i][3]; if (tt > reach + 1.0) continue; if (t < best) { best = t; hitI = i; }
    }
    if (hitI < 0) return false;
    int dmg = mobWeaponDmg(held); if (crit) dmg = (int)(dmg * 1.5f); if (hitI == 1) dmg = dmg / 4 + 1;
    if (netOn && !netHost) { NBuf nb = netMk(NP_DRAGONHIT); nb.put<int16_t>((int16_t)dmg); netBroadcast(nb); d.hurtT = 10; return true; }
    dragonApplyDmg(dmg); return true;
}
static void dragonBox(float u, float v, float x0, float y0, float z0, float w, float h, float d) {   // ModelBox 256x256; coords em px do modelo (y para baixo -> invertido ao desenhar)
    const float x1 = x0 + w, y1 = y0 + h, z1 = z0 + d, k = 1.f / 256.f;
    auto Q = [&](float a0, float b0, float a1, float b1, float X0, float Y0, float Z0, float X1, float Y1, float Z1, float X2, float Y2, float Z2, float X3, float Y3, float Z3) {
        glTexCoord2f(a0 * k, b0 * k); glVertex3f(X0, -Y0, Z0); glTexCoord2f(a1 * k, b0 * k); glVertex3f(X1, -Y1, Z1);
        glTexCoord2f(a1 * k, b1 * k); glVertex3f(X2, -Y2, Z2); glTexCoord2f(a0 * k, b1 * k); glVertex3f(X3, -Y3, Z3); };
    const float u1 = u + d, u2 = u1 + w, u3 = u2 + d, u4 = u3 + w, v1 = v + d, v2 = v1 + h;
    Q(u1, v, u2, v1, x1, y0, z1, x0, y0, z1, x0, y0, z0, x1, y0, z0);        // topo
    Q(u2, v1, u3, v, x1, y1, z0, x0, y1, z0, x0, y1, z1, x1, y1, z1);        // base
    if (h <= 0.f) return;
    Q(u1, v1, u2, v2, x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0);       // frente (-z)
    Q(u3, v1, u4, v2, x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1);       // costas (+z)
    Q(u2, v1, u3, v2, x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1);       // +x
    Q(u, v1, u1, v2, x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0);        // -x
}
static GLuint gDragonEyeTex = 0;
static void drawDragonPass(float pt, bool eyes) {   // patchEnd.11: passada 2 = dragon_eyes.png aditivo sobre o mesmo modelo
    const DragonEnt& d = gDragon; if (!d.on || gDim != 2) return; if (eyes && d.dead) return;
    if (eyes && !gDragonEyeTex) {
        std::vector<uint8_t> px; int w = 0, h = 0; if (!packRGBA("entity/enderdragon/dragon_eyes.png", px, w, h)) { w = h = 256; px.assign(256 * 256 * 4, 0); }
        glGenTextures(1, &gDragonEyeTex); glBindTexture(GL_TEXTURE_2D, gDragonEyeTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    }
    if (!gDragonTex) {
        std::vector<uint8_t> px; int w = 0, h = 0; const bool ok = packRGBA("entity/enderdragon/dragon.png", px, w, h);
        if (!ok) { w = h = 256; px.assign(256 * 256 * 4, 0); for (int i = 0; i < 256 * 256; i++) { px[i * 4] = 60; px[i * 4 + 1] = 40; px[i * 4 + 2] = 70; px[i * 4 + 3] = 255; } }
        glGenTextures(1, &gDragonTex); glBindTexture(GL_TEXTURE_2D, gDragonTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    }
    auto L = [&](double a, double b) { return (float)(a + (b - a) * pt); };
    const float wing = L(d.pwing, d.wing), yaw = L(d.pyaw, d.yaw), fl = sinf(wing) * .6f + .3f, sw = d.dead ? 0.f : sinf(wing * .5f);
    GLint prev = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
    glBindTexture(GL_TEXTURE_2D, eyes ? gDragonEyeTex : gDragonTex); glEnable(GL_TEXTURE_2D); glDisable(GL_CULL_FACE); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .1f); glDisable(GL_BLEND);
    if (eyes) { glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE); glDepthMask(GL_FALSE); glDepthFunc(GL_LEQUAL); }
    const float c = (d.hurtT > 0 && !eyes) ? .55f : 1.f; glColor4f(1.f, c, c, 1.f);
    glPushMatrix(); glTranslatef(L(d.px, d.x), L(d.py, d.y), L(d.pz, d.z)); glRotatef(yaw, 0, 1, 0); glScalef(.125f, .125f, .125f); glTranslatef(0.f, 15.f, -24.f);   // origem no centro do corpo (px)
    glBegin(GL_QUADS);
    dragonBox(0, 0, -12, 3 - 15, -16 + 24 - 24 + 0, 24, 24, 64);   // corpo
    glEnd();
    // pescoco (5 segmentos) + cabeca, em direcao a -z
    for (int i = 0; i < 5; i++) {
        glPushMatrix(); glTranslatef(0.f, 6.f + (i + 1) * 3.f + sw * i * 1.5f, -8.f - 11.f * (i + 1) + 24.f - 24.f); glBegin(GL_QUADS); dragonBox(192, 104, -5, -5, -5, 10, 10, 10); glEnd(); glPopMatrix();
    }
    glPushMatrix(); glTranslatef(0.f, 6.f + 6.f * 3.f + sw * 7.f, -8.f - 66.f); glBegin(GL_QUADS);
    dragonBox(112, 30, -6, -1, -24, 12, 5, 16); dragonBox(112, 0, -5, -3, -22, 2, 2, 4); dragonBox(112, 0, 3, -3, -22, 2, 2, 4); glEnd();
    glRotatef(-(.5f + .5f * sinf(wing * .5f)) * 14.f, 1, 0, 0); glBegin(GL_QUADS); dragonBox(176, 65, -6, 0, -16, 12, 4, 16); glEnd(); glPopMatrix();
    // cauda (12 segmentos)
    for (int i = 0; i < 12; i++) {
        glPushMatrix(); glTranslatef(sinf(wing * .3f - i * .4f) * i * .8f, 12.f - i * .6f, 64.f - 24.f + 3.f + i * 10.f); glBegin(GL_QUADS); dragonBox(192, 104, -5, -5, -5, 10, 10, 10); glEnd(); glPopMatrix();
    }
    // asas (esq/dir): osso + pele + ponta
    for (int s = 0; s < 2; s++) {
        glPushMatrix(); glTranslatef(s ? 12.f : -12.f, 8.f, 2.f - 8.f + 24.f - 24.f + 8.f); if (s) glScalef(-1.f, 1.f, 1.f);
        glRotatef(-fl * 57.29578f, 0, 0, 1);   // bate para cima/baixo
        glBegin(GL_QUADS); dragonBox(112, 88, -56, -4, -4, 56, 8, 8); dragonBox(-56, 88, -56, 0, 2, 56, 0, 56); glEnd();
        glTranslatef(-56.f, 0.f, 0.f); glRotatef(fl * 40.f * 1.f, 0, 0, 1);
        glBegin(GL_QUADS); dragonBox(112, 136, -56, -2, -2, 56, 4, 4); dragonBox(-56, 144, -56, 0, 2, 56, 0, 56); glEnd();
        glPopMatrix();
    }
    // pernas
    for (int s = 0; s < 2; s++) for (int q = 0; q < 2; q++) {
        glPushMatrix(); glTranslatef(s ? 12.f : -12.f, 18.f, q ? 40.f : 4.f); glRotatef(15.f + sw * 8.f, 1, 0, 0); glBegin(GL_QUADS); dragonBox(112, 104, -4, 0, -4, 8, 24, 8); glEnd(); glPopMatrix();
    }
    glPopMatrix(); glColor4f(1.f, 1.f, 1.f, 1.f); glBindTexture(GL_TEXTURE_2D, (GLuint)prev);
    if (eyes) { glDisable(GL_BLEND); glDepthMask(GL_TRUE); glDepthFunc(GL_LEQUAL); glEnable(GL_ALPHA_TEST); }
}
static void drawDragon(float pt) { drawDragonPass(pt, false); drawDragonPass(pt, true); }

// patchEnd.3: barra de vida do dragao no topo da tela (BossInfo): so no End, com o dragao vivo
static void dragonBar(int sw, int G) {
    if (gDim != 2 || !gDragon.on || gDragon.dead > 0) return;
    const float fr = std::max(0.f, std::min(1.f, gDragon.hp / 200.f)); const float bw = 182.f, x = (sw - bw) / 2.f;
    OX = 0; OY = 0; (void)G;
    uText(sw / 2.f - 20.f, 4.f, "Ender Dragon", 0xFFFFFF, true);
    uBox(x, 14.f, bw, 5.f, 1.f, 0x000000);
    if (fr > 0.f) uBox(x + 1.f, 15.f, (bw - 2.f) * fr, 3.f, 1.f, 0xC020C0);
}

// ---- patchEnd.5: Cristais do End (EntityEnderCrystal) no topo dos 10 pilares; curam o dragao; 1 golpe/flecha-explosao os destroi ----
static Chunk* cystChunk(int x, int z) { return getC((int)floor(x / (double)CW), (int)floor(z / (double)CW)); }   // getC usa coordenadas de CHUNK
struct EndCrystal { double x, y, z; bool alive; };
static std::vector<std::array<double, 3>> gCrystBoom; static GLuint gBeamTex = 0;
static EndCrystal gCryst[10]; static bool gCrystInit[10]; static GLuint gCrystTex = 0; static int gCrystT = 0;
static void crystalsTick(Phys& P) {
    for (auto& b : gCrystBoom) explosionAt(b[0], b[1] + .8, b[2], 6.f, P, "explodido por um cristal do End", nullptr);   // explosao real (forca 6, como EntityEnderCrystal)
    gCrystBoom.clear();
    if (gDim != 2) { for (int i = 0; i < 10; i++) gCrystInit[i] = false; return; }
    for (int i = 0; i < 10; i++) if (gCrystInit[i] && gCryst[i].alive && cystChunk((int)floor(gCryst[i].x), (int)floor(gCryst[i].z)) && getB((int)floor(gCryst[i].x), (int)floor(gCryst[i].y), (int)floor(gCryst[i].z)) != XB_FIRE) gCryst[i].alive = false;   // fogo sumiu (outro jogador destruiu): cristal morto
    if (++gCrystT % 20) return;
    for (int j = 0; j < 10; j++) {
        if (gCrystInit[j]) continue;
        const double a = 2.0 * (-3.14159265358979 + 3.14159265358979 / 10.0 * j); const int px = (int)floor(42.0 * cos(a)), pz = (int)floor(42.0 * sin(a));
        if (!cystChunk(px, pz)) continue;
        for (int y = CH - 3; y > 40; y--) if (getB(px, y, pz) == XB_FIRE && getB(px, y - 1, pz) == BEDROCK) { gCryst[j] = {px + .5, y + .2, pz + .5, true}; gCrystInit[j] = true; break; }
    }
}
static int crystalsAlive(double x, double y, double z, double r) { int n = 0; if (gDim != 2) return 0; for (int i = 0; i < 10; i++) if (gCrystInit[i] && gCryst[i].alive) { const double dx = gCryst[i].x - x, dy = gCryst[i].y - y, dz = gCryst[i].z - z; if (dx * dx + dy * dy + dz * dz < r * r) n++; } return n; }
static void crystalKill(int i) {
    EndCrystal& c = gCryst[i]; c.alive = false; gCrystBoom.push_back({c.x, c.y, c.z}); const int bx = (int)floor(c.x), by = (int)floor(c.y), bz = (int)floor(c.z);
    if (getB(bx, by, bz) == XB_FIRE) { setBM(bx, by, bz, AIR, 0); netSendBlock(bx, by, bz, AIR, 0); }
    if (gDragon.on && gDragon.dead == 0) { const double dx = gDragon.x - c.x, dz = gDragon.z - c.z; if (dx * dx + dz * dz < 40.0 * 40.0) gDragon.hp = std::max(1.f, gDragon.hp - 10.f); }   // explosao fere o dragao se estiver perto
}
static bool crystalHit(const float* eye, const float* dir, float reach, int dmg) {
    (void)dmg; int bi = -1; double best = 1e9;
    for (int i = 0; i < 10; i++) {
        if (!gCrystInit[i] || !gCryst[i].alive) continue;
        const double ox = gCryst[i].x - eye[0], oy = gCryst[i].y + .8 - eye[1], oz = gCryst[i].z - eye[2], t = ox * dir[0] + oy * dir[1] + oz * dir[2];
        if (t < 0 || t > reach + 1.5) continue; const double qx = ox - dir[0] * t, qy = oy - dir[1] * t, qz = oz - dir[2] * t;
        if (qx * qx + qy * qy + qz * qz > 1.0) continue;
        if (raySolidDistance(eye, dir, (float)t) >= 0.f) continue;   // patchEnd.11: grade/bloco no caminho bloqueia o golpe
        if (t < best) { best = t; bi = i; }
    }
    if (bi < 0) return false; crystalKill(bi); return true;
}
static void crystalsDraw(float pt) {
    if (gDim != 2) return; (void)pt;
    if (!gCrystTex) {
        std::vector<uint8_t> px; int w = 0, h = 0;
        if (!packRGBA("entity/endercrystal/endercrystal.png", px, w, h)) { w = h = 4; px.assign(64, 255); for (int i = 0; i < 16; i++) { px[i * 4] = 200; px[i * 4 + 1] = 90; px[i * 4 + 2] = 220; } }
        glGenTextures(1, &gCrystTex); glBindTexture(GL_TEXTURE_2D, gCrystTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    }
    GLint prev = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
    glBindTexture(GL_TEXTURE_2D, gCrystTex); glEnable(GL_TEXTURE_2D); glDisable(GL_CULL_FACE); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .1f); glColor4f(1.f, 1.f, 1.f, 1.f);
    const float tm = (float)SDL_GetTicks() * .001f;
    if (gDragon.on && gDragon.dead == 0) {   // raio de cura: fita cruzada de cada cristal ate o dragao
        if (!gBeamTex) {
            std::vector<uint8_t> px; int w = 0, h = 0;
            if (!packRGBA("entity/endercrystal/endercrystal_beam.png", px, w, h)) { w = h = 2; px.assign(16, 255); for (int i = 0; i < 4; i++) { px[i * 4] = 220; px[i * 4 + 1] = 120; px[i * 4 + 2] = 255; } }
            glGenTextures(1, &gBeamTex); glBindTexture(GL_TEXTURE_2D, gBeamTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
        }
        glBindTexture(GL_TEXTURE_2D, gBeamTex);
        for (int i = 0; i < 10; i++) {
            if (!gCrystInit[i] || !gCryst[i].alive) continue;
            const double ax = gCryst[i].x, ay = gCryst[i].y + .9, az = gCryst[i].z, bx = gDragon.x, by = gDragon.y + 2., bz = gDragon.z;
            double vx = bx - ax, vy = by - ay, vz = bz - az; const double L = sqrt(vx * vx + vy * vy + vz * vz); if (L < 1. || L > 90.) continue; vx /= L; vy /= L; vz /= L;
            double px = -vz, py = 0, pz = vx; double pl = sqrt(px * px + pz * pz); if (pl < 1e-4) { px = 1; pz = 0; pl = 1; } px /= pl; pz /= pl;
            const double qx = vy * pz - vz * py, qy = vz * px - vx * pz, qz = vx * py - vy * px;   // segunda direcao perpendicular
            const float hw = .15f, v0 = -tm * 1.5f, v1 = v0 + (float)L * .5f;
            glBegin(GL_QUADS);
            for (int p = 0; p < 2; p++) {
                const double ox = p ? qx : px, oy = p ? qy : py, oz = p ? qz : pz;
                glTexCoord2f(0, v0); glVertex3d(ax - ox * hw, ay - oy * hw, az - oz * hw); glTexCoord2f(1, v0); glVertex3d(ax + ox * hw, ay + oy * hw, az + oz * hw);
                glTexCoord2f(1, v1); glVertex3d(bx + ox * hw, by + oy * hw, bz + oz * hw); glTexCoord2f(0, v1); glVertex3d(bx - ox * hw, by - oy * hw, bz - oz * hw);
            }
            glEnd();
        }
        glBindTexture(GL_TEXTURE_2D, gCrystTex);
    }
    for (int i = 0; i < 10; i++) {
        if (!gCrystInit[i] || !gCryst[i].alive) continue;
        const float bob = sinf(tm * 2.f + i) * .12f;
        for (int k = 0; k < 2; k++) {   // vidro externo e nucleo interno girando
            const float sc = k ? .36f : .5f;
            glPushMatrix(); glTranslatef((float)gCryst[i].x, (float)gCryst[i].y + .9f + bob, (float)gCryst[i].z); glRotatef(tm * (k ? 140.f : 70.f), 0, 1, 0); glRotatef(k ? 45.f : 0.f, 1, 0, 1);
            glBegin(GL_QUADS);
            const float V[6][4][3] = {{{-1,-1,-1},{-1,-1,1},{-1,1,1},{-1,1,-1}},{{1,-1,-1},{1,1,-1},{1,1,1},{1,-1,1}},{{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1}},{{-1,1,-1},{-1,1,1},{1,1,1},{1,1,-1}},{{-1,-1,-1},{-1,1,-1},{1,1,-1},{1,-1,-1}},{{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}}};
            const float U[4][2] = {{0, 0}, {0, .5f}, {.5f, .5f}, {.5f, 0}};
            for (int f = 0; f < 6; f++) for (int q = 0; q < 4; q++) { glTexCoord2f(U[q][0], U[q][1]); glVertex3f(V[f][q][0] * sc, V[f][q][1] * sc, V[f][q][2] * sc); }
            glEnd(); glPopMatrix();
        }
    }
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev); glColor4f(1.f, 1.f, 1.f, 1.f);
}

// ---- patchEnd.7: flecha (do jogador local) acerta cristal (esfera r=1) ou dragao (cabeca r=3 dano cheio; corpo r=4,5 dano/4+1) ----
static bool arrowHitEnd(const double* o, const double* d, double bt, int dmg) {
    auto sph = [&](double cx, double cy, double cz, double r) { const double ox = cx - o[0], oy = cy - o[1], oz = cz - o[2], t = ox * d[0] + oy * d[1] + oz * d[2];
        if (t < -r) return -1.0; const double q2 = ox * ox + oy * oy + oz * oz - t * t, r2 = r * r; if (q2 > r2) return -1.0; const double e = t - sqrt(r2 - q2); return e < 0 ? 0.0 : e; };
    double best = bt + 1e-9; int kind = -1, idx = -1;
    for (int i = 0; i < 10; i++) { if (!gCrystInit[i] || !gCryst[i].alive) continue; const double t = sph(gCryst[i].x, gCryst[i].y + .8, gCryst[i].z, 1.0); if (t >= 0 && t < best) { best = t; kind = 0; idx = i; } }
    DragonEnt& dr = gDragon;
    if (dr.on && dr.dead == 0) {
        const double hx = dr.x - sin(dr.yaw * .01745329) * 6., hz = dr.z - cos(dr.yaw * .01745329) * 6.;
        double t = sph(hx, dr.y + 1., hz, 3.0); if (t >= 0 && t < best) { best = t; kind = 1; }
        t = sph(dr.x, dr.y + 1., dr.z, 4.5); if (t >= 0 && t < best) { best = t; kind = 2; }
    }
    if (kind < 0) return false;
    if (kind == 0) { crystalKill(idx); return true; }
    const int dd = kind == 2 ? dmg / 4 + 1 : dmg;
    if (netOn && !netHost) { NBuf nb = netMk(NP_DRAGONHIT); nb.put<int16_t>((int16_t)dd); netBroadcast(nb); dr.hurtT = 10; return true; }
    dragonApplyDmg(dd); return true;
}
