// patchBlood.1: DIMENSAO RUBRA (gDim == 3) - o "mundo invertido": o MESMO mundo (mesma seed e mesmas coordenadas), mas corrompido.
// Terreno IDENTICO ao do overworld (mesmos blocos), tudo tingido de vermelho (malha em bloodMeshTint; grama/folhas/agua tingidas de
// sangue (cores em biomeSampleCol/biomeWaterTint), ceu carmesim parado num crepusculo eterno, tempestade de sangue constante
// (chuva vermelha + raios, weather.inc) e nuvens vermelhas (nuvens ja sao desenhadas pelo overworld: aqui so troca a cor).
// ENTRADA: so dormindo (sleep.inc rola a chance durante o sono). SAIDA: dormir numa cama dentro dela (todos os jogadores da dimensao
// precisam estar dormindo, como no overworld) ou morrer. Ao chegar, uma cama vermelha e colocada no ponto de chegada.
// Salvo em saves/<mundo>/chunks16r.dat (nether.inc/menu.inc). Incluido DEPOIS de propsys.inc (usa Phys/net/survival) e ANTES de nether.inc
// (dimSwitch e declarado em main.cpp).

// (gBloodDreamPct = chance % do sono virar pesadelo e gBloodSafe = "acabou de voltar": declarados em main.cpp)
static bool sleepOthersInOverworld(); static void sleepSkipNight(); static void split2Follow(const Phys& p1); static bool sleepBedGeom(int x, int y, int z, int& fx, int& fz, int& hx, int& hz); static void sleepWakeSpot(int fx, int y, int fz, int hx, int hz, double* o);   // sleep.inc / splitscreen.inc
static const int BLOOD_BED_COLOR = 14; // cama vermelha (var 14 de XB_BED)

static inline uint32_t bdh(int x, int y, int z, uint32_t s) { uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)z * 2147483629u + s * 1274126177u; h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16); }

// ---- cores ----
static void bloodGrassCol(int ix, int iz, float* a, float* c) {   // grama (a) e folha (c): vermelho escuro com manchas largas (blocos de 6x6 + variacao fina)
    const float n = (bdh(ix / 6 - (ix < 0), 0, iz / 6 - (iz < 0), 77u) & 255u) / 255.f, f = (bdh(ix, 1, iz, 78u) & 255u) / 255.f;
    a[0] = .38f + .22f * n + .05f * f; a[1] = .015f; a[2] = .03f + .02f * n;
    c[0] = .52f + .25f * n + .06f * f; c[1] = .02f + .02f * f; c[2] = .035f;
}
static void bloodWaterTint(float* w) { w[0] = 1.f; w[1] = .07f; w[2] = .08f; }   // agua = sangue (a textura azul vira vermelho escuro)
static void bloodUwHook(float* uw) { uw[0] = .26f; uw[1] = .01f; uw[2] = .015f; }   // debaixo do sangue: neblina vermelha densa

// ---- ceu: cores carmesim; o clarao dos raios (gFlash) vira branco-rosado ----
static void bloodSkyHook(float* top, float* hor) {
    if (gDim != 3) return;
    const float fl = gFlash, pul = .5f + .5f * sinf((float)SDL_GetTicks() * .0004f);   // respiracao lenta do ceu
    top[0] = std::min(1.f, .17f + .05f * pul + fl * .9f); top[1] = std::min(1.f, .0f + fl * .35f); top[2] = std::min(1.f, .012f + fl * .35f);
    hor[0] = std::min(1.f, .42f + .10f * pul + fl * .6f); hor[1] = std::min(1.f, .025f + fl * .3f); hor[2] = std::min(1.f, .035f + fl * .3f);
    gSky.sunB = std::max(gSky.sunB, .40f); gSkyScale = std::max(gSkyScale, .42f); gSkyRB = 1.f;   // nunca escuro demais (e avermelhado)
    gSky.cloudC[0] = .55f + .2f * pul; gSky.cloudC[1] = .035f; gSky.cloudC[2] = .045f; gSky.cloud = .5f;   // nuvens de sangue
}

// ---- terreno: IDENTICO ao overworld (terra continua terra, grama continua grama...). O vermelho vem so da cor: bloodMeshTint (fim de build() em
// world.inc) multiplica a cor de todos os vertices solidos; grama/folhas/agua tem cor propria (bloodGrassCol/bloodWaterTint). Nada e trocado aqui. ----
static void bloodTransformChunk(Chunk* c, int cx, int cz) { (void)c; (void)cx; (void)cz; }
static void bloodMeshTint(std::vector<V>& m) {   // tudo "versao vermelha": mantem o vermelho e derruba verde/azul (terra marrom -> terra avermelhada, pedra cinza -> rosada)
    for (V& q : m) { q.g *= .42f; q.b *= .40f; }
}

// ---- entrada / saida ----
static bool bloodBedSpot(int x, int z, int* yOut) {   // cama de 2 blocos (x,z)+(x+1,z): chao solido seco e plano, ar livre em cima
    int ys[2];
    for (int k = 0; k < 2; k++) {
        int y = CH - 3; for (; y > 1; y--) { const int b = getB(x + k, y, z); if (b == AIR || b == LEAVES || (b >= XB_PLANT && b < XB_SAPLING + 6) || b == XB_DEADBUSH) continue; break; }
        const int b = getB(x + k, y, z); if (y <= 1 || !solid(b) || isLiquid(b) || b == LEAVES || b == LOG) return false; ys[k] = y;
    }
    if (ys[0] != ys[1]) return false;
    const int y = ys[0] + 1; if (y + 2 >= CH) return false;
    for (int k = 0; k < 2; k++) for (int yy = y; yy <= y + 1; yy++) { const int b = getB(x + k, yy, z); if (b != AIR && !(b >= XB_PLANT && b < XB_SAPLING + 6) && b != XB_DEADBUSH) return false; }
    *yOut = y; return true;
}
static void bloodPlaceBed(int x, int y, int z) {   // pf = 1 (cabeca para +X): meta = dir | cor << 3, cabeca = meta | 4
    const int meta = 1 | (BLOOD_BED_COLOR << 3);
    setBM(x, y, z, XB_BED, meta); netSendBlock(x, y, z, XB_BED, meta);
    setBM(x + 1, y, z, XB_BED, meta | 4); netSendBlock(x + 1, y, z, XB_BED, meta | 4);
}
static void bloodEnter(Phys& P) {   // chamado por sleep.inc no meio do sono (tela escura): o jogador acorda no mesmo ponto do mundo corrompido
    if (gDim != 0) return;
    const int X = (int)floor(P.x), Z = (int)floor(P.z);
    dimSwitch(3);
    for (int dz = -2; dz <= 2; dz++) for (int dx = -2; dx <= 2; dx++) if (!getC(fdiv(X, CW) + dx, fdiv(Z, CW) + dz)) gen(fdiv(X, CW) + dx, fdiv(Z, CW) + dz);
    int bx = X, bz = Z, by = -1;
    for (int r = 0; r <= 8 && by < 0; r++) for (int dz = -r; dz <= r && by < 0; dz++) for (int dx = -r; dx <= r; dx++) {
        if (std::max(abs(dx), abs(dz)) != r) continue; int y; if (bloodBedSpot(X + dx, Z + dz, &y)) { bx = X + dx; bz = Z + dz; by = y; break; } }
    if (by < 0) {   // sem lugar plano: aplaina um (chao de terra, ar em cima)
        int y = CH - 3; while (y > 2 && !solid(getB(X, y, Z))) y--; by = std::max(2, y) + 1; bx = X; bz = Z;
        for (int k = 0; k < 2; k++) { setB(bx + k, by - 1, bz, DIRT); setB(bx + k, by, bz, AIR); setB(bx + k, by + 1, bz, AIR); }
    }
    bloodPlaceBed(bx, by, bz);
    P.x = P.px = bx + .5; P.z = P.pz = bz + .5; P.y = P.py = by + .5625 + .02;   // em pe em cima da cama
    P.mx = P.my = P.mz = 0; P.onGround = false; gFall = 0.f; gFire = 0; gKick = true;
    gNetherYaw = 0.f; gNetherPitch = 0.f; gNetherTurn = true; split2Follow(P);
    chatAdd("Você adormeceu... e acordou num pesadelo. A chuva é de sangue.", 0xFF5555);
    chatAdd("Durma na cama vermelha para escapar (todos na dimensão precisam estar dormindo).", 0xFF8888);
}
static void bloodLeave(Phys& P) {   // volta ao overworld, na cama onde adormeceu (o ponto de renascimento)
    if (gDim != 3) return;
    dimSwitch(0);
    P.x = P.px = gSpX; P.z = P.pz = gSpZ; P.y = P.py = gSpY; P.mx = P.my = P.mz = 0; P.onGround = false; gRespHold = 200; survRespawnFix(P);
    { int fx, fz, hx, hz; const int by = (int)floor(gSpY - .6 + .01); if (sleepBedGeom((int)floor(gSpX), by, (int)floor(gSpZ), fx, fz, hx, hz)) { double o[3]; sleepWakeSpot(fx, by, fz, hx, hz, o); P.x = P.px = o[0]; P.y = P.py = o[1]; P.z = P.pz = o[2]; } }   // patchSleep.4: acorda ao lado da cama, nao dentro dela
    gFall = 0.f; gFire = 0; gKick = true; gBloodSafe = true; split2Follow(P);
    if ((!netOn || netHost) && gDayCycle && (gWorldTime % DAY_TICKS) >= 12500 && !sleepOthersInOverworld()) sleepSkipNight();   // sonhou a noite inteira: amanhece (so se ninguem mais esta acordado la)
    chatAdd("Você acordou. Foi só um pesadelo...", 0xFFAAAA);
}
