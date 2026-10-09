// ================= mobs com IA básica: porco, galinha, vaca, ovelha =================
// Modelos = ModelPig / ModelChicken / ModelCow / ModelSheep2+1 do Java 1.8.9 (mesmas caixas, pivôs e UVs).
// Medidas em pixels do modelo Java (y para baixo, chão em y=24). Aqui: x_nosso = -x_java, y_nosso = 24 - y_java (caixas viram [-y1,-y0]);
// rotateAngleX = +90° do Java (corpos deitados) = -90° aqui. Texturas vêm do resource pack ativo (64x64 ou 64x32).
// IA (goals do 1.8.9, sem pathfinding A*): Swim, Panic (ao apanhar), Wander, WatchClosest, LookIdle.
//   porco/vaca/ovelha = IA "animal" (Wander 1/120 por tick, raio 10x7, prefere grama);
//   galinha = IA própria: passeios curtos e frequentes, bicadas no chão, plana ao cair (asas), põe ovo a cada 5-10 min, pânico 1.4x.
enum { M_PIG, M_CHICKEN, M_COW, M_SHEEP, M_N };
// spd = atributo movementSpeed do 1.8.9; panic = multiplicador do EntityAIPanic; ai: 0 = animal, 1 = galinha
struct MobDef { const char* nome; const char* tex; const char* wool; float w, h; int hp; float spd, panic; int ai; };
static const MobDef MD[M_N] = {
    {"porco",   "entity/pig/pig_temperate.png|entity/pig/pig.png", "", .9f, .9f, 10, .25f, 1.25f, 0},
    {"galinha", "entity/chicken/chicken_temperate.png|entity/chicken/temperate_chicken.png|entity/chicken.png", "", .4f, .7f, 4, .25f, 1.4f, 1},
    {"vaca",    "entity/cow/cow_temperate.png|entity/cow/cow.png", "", .9f, 1.3f, 10, .2f, 2.0f, 0},
    {"ovelha",  "entity/sheep/sheep.png", "entity/sheep/sheep_wool.png|entity/sheep/sheep_fur.png", .9f, 1.3f, 8, .23f, 1.25f, 0}};
// kind: 0 fixo, 1 cabeça, 2/3 perna (fase 0 / fase π), 4/5 asa direita/esquerda (galinha). red = cópia vermelha p/ o flash de dano/morte
struct MPart { std::vector<V> m, red; float px, py, pz, rx; bool wool; int kind; };
static std::vector<MPart> MP[M_N];
static GLuint mobTex[M_N][2]; static bool mobReady = false;
struct MobEnt {
    int type = 0; double x = 0, y = 0, z = 0; float yaw = 0; int hp = 1, dead = 0;   // dead: 0 vivo; >0 = ticks desde a morte (some aos 21, como o deathTime 20 do Java)
    double px = 0, py = 0, pz = 0, mx = 0, my = 0, mz = 0; float pyaw = 0;           // p* = tick anterior (interpolação); m* = motionX/Y/Z
    float hyaw = 0, hpit = 0, phyaw = 0, phpit = 0;                                  // cabeça relativa ao corpo, em graus (hpit > 0 = olhando para baixo)
    float limb = 0, limbAmt = 0, plimbAmt = 0, fall = 0;
    bool ground = false, collH = false, inWater = false;
    int rideBoat = 0;   // id do bote em que o animal está sentado (0 = nenhum)
    int hurt = 0, resist = 0, lastDmg = 0, panic = 0, jumpCd = 0, stuck = 0, lookT = 0, idleT = 0, peckT = 0, eggT = 0;
    bool hasT = false; double tx = 0, tz = 0; float idleYaw = 0;                      // alvo do passeio
    float wing = 0, pwing = 0, flap = 0, pflap = 0, flapD = 1.f;                      // galinha: wingRotation, destPos, wingRotDelta
};
static std::vector<MobEnt> MOBS;
static float gPlayerYaw = 0.f;
static bool gHitbox = false;   // /hitbox (equivalente ao F3+B): desenha a caixa de colisão de cada mob
struct Puff { float x, y, z, px, py, pz, vx, vy, vz, s, c; int age, max; };           // fumaça da morte (EntityExplodeFX)
static std::vector<Puff> PUFFS;

static std::vector<V> bx(float x0, float x1, float y0, float y1, float z0, float z1, float u, float v, float w, float h, float d, float g = 0) {
    std::vector<V> o; boxM(o, x0, x1, y0, y1, z0, z1, u, v, w, h, d, g); return o;
}
static void shadeBody(std::vector<V>& m) {   // boxM sombreia pelas faces locais; o corpo é girado: [frente=barriga][costas=dorso][laterais][topo/base=peito/traseira]
    for (size_t i = 0; i < m.size(); i++) { const size_t k = i % 24; const float s = k < 4 ? .55f : k < 8 ? 1.f : k < 16 ? .7f : .85f; m[i].r = m[i].g = m[i].b = s; }
}
static bool mobTexLoad(const char* names, GLuint& tex, float& vs, const int fb[3]) {
    std::vector<uint8_t> px; int w = 0, h = 0; bool ok = false;
    std::string all = names; size_t a = 0;
    while (!ok && a <= all.size()) { size_t b = all.find('|', a); if (b == std::string::npos) b = all.size(); ok = packRGBA(all.substr(a, b - a), px, w, h); a = b + 1; }
    if (ok && (w < 64 || (h * 2 != w && h != w))) ok = false;
    if (!ok) { w = h = 64; px.assign(64 * 64 * 4, 0); for (int i = 0; i < 64 * 64; i++) { px[i * 4] = fb[0]; px[i * 4 + 1] = fb[1]; px[i * 4 + 2] = fb[2]; px[i * 4 + 3] = 255; } }
    if (!tex) glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    vs = (float)w / (float)h;   // boxM normaliza por 64 nos dois eixos; textura 64x32 precisa de v * 2
    return ok;
}
static void loadMobs() {   // precisa de contexto GL e do pack já aberto; chamado ao desenhar o 1º mob
    mobReady = true;
    static const int FB[M_N][2][3] = {{{240,160,160},{0,0,0}}, {{245,245,245},{0,0,0}}, {{90,60,40},{0,0,0}}, {{190,190,185},{240,240,240}}};
    float vs[M_N][2] = {}; int fromPack = 0;
    for (int t = 0; t < M_N; t++) {
        fromPack += mobTexLoad(MD[t].tex, mobTex[t][0], vs[t][0], FB[t][0]);
        if (*MD[t].wool) fromPack += mobTexLoad(MD[t].wool, mobTex[t][1], vs[t][1], FB[t][1]);
    }
    auto A = [&](int t, std::vector<V> m, float px, float py, float pz, float rx = 0, bool wool = false, int kind = 0) { MP[t].push_back({std::move(m), {}, px, py, pz, rx, wool, kind}); };
    auto body = [&](std::vector<V> m) { shadeBody(m); return m; };
    auto legK = [](float x, float z) { return x * z > 0 ? 2 : 3; };   // pernas em diagonal andam juntas (ModelQuadruped: leg1/leg4 fase 0, leg2/leg3 fase π)
    // porco: cabeça (0,12,-6) + focinho, corpo (0,13,2), pernas y=6
    { std::vector<V> h = bx(-4,4, -4,4, -8,0, 0,0, 8,8,8), s = bx(-2,2, -3,0, -9,-8, 16,16, 4,3,1); h.insert(h.end(), s.begin(), s.end());
      A(M_PIG, body(bx(-5,5, -6,10, -7,1, 28,8, 10,16,8)), 0,13,2, -90); A(M_PIG, h, 0,12,-6, 0, false, 1);
      for (float x : {-3.f, 3.f}) for (float z : {7.f, -5.f}) A(M_PIG, bx(-2,2, -6,0, -2,2, 0,16, 4,6,4), x,6,z, 0, false, legK(x, z)); }
    // galinha: cabeça (0,9,-4) + bico + barbela, corpo (0,8,0), pernas (±..,5,1), asas y=11
    { std::vector<V> h = bx(-2,2, 0,6, -2,1, 0,0, 4,6,3), b = bx(-2,2, 2,4, -4,-2, 14,0, 4,2,2), c = bx(-1,1, 0,2, -3,-1, 14,4, 2,2,2);
      h.insert(h.end(), b.begin(), b.end()); h.insert(h.end(), c.begin(), c.end());
      A(M_CHICKEN, body(bx(-3,3, -4,4, -3,3, 0,9, 6,8,6)), 0,8,0, -90); A(M_CHICKEN, h, 0,9,-4, 0, false, 1);
      A(M_CHICKEN, bx(-2,1, -5,0, -3,0, 26,0, 3,5,3), 2,5,1, 0, false, 2); A(M_CHICKEN, bx(-2,1, -5,0, -3,0, 26,0, 3,5,3), -1,5,1, 0, false, 3);
      A(M_CHICKEN, bx(-1,0, -4,0, -3,3, 24,13, 1,4,6), 4,11,0, 0, false, 4); A(M_CHICKEN, bx(0,1, -4,0, -3,3, 24,13, 1,4,6), -4,11,0, 0, false, 5); }
    // vaca: cabeça (0,20,-8) + chifres, corpo (0,19,2) + úbere, pernas y=12 em (±4, 7/-6)
    { std::vector<V> h = bx(-4,4, -4,4, -6,0, 0,0, 8,8,6), h1 = bx(4,5, 2,5, -4,-3, 22,0, 1,3,1), h2 = bx(-5,-4, 2,5, -4,-3, 22,0, 1,3,1);
      h.insert(h.end(), h1.begin(), h1.end()); h.insert(h.end(), h2.begin(), h2.end());
      std::vector<V> b = bx(-6,6, -8,10, -7,3, 18,4, 12,18,10), u = bx(-2,2, -8,-2, -8,-7, 52,0, 4,6,1); b.insert(b.end(), u.begin(), u.end());
      A(M_COW, body(b), 0,19,2, -90); A(M_COW, h, 0,20,-8, 0, false, 1);
      for (float x : {-4.f, 4.f}) for (float z : {7.f, -6.f}) A(M_COW, bx(-2,2, -12,0, -2,2, 0,16, 4,12,4), x,12,z, 0, false, legK(x, z)); }
    // ovelha: pele (sheep.png) + lã (sheep_wool.png): cabeça (0,18,-8), corpo (0,19,2), pernas y=12 em (±3, 7/-5)
    A(M_SHEEP, bx(-3,3, -2,4, -6,2, 0,0, 6,6,8), 0,18,-8, 0, false, 1); A(M_SHEEP, body(bx(-4,4, -6,10, -7,-1, 28,8, 8,16,6)), 0,19,2, -90);
    for (float x : {-3.f, 3.f}) for (float z : {7.f, -5.f}) A(M_SHEEP, bx(-2,2, -12,0, -2,2, 0,16, 4,12,4), x,12,z, 0, false, legK(x, z));
    A(M_SHEEP, bx(-3,3, -2,4, -4,2, 0,0, 6,6,6, .6f), 0,18,-8, 0, true, 1); A(M_SHEEP, body(bx(-4,4, -6,10, -7,-1, 28,8, 8,16,6, 1.75f)), 0,19,2, -90, true);
    for (float x : {-3.f, 3.f}) for (float z : {7.f, -5.f}) A(M_SHEEP, bx(-2,2, -6,0, -2,2, 0,16, 4,6,4, .5f), x,12,z, 0, true, legK(x, z));
    for (int t = 0; t < M_N; t++) for (MPart& p : MP[t]) {
        for (V& v : p.m) v.v *= vs[t][p.wool ? 1 : 0];
    }
    printf("Mobs: %d textura(s) vieram do resource pack\n", fromPack);
}

// ------------------------------- desenho -------------------------------
static void drawMob(const MobEnt& e, float pt) {   // origem = pés, olhando para -Z; pt = partialTicks
    if (!mobReady) loadMobs();
    const MobDef& d = MD[e.type];
    auto L = [&](double a, double b) { return (float)(a + (b - a) * pt); };
    glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f);
    glPushMatrix(); glTranslatef(L(e.px, e.x), L(e.py, e.y), L(e.pz, e.z)); glRotatef(L(e.pyaw, e.yaw), 0, 1, 0);
    // RendererLivingEntity.rotateCorpse: f = sqrt((deathTime + pt - 1) / 20 * 1.6) limitado a 1; gira 90° em Z em torno dos PÉS (o corpo deita no chão)
    if (e.dead > 0) { const float f = std::min(1.f, sqrtf(std::max(0.f, (e.dead - 2 + pt) / 20.f * 1.6f))); glRotatef(90.f * f, 0, 0, 1); }
    const float hy = L(e.phyaw, e.hyaw), hpt = L(e.phpit, e.hpit), amt = L(e.plimbAmt, e.limbAmt), ls = e.limb - e.limbAmt * (1.f - pt);
    const float fl = d.ai == 1 ? (sinf(L(e.pwing, e.wing)) + 1.f) * L(e.pflap, e.flap) * 57.29578f : 0.f;   // ModelChicken: rotateAngleZ das asas
    auto draw = [&](bool red) {
        for (int w = 0; w < 2; w++) {
            if (w && !*d.wool) break;
            if (!red) glBindTexture(GL_TEXTURE_2D, mobTex[e.type][w]);
            for (const MPart& p : MP[e.type]) {
                if (p.wool != (w == 1)) continue;
                float rx = p.rx, ry = 0, rz = 0;
                switch (p.kind) {   // ModelQuadruped / ModelChicken.setRotationAngles (rotação X do nosso sistema = -X do Java)
                    case 1: rx = -hpt; ry = hy; break;
                    case 2: rx = -57.29578f * cosf(ls * .6662f) * 1.4f * amt; break;
                    case 3: rx = -57.29578f * cosf(ls * .6662f + 3.14159265f) * 1.4f * amt; break;
                    case 4: rz = fl; break;
                    case 5: rz = -fl; break;
                }
                part(red ? p.red : p.m, p.px, p.py, p.pz, rx, ry, rz);
            }
        }
    };
    gEntHurt = (e.hurt > 0 || e.dead > 0) ? 1.f : 0.f;   // flash vermelho ao apanhar e na morte: tinge a propria textura (a copia vermelha sem textura pintava a caixa inteira dos pes da galinha)
    draw(false); gEntHurt = 0.f;
    glPopMatrix(); glDisable(GL_ALPHA_TEST);
}
static void drawPuffs(float pt) {   // bolinhas cinza que sobem devagar, sempre viradas para a câmera (usa a matriz atual: direita/cima da câmera)
    if (PUFFS.empty()) return;
    float m[16]; glGetFloatv(GL_MODELVIEW_MATRIX, m);
    const float rx = m[0], ry = m[4], rz = m[8], ux = m[1], uy = m[5], uz = m[9];
    const GLboolean wasBlend = glIsEnabled(GL_BLEND);
    glDisable(GL_TEXTURE_2D); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    for (const Puff& p : PUFFS) {
        const float x = p.px + (p.x - p.px) * pt, y = p.py + (p.y - p.py) * pt, z = p.pz + (p.z - p.pz) * pt, s = .1f * p.s;
        glColor4f(p.c, p.c, p.c, std::max(0.f, .8f * (1.f - (p.age + pt) / p.max)));
        glBegin(GL_TRIANGLE_FAN); glVertex3f(x, y, z);
        for (int k = 0; k <= 8; k++) { const float t = k * .785398f, cs = cosf(t) * s, sn = sinf(t) * s; glVertex3f(x + rx * cs + ux * sn, y + ry * cs + uy * sn, z + rz * cs + uz * sn); }
        glEnd();
    }
    glDepthMask(GL_TRUE); if (!wasBlend) glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
}
static void drawHitboxes(float pt) {   // F3+B: caixa branca = AABB do mob, retângulo vermelho = altura dos olhos
    glDisable(GL_TEXTURE_2D); glDisable(GL_ALPHA_TEST); glLineWidth(2.f); glBegin(GL_LINES);
    for (const MobEnt& e : MOBS) {
        const float x = (float)(e.px + (e.x - e.px) * pt), y = (float)(e.py + (e.y - e.py) * pt), z = (float)(e.pz + (e.z - e.pz) * pt);
        const float hw = MD[e.type].w * .5f, h = MD[e.type].h, ey = y + h * (e.type == M_SHEEP ? .95f : .85f);
        const float X0 = x - hw, X1 = x + hw, Z0 = z - hw, Z1 = z + hw, Y0 = y, Y1 = y + h;
        auto L = [](float a, float b, float c, float d, float f, float g) { glVertex3f(a, b, c); glVertex3f(d, f, g); };
        glColor3f(1.f, 1.f, 1.f);
        L(X0,Y0,Z0,X1,Y0,Z0); L(X1,Y0,Z0,X1,Y0,Z1); L(X1,Y0,Z1,X0,Y0,Z1); L(X0,Y0,Z1,X0,Y0,Z0);
        L(X0,Y1,Z0,X1,Y1,Z0); L(X1,Y1,Z0,X1,Y1,Z1); L(X1,Y1,Z1,X0,Y1,Z1); L(X0,Y1,Z1,X0,Y1,Z0);
        L(X0,Y0,Z0,X0,Y1,Z0); L(X1,Y0,Z0,X1,Y1,Z0); L(X1,Y0,Z1,X1,Y1,Z1); L(X0,Y0,Z1,X0,Y1,Z1);
        glColor3f(1.f, 0.f, 0.f);
        L(X0,ey,Z0,X1,ey,Z0); L(X1,ey,Z0,X1,ey,Z1); L(X1,ey,Z1,X0,ey,Z1); L(X0,ey,Z1,X0,ey,Z0);
    }
    glEnd(); glLineWidth(1.f); glColor4f(1.f, 1.f, 1.f, 1.f); glEnable(GL_TEXTURE_2D);
}
static void drawMobs(float pt = 1.f) { for (const MobEnt& e : MOBS) { entLightAt(e.x, e.y + .7, e.z, gEntDim); const bool es = entShaderOn(); drawMob(e, pt); if (es) skyShaderOff(); } entDimReset(); drawPuffs(pt); if (gHitbox) drawHitboxes(pt); }

// ------------------------------- spawn, itens e dano -------------------------------
static void spawnMob(int type, double x, double y, double z, float yaw) {
    MobEnt e; e.type = type; e.x = e.px = x; e.y = e.py = y; e.z = e.pz = z; e.yaw = e.pyaw = yaw; e.hp = MD[type].hp; e.eggT = 6000 + rand() % 6000;
    MOBS.push_back(e);
}
static void spawnMobFront(int type, const Phys& P, float yaw, float side = 0.f) {   // 2 blocos à frente (side = deslocamento lateral), no chão, virado para o jogador
    const float r = yaw * .0174533f;
    const double x = P.x - sin(r) * 2.0 + cos(r) * side, z = P.z - cos(r) * 2.0 - sin(r) * side;
    double y = P.y;
    for (int by = (int)floor(P.y) + 3; by >= 0; by--) if (solid(getB((int)floor(x), by, (int)floor(z)))) { y = by + 1.0; break; }
    spawnMob(type, x, y, z, yaw + 180.f);
}
static int eggMob(int id) { return id >= IT_EGG_PIG && id <= IT_EGG_SHEEP ? id - IT_EGG_PIG : -1; }
static int mobWeaponDmg(int id) {   // dano base do 1.8.9 (4 + material nas espadas, 3 machados, 2 picaretas, 1 pás); sem arma = 1
    static const int S[5] = {4, 5, 6, 4, 7}, AX[5] = {3, 4, 5, 3, 6}, PK[5] = {2, 3, 4, 2, 5}, SH[5] = {1, 2, 3, 1, 4};
    if (id == IT_WSWORD) return S[0]; if (id >= IT_SSWORD && id <= IT_DSWORD) return S[id - IT_SSWORD + 1];
    if (id >= IT_WAXE && id < IT_WAXE + 5) return AX[id - IT_WAXE]; if (id >= IT_WSHOVEL && id < IT_WSHOVEL + 5) return SH[id - IT_WSHOVEL];
    if (id == IT_WPICK) return PK[0]; if (id == IT_SPICK) return PK[1]; if (id >= IT_IPICK && id <= IT_DPICK) return PK[id - IT_IPICK + 2];
    return 1;
}
static void mobDrops(const MobEnt& e) {   // tabelas do 1.8.9: porco 1-3 costeleta; galinha 0-2 penas + 1 frango; vaca 0-2 couro + 1-3 carne; ovelha 1 lã + 1-2 carneiro
    auto D = [&](int id, int n) { for (int i = 0; i < n; i++) spawnDrop(e.x + (frand() - .5) * .4, e.y + MD[e.type].h * .5, e.z + (frand() - .5) * .4, frand() * .2 - .1, .2, frand() * .2 - .1, id, 1, 10); };
    auto R = [&](int lo, int hi) { return lo + rand() % (hi - lo + 1); };
    switch (e.type) {
        case M_PIG: D(IT_PORK, R(1, 3)); break;
        case M_CHICKEN: D(IT_FEATHER, R(0, 2)); D(IT_CHICKEN, 1); break;
        case M_COW: D(IT_LEATHER, R(0, 2)); D(IT_BEEF, R(1, 3)); break;
        case M_SHEEP: D(WOOL, 1); D(IT_MUTTON, R(1, 2)); break;
    }
}
// EntityLivingBase.attackEntityFrom (1.8.9): hurtResistantTime = 20 ticks após levar dano; na 1ª metade (resist > 10) só passa a diferença
// se o novo golpe for MAIOR que o último (lastDamage); na 2ª metade o golpe é integral. Golpe bloqueado = sem dano e sem knockback.
// Depois: knockBack(0.4 + 0.5 por nível extra, ex.: correndo), flash vermelho de 10 ticks e Panic de 100 ticks.
static bool mobHurt(MobEnt& e, int dmg, float kx, float kz, bool attacked, float kbExtra = 0.f, float lx = 0.f, float lz = 0.f) {   // kx,kz = atacante -> alvo (posicao); lx,lz = direcao do olhar (parte do sprint)
    if (e.dead || dmg <= 0) return false;
    if (e.resist > 10) { if (dmg <= e.lastDmg) return false; e.hp -= dmg - e.lastDmg; e.lastDmg = dmg; }
    else { e.hp -= dmg; e.lastDmg = dmg; e.resist = 20; }
    e.hurt = 10;
    if (attacked) {
        const float l = sqrtf(kx * kx + kz * kz);
        e.mx /= 2; e.mz /= 2; e.my = std::min(.4, e.my / 2 + .4);
        if (l > 1e-4f) { e.mx += kx / l * .4; e.mz += kz / l * .4; if (kbExtra > 0.f) { const float ll = sqrtf(lx * lx + lz * lz); const float ux = ll > 1e-4f ? lx / ll : kx / l, uz = ll > 1e-4f ? lz / ll : kz / l; e.mx += ux * kbExtra * .5; e.mz += uz * kbExtra * .5; e.my += .1; } }
        e.panic = 100; e.hasT = false;
    }
    if (e.hp <= 0) { e.dead = 1; e.hasT = false; e.rideBoat = 0; mobDrops(e); }
    return true;
}
// crit = caindo no ar (fallDistance > 0, fora d'água): dano x1.5; sprint = knockback extra. Sem instakill: o criativo causa o dano normal da arma.
static bool mobAttack(const float* eye, const float* dir, int held, bool crit = false, bool sprint = false, float reach = 3.f) {   // clique esquerdo: acerta o mob mais próximo na mira (alcance 3), se não houver bloco na frente
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead) continue;
        const float hw = MD[e.type].w * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + MD[e.type].h + .1f, (float)e.z + hw};   // caixa + 0.1 em todos os lados
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    if (best < 0) return false;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return false;   // bloco na frente do mob
    int dmg = mobWeaponDmg(held); if (crit) dmg = (int)(dmg * 1.5f);   // 1.8.9: i *= 1.5 em float, depois o dano vira inteiro de vida (meio coração = 1)
    { const MobEnt& t = MOBS[best]; float px = (float)t.x - eye[0], pz = (float)t.z - eye[2]; if (px * px + pz * pz < 1e-6f) { px = dir[0]; pz = dir[2]; }   // 1.8.9: o recuo base vai do atacante para o alvo
      mobHurt(MOBS[best], dmg, px, pz, true, sprint ? 1.f : 0.f, dir[0], dir[2]); }
    return true;
}

// ------------------------------- física e IA (20 ticks/s) -------------------------------
static float mwrap(float a) { while (a > 180.f) a -= 360.f; while (a < -180.f) a += 360.f; return a; }
static float mturn(float cur, float want, float step) { return cur + std::max(-step, std::min(step, mwrap(want - cur))); }
static void moveMob(MobEnt& e, double mx, double my, double mz) {   // Entity.moveEntity (Y, X, Z + degrau de 0.6), com a caixa do mob
    const double hw = MD[e.type].w * .5;
    AABB bb{e.x - hw, e.y, e.z - hw, e.x + hw, e.y + MD[e.type].h, e.z + hw};
    double x = mx, y = my, z = mz; const double d6 = x, d7 = y, d8 = z;
    std::vector<AABB> list; collBoxes(bb.addCoord(x, y, z), list); boatCollect(bb.addCoord(x, y, z), list, 0);
    const AABB bb0 = bb;
    for (auto& b : list) y = b.calcY(bb, y);
    bb = bb.offset(0, y, 0);
    const bool canStep = e.ground || (d7 != y && d7 < 0);
    for (auto& b : list) x = b.calcX(bb, x);
    bb = bb.offset(x, 0, 0);
    for (auto& b : list) z = b.calcZ(bb, z);
    bb = bb.offset(0, 0, z);
    if (canStep && (d6 != x || d8 != z)) {
        std::vector<AABB> l1; collBoxes(bb0.addCoord(d6, .6, d8), l1); boatCollect(bb0.addCoord(d6, .6, d8), l1, 0);
        AABB a = bb0; const AABB a5 = a.addCoord(d6, 0, d8);
        double dy = .6; for (auto& b : l1) dy = b.calcY(a5, dy);
        a = a.offset(0, dy, 0);
        double ax = d6; for (auto& b : l1) ax = b.calcX(a, ax);
        a = a.offset(ax, 0, 0);
        double az = d8; for (auto& b : l1) az = b.calcZ(a, az);
        a = a.offset(0, 0, az);
        if (ax * ax + az * az > x * x + z * z) {
            x = ax; z = az; y = -dy; bb = a;
            for (auto& b : l1) y = b.calcY(bb, y);
            bb = bb.offset(0, y, 0);
        }
    }
    e.x = (bb.x0 + bb.x1) / 2; e.y = bb.y0; e.z = (bb.z0 + bb.z1) / 2;
    e.collH = d6 != x || d8 != z; e.ground = d7 != y && d7 < 0;
    if (d6 != x) e.mx = 0; if (d8 != z) e.mz = 0; if (d7 != y) e.my = 0;
}
static bool mobSafeStep(const MobEnt& e, float yawDeg) {   // ponto logo à frente: tem chão (até 4 blocos abaixo) e não é água (navegador de terra evita água e precipícios)
    const float r = yawDeg * .0174533f;
    const int x = (int)floor(e.x - sin(r) * .7), z = (int)floor(e.z - cos(r) * .7), by = (int)floor(e.y + .001);
    if (!e.inWater && (isWater(getB(x, by, z)) || isWater(getB(x, by - 1, z)))) return false;
    for (int k = 1; k <= 4; k++) if (solid(getB(x, by - k, z))) return true;
    return false;
}
static void pickTarget(MobEnt& e, int rxz, int ry) {   // RandomPositionGenerator.findRandomTarget: 10 sorteios, vence o maior peso (grama = 10, resto ~0.5 como EntityAnimal.getBlockPathWeight)
    float best = -1e9f;
    for (int i = 0; i < 10; i++) {
        const int x = (int)floor(e.x) + rand() % (2 * rxz + 1) - rxz, z = (int)floor(e.z) + rand() % (2 * rxz + 1) - rxz;
        int gy = (int)floor(e.y) + rand() % (2 * ry + 1) - ry;
        for (int k = 0; k < 8 && gy > 1 && !solid(getB(x, gy - 1, z)); k++) gy--;   // desce até o chão
        if (!solid(getB(x, gy - 1, z)) || solid(getB(x, gy, z)) || solid(getB(x, gy + 1, z)) || isWater(getB(x, gy, z))) continue;
        const float w = getB(x, gy - 1, z) == GRASS ? 10.f : .5f;
        if (w > best) { best = w; e.tx = x + .5; e.tz = z + .5; e.hasT = true; e.stuck = 0; }
    }
}
static void tickMob(MobEnt& e, double ppx, double ppy, double ppz) {
    const MobDef& d = MD[e.type];
    e.px = e.x; e.py = e.y; e.pz = e.z; e.pyaw = e.yaw; e.phyaw = e.hyaw; e.phpit = e.hpit; e.plimbAmt = e.limbAmt; e.pwing = e.wing; e.pflap = e.flap;
    if (e.rideBoat) {   // sentado no bote: sem IA nem física, só anima (a posição vem de boatTickAll)
        if (e.hurt > 0) e.hurt--; if (e.resist > 0) e.resist--;
        e.limbAmt = 0.f; e.mx = e.my = e.mz = 0; e.fall = 0.f; e.panic = 0; e.hasT = false; e.ground = true; e.inWater = false; return;
    }
    if (e.hurt > 0) e.hurt--;
    if (e.resist > 0) e.resist--;
    if (e.jumpCd > 0) e.jumpCd--;
    { const double hw = d.w * .5, y0 = e.y + .4, y1 = std::max(e.y + .41, e.y + d.h - .4);   // handleWaterMovement: caixa encolhida
      e.inWater = anyLiquid(AABB{e.x - hw + .001, y0, e.z - hw + .001, e.x + hw - .001, y1, e.z + hw - .001}); }
    const double pdx = ppx - e.x, pdz = ppz - e.z, pdy = ppy + 1.62 - (e.y + d.h * .85), pd2 = pdx * pdx + pdz * pdz;
    float fwd = 0.f; bool jump = false;
    if (!e.dead) {
        // --- Panic (prioridade 1) > Wander (6): EntityMoveHelper gira o corpo no máx. 30°/tick ---
        float mul = 1.f;
        if (e.panic > 0) { e.panic--; mul = d.panic; if (!e.hasT) pickTarget(e, 5, 4); }
        else if (!e.hasT) {
            if (d.ai == 1) { if (rand() % 60 == 0) pickTarget(e, 6, 3); }   // galinha: passeios curtos e frequentes
            else if (rand() % 120 == 0) pickTarget(e, 10, 7);                // EntityAIWander
        }
        if (e.hasT) {
            const double dx = e.tx - e.x, dz = e.tz - e.z, sp = mul * d.spd, tol = std::max(.3, sp * sp * 2.2);   // tol = distância andada por tick (evita passar direto)
            if (dx * dx + dz * dz < tol * tol) e.hasT = false;
            else {
                e.yaw = mturn(e.yaw, (float)(atan2(-dx, -dz) * 57.29578), 30.f);
                if (!mobSafeStep(e, e.yaw)) e.hasT = false;
                else { fwd = (float)sp; if (e.collH && e.ground) jump = true; }   // bateu em degrau/bloco: pula (JumpHelper)
            }
        }
        // --- cabeça: bicada (galinha) > WatchClosest (2%/tick, 6 blocos, 40-79 ticks) > LookIdle (2%/tick, 20-39 ticks) ---
        float ty = 0.f, tp = 0.f;
        if (e.lookT > 0 && pd2 > 36.0) e.lookT = 0;
        if (e.peckT > 0) { e.peckT--; tp = (e.peckT % 10 < 5) ? 65.f : 15.f; }
        else if (e.lookT > 0) { e.lookT--; ty = mwrap((float)(atan2(-pdx, -pdz) * 57.29578) - e.yaw); tp = (float)(atan2(-pdy, sqrt(pd2)) * 57.29578); }
        else if (e.idleT > 0) { e.idleT--; ty = mwrap(e.idleYaw - e.yaw); }
        else if (pd2 <= 36.0 && frand() < .02f) e.lookT = 40 + rand() % 40;
        else if (frand() < .02f) { e.idleT = 20 + rand() % 20; e.idleYaw = frand() * 360.f; }
        else if (d.ai == 1 && !e.hasT && rand() % 50 == 0) e.peckT = 20;
        ty = std::max(-75.f, std::min(75.f, ty)); tp = std::max(-40.f, std::min(70.f, tp));
        e.hyaw += std::max(-10.f, std::min(10.f, ty - e.hyaw)); e.hpit += std::max(-40.f, std::min(40.f, tp - e.hpit));
        if (d.ai == 1 && --e.eggT <= 0) { e.eggT = 6000 + rand() % 6000; spawnDrop(e.x, e.y + .3, e.z, 0, .1, 0, IT_EGG, 1, 10); }   // EntityChicken: ovo a cada 5-10 min
        if (e.inWater && frand() < .8f) jump = true;   // EntityAISwimming
    }
    if (jump) { if (e.inWater) e.my += .04; else if (e.ground && e.jumpCd == 0) { e.my = .42; e.jumpCd = 10; } }
    // moveEntityWithHeading: aceleração = forward * friction; no chão friction = aiMoveSpeed (= forward), no ar/água 0.02
    const double r = e.yaw * .017453292519943295, ac = (e.ground && !e.inWater) ? fwd * fwd : fwd * .02;
    e.mx += -sin(r) * ac; e.mz += -cos(r) * ac;
    moveMob(e, e.mx, e.my, e.mz);
    if (e.inWater) { e.mx *= .8; e.my *= .8; e.mz *= .8; e.my -= .02; if (e.collH) e.my = .3; }
    else { const double f4 = e.ground ? .546 : .91; e.my -= .08; e.my *= .98; e.mx *= f4; e.mz *= f4; }
    // dano de queda (EntityLivingBase.fall: > 3 blocos; galinha é imune)
    { const double dy = e.y - e.py;
      if (e.inWater) e.fall = 0.f;
      else if (e.ground) { if (e.fall > 3.f && d.ai == 0) mobHurt(e, (int)ceil(e.fall - 3.f), 0, 0, false); e.fall = 0.f; }
      else if (dy < 0) e.fall -= (float)dy; }
    if (d.ai == 1) {   // EntityChicken.onLivingUpdate: asas batem no ar e a queda é amortecida (motionY *= 0.6)
        e.flap = std::max(0.f, std::min(1.f, e.flap + (e.ground ? -.3f : 1.2f)));
        if (!e.ground && e.flapD < 1.f) e.flapD = 1.f;
        e.flapD *= .9f; e.wing += e.flapD * 2.f;
        if (!e.ground && e.my < 0) e.my *= .6;
    }
    const double mdx = e.x - e.px, mdz = e.z - e.pz;
    float f6 = (float)sqrt(mdx * mdx + mdz * mdz) * 4.f; if (f6 > 1.f) f6 = 1.f;
    e.limbAmt += (f6 - e.limbAmt) * .4f; e.limb += e.limbAmt;
    if (e.hasT && fwd > 0.f) { if (mdx * mdx + mdz * mdz < 1e-4) { if (++e.stuck > 20) { e.hasT = false; e.stuck = 0; } } else e.stuck = 0; }   // preso: desiste do alvo
    else e.stuck = 0;
}
static void spawnPoof(const MobEnt& e) {   // EntityLivingBase.onDeathUpdate (deathTime 20): 20 partículas "explode" espalhadas pelo corpo
    const float w = MD[e.type].w, h = MD[e.type].h;
    auto G = []() { return (frand() + frand() + frand() + frand() - 2.f) * .0346f; };   // ~ gaussiana * 0.02
    for (int i = 0; i < 20; i++) {
        Puff p; p.x = p.px = (float)e.x + (frand() * 2.f - 1.f) * w; p.y = p.py = (float)e.y + frand() * h; p.z = p.pz = (float)e.z + (frand() * 2.f - 1.f) * w;
        p.vx = G() + (frand() * 2.f - 1.f) * .05f; p.vy = G() + (frand() * 2.f - 1.f) * .05f; p.vz = G() + (frand() * 2.f - 1.f) * .05f;
        p.s = frand() * frand() * 6.f + 1.f; p.c = frand() * .3f + .7f; p.max = (int)(16.f / (frand() * .8f + .2f)) + 2; p.age = 0;
        PUFFS.push_back(p);
    }
}
// Entity.applyEntityCollision (1.8.9): afasta os dois pela diferença de posição; força = 0.05 / sqrt(max(|dx|,|dz|)) (no máx. 0.05), só em X/Z.
static void pushPair(double ax, double az, double& amx, double& amz, double bx, double bz, double& bmx, double& bmz) {
    double d0 = bx - ax, d1 = bz - az, d2 = std::max(fabs(d0), fabs(d1));
    if (d2 < .01) return;
    d2 = sqrt(d2); d0 /= d2; d1 /= d2;
    const double d3 = std::min(1.0, 1.0 / d2); d0 *= d3 * .05; d1 *= d3 * .05;
    amx -= d0; amz -= d1; bmx += d0; bmz += d1;
}
static AABB mobBox(const MobEnt& e) { const double hw = MD[e.type].w * .5; return {e.x - hw, e.y, e.z - hw, e.x + hw, e.y + MD[e.type].h, e.z + hw}; }
static void mobPush(Phys& P) {   // cada entidade empurra as vizinhas (caixa + 0.2 em X/Z); como no Java, cada par é processado 2x por tick (uma por entidade)
    for (size_t i = 0; i < MOBS.size(); i++) {
        MobEnt& a = MOBS[i]; if (a.dead || a.rideBoat) continue;
        if (a.y < -64.0) { a.dead = true; continue; }   // caiu no vazio
        const AABB ba = mobBox(a), ex = {ba.x0 - .2, ba.y0, ba.z0 - .2, ba.x1 + .2, ba.y1, ba.z1 + .2};
        for (size_t j = 0; j < MOBS.size(); j++) {
            if (j == i || MOBS[j].dead || MOBS[j].rideBoat || !ex.intersects(mobBox(MOBS[j]))) continue;
            pushPair(a.x, a.z, a.mx, a.mz, MOBS[j].x, MOBS[j].z, MOBS[j].mx, MOBS[j].mz);
        }
        if (ex.intersects(physBox(P))) pushPair(a.x, a.z, a.mx, a.mz, P.x, P.z, P.mx, P.mz);   // mob -> jogador
    }
    const AABB pb = physBox(P), ex = {pb.x0 - .2, pb.y0, pb.z0 - .2, pb.x1 + .2, pb.y1, pb.z1 + .2};
    for (MobEnt& m : MOBS) if (!m.dead && !m.rideBoat && ex.intersects(mobBox(m))) pushPair(P.x, P.z, P.mx, P.mz, m.x, m.z, m.mx, m.mz);   // jogador -> mob
}
static void tickMobs(Phys& P) {
    const double ppx = P.x, ppy = P.y, ppz = P.z;
    mobPush(P);
    for (size_t i = 0; i < PUFFS.size();) {
        Puff& p = PUFFS[i]; p.px = p.x; p.py = p.y; p.pz = p.z;
        if (p.age++ >= p.max) { PUFFS.erase(PUFFS.begin() + i); continue; }
        p.vy += .004f; p.x += p.vx; p.y += p.vy; p.z += p.vz; p.vx *= .9f; p.vy *= .9f; p.vz *= .9f; i++;
    }
    for (size_t i = 0; i < MOBS.size();) {
        MobEnt& e = MOBS[i];
        if (e.y < -30) { MOBS.erase(MOBS.begin() + i); continue; }
        if (!getC(fdiv((int)floor(e.x), CW), fdiv((int)floor(e.z), CW))) { i++; continue; }   // chunk não carregado: congela
        tickMob(e, ppx, ppy, ppz);
        if (e.dead && ++e.dead >= 21) { spawnPoof(e); MOBS.erase(MOBS.begin() + i); continue; }
        i++;
    }
}
