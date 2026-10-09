// ================= sobrevivência: vida, fome, dano, regeneração, morte =================
// Valores em "meios": 20 = 10 corações / 10 coxas (como o Java 1.8.9). Só vale em sobrevivência (F4 / /gamemode 0).
static void chatAdd(const std::string& s, unsigned col);   // definido em chat_debug.inc
static int gBowT = -1;        // arco: ticks de carga (-1 = sem carregar) (bow.inc)
static bool gBowOn = false;   // carregando: andar a 20% e sem sprint (usar item, como no Java)
static void applyGm(int g) { gGm = g & 3; creative = (gGm == 1 || gGm == 3); }
static void setGm(int g);   // chat_debug.inc
static int gHP = 20, gFood = 20;
static double gSpX = 8.5, gSpY = 60, gSpZ = 8.5;   // ponto de spawn seguro (calculado ao carregar o mundo, menu.inc)
static float gSat = 5.f, gExh = 0.f, gFall = 0.f;
static int gFire = 0;   // ticks em chamas (lava = 300 = 15 s; agua apaga)
static int gHurt = 0, gInvul = 0, gRegenT = 0, gDeadT = 0, gEatCd = 0;
static bool gDead = false, gWantRespawn = false;
static std::string gDeathWhy;
static float gHurtAtYaw = 0.f, gYawNow = 0.f;   // attackedAtYaw (1.8.9) e yaw atual do jogador
static void deathDropAll(); static void netDeathAnnounce(const std::string& why); static void netRespawnAnnounce(); static void netHurtAnnounce();   // pvp.inc

static void localDeathPose(float pt) {   // RendererLivingEntity.rotateCorpse: o corpo tomba ate 90 graus em torno dos pes (F5 / reflexo / sombra)
    if (!gDead || creative) return; float f = sqrtf(std::max(0.f, (gDeadT + pt - 1.f) / 20.f * 1.6f)); if (f > 1.f) f = 1.f; glRotatef(-f * 90.f, 0, 0, 1);
}
static int gLastDmg = 0;   // dano do ultimo golpe (1.8.9 lastDamage)
static int armorPoints() {   // pontos de defesa vanilla 1.8.9: couro 1/3/2/1, ferro 2/6/5/2, ouro 2/5/3/1, diamante 3/8/6/3
    static const int T[4][4] = {{1, 3, 2, 1}, {2, 6, 5, 2}, {2, 5, 3, 1}, {3, 8, 6, 3}}; int a = 0;
    for (int s = 0; s < 4; s++) if (armorOk(s, armor[s].id)) a += T[(armor[s].id - IT_A0) / 4][s];
    return a;
}
static bool survDamage(int n, const char* why, bool force = false) {   // force ignora a invulnerabilidade e a armadura (queda, vazio, fome, comando)
    if (creative || gDead || n <= 0) return false;
    if (!force) { if (const int ap = armorPoints()) n = std::max(1, (int)lroundf(n * (25 - ap) / 25.f)); }   // ArmorProperties: dano * (25 - defesa) / 25
    if (!force && gInvul > 10) { if (n <= gLastDmg) return false; const int d = n - gLastDmg; gLastDmg = n; gHP = std::max(0, gHP - d); gHurt = 20; }   // 1.8.9: 1a metade dos 20 ticks so passa a diferenca
    else { if (gInvul > 0 && !force) { /* 2a metade: golpe integral */ } gLastDmg = n; gHP = std::max(0, gHP - n); gHurt = 20; gInvul = 20; }
    gHurtAtYaw = 0.f; netHurtAnnounce();   // so chega aqui se o dano foi aplicado (criativo/invulneravel ja retornaram acima)
    if (gHP <= 0) { gDead = true; gDeadT = 0; gDeathWhy = why; chatAdd(std::string("Você morreu: ") + why, 0xFF5555); deathDropAll(); netDeathAnnounce(why); }
    return true;
}
static void survExh(float a) { if (!creative && !gDead) gExh += a; }
static int foodValue(int id) {
    switch (id) { case IT_APPLE: return 4; case IT_GAPPLE: return 4; case IT_BREAD: return 5; case IT_COOKIE: return 2;
                  case IT_BEEF: return 3; case IT_STEAK: return 8; case IT_CARROT: return 3;
                  case IT_PORK: return 3; case IT_CPORK: return 8; case IT_CHICKEN: return 2; case IT_CCHICKEN: return 6; case IT_MUTTON: return 2; case IT_CMUTTON: return 6;
                  default: return 0; }
}
static bool survTryEat(Slot& s) {   // botão direito com comida na mão (instantâneo) quando a fome não está cheia
    if (creative || gDead || gEatCd > 0 || !s.id) return false;
    const int fv = foodValue(s.id);
    if (!fv || gFood >= 20) return false;
    gFood = std::min(20, gFood + fv); gSat = std::min((float)gFood, gSat + fv * .6f); gEatCd = 16;
    if (--s.n <= 0) s = Slot();
    return true;
}
static int gRespHold = 0;   // ticks restantes para corrigir a altura do renascimento
static void survRespawnFix(Phys& P) {   // coloca o jogador em cima do chao do spawn (nunca dentro de blocos)
    if (gRespHold <= 0) return;
    const int sx = (int)floor(gSpX), sz = (int)floor(gSpZ);
    if (!getC(fdiv(sx, CW), fdiv(sz, CW))) { P.x = P.px = gSpX; P.z = P.pz = gSpZ; P.y = P.py = gSpY; P.mx = P.my = P.mz = 0; return; }   // chunk ainda nao carregou: fica parado
    int y = std::max(1, std::min(CH - 4, (int)floor(gSpY + .01)));
    while (y < CH - 3 && (solid(getB(sx, y, sz)) || solid(getB(sx, y + 1, sz)))) y++;   // dentro de bloco: sobe
    for (int k = 0; k < 12 && y > 1 && !solid(getB(sx, y - 1, sz)); k++) y--;          // no ar: desce ate o chao
    P.x = P.px = gSpX; P.z = P.pz = gSpZ; P.y = P.py = y; P.mx = P.my = P.mz = 0; P.onGround = true; gRespHold = 0;
}
static void survRespawn(Phys& P) {
    if (gHard && gDead) { gDead = false; gWantRespawn = false; gHP = 20; gFood = 20; gFire = 0; gInvul = 20; setGm(3); P.flying = true; netRespawnAnnounce(); return; }   // hardcore: não renasce, vira espectador
    gHP = 20; gFood = 20; gSat = 5.f; gExh = 0.f; gFall = 0.f; gHurt = 0; gInvul = 20; gLastDmg = 1000; gRegenT = 0; gFire = 0; gDead = false; gWantRespawn = false; netRespawnAnnounce();
    P.x = P.px = gSpX; P.z = P.pz = gSpZ; P.y = P.py = gSpY; P.mx = P.my = P.mz = 0;
    P.onGround = false; gRespHold = 200;   // segura o jogador no ar ate o chunk do spawn existir; depois acha o chao (survRespawnFix)
    survRespawnFix(P);
}
static void survTick(Phys& P) {   // 1x por tick (20 Hz), depois do movimento
    if (gInvul > 0) --gInvul;
    if (gRespHold > 0) { --gRespHold; survRespawnFix(P); }
    if (gHurt > 0) --gHurt;
    if (gEatCd > 0) --gEatCd;
    if (creative) { gFall = 0.f; if (gDead) gWantRespawn = true; return; }   // trocar para criativo morto = renasce
    if (gDead) { ++gDeadT; P.flying = false; return; }   // morto: sem voo (nao sobe), so a gravidade   // como no Minecraft: so renasce ao clicar em "Renascer"
    // vazio: abaixo de y=-64 o MC causa 4 de dano por tick, ignorando a invulnerabilidade (so o criativo escapa, retornado acima)
    if (P.y < -64.0) survDamage(4, "Caiu para fora do mundo", true);
    // lava: 4 de dano (a cada 10 ticks) + pega fogo por 15 s; fogo da 1 de dano por segundo; agua apaga
    if (P.inLava) { survDamage(4, "Tentou nadar na lava"); gFire = 300; }
    if (P.inWater) gFire = 0;
    if (gFire > 0) { if (gFire % 20 == 0) survDamage(1, "Pegou fogo"); --gFire; }
    if (P.inLava) gFall = 0.f;
    // queda: acumula a descida; ao pisar no chão, dano = ceil(distância - 3). Água/voo zeram.
    if (P.inWater || P.flying) gFall = 0.f;
    else if (P.onGround) { if (gFall > 3.f) survDamage((int)ceilf(gFall - 3.f), "Caiu de uma altura elevada", true); gFall = 0.f; }
    else if (P.y < P.py) gFall += (float)(P.py - P.y);
    // exaustão por movimento (correr 0,1/m, andar 0,01/m)
    const float d = (float)sqrt((P.x - P.px) * (P.x - P.px) + (P.z - P.pz) * (P.z - P.pz));
    if (P.onGround) gExh += d * (P.sprint ? .1f : .01f);
    if (gExh > 4.f) { gExh -= 4.f; if (gSat > 0.f) gSat = std::max(0.f, gSat - 1.f); else if (gFood > 0) --gFood; }
    // regeneração (fome >= 9 coxas) e inanição (fome 0, para em meio coração)
    if (gFood >= 18 && gHP < 20) { if (++gRegenT >= 80) { gRegenT = 0; ++gHP; gExh += 3.f; } }
    else if (gFood <= 0) { if (++gRegenT >= 80) { gRegenT = 0; if (gHP > 1) survDamage(1, "Morreu de fome", true); } }
    else gRegenT = 0;
}

// ---- ícones em pixel-art 9x8 (como o Java): '#' contorno, 'r' preenchimento, 'w' brilho, 'b' osso ----
static const char* const HEART_M[8] = {".###.###.", "#rwr#rrr#", "#rwrrrrr#", "#rrrrrrr#", ".#rrrrr#.", "..#rrr#..", "...#r#...", "....#...."};
static const char* const HEART_H[8] = {".###.###.", "#rwr#rrr#", "#rkkrkkr#", "#rrrrrrr#", ".#rrrrr#.", "..#rrr#..", "...#r#...", "....#...."};   // coração hardcore (olhos)
static const char* const MEAT_M[8]  = {"....####.", "...#rrrr#", "..#rrrrr#", "..#rrrrr#", ".#rrrrr#.", "#wrrr##..", "#bb##....", ".##......"};
static void survIcon(const char* const* m, float x, float y, int st, bool leftHalf, unsigned out, unsigned fill, unsigned dark, unsigned hi) {   // st: 0 vazio, 1 meio, 2 cheio
    for (int r = 0; r < 8; r++) for (int c = 0; c < 9; c++) {
        const char ch = m[r][c]; if (ch == '.') continue;
        const bool on = st == 2 || (st == 1 && (leftHalf ? c <= 4 : c >= 4));
        unsigned col = out;
        if (ch == 'r') col = on ? fill : dark;
        else if (ch == 'w') col = on ? hi : dark;
        else if (ch == 'b') col = on ? 0xEEEEEE : dark;
        else if (ch == 'k') col = 0x000000;
        U(x + c, y + r, 1, 1, col);
    }
}
static void survDrawHud(int w, int h) {   // chamado por drawHud com OX/OY na origem da hotbar (182x22 unidades)
    if (creative) return;
    const bool flash = gHurt > 0 && (gHurt / 3) % 2 == 0;
    const bool low = gHP <= 6 && !gDead;
    for (int i = 0; i < 10; i++) {
        const int hs = std::max(0, std::min(2, gHP - i * 2)), fs = std::max(0, std::min(2, gFood - i * 2));
        const float jit = (low && ((SDL_GetTicks() / 100 + i * 3) % 7 == 0)) ? -2.f : 0.f;
        survIcon(gHard ? HEART_H : HEART_M, (float)(i * 8), -11.f + jit, hs, true, flash ? 0xFFFFFF : 0x000000, 0xE02020, 0x3A1010, 0xFF8888);
        survIcon(MEAT_M, (float)(173 - i * 8), -11.f, fs, false, 0x2A1A0A, 0xC27A2C, 0x2E2218, 0xE8A858);
    }
}
