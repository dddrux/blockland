#include <set>
// ================= mobs com IA básica: porco, galinha, vaca, ovelha =================
// Modelos = ModelPig / ModelChicken / ModelCow / ModelSheep2+1 do Java 1.8.9 (mesmas caixas, pivôs e UVs).
// Medidas em pixels do modelo Java (y para baixo, chão em y=24). Aqui: x_nosso = -x_java, y_nosso = 24 - y_java (caixas viram [-y1,-y0]);
// rotateAngleX = +90° do Java (corpos deitados) = -90° aqui. Texturas vêm do resource pack ativo (64x64 ou 64x32).
// IA (goals do 1.8.9, sem pathfinding A*): Swim, Panic (ao apanhar), Wander, WatchClosest, LookIdle.
//   porco/vaca/ovelha = IA "animal" (Wander 1/120 por tick, raio 10x7, prefere grama);
//   galinha = IA própria: passeios curtos e frequentes, bicadas no chão, plana ao cair (asas), põe ovo a cada 5-10 min, pânico 1.4x.
enum { M_PIG, M_CHICKEN, M_COW, M_SHEEP, M_VILLAGER, M_PIGMAN, M_ZOMBIE, M_SKELETON, M_CREEPER, M_N };   // 6..8 = hostis novos (hostile.inc)
// spd = atributo movementSpeed do 1.8.9; panic = multiplicador do EntityAIPanic; ai: 0 = animal, 1 = galinha, 2 = aldeão, 3 = porco-zumbi (neutro; irritado persegue e ataca), 4 = zumbi, 5 = esqueleto, 6 = creeper (>= 4: hostis de hostile.inc)
struct MobDef { const char* nome; const char* tex; const char* wool; float w, h; int hp; float spd, panic; int ai; };
static const MobDef MD[M_N] = {
    {"porco",   "entity/pig/pig_temperate.png|entity/pig/pig.png", "", .9f, .9f, 10, .25f, 1.25f, 0},
    {"galinha", "entity/chicken/chicken_temperate.png|entity/chicken/temperate_chicken.png|entity/chicken.png", "", .4f, .7f, 4, .25f, 1.4f, 1},
    {"vaca",    "entity/cow/cow_temperate.png|entity/cow/cow.png", "", .9f, 1.3f, 10, .2f, 2.0f, 0},
    {"ovelha",  "entity/sheep/sheep.png", "entity/sheep/sheep_wool.png|entity/sheep/sheep_fur.png", .9f, 1.3f, 8, .23f, 1.25f, 0},
    {"aldeao",  "entity/villager/villager.png|entity/villager/type/plains.png|entity/villager.png", "", .6f, 1.8f, 20, .5f, 1.f, 2},
    {"porco-zumbi", "entity/piglin/zombified_piglin.png", "", .6f, 1.95f, 20, .23f, 1.f, 3},
    {"zumbi",   "entity/zombie/zombie.png", "", .6f, 1.95f, 20, .23f, 1.f, 4},
    {"esqueleto", "entity/skeleton/skeleton.png", "", .6f, 1.99f, 20, .25f, 1.f, 5},
    {"creeper", "entity/creeper/creeper.png", "", .6f, 1.7f, 20, .25f, 1.f, 6}};   // EntityVillager 1.8.9: 20 de vida, velocidade 0.5 (passeio a 0.6x)
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
    int fuse = 0, atk = 0, burn = 0; bool chase = false;   // hostis: pavio do creeper, recarga de ataque/flecha, queima ao sol, perseguindo
    int talkT = 0; bool hasHome = false; double homeX = 0, homeZ = 0;   // aldeao: conversa (para e olha o jogador) e centro da aldeia
    int rideBoat = 0;   // id do bote em que o animal está sentado (0 = nenhum)
    int uid = 0, revId = 0, revT = 0, fire = 0; float aim = 0, paim = 0;   // uid (lazy, ver mobUid); revide: uid do atacante + ticks; fogo: ticks restantes; aim: mira do arco do esqueleto (0..1)
    int air = 300;   // patch5.78: ar do mob (afogamento, igual ao jogador)
    int sayT = -80, hurt = 0, resist = 0, lastDmg = 0, panic = 0, jumpCd = 0, stuck = 0, lookT = 0, idleT = 0, peckT = 0, eggT = 0;
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
    static const int FB[M_N][2][3] = {{{240,160,160},{0,0,0}}, {{245,245,245},{0,0,0}}, {{90,60,40},{0,0,0}}, {{190,190,185},{240,240,240}}, {{130,95,70},{0,0,0}}, {{200,130,130},{0,0,0}}, {{70,130,70},{0,0,0}}, {{200,200,195},{0,0,0}}, {{60,160,50},{0,0,0}}};
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
    // aldeao (ModelVillager 1.8.9, textura 64x64): cabeca (0,0,0) + nariz, corpo + tunica (g .5), bracos cruzados (pivo 0,3,-1, rotX -0.75), pernas (+-2,12,0)
    {   auto mirU = [](std::vector<V> m) { for (int q = 0; q < (int)m.size() / 4; q++) { float lo = 1e9f, hi = -1e9f; for (int k = 0; k < 4; k++) { lo = std::min(lo, m[q * 4 + k].u); hi = std::max(hi, m[q * 4 + k].u); }
            for (int k = 0; k < 4; k++) m[q * 4 + k].u = lo + hi - m[q * 4 + k].u; }
            if (m.size() >= 16) for (int k = 0; k < 4; k++) { std::swap(m[8 + k].u, m[12 + k].u); std::swap(m[8 + k].v, m[12 + k].v); } return m; };   // ModelRenderer.mirror
        std::vector<V> h = bx(-4,4, 0,10, -4,4, 0,0, 8,10,8), n = bx(-1,1, -1,3, -6,-4, 24,0, 2,4,2); h.insert(h.end(), n.begin(), n.end());
        A(M_VILLAGER, h, 0,24,0, 0, false, 1);
        { std::vector<V> b = bx(-4,4, -12,0, -3,3, 16,20, 8,12,6), j = bx(-4,4, -18,0, -3,3, 0,38, 8,18,6, .5f); b.insert(b.end(), j.begin(), j.end()); A(M_VILLAGER, b, 0,24,0); }
        { std::vector<V> a1 = bx(4,8, -6,2, -2,2, 44,22, 4,8,4), a2 = mirU(bx(-8,-4, -6,2, -2,2, 44,22, 4,8,4)), a3 = bx(-4,4, -6,-2, -2,2, 40,38, 8,4,4);
          a1.insert(a1.end(), a2.begin(), a2.end()); a1.insert(a1.end(), a3.begin(), a3.end()); A(M_VILLAGER, a1, 0,21,-1, 42.97f); }
        A(M_VILLAGER, bx(-2,2, -12,0, -2,2, 0,22, 4,12,4), 2,12,0, 0, false, 6); A(M_VILLAGER, mirU(bx(-2,2, -12,0, -2,2, 0,22, 4,12,4)), -2,12,0, 0, false, 7); }
    {   auto rotZ = [](std::vector<V> m, float cx, float cy, float deg) { const float c = cosf(deg * .0174533f), sn = sinf(deg * .0174533f); for (V& v : m) { const float x = v.x - cx, y = v.y - cy; v.x = cx + x * c - y * sn; v.y = cy + x * sn + y * c; } return m; };
        std::vector<V> h = bx(-5,5, 0,8, -4,4, 0,0, 10,8,8), sn = bx(-2,2, 0,4, -5,-4, 31,1, 4,4,1), n1 = bx(-3,-2, 0,2, -5,-4, 2,4, 1,2,1), n2 = bx(2,3, 0,2, -5,-4, 2,0, 1,2,1);
        std::vector<V> e1 = rotZ(bx(-1,0, 1,6, -2,2, 51,6, 1,5,4), -4.5f, 6.f, -30.f), e2 = rotZ(bx(0,1, 1,6, -2,2, 39,6, 1,5,4), 4.5f, 6.f, 30.f);   // orelhas giradas 30 graus em torno da juncao com a cabeca
        for (auto* q : {&sn, &n1, &n2, &e1, &e2}) h.insert(h.end(), q->begin(), q->end());
        A(M_PIGMAN, h, 0,24,0, 0, false, 1);
        A(M_PIGMAN, body(bx(-4,4, -12,0, -2,2, 16,16, 8,12,4)), 0,24,0);
        A(M_PIGMAN, bx(-1,3, -10,2, -2,2, 40,16, 4,12,4), 5,22,0, 90);   // bracos esticados para frente (pose de zumbi)
        A(M_PIGMAN, bx(-3,1, -10,2, -2,2, 32,48, 4,12,4), -5,22,0, 90);
        A(M_PIGMAN, bx(-2,2, -12,0, -2,2, 0,16, 4,12,4), 1.9f,12,0, 0, false, 2); A(M_PIGMAN, bx(-2,2, -12,0, -2,2, 16,48, 4,12,4), -1.9f,12,0, 0, false, 3); }
    // --- hostis (ModelBiped/ModelCreeper 1.8.9): cabeca (0,24,0), corpo, bracos (0,22,0) rot X, pernas (+-2,12,0); zumbis/piglin: bracos esticados a frente (rot 90) ---
    {   auto mirB = [](std::vector<V> m) { for (int q = 0; q < (int)m.size() / 4; q++) { float lo = 1e9f, hi = -1e9f; for (int k = 0; k < 4; k++) { lo = std::min(lo, m[q * 4 + k].u); hi = std::max(hi, m[q * 4 + k].u); }
            for (int k = 0; k < 4; k++) m[q * 4 + k].u = lo + hi - m[q * 4 + k].u; }
            if (m.size() >= 16) for (int k = 0; k < 4; k++) { std::swap(m[8 + k].u, m[12 + k].u); std::swap(m[8 + k].v, m[12 + k].v); } return m; };
        auto biped64 = [&](int t, std::vector<V> head) {   // textura 64x64 (bracos/pernas esquerdos tem UV proprio)
            A(t, head, 0,24,0, 0, false, 1); A(t, bx(-4,4, -12,0, -2,2, 16,16, 8,12,4), 0,24,0);
            A(t, bx(4,8, -10,2, -2,2, 40,16, 4,12,4), 0,22,0, 90); A(t, mirB(bx(-8,-4, -10,2, -2,2, 40,16, 4,12,4)), 0,22,0, 90);   /* patchV.5: zumbi 64x32 do 1.12 nao tem UV de braco/perna esquerdos: espelha os direitos */
            A(t, bx(-2,2, -12,0, -2,2, 0,16, 4,12,4), 2,12,0, 0, false, 2); A(t, mirB(bx(-2,2, -12,0, -2,2, 0,16, 4,12,4)), -2,12,0, 0, false, 3); };
        biped64(M_ZOMBIE, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8));
        A(M_SKELETON, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8), 0,24,0, 0, false, 1); A(M_SKELETON, bx(-4,4, -12,0, -2,2, 16,16, 8,12,4), 0,24,0);
        A(M_SKELETON, bx(4,6, -10,2, -1,1, 40,16, 2,12,2), 0,22,0, 85, false, 8); A(M_SKELETON, mirB(bx(-6,-4, -10,2, -1,1, 40,16, 2,12,2)), 0,22,0, 90, false, 9);   // bracos mirando (arco)
        A(M_SKELETON, bx(-1,1, -12,0, -1,1, 0,16, 2,12,2), 2,12,0, 0, false, 2); A(M_SKELETON, mirB(bx(-1,1, -12,0, -1,1, 0,16, 2,12,2)), -2,12,0, 0, false, 3);
        A(M_CREEPER, bx(-4,4, 0,8, -4,4, 0,0, 8,8,8), 0,18,0, 0, false, 1); A(M_CREEPER, bx(-4,4, -12,0, -2,2, 16,16, 8,12,4), 0,18,0);
        for (float x : {-2.f, 2.f}) for (float z : {4.f, -4.f}) A(M_CREEPER, bx(-2,2, -6,0, -2,2, 0,16, 4,6,4), x,6,z, 0, false, legK(x, z)); }
    for (int t = 0; t < M_N; t++) for (MPart& p : MP[t]) {
        for (V& v : p.m) v.v *= vs[t][p.wool ? 1 : 0];
    }
    printf("Mobs: %d textura(s) vieram do resource pack\n", fromPack);
}

// ------------------------------- desenho -------------------------------
static void drawMobFire(const MobEnt& e, float pt);   // nether.inc (usa os tiles animados fire_0/fire_1)
static int gMobUidN = 0, gMobAtkUid = 0;   // gMobAtkUid: uid do mob que esta causando o dano (lido por mobHurt para o revide); 0 = jogador/ambiente
static int mobUid(MobEnt& e) { if (!e.uid) e.uid = ++gMobUidN; return e.uid; }
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
    const float aimF = e.type == M_SKELETON ? L(e.paim, e.aim) : 0.f;
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
                    case 6: rx = -57.29578f * cosf(ls * .6662f) * 1.4f * amt * .5f; break;   // ModelVillager: pernas a metade da amplitude
                    case 7: rx = -57.29578f * cosf(ls * .6662f + 3.14159265f) * 1.4f * amt * .5f; break;
                    case 8: rx = 50.f + (p.rx - 50.f) * aimF; break;   // esqueleto: braco do arco (relaxado -> mira)
                    case 9: rx = 15.f + (p.rx - 15.f) * aimF; break;   // esqueleto: braco que puxa a corda
                    case 4: rz = fl; break;
                    case 5: rz = -fl; break;
                }
                part(red ? p.red : p.m, p.px, p.py, p.pz, rx, ry, rz);
            }
        }
    };
    if (e.type == M_CREEPER && e.fuse > 0) { float f = std::min(1.f, (e.fuse + pt) / 30.f); const float f2 = 1.f + sinf(f * 100.f) * f * .01f; f *= f; f *= f; glScalef((1.f + f * .4f) * f2, (1.f + f * .1f) / f2, (1.f + f * .4f) * f2); }   // EntityCreeper: incha ao acender o pavio
    gEntHurt = (e.hurt > 0 || e.dead > 0 || (e.type == M_CREEPER && e.fuse > 0 && ((e.fuse / 3) & 1) == 0)) ? 1.f : 0.f;   // flash vermelho ao apanhar e na morte: tinge a propria textura (a copia vermelha sem textura pintava a caixa inteira dos pes da galinha)
    draw(false); gEntHurt = 0.f;
    if (e.type == M_SKELETON && e.dead == 0) {   // arco na mao direita: mesma cadeia do jogador (braco -> -90 em X -> punho); puxa em 3 estagios (bow_pulling_0..2) nos 20 ticks antes do tiro
        const int t = (aimF > .5f && e.atk < 20) ? 20 - e.atk : -1;
        glPushMatrix(); glTranslatef(5 / 16.f, 22 / 16.f, 0); glRotatef(50.f + (85.f - 50.f) * aimF, 1, 0, 0);
        glRotatef(-90, 1, 0, 0); glTranslatef(0, .125f, -.625f); drawHeld(bowPullItemT(IT_BOW, t), false); glPopMatrix();
    }
    if (e.fire > 0 && e.dead == 0) drawMobFire(e, pt);
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
static int mobPkItem(const char* file) { for (size_t i = 0; i < PK.size(); i++) if (PK[i].file == file) return PK0 + (int)i; return 0; }   // item do pack pelo nome
static int eggMob(int id) {
    { static const int ze = mobPkItem("item/zombie_spawn_egg"), se = mobPkItem("item/skeleton_spawn_egg"), ce = mobPkItem("item/creeper_spawn_egg"); if (id) { if (id == ze) return M_ZOMBIE; if (id == se) return M_SKELETON; if (id == ce) return M_CREEPER; } }
    static const int vegg = pkFind("item/villager_spawn_egg"), pegg = pkFind("item/zombified_piglin_spawn_egg"); if (vegg && id == vegg) return M_VILLAGER; if (pegg && id == pegg) return M_PIGMAN; return id >= IT_EGG_PIG && id <= IT_EGG_SHEEP ? id - IT_EGG_PIG : -1; }
static int mobEggItem(int t) {   // inverso de eggMob: ovo do mob (0 = nao tem); usado no pick block (botao do meio) em entidade
    switch (t) {
        case M_PIG: return IT_EGG_PIG; case M_CHICKEN: return IT_EGG_CHICKEN; case M_COW: return IT_EGG_COW; case M_SHEEP: return IT_EGG_SHEEP;
        case M_ZOMBIE: return mobPkItem("item/zombie_spawn_egg"); case M_SKELETON: return mobPkItem("item/skeleton_spawn_egg"); case M_CREEPER: return mobPkItem("item/creeper_spawn_egg");
        case M_VILLAGER: return pkFind("item/villager_spawn_egg"); case M_PIGMAN: return pkFind("item/zombified_piglin_spawn_egg");
    }
    return 0;
}
static bool mobInteract(const float* eye, const float* dir, float reach) {   // clique direito num aldeao: ele para e olha para o jogador
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead || MD[e.type].ai != 2) continue;
        const float hw = MD[e.type].w * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + MD[e.type].h + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    if (best < 0) return false;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return false;
    MOBS[best].talkT = 100; MOBS[best].hasT = false; return true;
}
struct VilSpawn { double x, y, z; }; static std::vector<VilSpawn> gVilQ;   // aldeoes pedidos pela geracao de aldeias (criados no tick)
static void vilQueueVillagers(double x, double y, double z) { static std::set<std::pair<long long, long long>> done; if (!done.insert({(long long)floor(x), (long long)floor(z)}).second) return; for (int i = 0; i < 3; i++) gVilQ.push_back({x + (i - 1) * 1.2, y, z + 2.5}); }
static void vilSpawnPending() {
    std::vector<VilSpawn> keep;
    for (const VilSpawn& s : gVilQ) {
        const int bx = (int)floor(s.x), bz = (int)floor(s.z);
        Chunk* ch = getC(fdiv(bx, CW), fdiv(bz, CW));
        if (!ch || !ch->lc) { keep.push_back(s); continue; }   // chunk ainda nao pronto: tenta de novo no proximo tick (antes o pedido era descartado e o aldeao nascia na terra)
        int c = 0; for (const MobEnt& m : MOBS) if (m.type == M_VILLAGER && fabs(m.x - s.x) < 24 && fabs(m.z - s.z) < 24) c++;
        if (c >= 3) continue;
        // chao livre mais proximo da altura pedida: bloco solido embaixo e 2 celulas livres (nao solidas, sem liquido) em cima; evita telhado e interior de parede
        int fy = -1, bestD = 1 << 30;
        for (int by = std::min(CH - 3, (int)floor(s.y) + 4); by >= std::max(1, (int)floor(s.y) - 6); by--) {
            if (!solid(getB(bx, by - 1, bz)) || solid(getB(bx, by, bz)) || solid(getB(bx, by + 1, bz)) || isLiquid(getB(bx, by, bz))) continue;
            const int dd = abs(by - (int)floor(s.y)); if (dd < bestD) { bestD = dd; fy = by; }
        }
        if (fy < 0) continue;   // sem lugar livre naquela coluna: nao nasce dentro de bloco
        spawnMob(M_VILLAGER, s.x, (double)fy, s.z, frand() * 360.f); MOBS.back().hasHome = true; MOBS.back().homeX = s.x; MOBS.back().homeZ = s.z - 2.5;
    }
    gVilQ.swap(keep);
}
static int mobWeaponDmg(int id) {   // dano base do 1.8.9 (4 + material nas espadas, 3 machados, 2 picaretas, 1 pás); sem arma = 1
    static const int S[5] = {4, 5, 6, 4, 7}, AX[5] = {3, 4, 5, 3, 6}, PK[5] = {2, 3, 4, 2, 5}, SH[5] = {1, 2, 3, 1, 4};
    if (id == IT_WSWORD) return S[0]; if (id >= IT_SSWORD && id <= IT_DSWORD) return S[id - IT_SSWORD + 1];
    if (id >= IT_WAXE && id < IT_WAXE + 5) return AX[id - IT_WAXE]; if (id >= IT_WSHOVEL && id < IT_WSHOVEL + 5) return SH[id - IT_WSHOVEL];
    if (id == IT_WPICK) return PK[0]; if (id == IT_SPICK) return PK[1]; if (id >= IT_IPICK && id <= IT_DPICK) return PK[id - IT_IPICK + 2];
    return 1;
}
static void mobDrops(const MobEnt& e) {   // tabelas do 1.8.9: porco 1-3 costeleta; galinha 0-2 penas + 1 frango; vaca 0-2 couro + 1-3 carne; ovelha 1 lã + 1-2 carneiro
    auto D = [&](int id, int n) { if (!id) return; for (int i = 0; i < n; i++) spawnDrop(e.x + (frand() - .5) * .4, e.y + MD[e.type].h * .5, e.z + (frand() - .5) * .4, frand() * .2 - .1, .2, frand() * .2 - .1, id, 1, 10); };
    auto R = [&](int lo, int hi) { return lo + rand() % (hi - lo + 1); };
    switch (e.type) {
        case M_PIG: D(IT_PORK, R(1, 3)); break;
        case M_CHICKEN: D(IT_FEATHER, R(0, 2)); D(IT_CHICKEN, 1); break;
        case M_COW: D(IT_LEATHER, R(0, 2)); D(IT_BEEF, R(1, 3)); break;
        case M_SHEEP: D(WOOL, 1); D(IT_MUTTON, R(1, 2)); break;
        case M_ZOMBIE: D(mobPkItem("item/rotten_flesh"), R(0, 2)); break;
        case M_SKELETON: D(IT_BONE, R(0, 2)); D(IT_ARROW, R(0, 2)); break;
        case M_CREEPER: D(mobPkItem("item/gunpowder"), R(0, 2)); break;
        case M_PIGMAN: { static const int rf = pkFind("item/rotten_flesh"), gn = pkFind("item/gold_nugget"); if (rf) D(rf, R(0, 1)); if (gn) D(gn, R(0, 1)); break; }
    }
}
// EntityLivingBase.attackEntityFrom (1.8.9): hurtResistantTime = 20 ticks após levar dano; na 1ª metade (resist > 10) só passa a diferença
// se o novo golpe for MAIOR que o último (lastDamage); na 2ª metade o golpe é integral. Golpe bloqueado = sem dano e sem knockback.
// Depois: knockBack(0.4 + 0.5 por nível extra, ex.: correndo), flash vermelho de 10 ticks e Panic de 100 ticks.
static bool mobSyncGuestHit(MobEnt& e, int dmg, float kx, float kz, bool attacked, float kbExtra, float lx, float lz);   // mobsync.inc: convidado manda o golpe ao host
static bool mobHurt(MobEnt& e, int dmg, float kx, float kz, bool attacked, float kbExtra = 0.f, float lx = 0.f, float lz = 0.f) {   // kx,kz = atacante -> alvo (posicao); lx,lz = direcao do olhar (parte do sprint)
    if (e.dead || dmg <= 0) return false;
    if (mobSyncGuestHit(e, dmg, kx, kz, attacked, kbExtra, lx, lz)) return true;   // convidado em rede: quem decide o dano e o host
    if (e.resist > 10) { if (dmg <= e.lastDmg) return false; e.hp -= dmg - e.lastDmg; e.lastDmg = dmg; }
    else { e.hp -= dmg; e.lastDmg = dmg; e.resist = 20; }
    e.hurt = 10; sndMob(e.type, e.x, e.y + .8, e.z, e.hp <= 0 ? 2 : 1);   // som de dano / morte (audio.inc)
    if (gMobAtkUid && gMobAtkUid != mobUid(e) && (e.type == M_ZOMBIE || e.type == M_SKELETON)) { e.revId = gMobAtkUid; e.revT = 400; }   // EntityAIHurtByTarget: revida em quem me feriu (20 s)
    if (attacked) {
        const float l = sqrtf(kx * kx + kz * kz);
        e.mx /= 2; e.mz /= 2; e.my = std::min(.4, e.my / 2 + .4);
        if (l > 1e-4f) { e.mx += kx / l * .4; e.mz += kz / l * .4; if (kbExtra > 0.f) { const float ll = sqrtf(lx * lx + lz * lz); const float ux = ll > 1e-4f ? lx / ll : kx / l, uz = ll > 1e-4f ? lz / ll : kz / l; e.mx += ux * kbExtra * .5; e.mz += uz * kbExtra * .5; e.my += .1; } }
        if (MD[e.type].ai == 3) { e.panic = 600; for (MobEnt& o : MOBS) if (&o != &e && !o.dead && MD[o.type].ai == 3 && fabs(o.x - e.x) < 32 && fabs(o.z - e.z) < 32) o.panic = std::max(o.panic, 600); }
        else if (MD[e.type].ai < 3) { e.panic = 100; e.hasT = false; }   // hostis novos (ai >= 4) nao fogem
        if (MD[e.type].ai == 3) e.hasT = false;
    }
    if (e.hp <= 0) { e.dead = 1; e.hasT = false; e.rideBoat = 0; mobDrops(e); survAddXp(MD[e.type].ai >= 4 || MD[e.type].ai < 3 ? 5 : 1 + rand() % 3); }
    return true;
}
// crit = caindo no ar (fallDistance > 0, fora d'água): dano x1.5; sprint = knockback extra. Sem instakill: o criativo causa o dano normal da arma.
static int mobPickType(const float* eye, const float* dir, float reach) {   // pick block em entidade: tipo do mob mirado (alcance reach, sem bloco na frente) ou -1
    int best = -1; float bt = reach;
    for (int i = 0; i < (int)MOBS.size(); i++) {
        const MobEnt& e = MOBS[i]; if (e.dead) continue;
        const float hw = MD[e.type].w * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + MD[e.type].h + .1f, (float)e.z + hw};
        float t0 = 0.f, t1 = bt; bool ok = true;
        for (int a = 0; a < 3 && ok; a++) {
            if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
            float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
        }
        if (ok && t0 < bt) { bt = t0; best = i; }
    }
    if (best < 0) return -1;
    const float tb = raySolidDistance(eye, dir, reach); if (tb >= 0.f && tb < bt) return -1;
    return MOBS[best].type;
}
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
static void hostileAI(MobEnt& e, double ppx, double ppy, double ppz); static void hostileTick(MobEnt& e, Phys& P); static void hostileSpawn(Phys& P); static void animalSpawn(Phys& P);   // hostile.inc
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
        float mul = d.ai == 2 ? .6f : 1.f;   // EntityAIWander(0.6) do aldeao
        if (d.ai == 2) { e.panic = 0; if (e.talkT > 0) { e.talkT--; e.hasT = false; e.lookT = std::max(e.lookT, 2); } }   // sem panico; conversando: para e olha o jogador
        if (d.ai >= 4) { e.panic = 0; hostileAI(e, ppx, ppy, ppz); }   // zumbi/esqueleto/creeper: persegue, recua ou acende o pavio
        if (d.ai == 3 && creative) e.panic = 0;   // mobs hostis ignoram jogador no criativo
        if (d.ai == 3 && e.panic > 0) {   // porco-zumbi irritado: corre ate o jogador e bate (dano 5, a cada 1 s)
            e.panic--; mul = 1.6f; e.tx = ppx; e.tz = ppz; e.hasT = true;
            if (e.talkT > 0) e.talkT--; else if (pd2 < 2.6 && fabs(pdy) < 1.8 && !gDead) { e.talkT = 20; survDamage(5, "Foi morto por Porco-zumbi"); }
        }
        else if (e.panic > 0) { e.panic--; mul = d.panic; if (!e.hasT) pickTarget(e, 5, 4); }
        else if (!e.hasT && e.talkT <= 0 && e.fuse <= 0) {
            if (d.ai == 2) { if (rand() % 120 == 0) { if (e.hasHome && (e.x - e.homeX) * (e.x - e.homeX) + (e.z - e.homeZ) * (e.z - e.homeZ) > 576.0) { e.tx = e.homeX; e.tz = e.homeZ; e.hasT = true; } else pickTarget(e, 10, 7); } }   // MoveTowardsRestriction: nao se afasta > 24 blocos da aldeia
            else if (d.ai == 1) { if (rand() % 60 == 0) pickTarget(e, 6, 3); }   // galinha: passeios curtos e frequentes
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
        if (d.ai == 1 && --e.eggT <= 0) { e.eggT = 6000 + rand() % 6000; spawnDrop(e.x, e.y + .3, e.z, 0, .1, 0, IT_EGG, 1, 10); sndPlayAt("mob/chicken/plop", e.x, e.y, e.z, 1.f, sndRandPitch(.1f)); }   // EntityChicken: ovo a cada 5-10 min
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
static bool mobTouchesFire(const AABB& r) {
    for (int y = (int)floor(r.y0); y <= (int)floor(r.y1); y++) for (int z = (int)floor(r.z0); z <= (int)floor(r.z1); z++) for (int x = (int)floor(r.x0); x <= (int)floor(r.x1); x++) if (getB(x, y, z) == XB_FIRE) return true;
    return false;
}
static void mobAirTick(MobEnt& e) {   // patch5.78: afogamento de mob (1.8.9): cabeca na agua gasta 1 de ar por tick; a -20 leva 2 de dano e o ar volta a 0; fora da agua enche na hora
    if (e.dead) return;
    const MobDef& d = MD[e.type];
    if (isWater(getB((int)floor(e.x), (int)floor(e.y + d.h * .85), (int)floor(e.z)))) { if (--e.air <= -20) { e.air = 0; mobHurt(e, 2, 0, 0, false); } } else e.air = 300;
}
static void mobFireTick(MobEnt& e) {
    mobAirTick(e);   // fogo/lava incendeiam todo mob (menos porco-zumbi); agua apaga; queimando = 1 de dano por segundo (como o Minecraft)
    if (e.dead || e.type == M_PIGMAN) { e.fire = 0; return; }
    const MobDef& d = MD[e.type]; const double hw = d.w * .5;
    const AABB bb{e.x - hw + .001, e.y + .001, e.z - hw + .001, e.x + hw - .001, e.y + d.h - .001, e.z + hw - .001};
    const int before = e.fire;
    if (anyLava(bb)) { mobHurt(e, 4, 0, 0, false); e.fire = std::max(e.fire, 300); }   // lava: 4 de dano (respeita o tempo de invulnerabilidade) e 15 s de fogo
    else if (mobTouchesFire(bb)) { mobHurt(e, 1, 0, 0, false); e.fire = std::max(e.fire, 160); }   // bloco de fogo: 8 s
    if (e.inWater) e.fire = 0;
    if (e.fire > 0 && before == 0 && MD[e.type].ai < 3) { e.panic = std::max(e.panic, 100); e.hasT = false; }   // animais fogem queimando
    if (e.fire > 0) { e.fire--; if (e.fire % 20 == 0 && !e.dead) mobHurt(e, 1, 0, 0, false); }
}
static bool gSplit = false;   // split screen ligado (splitscreen.inc / gamepad.inc)
static bool split2MobTick(MobEnt& e, Phys& P1);   // splitscreen.inc
static bool mobSyncGuestTick(); static bool mobRemoteTick(MobEnt& e, Phys& P); static void mobSyncHostTick(Phys& P);   // mobsync.inc
static void tickMobs(Phys& P) {
    const bool guest = mobSyncGuestTick();   // convidado em rede: nao simula, so interpola o que o host manda
    if (!guest) { hostileSpawn(P); animalSpawn(P); }
    const double ppx = P.x, ppy = P.y, ppz = P.z;
    if (!guest) { if (!gVilQ.empty()) vilSpawnPending(); mobPush(P); }
    for (size_t i = 0; i < PUFFS.size();) {
        Puff& p = PUFFS[i]; p.px = p.x; p.py = p.y; p.pz = p.z;
        if (p.age++ >= p.max) { PUFFS.erase(PUFFS.begin() + i); continue; }
        p.vy += .004f; p.x += p.vx; p.y += p.vy; p.z += p.vz; p.vx *= .9f; p.vy *= .9f; p.vz *= .9f; i++;
    }
    for (size_t i = 0; !guest && i < MOBS.size();) {
        MobEnt& e = MOBS[i];
        if (gSndOn && !e.dead) { const MobSnd* ms = mobSndFor(e.type); if (ms && ms->idle && rand() % 1000 < e.sayT++) { e.sayT = -80; sndMob(e.type, e.x, e.y + .8, e.z, 0); } }   // som ocioso (EntityLiving.getTalkInterval: 1/1000 que cresce por tick)
        if (e.y < -30) { MOBS.erase(MOBS.begin() + i); continue; }
        if (!getC(fdiv((int)floor(e.x), CW), fdiv((int)floor(e.z), CW))) { i++; continue; }   // chunk não carregado: congela
        if (mobRemoteTick(e, P)) mobFireTick(e);   // host: jogador remoto e o alvo mais proximo
        else if (gSplit && split2MobTick(e, P)) mobFireTick(e);   // jogador 2 e o alvo mais proximo: roda no contexto dele
        else { tickMob(e, ppx, ppy, ppz); hostileTick(e, P); mobFireTick(e); }
        if (e.dead && ++e.dead >= 21) { spawnPoof(e); MOBS.erase(MOBS.begin() + i); continue; }
        i++;
    }
    mobSyncHostTick(P);   // host: envia os mobs aos convidados
}
