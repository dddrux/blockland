// ================= sobrevivência: vida, fome, dano, regeneração, morte =================
// Valores em "meios": 20 = 10 corações / 10 coxas (como o Java 1.8.9). Só vale em sobrevivência (F4 / /gamemode 0).
static void chatAdd(const std::string& s, unsigned col);   // definido em chat_debug.inc
static int gBowT = -1;        // arco: ticks de carga (-1 = sem carregar) (bow.inc)
static bool gBowOn = false;   // carregando: andar a 20% e sem sprint (usar item, como no Java)
static void applyGm(int g) { gGm = g & 3; creative = (gGm == 1 || gGm == 3); }
static void setGm(int g);   // chat_debug.inc
static int gHP = 20, gFood = 20;
static int gXpTot = 0;   // patchHud.1: pontos de XP totais (formula do Java: nivel e barra derivam daqui)
static int xpToNext(int L) { return L <= 15 ? 2 * L + 7 : L <= 30 ? 5 * L - 38 : 9 * L - 158; }
static int xpLevel(int t, float* frac = nullptr) { int L = 0; while (t >= xpToNext(L)) { t -= xpToNext(L); ++L; } if (frac) *frac = (float)t / xpToNext(L); return L; }
static void survAddXp(int n) { if (creative || n <= 0) return; const int l0 = xpLevel(gXpTot); gXpTot += n; sndEvtUi(xpLevel(gXpTot) > l0 ? "levelup" : "orb"); }   // patchAud.6: som de XP / subir de nivel
static void survOreXp(int b) { if (b == COALORE) survAddXp(rand() % 3); else if (b == LAPISORE) survAddXp(2 + rand() % 4); else if (b == REDSTONEORE) survAddXp(1 + rand() % 5); else if (b == DIAMONDORE) survAddXp(3 + rand() % 5); }
static double gSpX = 8.5, gSpY = 60, gSpZ = 8.5;   // ponto de spawn seguro (calculado ao carregar o mundo, menu.inc)
static float gSat = 5.f, gExh = 0.f, gFall = 0.f;
static int gAir = 300;   // patch5.76: ar do jogador em ticks (300 = 15 s; so cai com a CABECA na agua; -20 => 2 de dano e volta a 0)
static int gFire = 0;   // ticks em chamas (lava = 300 = 15 s; agua apaga)
static int gHurt = 0, gInvul = 0, gRegenT = 0, gDeadT = 0, gEatCd = 0;
static bool gDead = false, gWantRespawn = false;
static std::string gDeathWhy;
static float gHurtAtYaw = 0.f, gYawNow = 0.f;   // attackedAtYaw (1.8.9) e yaw atual do jogador
static int gSplitCtx = 0;   // 2 enquanto o estado do jogador 2 esta trocado no lugar do jogador 1
static void deathDropAll(); static void netDeathAnnounce(const std::string& why); static void netRespawnAnnounce(); static void netHurtAnnounce();   // pvp.inc
static bool gHurtWake = false;   // patchProp.14: levou dano -> acorda da cama (propSleepTick limpa)

static void localDeathPose(float pt) {   // RendererLivingEntity.rotateCorpse: o corpo tomba ate 90 graus em torno dos pes (F5 / reflexo / sombra)
    if (!gDead || creative) return; float f = sqrtf(std::max(0.f, (gDeadT + pt - 1.f) / 20.f * 1.6f)); if (f > 1.f) f = 1.f; glRotatef(-f * 90.f, 0, 0, 1);
}
static int gLastDmg = 0;   // dano do ultimo golpe (1.8.9 lastDamage)
static int armorPoints() {   // pontos de defesa vanilla 1.8.9: couro 1/3/2/1, ferro 2/6/5/2, ouro 2/5/3/1, diamante 3/8/6/3
    static const int T[4][4] = {{1, 3, 2, 1}, {2, 6, 5, 2}, {2, 5, 3, 1}, {3, 8, 6, 3}}; int a = 0;
    for (int s = 0; s < 4; s++) if (armorOk(s, armor[s].id)) a += isArmor(armor[s].id) ? T[(armor[s].id - IT_A0) / 4][s] : propArmorPts(armor[s].id);
    return a;
}
static bool isSwordId(int id) { return id == IT_WSWORD || (id >= IT_SSWORD && id <= IT_DSWORD); }
static bool isDigToolId(int id) { return id == IT_WPICK || id == IT_SPICK || (id >= IT_IPICK && id <= IT_DPICK) || (id >= IT_WAXE && id <= IT_DSHOVEL); }
static bool itemWear(Slot& s, int n = 1) {   // patchDur.1: gasta durabilidade; quebra (some) ao chegar no maximo. No criativo nao gasta
    if (creative || !s.id || n <= 0) return false; const int mx = itemMaxDur(s.id); if (mx <= 0) return false;
    s.d += n; if (s.d >= mx) { s = Slot(); sndPlay("random/break", .8f, .9f + (rand() % 100) / 500.f); return true; } return false;
}
static void toolWear(Slot& s) { if (isDigToolId(s.id) || (s.id && s.id == shearsId())) itemWear(s, 1); else if (isSwordId(s.id)) itemWear(s, 2); }   // quebrar bloco: ferramenta 1, espada 2 (1.12)
static void meleeWear(Slot& s) { if (isSwordId(s.id)) itemWear(s, 1); else if (isDigToolId(s.id) || (s.id >= IT_WHOE && s.id <= IT_DHOE)) itemWear(s, 2); }   // golpe em mob: espada 1, resto 2
static void armorWear(int raw) { const int d = std::max(1, raw / 4); for (int k = 0; k < 4; k++) if (armorOk(k, armor[k].id)) itemWear(armor[k], d); }   // cada peca perde max(1, dano/4)
static bool survDamage(int n, const char* why, bool force);
static void mechCactus(const Phys& P) {   // patchMech.1: cacto machuca (1 de dano) ao encostar nas laterais; invulnerabilidade de survDamage limita a taxa
    if (!gCactusId || creative || P.flying || P.noclip) return;
    for (int x = (int)floor(P.x - .301); x <= (int)floor(P.x + .301); x++) for (int z = (int)floor(P.z - .301); z <= (int)floor(P.z + .301); z++) for (int y = (int)floor(P.y); y <= (int)floor(P.y + 1.79); y++)
        if (getB(x, y, z) == gCactusId) { survDamage(1, "Foi espetado ate a morte", false); return; }
}
static bool survDamage(int n, const char* why, bool force = false) {   // force ignora a invulnerabilidade e a armadura (queda, vazio, fome, comando)
    if (creative || gDead || n <= 0) return false;
    const int rawDmg = n;
    if (!force) { if (const int ap = armorPoints()) n = std::max(1, (int)lroundf(n * (25 - ap) / 25.f)); }   // ArmorProperties: dano * (25 - defesa) / 25
    if (!force && gInvul > 10) { if (n <= gLastDmg) return false; const int d = n - gLastDmg; gLastDmg = n; gHP = std::max(0, gHP - d); gHurt = 20; }   // 1.8.9: 1a metade dos 20 ticks so passa a diferenca
    else { if (gInvul > 0 && !force) { /* 2a metade: golpe integral */ } gLastDmg = n; gHP = std::max(0, gHP - n); gHurt = 20; gInvul = 20; }
    if (!force) armorWear(rawDmg);
    sndEvtUi(force && (strstr(why, "Caiu") || strstr(why, "queda")) ? (n >= 5 ? "player.fall.big" : "player.fall.small") : "player.hurt");   // patchAud.2
    gHurtAtYaw = 0.f; gHurtWake = true; netHurtAnnounce();   // so chega aqui se o dano foi aplicado (criativo/invulneravel ja retornaram acima)
    if (gHP <= 0) { gDead = true; gDeadT = 0; gDeathWhy = why; if (gSplitCtx != 2) chatAdd(std::string("Você morreu: ") + why, 0xFF5555); deathDropAll(); netDeathAnnounce(why); }
    return true;
}
static void survExh(float a) { if (!creative && !gDead) gExh += a; }
// ---- comida (valores do Minecraft 1.8.9): fome = pontos de comida; saturacao = fome x modificador x 2 (limitada ao nivel de fome) ----
struct FoodInfo { int h; float m; int fx; };   // fx: 0 nenhum | 1 Fome 30 s (80%) | 2 Fome 30 s (30%) | 3 Regeneracao II 5 s | 4 Veneno 5 s | 5 Veneno 5 s (60%)
static bool foodInfo(int id, FoodInfo& f) {
    switch (id) {
        case IT_APPLE: f = {4, .3f, 0}; return true;   case IT_GAPPLE: f = {4, 1.2f, 3}; return true;
        case IT_BREAD: f = {5, .6f, 0}; return true;   case IT_COOKIE: f = {2, .1f, 0}; return true;
        case IT_BEEF: f = {3, .3f, 0}; return true;    case IT_STEAK: f = {8, .8f, 0}; return true;
        case IT_CARROT: f = {3, .6f, 0}; return true;
        case IT_PORK: f = {3, .3f, 0}; return true;    case IT_CPORK: f = {8, .8f, 0}; return true;
        case IT_CHICKEN: f = {2, .3f, 2}; return true; case IT_CCHICKEN: f = {6, .6f, 0}; return true;
        case IT_MUTTON: f = {2, .3f, 0}; return true;  case IT_CMUTTON: f = {6, .8f, 0}; return true;
    }
    if (!isPk(id)) return false;
    static const struct { const char* n; FoodInfo f; } T[] = {
        {"item/rotten_flesh", {4, .1f, 1}}, {"item/potato", {1, .3f, 0}}, {"item/baked_potato", {5, .6f, 0}}, {"item/poisonous_potato", {2, .3f, 5}},
        {"item/melon", {2, .3f, 0}}, {"item/melon_slice", {2, .3f, 0}}, {"item/pumpkin_pie", {8, .3f, 0}}, {"item/spider_eye", {2, .8f, 4}},
        {"item/golden_apple", {4, 1.2f, 3}}, {"item/enchanted_golden_apple", {4, 1.2f, 3}}, {"item/golden_carrot", {6, 1.2f, 0}}, {"item/carrot", {3, .6f, 0}},
        {"item/cod", {2, .1f, 0}}, {"item/fish_cod_raw", {2, .1f, 0}}, {"item/cooked_cod", {5, .6f, 0}}, {"item/fish_cod_cooked", {5, .6f, 0}},
        {"item/salmon", {2, .1f, 0}}, {"item/fish_salmon_raw", {2, .1f, 0}}, {"item/cooked_salmon", {6, .8f, 0}}, {"item/fish_salmon_cooked", {6, .8f, 0}},
        {"item/tropical_fish", {1, .1f, 0}}, {"item/fish_clownfish_raw", {1, .1f, 0}},
        {"item/beetroot", {1, .6f, 0}}, {"item/beetroot_soup", {6, .6f, 0}}, {"item/mushroom_stew", {6, .6f, 0}},
        {"item/rabbit", {3, .3f, 0}}, {"item/rabbit_raw", {3, .3f, 0}}, {"item/cooked_rabbit", {5, .6f, 0}}, {"item/rabbit_cooked", {5, .6f, 0}}, {"item/rabbit_stew", {10, .6f, 0}}};
    const std::string& fn = PK[id - PK0].file;
    for (const auto& e : T) if (fn == e.n) { f = e.f; return true; }
    return false;
}
static int foodValue(int id) { FoodInfo f; return foodInfo(id, f) ? f.h : 0; }
static bool foodAlwaysEdible(int id) { if (id == IT_GAPPLE) return true; if (!isPk(id)) return false; const std::string& fn = PK[id - PK0].file; return fn == "item/golden_apple" || fn == "item/enchanted_golden_apple"; }
static bool survCanEat(int id) { return !creative && !gDead && id && foodValue(id) > 0 && (gFood < 20 || foodAlwaysEdible(id)); }
static std::string foodTip(int id) {   // linha extra do tooltip: o que a comida recupera
    FoodInfo f; if (!foodInfo(id, f)) return "";
    char b[160]; const float sat = f.h * f.m * 2.f; snprintf(b, sizeof b, "Fome: +%d | Saturação: +%.1f", f.h, sat);
    for (char* c = b; *c; c++) if (*c == '.') *c = ',';
    static const char* FX[] = {"", " | Fome (80%)", " | Fome (30%)", " | Regeneração", " | Veneno", " | Veneno (60%)"};
    return std::string(b) + FX[f.fx];
}
static int gEatT = -1, gHungerT = 0, gRegenFx = 0, gPoisonT = 0; static bool gEatOn = false; static Uint32 gEatMs = 0;   // comendo: ticks (0..31), efeitos de comida, instante do ultimo tick (interpolacao da animacao)
static void survEatNow(Slot& s) {   // termina de comer (apos 32 ticks): aplica fome, saturacao e efeitos
    FoodInfo f; if (!s.id || !foodInfo(s.id, f)) return;
    gFood = std::min(20, gFood + f.h); gSat = std::min((float)gFood, gSat + f.h * f.m * 2.f);
    switch (f.fx) {
        case 1: if (rand() % 100 < 80) gHungerT = 600; break;
        case 2: if (rand() % 100 < 30) gHungerT = 600; break;
        case 3: gRegenFx = 100; break;
        case 4: gPoisonT = 100; break;
        case 5: if (rand() % 100 < 60) gPoisonT = 100; break;
    }
    if (--s.n <= 0) s = Slot();
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
    if (gDim) dimSwitch(0);   // morreu no Nether: renasce na superficie
    if (gHard && gDead) { gDead = false; gWantRespawn = false; gHP = 20; gFood = 20; gFire = 0; gInvul = 20; setGm(3); P.flying = true; netRespawnAnnounce(); return; }   // hardcore: não renasce, vira espectador
    gXpTot = 0; gAir = 300; gHP = 20; gFood = 20; gSat = 5.f; gExh = 0.f; gFall = 0.f; gHurt = 0; gInvul = 20; gLastDmg = 1000; gRegenT = 0; gFire = 0; gHungerT = gRegenFx = gPoisonT = 0; gEatT = -1; gDead = false; gWantRespawn = false; netRespawnAnnounce();
    P.x = P.px = gSpX; P.z = P.pz = gSpZ; P.y = P.py = gSpY; P.mx = P.my = P.mz = 0;
    P.onGround = false; gRespHold = 200;   // segura o jogador no ar ate o chunk do spawn existir; depois acha o chao (survRespawnFix)
    survRespawnFix(P);
}
static void survTick(Phys& P) {   // 1x por tick (20 Hz), depois do movimento
    if (gInvul > 0) --gInvul;
    if (gRespHold > 0) { --gRespHold; survRespawnFix(P); }
    if (gHurt > 0) --gHurt;
    if (gEatCd > 0) --gEatCd;
    if (creative) { gFall = 0.f; gAir = 300; if (gDead) gWantRespawn = true; return; }   // trocar para criativo morto = renasce
    if (gDead) { ++gDeadT; P.flying = false; return; }   // morto: sem voo (nao sobe), so a gravidade   // como no Minecraft: so renasce ao clicar em "Renascer"
    if (gHungerT > 0) { --gHungerT; gExh += .025f; }   // efeito Fome I: +0,025 de exaustao por tick
    if (gRegenFx > 0) { --gRegenFx; if (gRegenFx % 25 == 0 && gHP < 20) { ++gHP; gExh += .5f; } }   // Regeneracao II: 1 de vida a cada 25 ticks
    if (gPoisonT > 0) { --gPoisonT; if (gPoisonT % 25 == 0 && gHP > 1) survDamage(1, "envenenado", true); }   // Veneno nao mata
    // vazio: abaixo de y=-64 o MC causa 4 de dano por tick, ignorando a invulnerabilidade (so o criativo escapa, retornado acima)
    if (P.y < -64.0) survDamage(4, "Caiu para fora do mundo", true);
    // lava: 4 de dano (a cada 10 ticks) + pega fogo por 15 s; fogo da 1 de dano por segundo; agua apaga
    if (P.inLava) { survDamage(4, "Tentou nadar na lava"); gFire = 300; }
    if (P.inWater) gFire = 0;
    if (gFire > 0) { if (gFire % 20 == 0) survDamage(1, "Pegou fogo"); --gFire; }
    if (P.inLava) gFall = 0.f;
    // patch5.76: afogamento (1.8.9: cabeca na agua gasta 1 de ar por tick; ao chegar a -20 leva 2 de dano ignorando armadura e o ar volta a 0; fora da agua o ar enche na hora) e sufocamento (olhos dentro de bloco opaco: 1 de dano, limitado pela invulnerabilidade)
    { const int ex = (int)floor(P.x), ey = (int)floor(P.y + 1.62), ez = (int)floor(P.z), eb = getB(ex, ey, ez);
      if (isWater(eb)) { if (--gAir <= -20) { gAir = 0; survDamage(2, "Se afogou", true); } } else gAir = 300;
      bool inside = false;   // patchSuf.1: como Entity.isEntityInsideOpaqueBlock: 8 pontos em torno dos olhos (largura 0,6 * 0,8, altura 0,1); encostar o rosto na quina ja sufoca
      for (int i = 0; i < 8 && !inside; i++) { const int bx = (int)floor(P.x + ((i & 1) - .5) * .48), by = (int)floor(P.y + 1.62 + (((i >> 1) & 1) - .5) * .1), bz = (int)floor(P.z + (((i >> 2) & 1) - .5) * .48), bb = getB(bx, by, bz);
          if (bb != AIR && !isLiquid(bb) && solid(bb) && lightOpacL(bb) >= 15) inside = true; }
      if (inside) survDamage(1, "Sufocou numa parede"); }
    // queda: acumula a descida; ao pisar no chão, dano = ceil(distância - 3). Água/voo zeram.
    if (P.inWater || P.flying || physInWeb(P) || (P.onGround && inWaterNow(P))) gFall = 0.f;   // patchMLG.1: P.inWater e do INICIO do tick (pre-movimento); ao pousar reavalia com a posicao nova (Entity.updateFallState chama handleWaterMovement antes de cobrar a queda) => balde de agua no ultimo instante e queda em agua rasa anulam o dano   // agua/voo/teia (Entity.setInWeb) zeram a queda
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
static const char* const BUBBLE_M[8] = {"..#####..", ".#rrrrr#.", "#rwrrrrr#", "#rwrrrrr#", "#rrrrrrr#", "#rrrrrrr#", ".#rrrrr#.", "..#####.."};   // bolha de ar (patch5.77)
// bolhas cheias = ceil((ar - 2) * 10 / 300) e estouradas = ceil(ar * 10 / 300) - cheias (como o GuiIngame); so aparecem com ar < 300
static int survBubbles(int air, int* popped = nullptr) { const int full = air <= 2 ? 0 : (int)ceil((air - 2) * 10.0 / 300.0), tot = air <= 0 ? 0 : (int)ceil(air * 10.0 / 300.0); if (popped) *popped = std::max(0, tot - full); return std::min(10, full); }
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
static void survIconTex(float x, float y, int su, int sv) { uTexRegion(rtIcons, x, y, 9, 9, (float)su, (float)sv, 9, 9, 256, 256); }
static void survDrawHud(int w, int h) {   // chamado por drawHud com OX/OY na origem da hotbar (182x22 unidades)
    if (creative) return;
    const bool flash = gHurt > 0 && (gHurt / 3) % 2 == 0;
    const bool low = gHP <= 6 && !gDead;
    for (int i = 0; i < 10; i++) {
        const int hs = std::max(0, std::min(2, gHP - i * 2)), fs = std::max(0, std::min(2, gFood - i * 2));
        const float jit = (low && ((SDL_GetTicks() / 100 + i * 3) % 7 == 0)) ? -2.f : 0.f;
        if (rtIcons) {   // patchHud.1: icones oficiais do icons.png (coracao y=0, hardcore y=45, fome y=27)
            const int hy = gHard ? 45 : 0;
            survIconTex((float)(i * 8), -17.f + jit, flash ? 25 : 16, hy); if (hs == 2) survIconTex((float)(i * 8), -17.f + jit, 52, hy); else if (hs == 1) survIconTex((float)(i * 8), -17.f + jit, 61, hy);
            survIconTex((float)(173 - i * 8), -17.f, 16, 27); if (fs == 2) survIconTex((float)(173 - i * 8), -17.f, 52, 27); else if (fs == 1) survIconTex((float)(173 - i * 8), -17.f, 61, 27);
            continue; }
        survIcon(gHard ? HEART_H : HEART_M, (float)(i * 8), -17.f + jit, hs, true, flash ? 0xFFFFFF : 0x000000, 0xE02020, 0x3A1010, 0xFF8888);
        survIcon(MEAT_M, (float)(173 - i * 8), -17.f, fs, false, 0x2A1A0A, 0xC27A2C, 0x2E2218, 0xE8A858);
    }
    { float fr = 0.f; const int L = xpLevel(gXpTot, &fr); const int fw = std::max(0, std::min(182, (int)(fr * 183)));   // barra de XP (182x5) acima da hotbar + nivel
        if (rtIcons) { uTexRegion(rtIcons, 0, -7.f, 182, 5, 0, 64, 182, 5, 256, 256); if (fw) uTexRegion(rtIcons, 0, -7.f, (float)fw, 5, 0, 69, (float)fw, 5, 256, 256); }
        else { U(0, -7.f, 182, 5, 0x2A2A2A); if (fw) U(0, -7.f, (float)fw, 5, 0x80FF20); }
        if (L > 0) { char nb[16]; snprintf(nb, sizeof nb, "%d", L); const float tx = 91.f - 3.f * (float)strlen(nb); uText(tx - 1, -17.f, nb, 0x000000); uText(tx + 1, -17.f, nb, 0x000000); uText(tx, -18.f, nb, 0x000000); uText(tx, -16.f, nb, 0x000000); uText(tx, -17.f, nb, 0x80FF20); } }
    if (gAir < 300 && !gDead) { int pop = 0; const int full = survBubbles(gAir, &pop);   // patch5.77: bolhas de ar acima da barra de fome
        for (int i = 0; i < full + pop && i < 10; i++) { if (rtIcons) survIconTex((float)(173 - i * 8), -27.f, i < full ? 16 : 25, 18); else survIcon(BUBBLE_M, (float)(173 - i * 8), -27.f, i < full ? 2 : 0, false, 0x14304A, 0x6FA8F0, 0x14304A, 0xCFE6FF); } }
}
