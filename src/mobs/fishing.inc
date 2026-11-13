// ================= vara de pesca (BUG-019, EntityFishHook do 1.12.2, simplificado) =================
// Botao direito com a vara: lanca a boia (vel. 1,5, gravidade 0,03, atrito 0,92). Na agua ela flutua; apos 5-30 s vem a mordida
// (boia afunda + som de splash, janela de ~1 s); botao direito de novo: recolhe e, se estava na mordida, entrega o peixe/lixo/tesouro
// (85% peixe, 10% lixo, 5% tesouro). Gasta 1 de durabilidade (2 se a boia ficou em terra firme). Solo/host e convidado: so quem pesca ve a boia
// (sem pacote de rede; o item pescado cai direto no inventario local).
struct Bobber { double x = 0, y = 0, z = 0, px = 0, py = 0, pz = 0, vx = 0, vy = 0, vz = 0; int age = 0, wait = 0, bite = 0; bool water = false, ground = false; };
static Bobber gBob; static bool gRodPrev = false;
static void rodInit() {
    static bool done = false; if (done) return;
    if (cxIndex().empty()) return;   // indice dos itens do pack ainda nao montado
    done = true; gRodId = cxId("fishing_rod_uncast|fishing_rod"); gRodCastId = cxId("fishing_rod_cast");
}
static void rodReel(Slot& held, const Phys& P, bool success);
static void rodCancel() { gBobOn = false; gBob = Bobber(); }
static void rodCast(const Phys& P, float yaw, float pitch) {
    const float cy = cosf(yaw * .0174533f), sy = sinf(yaw * .0174533f), cp = cosf(pitch * .0174533f), sp = sinf(pitch * .0174533f);
    double d[3] = {-sy * cp, sp, -cy * cp};
    for (int a = 0; a < 3; a++) d[a] += arrowGauss() * .0075;
    const double l = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    gBob = Bobber(); gBob.x = gBob.px = P.x - sy * .3; gBob.y = gBob.py = P.y + (P.sneak ? 1.54 : 1.62) - .1; gBob.z = gBob.pz = P.z - cy * .3;
    gBob.vx = d[0] / l * 1.5; gBob.vy = d[1] / l * 1.5; gBob.vz = d[2] / l * 1.5; gBobOn = true;
    sndEvtAt("bow.shoot", P.x, P.y + 1.4, P.z, .25f);
}
static int rodLoot() {   // 85% peixe, 10% lixo, 5% tesouro (sem encantamentos): 0 = nada disponivel no pack
    const int r = rand() % 100; static int cod = -1;
    if (cod < 0) cod = 0;
    auto pick = [](std::initializer_list<const char*> L) { std::vector<int> v; for (const char* n : L) { const int id = cxId(n); if (id) v.push_back(id); } return v.empty() ? 0 : v[rand() % v.size()]; };
    if (r < 85) { const int f = rand() % 100; const char* n = f < 60 ? "fish_cod_raw|cod" : f < 85 ? "fish_salmon_raw|salmon" : f < 87 ? "fish_clownfish_raw|clownfish" : "fish_pufferfish_raw|pufferfish"; const int id = cxId(n); return id ? id : cxId("fish_cod_raw|cod"); }
    if (r < 95) { const int j = rand() % 8; switch (j) { case 0: return IT_STICK; case 1: return cxId("string"); case 2: return cxId("bone"); case 3: return cxId("leather"); case 4: return cxId("rotten_flesh"); case 5: return cxId("bowl"); case 6: return cxId("stick"); default: return pick({"fish_cod_raw|cod", "bone", "string"}); } }
    switch (rand() % 4) { case 0: return IT_BOW; case 1: return cxId("saddle"); case 2: return cxId("name_tag"); default: return cxId("lily_pad|waterlily"); }
}
static void rodReel(Slot& held, const Phys& P, bool success) {
    if (!gBobOn) return;
    int wear = gBob.water ? 0 : 2;
    if (success) {
        const int id = rodLoot();
        if (id) {   // o item voa da boia ate o jogador (EntityFishHook.handleHookRetraction)
            const double dx = P.x - gBob.x, dy = P.y + 1.0 - gBob.y, dz = P.z - gBob.z, dl = std::max(.1, sqrt(dx * dx + dy * dy + dz * dz));
            spawnDrop(gBob.x, gBob.y + .1, gBob.z, dx / dl * .2, dy / dl * .2 + std::min(.3, sqrt(dl) * .08), dz / dl * .2, id, 1, 5);
        }
        sndEvtAt("splash", gBob.x, gBob.y, gBob.z, .6f); wear = 1;
    }
    if (wear > 0) itemWear(held, wear);
    rodCancel();
}
static void rodStep(const Phys& P) {   // EntityFishHook.onUpdate (1 tick)
    Bobber& b = gBob; b.px = b.x; b.py = b.y; b.pz = b.z; b.age++;
    const int fx = (int)floor(b.x), fy = (int)floor(b.y), fz = (int)floor(b.z);
    b.water = isWater(getB(fx, fy, fz));
    if (b.ground) return;
    if (b.water) {   // flutua: mola ate a superficie (topo do bloco - 0,12)
        const bool above = isWater(getB(fx, fy + 1, fz));
        b.vx *= .8; b.vz *= .8; b.vy = above ? std::min(.15, b.vy + .05) : ((fy + .88) - b.y) * .35;
        // espera pela mordida so quando quase parada na superficie
        if (b.age > 10 && fabs(b.vx) + fabs(b.vz) < .05 && !above) {
            if (b.wait <= 0 && b.bite <= 0) b.wait = 100 + rand() % 500;
            if (b.bite > 0) { if (--b.bite == 0) b.wait = 100 + rand() % 500; }
            else if (--b.wait <= 0) { b.bite = 15 + rand() % 15; b.wait = 0; sndEvtAt("splash", b.x, b.y, b.z, .5f); }
        }
    } else { b.vy -= .03; b.vx *= .92; b.vy *= .92; b.vz *= .92; }
    double nx = b.x + b.vx, ny = b.y + b.vy, nz = b.z + b.vz;
    if (solid(getB((int)floor(nx), (int)floor(ny), (int)floor(nz)))) {   // bateu em bloco solido: para (em terra firme conta como "ground")
        b.vx = b.vy = b.vz = 0; b.ground = !b.water; nx = b.x; ny = b.y; nz = b.z;
    }
    b.x = nx; b.y = ny; b.z = nz;
}
static void rodTick(Phys& P, bool can, Slot& held, float yaw, float pitch) {   // 1x por tick, depois do movimento
    rodInit();
    const bool rmb = gUseOverride >= 0 ? gUseOverride != 0 : ((gpMouseState() & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || (touchUse && gTouchUseItem));
    const bool holding = gRodId && held.id == gRodId;
    if (gBobOn) {   // cancelamentos: trocou de item, morreu, longe demais, mudou de dimensao/mundo
        const double dx = gBob.x - P.x, dy = gBob.y - P.y, dz = gBob.z - P.z;
        if (!holding || gDead || dx * dx + dy * dy + dz * dz > 1024.0 || gBob.age > 24000) { rodCancel(); }
    }
    const bool press = rmb && !gRodPrev; gRodPrev = rmb;
    if (!press || !can || gDead || gGm == 3 || !holding) { if (gBobOn) rodStep(P); return; }
    if (gBobOn) rodReel(held, P, gBob.bite > 0);
    else rodCast(P, yaw, pitch);
    if (gBobOn) rodStep(P);
}
static void fishingDraw(float pt, const Phys& P, float yawDeg, float pitchDeg) {
    if (!gBobOn) return;
    const double bx = gBob.px + (gBob.x - gBob.px) * pt, bz = gBob.pz + (gBob.z - gBob.pz) * pt;
    double by = gBob.py + (gBob.y - gBob.py) * pt; if (gBob.bite > 0) by -= .18;   // afunda na mordida
    const float cy = cosf(yawDeg * .0174533f), sy = sinf(yawDeg * .0174533f);
    const double ex = P.x - sy * .55 + cy * .38, ey = P.y + (P.sneak ? 1.54 : 1.62) - .32, ez = P.z - cy * .55 - sy * .38;   // ponta da vara (aprox.)
    rdDisable(GL_TEXTURE_2D); rdDisable(GL_ALPHA_TEST); glColor4f(1.f, 1.f, 1.f, 1.f);
    const float s = .1f, rx = cy * s, rz = -sy * s;   // billboard: direita da camera
    glBegin(GL_QUADS);   // metade de cima vermelha, de baixo branca
    glColor3f(.85f, .1f, .1f); glVertex3f((float)(bx - rx), (float)(by + s), (float)(bz - rz)); glVertex3f((float)(bx + rx), (float)(by + s), (float)(bz + rz)); glVertex3f((float)(bx + rx), (float)by, (float)(bz + rz)); glVertex3f((float)(bx - rx), (float)by, (float)(bz - rz));
    glColor3f(1.f, 1.f, 1.f); glVertex3f((float)(bx - rx), (float)by, (float)(bz - rz)); glVertex3f((float)(bx + rx), (float)by, (float)(bz + rz)); glVertex3f((float)(bx + rx), (float)(by - s), (float)(bz + rz)); glVertex3f((float)(bx - rx), (float)(by - s), (float)(bz - rz));
    glEnd();
    glColor4f(.05f, .05f, .05f, 1.f); glBegin(GL_LINES);   // linha: 16 segmentos com leve curva (RenderFish)
    double lx = ex, ly = ey, lz = ez;
    for (int i = 1; i <= 16; i++) {
        const double t = i / 16.0, k = (t * t + t) * .5, nx = ex + (bx - ex) * t, nz = ez + (bz - ez) * t, ny = ey + (by + .25 - ey) * k;
        glVertex3f((float)lx, (float)ly, (float)lz); glVertex3f((float)nx, (float)ny, (float)nz); lx = nx; ly = ny; lz = nz;
    }
    glEnd(); glColor4f(1.f, 1.f, 1.f, 1.f); rdEnable(GL_TEXTURE_2D);
}
