// ================= ENDER DRAGON (patchEnd.1) =================
// Entidade propria (fora de MOBS): modelo ModelDragon simplificado (caixas, textura entity/enderdragon/dragon.png 256x256), 200 de vida,
// voa em circulo no End e mergulha no jogador (10 de dano). Cabeca = dano total; corpo/asas = 1/4+1. Ao morrer (10 s) surge o ovo do dragao sobre
// um pilar de bedrock em (0,topo,0). So o host/solo simula; nao ha sincronia de rede (VIS: nada visto rodando).
struct DragonEnt { bool on = false; double x = 0, y = 90, z = 60, px = 0, py = 90, pz = 60, vx = 0, vy = 0, vz = 0; float yaw = 0, pyaw = 0, hp = 200.f, wing = 0, pwing = 0, ang = 0; int dead = 0, hurtT = 0, atkCd = 0, phase = 0, phT = 0; float rbY[64] = {}, rbYaw[64] = {}; int rbI = 0; };   // patchEnd.12: rb* = ringBuffer do EntityDragon (yaw/Y dos ultimos 64 ticks) que dobra pescoco e cauda
static void dragonRing(DragonEnt& d) { d.rbI = (d.rbI + 1) & 63; d.rbYaw[d.rbI] = -d.yaw; d.rbY[d.rbI] = (float)d.y; }   // yaw guardado no sinal do Minecraft (cabeca = -z)
struct EndCrystal { double x, y, z; bool alive; }; static EndCrystal gCryst[10]; static bool gCrystInit[10];
static void crystalsTick(Phys& P); static bool crystalHit(const float* eye, const float* dir, float reach, int dmg); static int crystalsAlive(double x, double y, double z, double r);
static void dragonRingFill(DragonEnt& d) { for (int i = 0; i < 64; i++) { d.rbYaw[i] = -d.yaw; d.rbY[i] = (float)d.y; } }
static int gHealC = -1; static int crystalNearest(double x, double y, double z, double r);   // patchEnd.12: UM cristal cura o dragao (healingCrystal)
static DragonEnt gDragon; static bool gDragonKilled = false; static GLuint gDragonTex = 0;
static void dragonWorldSave(const std::string& dir) { const std::string p = "saves/" + dir + "/dragon.dat"; if (gDragonKilled) { FILE* f = fopen(p.c_str(), "wb"); if (f) { fputs("1", f); fclose(f); } } else remove(p.c_str()); }
static void dragonWorldLoad(const std::string& dir) { FILE* f = fopen(("saves/" + dir + "/dragon.dat").c_str(), "rb"); gDragonKilled = f != nullptr; if (f) fclose(f); gDragon = DragonEnt(); }
static int dragonTop(bool& built) {   // patchFix.26: y da base da fonte no centro (0,0); built = fonte ja existe (pilar de bedrock/ovo no topo)
    const int egg = pkFind("block/dragon_egg"); int y = CH - 2;
    for (; y > 0; y--) { const int b = getB(0, y, 0); if (b != AIR && !isLiquid(b) && !(egg && b == egg)) break; }
    built = false; if (getB(0, y, 0) == BEDROCK) { built = true; while (y > 0 && getB(0, y, 0) == BEDROCK) y--; y++; }
    return y;
}
static bool dragonEggSpawn(bool first = true, bool active = true) {   // patchEnd.13 + patchFix.26: fonte de bedrock SEMPRE existe (active=false: so a moldura, sem portal); active=true (morte do dragao) enche o miolo com XB_PORTAL meta 2 (leva ao spawn via endTravel) + ovo so na 1a morte
    if (!getC(0, 0)) return false;
    bool built = false; const int top = dragonTop(built);
    if (top < 1 || top > CH - 8) return false;
    if (built && !active) return true;
    auto put = [&](int x, int y, int z, int b, int m) { if (!getC(fdiv(x, CW), fdiv(z, CW))) return; setBM(x, y, z, b, m); netSendBlock(x, y, z, b, m); };
    for (int dz = -4; dz <= 4; dz++) for (int dx = -4; dx <= 4; dx++) {
        const double d = sqrt((double)(dx * dx + dz * dz)); if (d > 3.5) continue;
        put(dx, top, dz, BEDROCK, 0);   // base da fonte (raio 3,5)
        if (d <= 2.5 && (dx || dz)) { if (active) put(dx, top + 1, dz, XB_PORTAL, 2); } else if (d > 2.5) put(dx, top + 1, dz, BEDROCK, 0);   // miolo = portal (so ativo apos matar o dragao); borda = bedrock
    }
    for (int i = 1; i <= 3; i++) put(0, top + i, 0, BEDROCK, 0);   // pilar central
    const int egg = pkFind("block/dragon_egg"); if (active && first && egg) put(0, top + 4, 0, egg, 0);
    return true;
}
// ---- patchEnd.14: creditos apos a 1a morte do dragao (texto proprio; Esc/Enter pula); ao fim volta ao spawn do Overworld ----
static Uint32 gCreditsT0 = 0; static bool gCredits = false, gCreditsEnd = false;
static void creditsStart() { gCredits = true; gCreditsEnd = false; gCreditsT0 = SDL_GetTicks(); }
static void creditsSkip() { if (gCredits) { gCredits = false; gCreditsEnd = true; } }
static void drawCredits(int w, int h) {   // patchEnd.17: fundo de terra + logo do menu; dialogo "O ultimo bloco" (duas vozes) rolando; sem segurar o 1o quadro de preto
    rdViewport(gVpX, gVpY, w, h); rdMatrixMode(GL_PROJECTION); rdLoadIdentity(); rdOrtho(0, w, h, 0, -1, 1); rdMatrixMode(GL_MODELVIEW); rdLoadIdentity();
    rdDisable(GL_DEPTH_TEST); rdDisable(GL_FOG); rdDisable(GL_CULL_FACE); rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h); OX = 0; OY = 0; const int sw = (w + G - 1) / G, sh = (h + G - 1) / G;
    if (!mtDirt) mtDirt = mtLoad("gui/options_background.png", "assets/textures/blocks/dirt_side.png", true);
    if (!mtLogo) mtLogo = mtLoad("__x", "assets/textures/gui/logo.png", false);
    const float uiA0 = gUiA; gUiA = 1.f;
    if (mtDirt) mBg(w, h, 0, 0, (float)w, (float)h, .3f); else { rdDisable(GL_TEXTURE_2D); glColor4f(.2f, .14f, .08f, 1.f); glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd(); rdEnable(GL_TEXTURE_2D); }
    // prefixo: # secao | * direcao de cena | 1 voz 1 | 2 voz 2 | ~ centralizado | vazio = espaco
    static const char* L[] = {
        "*A tela escurece lentamente. Os sons do jogo desaparecem. Os créditos começam a subir enquanto pequenos blocos flutuam no vazio.", "",
        "#O ÚLTIMO BLOCO", "*Uma história sobre mundos que nunca ficam prontos.", "",
        "1VOZ 1: Então... ele conseguiu.", "2VOZ 2: Conseguiu o quê?", "1VOZ 1: Chegou ao fim.", "2VOZ 2: Ah. Você diz isso como se houvesse um lugar para onde chegar.",
        "1VOZ 1: Existe, não existe? Uma última construção. Um castelo que toca as nuvens. Uma cidade onde todos os blocos finalmente encontram seu lugar.",
        "2VOZ 2: Você já viu uma cidade assim?", "1VOZ 1: Não.", "2VOZ 2: Então como sabe que ela existe?", "*Pausa.",
        "1VOZ 1: Porque ele passou a vida inteira tentando construí-la.", "2VOZ 2: E você acha que era isso que ele queria?", "", "",
        "#OS PRIMEIROS BLOCOS", "*Quatro paredes tortas. Um telhado incompleto. Uma porta colocada do lado errado.", "",
        "1VOZ 1: Lembra da primeira casa dele?", "2VOZ 2: Aquela que não tinha teto?", "1VOZ 1: Ele dizia que era uma janela.", "2VOZ 2: E a escada que não levava a lugar nenhum?",
        "1VOZ 1: Arquitetura experimental.", "2VOZ 2: E o buraco no chão?", "1VOZ 1: Esse era para ser um porão.", "2VOZ 2: Claro.",
        "*Os dois ficam em silêncio enquanto a pequena casa desaparece no horizonte.",
        "1VOZ 1: Ele poderia ter construído algo melhor.", "2VOZ 2: Poderia.", "1VOZ 1: Poderia ter aprendido todas as técnicas, dominado todas as ferramentas e criado algo perfeito.", "2VOZ 2: Poderia.",
        "1VOZ 1: Então por que não fez isso?", "2VOZ 2: Porque, naquela primeira noite, começou a chover. E aquela casa horrível era o único lugar onde ele podia ficar seco.", "", "",
        "#OS OUTROS JOGADORES", "*Pontes entre montanhas, torres absurdas, casas abandonadas e cidades inteiras.", "",
        "1VOZ 1: Ele não construiu tudo sozinho.", "2VOZ 2: Não.", "1VOZ 1: Algumas pessoas apareceram, construíram coisas e foram embora.", "2VOZ 2: Outras destruíram o que ele construiu.",
        "1VOZ 1: Algumas colocaram blocos onde não deveriam.", "2VOZ 2: Algumas colocaram blocos exatamente onde deveriam.",
        "1VOZ 1: E houve aqueles que passaram três horas construindo uma máquina que não fazia absolutamente nada.", "2VOZ 2: Essa era minha favorita.", "1VOZ 1: Por quê?",
        "2VOZ 2: Porque, quando finalmente terminou, ele chamou os amigos para mostrar.", "*Uma pequena máquina de blocos gira uma engrenagem, acende uma lâmpada e para.",
        "1VOZ 1: Mas ela não fazia nada.", "2VOZ 2: Fazia, sim.", "1VOZ 1: O quê?", "2VOZ 2: Fazia as pessoas ficarem ali por mais três horas.", "", "",
        "#O MUNDO VAZIO", "*Portas abertas. Placas com mensagens esquecidas. Uma estrada que termina no meio de uma floresta.", "",
        "1VOZ 1: Um dia, todos foram embora.", "2VOZ 2: Sim.", "1VOZ 1: Os servidores ficaram vazios. As cidades pararam de crescer. Ninguém mais colocou um único bloco naquela grande torre.",
        "2VOZ 2: A torre continuou lá.", "1VOZ 1: Mas ninguém a visitava.", "2VOZ 2: Ela não precisava de visitantes para continuar sendo uma torre.",
        "1VOZ 1: Você não entende. Um mundo sem jogadores é apenas um monte de blocos.", "2VOZ 2: E um livro sem leitores é apenas um monte de páginas?", "1VOZ 1: Acho que sim.",
        "2VOZ 2: Então por que você ainda se lembra das histórias que leu?", "*Silêncio.", "1VOZ 1: Porque elas aconteceram.", "2VOZ 2: Exatamente.", "", "",
        "#O ÚLTIMO BLOCO", "*No topo de uma montanha, um único bloco de madeira na beirada, colocado ali pelo jogador muitos anos antes.", "",
        "1VOZ 1: Ele poderia ter colocado qualquer coisa aqui.", "2VOZ 2: Poderia.", "1VOZ 1: Ouro. Diamante. Uma escultura. Uma bandeira.", "2VOZ 2: Poderia.", "1VOZ 1: Mas colocou madeira.",
        "2VOZ 2: Talvez fosse o único bloco que tinha na mão.", "1VOZ 1: Você acha que ele sabia que esse seria o último?", "2VOZ 2: Ninguém sabe quando está colocando o último bloco.",
        "1VOZ 1: Então todo fim é um acidente?", "2VOZ 2: Não. Às vezes, é apenas alguém decidindo que já construiu o suficiente por hoje.", "*O bloco de madeira fica pequeno diante do horizonte.",
        "1VOZ 1: E agora?", "2VOZ 2: Agora, nada.", "1VOZ 1: Nada?", "2VOZ 2: O mundo está aqui. As construções estão aqui. As histórias estão aqui.", "1VOZ 1: Mas o jogador foi embora.", "2VOZ 2: Sim.",
        "1VOZ 1: E se ele nunca voltar?", "2VOZ 2: Então teremos sido um lugar que alguém visitou.", "*Pausa.", "1VOZ 1: Isso é suficiente?", "2VOZ 2: Para um mundo feito de blocos?",
        "*Pausa mais longa.", "2VOZ 2: Acho que é mais do que suficiente.", "", "",
        "#DESPEDIDA", "*Por alguns segundos, não aparece nada.", "",
        "1VOZ 1: Você acha que ele se lembra de nós?", "2VOZ 2: Talvez se lembre de uma casa.", "1VOZ 1: Uma cidade?", "2VOZ 2: Talvez.", "1VOZ 1: Daquela torre absurda?", "2VOZ 2: Com certeza.",
        "1VOZ 1: E de nós?", "2VOZ 2: Nós nunca fomos importantes.", "1VOZ 1: Então por que estamos aqui?", "2VOZ 2: Porque alguém precisa contar a história.",
        "1VOZ 1: Mas não fomos nós que construímos o mundo.", "2VOZ 2: Não.", "1VOZ 1: Não fomos nós que colocamos o primeiro bloco.", "2VOZ 2: Não.", "1VOZ 1: Nem o último.", "2VOZ 2: Principalmente o último.",
        "*Uma pequena risada.", "1VOZ 1: Então o que fizemos?", "2VOZ 2: Ficamos aqui enquanto ele construía.", "1VOZ 1: E isso basta?", "2VOZ 2: Você continua fazendo essa pergunta.",
        "1VOZ 1: Porque nunca sei a resposta.", "2VOZ 2: Talvez essa seja a única coisa que precisamos continuar construindo.", "", "", "",
        "~O mundo não precisa ser infinito.", "~Só precisa ser grande o bastante para caber uma história.", "", "",
        "#FIM", "", "~Obrigado por jogar!", "", "*Pressione Esc para voltar ao mundo", nullptr};
    // quebra de linha por largura (cache por largura da tela em unidades de GUI)
    struct CL { std::string s; unsigned c; bool ctr; };
    static std::vector<CL> W; static int wSw = -1;
    if (wSw != sw) { wSw = sw; W.clear(); const int mx = std::max(120, std::min(sw - 24, 330));
        for (int i = 0; L[i]; i++) { const char k = L[i][0]; const std::string t = L[i] + ((k == '#' || k == '*' || k == '1' || k == '2' || k == '~') ? 1 : 0);
            const unsigned col = k == '#' ? 0xFFFF55 : k == '*' ? 0xAAAAAA : k == '1' ? 0x9CC8FF : k == '2' ? 0xFFE08A : 0xFFFFFF; const bool ctr = k == '#' || k == '*' || k == '~' || t.empty();
            if (t.empty()) { W.push_back({"", col, true}); continue; }
            std::string cur; size_t p = 0;
            while (p < t.size()) { size_t q = t.find(' ', p); if (q == std::string::npos) q = t.size(); const std::string wd = t.substr(p, q - p), tr = cur.empty() ? wd : cur + " " + wd;
                if (!cur.empty() && uTextW(tr.c_str()) > mx) { W.push_back({cur, col, ctr}); cur = wd; } else cur = tr; p = q + 1; }
            if (!cur.empty()) W.push_back({cur, col, ctr}); } }
    const float LH = 12.f, SPD = 16.f, lw = std::min(sw * .6f, 256.f), lh = lw * 382.f / 1024.f;
    const float t = (SDL_GetTicks() - gCreditsT0) / 1000.f, y0 = sh - t * SPD;
    if (mtLogo) { const float ly = y0; if (ly > -lh && ly < sh) mIcon2(mtLogo, sw / 2 - lw / 2, ly, lw, lh); }
    const float ty0 = y0 + lh + 24.f; const float total = lh + 24.f + (float)W.size() * LH + sh;
    for (size_t i = 0; i < W.size(); i++) { const float y = ty0 + i * LH; if (y < -10.f || y > sh || W[i].s.empty()) continue;
        uText(W[i].ctr ? (float)(sw / 2 - uTextW(W[i].s.c_str()) / 2) : (float)(sw / 2 - std::min(sw - 24, 330) / 2), y, W[i].s.c_str(), W[i].c, true); }
    if (t * SPD > total) creditsSkip();
    gUiA = uiA0; rdEnable(GL_TEXTURE_2D); rdDisable(GL_BLEND); rdEnable(GL_DEPTH_TEST);
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
        d.x = x; d.y = y; d.z = z; d.yaw = yw; d.wing = wg; d.hp = (float)hp; d.dead = dead; d.hurtT = ht; d.on = true; if (!was) dragonRingFill(d); dragonRing(d);
    } else if (type == NP_DRAGONHIT) {
        const int dmg = b.get<int16_t>(); if (netHost && sender != 0) dragonApplyDmg(std::max(0, std::min(40, dmg)));
    } else if (type == NP_DRAGONBITE) {
        const int tid = b.get<uint8_t>(); const float kx = b.get<float>(), kz = b.get<float>();
        if (!netHost && sender == 0 && tid == netSelf && gPvpP && !creative && !gDead && survDamage(10, "Foi morto pelo Ender Dragon")) { gPvpP->mx += kx; gPvpP->mz += kz; gPvpP->my += .6; }
    }
}
static double dragonPerch() { for (int y = CH - 3; y > 1; y--) if (getB(5, y, 0) != AIR && !isLiquid(getB(5, y, 0))) return y + 1.; return 64.; }   // topo da ilha ao lado do portal
// patchEnd.13: coro vivo: sem apoio quebra; flores crescem (1/20 por varredura)
static void chorusFix(int x, int y, int z) {
    static const int D[7][3] = {{0, 0, 0}, {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}}; static const int ES = [] { const int e = pkFind("block/end_stone"); return e ? e : (int)STONE; }();
    auto ch = [&](int a, int b, int c) { return getB(a, b, c) == XB_DECOR && (getM(a, b, c) & 3) == 3; };
    for (auto& q : D) { const int a = x + q[0], b = y + q[1], c = z + q[2]; if (!ch(a, b, c) || getM(a, b, c) == 0xFF) continue;
        int m = 3; for (int k = 0; k < 6; k++) { const int na = a + D[k + 1][0], nb = b + D[k + 1][1], nc = c + D[k + 1][2]; if (ch(na, nb, nc) || (k == 2 && ES && getB(na, nb, nc) == ES)) m |= 1 << (2 + k); }
        if (m != getM(a, b, c)) { setBM(a, b, c, XB_DECOR, m); netSendBlock(a, b, c, XB_DECOR, m); } }
}
static void chorusScan(const Phys& P) {
    if ((gWorldTime % 20) != 0 || (netOn && !netHost)) return;
    static int row = 0; const int z = (int)floor(P.z) - 16 + row; row = (row + 1) % 33; static const int ES = [] { const int e = pkFind("block/end_stone"); return e ? e : (int)STONE; }();
    auto ld = [&](int a, int c) { return getC(fdiv(a - 1, CW), fdiv(c - 1, CW)) && getC(fdiv(a + 1, CW), fdiv(c + 1, CW)) && getC(fdiv(a, CW), fdiv(c, CW)); };
    auto air = [&](int a, int b, int c) { return ld(a, c) && getB(a, b, c) == AIR; };
    auto allE = [&](int a, int b, int c, int ex) { for (int d = 0; d < 4; d++) if (d != ex && !air(a + RS_DX[d], b, c + RS_DZ[d])) return false; return true; };
    for (int x = (int)floor(P.x) - 16; x <= (int)floor(P.x) + 16; x++) for (int y = std::min(CH - 4, (int)floor(P.y) + 28); y >= std::max(3, (int)floor(P.y) - 40); y--) {
        if (getB(x, y, z) != XB_DECOR) continue; const int m = getM(x, y, z); if ((m & 3) != 3 || !ld(x, z)) continue;
        const int bl = getB(x, y - 1, z); const bool belowCh = bl == XB_DECOR && (getM(x, y - 1, z) & 3) == 3;
        bool ok;
        if (m == 0xFF) ok = belowCh || (bl != AIR && bl == ES);
        else { ok = belowCh || (ES && bl == ES); for (int d = 0; d < 4 && !ok; d++) { const int a = x + RS_DX[d], c = z + RS_DZ[d]; if (getB(a, y, c) == XB_DECOR && (getM(a, y, c) & 3) == 3 && getM(a, y, c) != 0xFF) ok = true; } }
        if (!ok) { setBM(x, y, z, AIR, 0); netSendBlock(x, y, z, AIR, 0); chorusFix(x, y, z); continue; }
        if (m == 0xFF && rand() % 20 == 0) {   // crescimento (BlockChorusFlower.updateTick simplificado)
            int h = 0; while (h < 6 && getB(x, y - 1 - h, z) == XB_DECOR && (getM(x, y - 1 - h, z) & 3) == 3 && getM(x, y - 1 - h, z) != 0xFF) h++;
            if (h < 4 && air(x, y + 1, z) && allE(x, y + 1, z, -1)) { setBM(x, y, z, XB_DECOR, 3); netSendBlock(x, y, z, XB_DECOR, 3); setBM(x, y + 1, z, XB_DECOR, 0xFF); netSendBlock(x, y + 1, z, XB_DECOR, 0xFF); chorusFix(x, y, z); }
            else if (rand() % 3 == 0) { const int d = rand() % 4, a = x + RS_DX[d], c = z + RS_DZ[d]; int op = d; for (int k = 0; k < 4; k++) if (RS_DX[k] == -RS_DX[d] && RS_DZ[k] == -RS_DZ[d]) op = k;
                if (air(a, y, c) && air(a, y - 1, c) && allE(a, y, c, op)) { setBM(x, y, z, XB_DECOR, 3); netSendBlock(x, y, z, XB_DECOR, 3); setBM(a, y, c, XB_DECOR, 0xFF); netSendBlock(a, y, c, XB_DECOR, 0xFF); chorusFix(x, y, z); } }
        }
    }
}
static void dragonTick(Phys& P) {
    if (gCreditsEnd) { gCreditsEnd = false; if (gDim == 2) endTravel(P); }   // patchEnd.14: fim dos creditos -> spawn
    DragonEnt& d = gDragon; crystalsTick(P); if (gDim == 2) chorusScan(P);   // patchEnd.8: cristais tambem no convidado (estado = bloco de fogo, que ja sincroniza)
    if (gDim != 2) { if (!(netOn && !netHost)) d.on = false; return; }
    if (netOn && !netHost) { if (d.hurtT > 0 && d.on) d.hurtT--; return; }   // convidado: so desenha o que o host manda
    if (!d.on) { if (gDragonKilled) return; d = DragonEnt(); d.on = true; d.x = d.px = 0; d.z = d.pz = 70; d.y = d.py = 85; d.yaw = d.pyaw = 90.f; dragonRingFill(d); }
    d.px = d.x; d.py = d.y; d.pz = d.z; d.pyaw = d.yaw; d.pwing = d.wing;
    if (d.dead == 0) {   // patchEnd.12 (EntityDragon.updateDragonEnderCrystal): so UM cristal (o mais proximo, ate 32 blocos) cura, 1 HP a cada 10 ticks
        if (gHealC >= 0 && !(gCrystInit[gHealC] && gCryst[gHealC].alive)) gHealC = -1;
        else if (gHealC >= 0 && d.hp < 200.f && (gWorldTime % 10) == 0) d.hp = std::min(200.f, d.hp + 1.f);
        if (rand() % 10 == 0) gHealC = crystalNearest(d.x, d.y, d.z, 32.0);
    } else gHealC = -1;
    if (d.hurtT > 0) d.hurtT--; if (d.atkCd > 0) d.atkCd--;
    if (d.dead == 0 && !gDragonKilled && (!netOn || netHost) && (d.phT & 31) == 0) dragonEggSpawn(true, false);   // patchFix.26: moldura da fonte (portal inativo) desde o inicio da luta
    if (d.dead > 0) {   // morte: afunda girando; ao fim solta o ovo
        d.dead++; d.yaw += 20.f; d.y += .1; d.wing += .03f; dragonRing(d);   // patchFix.26 (EntityDragon.onDeathUpdate): sobe .1/tick girando 20 graus/tick, dissolvendo (dragon_exploding)
        if (d.dead == 2) { sndEvtAt("explode", d.x, d.y, d.z, 2.f); }
        if (d.dead < 200) {   // explosoes aleatorias em volta do corpo (EXPLOSION_HUGE) e, na 2a metade, feixe roxo subindo da fonte
            const int tl = blkTile(STONE, 1); const float cO[3] = {1.f, .8f, .45f}, cP[3] = {.85f, .35f, 1.f};
            if ((d.dead % 4) == 0) { for (int k = 0; k < 10; k++) addPart((float)(d.x + (frand() - .5) * 12.), (float)(d.y + 1. + (frand() - .5) * 5.), (float)(d.z + (frand() - .5) * 12.), (frand() - .5f) * .5f, (frand() - .5f) * .5f, (frand() - .5f) * .5f, tl, cO, 1.f, 2.4f, 1.3f); if ((d.dead % 8) == 0) sndEvtAt("explode", d.x + (frand() - .5) * 10., d.y, d.z + (frand() - .5) * 10., 1.f); }
            if (d.dead >= 100 && getC(0, 0)) { bool bt = false; const int ty = dragonTop(bt); for (int k = 0; k < 14; k++) addPart(.5f + (frand() - .5f) * 2.f, (float)(ty + 1 + (int)(frand() * 40.f)), .5f + (frand() - .5f) * 2.f, (frand() - .5f) * .06f, .9f, (frand() - .5f) * .06f, tl, cP, 1.f, 1.6f, 1.6f); }
        }
        if (d.dead == 200) { dragonEggSpawn(!gDragonKilled); endGateSpawn(); survAddXp(gDragonKilled ? 500 : 12000); if (!gDragonKilled) gCreditsPend = true; gDragonKilled = true; d.on = false; }   // patchEnd.3: XP (12000 na 1a morte, 500 nas outras)
        dragonNetSend(); return;
    }
    { const float sp2 = (float)sqrt(d.vx * d.vx + d.vz * d.vz); d.wing += .2f / (sp2 * 10.f + 1.f) * (float)pow(2.0, d.vy); } d.phT++;   // animTime do EntityDragon (ciclos)
    double TX = P.x, TY = P.y, TZ = P.z; int tid = -1; bool tOk = !gDead && gGm != 3 && !(creative && gGm == 1);   // tid -1 = host
    double pd = tOk ? sqrt((P.x - d.x) * (P.x - d.x) + (P.y + 1. - d.y) * (P.y + 1. - d.y) + (P.z - d.z) * (P.z - d.z)) : 1e9;
    if (netOn) for (const auto& kv : remotes) { const RemotePl& r = kv.second; if (!r.init || r.dim != 2 || r.gm == 3 || r.gm == 1 || r.deathT >= 0) continue;
        const double q = sqrt((r.x - d.x) * (r.x - d.x) + (r.y + 1. - d.y) * (r.y + 1. - d.y) + (r.z - d.z) * (r.z - d.z)); if (q < pd) { pd = q; TX = r.x; TY = r.y; TZ = r.z; tid = kv.first; tOk = true; } }
    double tx, ty, tz, spd = .75; const double perch = dragonPerch();
    switch (d.phase) {   // patchEnd.13: fases do EntityDragon: 0 circula (HoldingPattern), 1 investida (ChargingPlayer), 2 passada com bola de fogo (Strafe), 3 aproximacao, 4 pousado no portal (sopro), 5 decolagem
    case 0:
        { const double a0 = atan2(d.z, d.x) + .22, R = 55. + 8. * sin(d.phT * .011); d.ang = (float)a0; tx = cos(a0) * R; tz = sin(a0) * R; ty = 80. + 7. * sin(d.phT * .03); }   // patchFix.27: alvo = ponto A FRENTE do dragao na propria orbita (antes o alvo girava mais rapido que o dragao: ele cortava caminho e oscilava); raio e altura variam devagar
        if (d.phT > 300 && pd < 120. && tOk) { d.phT = 0; const int r = rand() % 4; d.phase = r == 0 ? 3 : r == 1 ? 1 : 2; d.atkCd = 0; }
        break;
    case 1: tx = TX; ty = TY + 1.; tz = TZ; spd = 1.1; if (d.phT > 140 || (pd < 6. && d.atkCd > 0)) { d.phase = 0; d.phT = 0; d.ang = atan2(d.z, d.x); } break;
    case 2: {   // mantem ~28 blocos do alvo, acima, e cospe UMA bola de fogo
        const double hx0 = TX - d.x, hz0 = TZ - d.z, hl = std::max(1., sqrt(hx0 * hx0 + hz0 * hz0));
        tx = TX - hx0 / hl * 24.; tz = TZ - hz0 / hl * 24.; ty = TY + 14.; spd = .9;
        if (d.atkCd == 0 && hl < 40. && d.phT > 20 && tOk) {
            const double ox = d.x - sin(d.yaw * .01745329) * 5., oz = d.z - cos(d.yaw * .01745329) * 5., oy = d.y + 1.5, vx = TX - ox, vy = TY + 1. - oy, vz = TZ - oz, l = std::max(1e-6, sqrt(vx * vx + vy * vy + vz * vz));
            FBALLS.push_back({ox, oy, oz, ox, oy, oz, vx / l * .6, vy / l * .6, vz / l * .6, 0}); fbNetSend(FBALLS.back()); d.atkCd = 200;
        }
        if (d.phT > 110) { d.phase = 0; d.phT = 0; d.ang = atan2(d.z, d.x); }
        break; }
    case 3: {   // desce em espiral ate o portal
        const double hd0 = sqrt(d.x * d.x + d.z * d.z); tx = 0; tz = 0; ty = perch + std::min(40., hd0 * .6); spd = .8;
        if (hd0 < 5. && d.y - perch < 3.) { d.phase = 4; d.phT = 0; d.vx = d.vy = d.vz = 0; d.hp = std::min(200.f, d.hp); }
        if (d.phT > 600) { d.phase = 0; d.phT = 0; }
        break; }
    case 4: {   // pousado: fica parado olhando o alvo e solta o sopro (3 de dano/0,5 s a ate 8 blocos da boca)
        tx = 0; tz = 0; ty = perch; spd = .3; d.x += (0 - d.x) * .2; d.z += (0 - d.z) * .2; d.y += (perch - d.y) * .2; d.vx = d.vy = d.vz = 0;
        if (tOk) { const float want = (float)(atan2(-(TX - d.x), -(TZ - d.z)) * 57.29578); float df = want - d.yaw; while (df > 180.f) df -= 360.f; while (df < -180.f) df += 360.f; d.yaw += std::max(-6.f, std::min(6.f, df)); }
        const double bx2 = d.x - sin(d.yaw * .01745329) * 6., bz2 = d.z - cos(d.yaw * .01745329) * 6.;
        if ((d.phT % 10) == 0 && tOk && tid < 0 && (P.x - bx2) * (P.x - bx2) + (P.z - bz2) * (P.z - bz2) < 64. && fabs(P.y - d.y) < 5.) survDamage(3, "Foi queimado pelo sopro do Ender Dragon");
        if (d.phT > 320) { d.phase = 5; d.phT = 0; }
        break; }
    default: tx = 0; tz = 0; ty = perch + 30.; spd = .8; if (d.phT > 70) { d.phase = 0; d.phT = 0; d.ang = atan2(d.z, d.x); } break;
    }
    if (d.phase == 4) { dragonRing(d); dragonNetSend(); return; }
    const double dx = tx - d.x, dy = ty - d.y, dz = tz - d.z, dl = std::max(1., sqrt(dx * dx + dy * dy + dz * dz)); double sp = spd;
    if (d.phase == 3 || d.phase >= 5) sp = spd * std::min(1., .35 + dl / 20.);   // patchFix.27: freia ao chegar (aproximacao/decolagem) em vez de ultrapassar o alvo e voltar
    {   // patchFix.27: aceleracao limitada (curvas em arco, sem trancos): ate .05/tick de variacao de velocidade
        double ax = (dx / dl * sp - d.vx) * .1, ay = (dy / dl * sp - d.vy) * .1, az = (dz / dl * sp - d.vz) * .1; const double al = sqrt(ax * ax + ay * ay + az * az), mx = .05;
        if (al > mx) { ax *= mx / al; ay *= mx / al; az *= mx / al; } d.vx += ax; d.vy += ay; d.vz += az;
    }
    d.x += d.vx; d.y += d.vy; d.z += d.vz;
    if (d.phase != 3 && d.phase < 5 && d.y < 66.) { d.y = 66.; if (d.vy < 0) d.vy *= .5; }   // nao afunda na ilha em voo normal
    if (fabs(d.vx) + fabs(d.vz) > .05) { const float ty2 = (float)(atan2(-d.vx, -d.vz) * 57.29578); float df = ty2 - d.yaw; while (df > 180.f) df -= 360.f; while (df < -180.f) df += 360.f; d.yaw += std::max(-4.f, std::min(4.f, df * .12f)); }   // giro suave (max 4 graus/tick)
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
    dragonRing(d); dragonNetSend();
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
    int dmg = fxMelee(mobWeaponDmg(held)); if (crit) dmg = (int)(dmg * 1.5f); if (hitI == 1) dmg = dmg / 4 + 1;
    if (netOn && !netHost) { NBuf nb = netMk(NP_DRAGONHIT); nb.put<int16_t>((int16_t)dmg); netBroadcast(nb); d.hurtT = 10; return true; }
    dragonApplyDmg(dmg); return true;
}
static void dragonBox(float u, float v, float x0, float y0, float z0, float w, float h, float d) {   // ModelBox 256x256; coords em px do modelo (y para baixo -> invertido ao desenhar)
    const float x1 = x0 + w, y1 = y0 + h, z1 = z0 + d, k = 1.f / 256.f;
    auto Q = [&](float a0, float b0, float a1, float b1, float X0, float Y0, float Z0, float X1, float Y1, float Z1, float X2, float Y2, float Z2, float X3, float Y3, float Z3) {
        glTexCoord2f(a0 * k, b0 * k); glVertex3f(X0, Y0, Z0); glTexCoord2f(a1 * k, b0 * k); glVertex3f(X1, Y1, Z1);
        glTexCoord2f(a1 * k, b1 * k); glVertex3f(X2, Y2, Z2); glTexCoord2f(a0 * k, b1 * k); glVertex3f(X3, Y3, Z3); };
    const float u1 = u + d, u2 = u1 + w, u3 = u2 + d, u4 = u3 + w, v1 = v + d, v2 = v1 + h;
    Q(u1, v, u2, v1, x1, y0, z1, x0, y0, z1, x0, y0, z0, x1, y0, z0);        // topo
    Q(u2, v1, u3, v, x1, y1, z0, x0, y1, z0, x0, y1, z1, x1, y1, z1);        // base
    if (h <= 0.f) return;
    Q(u1, v1, u2, v2, x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0);       // frente (-z)
    Q(u3, v1, u4, v2, x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1);       // costas (+z)
    Q(u2, v1, u3, v2, x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1);       // +x
    Q(u, v1, u1, v2, x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0);        // -x
}
static GLuint gDragonEyeTex = 0, gDragonExpTex = 0;
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
    GLint prev = (GLint)atlasTex;   // patchPerf.1: sem glGetIntegerv (sync CPU<->GPU); chamador deixa o atlas ligado
    if (d.dead && !gDragonExpTex) {
        std::vector<uint8_t> px; int w = 0, h = 0; const bool ok = packRGBA("entity/enderdragon/dragon_exploding.png", px, w, h); if (!ok) { gDragonExpTex = gDragonTex; } else {
        glGenTextures(1, &gDragonExpTex); glBindTexture(GL_TEXTURE_2D, gDragonExpTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data()); } }
    glBindTexture(GL_TEXTURE_2D, eyes ? gDragonEyeTex : (d.dead ? gDragonExpTex : gDragonTex)); rdEnable(GL_TEXTURE_2D); rdDisable(GL_CULL_FACE); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, d.dead ? std::min(.95f, std::max(.1f, d.dead / 200.f)) : .1f); rdDisable(GL_BLEND);
    if (eyes) { rdDisable(GL_ALPHA_TEST); rdEnable(GL_BLEND); rdBlendFunc(GL_ONE, GL_ONE); rdDepthMask(GL_FALSE); rdDepthFunc(GL_LEQUAL); }
    const float c = (d.hurtT > 0 && !eyes) ? .55f : 1.f; glColor4f(1.f, c, c, 1.f);
    // patchEnd.12: ModelDragon do 1.12.2 (256x256) com os mesmos pivos/caixas/UV e a animacao por ringBuffer (pescoco/cauda seguem o caminho)
    const float PI2 = 6.2831853f, DR = .017453292f, RD = 57.29578f; const float pt2 = d.dead ? 0.f : pt;
    auto wr = [](float a) { while (a > 180.f) a -= 360.f; while (a < -180.f) a += 360.f; return a; };
    auto mv = [&](int p, float& yw, float& yy) { const int i = (d.rbI - p) & 63, j = (d.rbI - p - 1) & 63; const float a = d.rbYaw[i], t = 1.f - pt2; yw = a + wr(d.rbYaw[j] - a) * t; yy = d.rbY[i] + (d.rbY[j] - d.rbY[i]) * t; };
    auto PT = [&](float px, float py, float pz, float rx, float ry, float rz) { rdTranslatef(px, py, pz); if (rz != 0.f) rdRotatef(rz * RD, 0, 0, 1); if (ry != 0.f) rdRotatef(ry * RD, 0, 1, 0); if (rx != 0.f) rdRotatef(rx * RD, 1, 0, 0); };
    auto BX = [&](float u, float v, float x, float y, float z, float w, float h, float dd) { glBegin(GL_QUADS); dragonBox(u, v, x, y, z, w, h, dd); glEnd(); };
    auto spineSeg = [&]() { BX(192, 104, -5, -5, -5, 10, 10, 10); BX(48, 0, -1, -9, -3, 2, 4, 6); };
    const float f = L(d.pwing, d.wing); float yw5, y5, yw10, y10, yw7, y7, yw6, y6, yw0, y0;
    mv(5, yw5, y5); mv(10, yw10, y10); mv(7, yw7, y7); mv(6, yw6, y6); mv(0, yw0, y0);
    const float f2 = d.dead ? 0.f : y5 - y10, f6 = wr(yw5 - yw10), f7 = wr(yw5 + f6 / 2.f);
    const float JAW = (sinf(f * PI2) + 1.f) * .2f; float f1 = sinf(f * PI2 - 1.f) + 1.f; f1 = (f1 * f1 + f1 * 2.f) * .05f;
    rdPushMatrix(); rdTranslatef(L(d.px, d.x), L(d.py, d.y) - 1.f, L(d.pz, d.z));
    rdRotatef(-yw7, 0, 1, 0); rdRotatef(f2 * 10.f, 1, 0, 0); rdTranslatef(0.f, 0.f, 1.f);   // RenderDragon.applyRotations (corpo usa o yaw de 7 ticks atras)
    rdTranslatef(0.f, 1.501f, 0.f); rdScalef(-1.f, -1.f, 1.f); rdTranslatef(0.f, f2 - 2.f, -3.f); rdRotatef(f2 * 2.f, 1, 0, 0); rdScalef(.0625f, .0625f, .0625f);
    {   // pescoco: 5 vertebras + cabeca
        float f3 = 8.f, f4 = 0.f, f5 = 1.5f;
        for (int i = 0; i < 5; i++) {
            float a, b; mv(5 - i, a, b);
            const float rY = wr(a - yw6) * DR * 1.5f, rX = cosf(i * .45f + f) * .15f + (b - y6) * DR * 1.5f * 5.f, rZ = -wr(a - f7) * DR * 1.5f;
            rdPushMatrix(); PT(f4, f3, f5, rX, rY, rZ); spineSeg(); rdPopMatrix();
            f3 += sinf(rX) * 10.f; f5 -= cosf(rY) * cosf(rX) * 10.f; f4 -= sinf(rY) * cosf(rX) * 10.f;
        }
        rdPushMatrix(); PT(f4, f3, f5, 0.f, wr(yw0 - yw6) * DR, -wr(yw0 - f7) * DR);
        BX(176, 44, -6, -1, -24, 12, 5, 16); BX(112, 30, -8, -8, -10, 16, 16, 16); BX(0, 0, -5, -12, -4, 2, 4, 6); BX(112, 0, -5, -3, -22, 2, 2, 4); BX(0, 0, 3, -12, -4, 2, 4, 6); BX(112, 0, 3, -3, -22, 2, 2, 4);
        rdPushMatrix(); PT(0.f, 4.f, -8.f, JAW, 0.f, 0.f); BX(176, 65, -6, 0, -16, 12, 4, 16); rdPopMatrix();   // mandibula
        rdPopMatrix();
    }
    rdPushMatrix(); rdTranslatef(0.f, 16.f, 0.f); rdRotatef(-f6 * 1.5f, 0, 0, 1); rdTranslatef(0.f, -16.f, 0.f);
    rdPushMatrix(); PT(0.f, 4.f, 8.f, 0.f, 0.f, 0.f);   // corpo + espinhos
    BX(0, 0, -12, 0, -16, 24, 24, 64); BX(220, 53, -1, -6, -10, 2, 6, 12); BX(220, 53, -1, -6, 10, 2, 6, 12); BX(220, 53, -1, -6, 30, 2, 6, 12); rdPopMatrix();
    {   // cauda: 12 vertebras
        float f3 = 10.f, f4 = 0.f, f5 = 60.f, f12 = 0.f; float a11, b11; mv(11, a11, b11);
        for (int k = 0; k < 12; k++) {
            float a, b; mv(12 + k, a, b); f12 += sinf(k * .45f + f) * .05f;
            const float rY = (wr(a - a11) * 1.5f + 180.f) * DR, rX = f12 + (b - b11) * DR * 1.5f * 5.f, rZ = wr(a - f7) * DR * 1.5f;
            rdPushMatrix(); PT(f4, f3, f5, rX, rY, rZ); spineSeg(); rdPopMatrix();
            f3 += sinf(rX) * 10.f; f5 -= cosf(rY) * cosf(rX) * 10.f; f4 -= sinf(rY) * cosf(rX) * 10.f;
        }
    }
    for (int j = 0; j < 2; j++) {   // asas e patas (a 2a volta espelha em X)
        const float f11 = f * PI2;
        rdPushMatrix(); PT(-12.f, 5.f, 2.f, .125f - cosf(f11) * .2f, .25f, (sinf(f11) + .125f) * .8f);   // asa
        BX(112, 88, -56, -4, -4, 56, 8, 8); BX(-56, 88, -56, 0, 2, 56, 0, 56);
        rdPushMatrix(); PT(-56.f, 0.f, 0.f, 0.f, 0.f, -(sinf(f11 + 2.f) + .5f) * .75f); BX(112, 136, -56, -2, -2, 56, 4, 4); BX(-56, 144, -56, 0, 2, 56, 0, 56); rdPopMatrix(); rdPopMatrix();
        rdPushMatrix(); PT(-12.f, 20.f, 2.f, 1.3f + f1 * .1f, 0.f, 0.f); BX(112, 104, -4, -4, -4, 8, 24, 8);   // pata dianteira
        PT(0.f, 20.f, -1.f, -.5f - f1 * .1f, 0.f, 0.f); BX(226, 138, -3, -1, -3, 6, 24, 6);
        PT(0.f, 23.f, 0.f, .75f + f1 * .1f, 0.f, 0.f); BX(144, 104, -4, 0, -12, 8, 4, 16); rdPopMatrix();
        rdPushMatrix(); PT(-16.f, 16.f, 42.f, 1.f + f1 * .1f, 0.f, 0.f); BX(0, 0, -8, -4, -8, 16, 32, 16);   // pata traseira
        PT(0.f, 32.f, -4.f, .5f + f1 * .1f, 0.f, 0.f); BX(196, 0, -6, -2, 0, 12, 32, 12);
        PT(0.f, 31.f, 4.f, .75f + f1 * .1f, 0.f, 0.f); BX(112, 0, -9, 0, -20, 18, 6, 24); rdPopMatrix();
        rdScalef(-1.f, 1.f, 1.f);
    }
    rdPopMatrix(); rdPopMatrix();
    glColor4f(1.f, 1.f, 1.f, 1.f); glBindTexture(GL_TEXTURE_2D, (GLuint)prev);
    if (eyes) { rdDisable(GL_BLEND); rdDepthMask(GL_TRUE); rdDepthFunc(GL_LEQUAL); rdEnable(GL_ALPHA_TEST); }
}
static void drawDragon(float pt) { drawDragonPass(pt, false); drawDragonPass(pt, true); }

// patchEnd.3: barra de vida do dragao no topo da tela (BossInfo): so no End, com o dragao vivo
static void dragonBar(int sw, int G) {
    if (gDim != 2 || !gDragon.on || gDragon.dead > 0) return;
    if (gPvpP) { const double dx = gPvpP->x - gDragon.x, dy = gPvpP->y - gDragon.y, dz = gPvpP->z - gDragon.z; if (dx * dx + dy * dy + dz * dz > 96.0 * 96.0) return; }   // patchFix.26: so aparece com o dragao a ate 96 blocos
    const float fr = std::max(0.f, std::min(1.f, gDragon.hp / 200.f)); const float bw = 182.f, x = (sw - bw) / 2.f;
    OX = 0; OY = 0; (void)G;
    uText(sw / 2.f - 20.f, 4.f, "Ender Dragon", 0xFFFFFF, true);
    U(x - 1.f, 13.f, bw + 2.f, 7.f, 0x000000, .85f);   // moldura escura (sem o chanfro branco do uBox)
    U(x, 14.f, bw, 5.f, 0x3B2A3F, 1.f);   // trilho
    if (fr > 0.f) { const float fw = bw * fr; U(x, 14.f, fw, 5.f, 0xA818B8, 1.f); U(x, 14.f, fw, 1.f, 0xE070F0, 1.f); U(x, 18.f, fw, 1.f, 0x6A0E78, 1.f); }   // preenchimento rosa com brilho em cima e sombra embaixo
    for (int k = 1; k < 20; k++) U(x + bw * k / 20.f, 14.f, 1.f, 5.f, 0x000000, .35f);   // divisorias (20 segmentos)
}

// ---- patchEnd.5: Cristais do End (EntityEnderCrystal) no topo dos 10 pilares; curam o dragao; 1 golpe/flecha-explosao os destroi ----
static Chunk* cystChunk(int x, int z) { return getC((int)floor(x / (double)CW), (int)floor(z / (double)CW)); }   // getC usa coordenadas de CHUNK
static std::vector<std::array<double, 3>> gCrystBoom; static GLuint gBeamTex = 0;
static GLuint gCrystTex = 0; static int gCrystT = 0;
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
static int crystalNearest(double x, double y, double z, double r) { int b = -1; double bd = r * r; for (int i = 0; i < 10; i++) if (gCrystInit[i] && gCryst[i].alive) { const double dx = gCryst[i].x - x, dy = gCryst[i].y - y, dz = gCryst[i].z - z, q = dx * dx + dy * dy + dz * dz; if (q < bd) { bd = q; b = i; } } return b; }
static void crystalKill(int i) {
    EndCrystal& c = gCryst[i]; c.alive = false; gCrystBoom.push_back({c.x, c.y, c.z}); const int bx = (int)floor(c.x), by = (int)floor(c.y), bz = (int)floor(c.z);
    if (getB(bx, by, bz) == XB_FIRE) { setBM(bx, by, bz, AIR, 0); netSendBlock(bx, by, bz, AIR, 0); }
    if (gDragon.on && gDragon.dead == 0 && i == gHealC) { gDragon.hp = std::max(1.f, gDragon.hp - 10.f); gHealC = -1; }   // patchEnd.12: destruir o cristal que cura fere o dragao (10, explosao)
}
static bool crystalHit(const float* eye, const float* dir, float reach, int dmg) {
    (void)dmg; int bi = -1; double best = 1e9;
    for (int i = 0; i < 10; i++) {
        if (!gCrystInit[i] || !gCryst[i].alive) continue;
        const double ox = gCryst[i].x - eye[0], oy = floor(gCryst[i].y + .01) + 2.0 - eye[1], oz = gCryst[i].z - eye[2], t = ox * dir[0] + oy * dir[1] + oz * dir[2];
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
    GLint prev = (GLint)atlasTex;   // patchPerf.1
    glBindTexture(GL_TEXTURE_2D, gCrystTex); rdEnable(GL_TEXTURE_2D); rdDisable(GL_CULL_FACE); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .1f); glColor4f(1.f, 1.f, 1.f, 1.f);
    const float tm = (float)SDL_GetTicks() * .001f;
    const int hc = (netOn && !netHost) ? crystalNearest(gDragon.x, gDragon.y, gDragon.z, 32.0) : gHealC;
    if (gDragon.on && gDragon.dead == 0 && hc >= 0) {   // raio de cura (so do cristal que cura): fita cruzada de cada cristal ate o dragao
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
            if (i != hc || !gCrystInit[i] || !gCryst[i].alive) continue;
            const double ax = gCryst[i].x, ay = floor(gCryst[i].y + .01) + 2.0, az = gCryst[i].z, bx = gDragon.x, by = gDragon.y + 2., bz = gDragon.z;
            double vx = bx - ax, vy = by - ay, vz = bz - az; const double L = sqrt(vx * vx + vy * vy + vz * vz); if (L < 1. || L > 60.) continue; vx /= L; vy /= L; vz /= L;
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
    // patchEnd.16: ModelEnderCrystal 1.12.2 (textura 64x32): base 12x4x12 em (0,16), vidro 8x8x8 em (0,0) x2 e nucleo 8x8x8 em (32,0); RenderEnderCrystal: escala 2, gira f*3, sobe/desce com f1, vidro externo + 2 camadas internas (x0.875) inclinadas 60 graus
    auto cBox = [](float u, float v, float x0, float y0, float z0, float w, float h, float d) {
        const float x1 = x0 + w, y1 = y0 + h, z1 = z0 + d, ku = 1.f / 64.f, kv = 1.f / 32.f, S = .0625f;
        auto Q = [&](float a0, float b0, float a1, float b1, float X0, float Y0, float Z0, float X1, float Y1, float Z1, float X2, float Y2, float Z2, float X3, float Y3, float Z3) {
            glTexCoord2f(a0 * ku, b0 * kv); glVertex3f(X0 * S, Y0 * S, Z0 * S); glTexCoord2f(a1 * ku, b0 * kv); glVertex3f(X1 * S, Y1 * S, Z1 * S);
            glTexCoord2f(a1 * ku, b1 * kv); glVertex3f(X2 * S, Y2 * S, Z2 * S); glTexCoord2f(a0 * ku, b1 * kv); glVertex3f(X3 * S, Y3 * S, Z3 * S); };
        const float u1 = u + d, u2 = u1 + w, u3 = u2 + d, u4 = u3 + w, v1 = v + d, v2 = v1 + h;
        glBegin(GL_QUADS);
        Q(u1, v, u2, v1, x1, y0, z1, x0, y0, z1, x0, y0, z0, x1, y0, z0); Q(u2, v1, u3, v, x1, y1, z0, x0, y1, z0, x0, y1, z1, x1, y1, z1);
        Q(u1, v1, u2, v2, x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0); Q(u3, v1, u4, v2, x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1);
        Q(u2, v1, u3, v2, x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1); Q(u, v1, u1, v2, x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0);
        glEnd(); };
    for (int i = 0; i < 10; i++) {
        if (!gCrystInit[i] || !gCryst[i].alive) continue;
        const float f = tm * 20.f + i * 7.f, yaw = f * 3.f; float f1 = sinf(f * .2f) / 2.f + .5f; f1 = f1 * f1 + f1;
        rdPushMatrix(); rdTranslatef((float)gCryst[i].x, floorf((float)gCryst[i].y + .01f) + 1.f, (float)gCryst[i].z);   // pes da entidade = topo do bedrock + 1 (base afundada -1 na escala 2)
        rdScalef(2.f, 2.f, 2.f); rdTranslatef(0.f, -.5f, 0.f);
        cBox(0, 16, -6, 0, -6, 12, 4, 12);   // base
        rdRotatef(yaw, 0, 1, 0); rdTranslatef(0.f, .8f + f1 * .2f, 0.f); rdRotatef(60.f, .7071f, 0.f, .7071f);
        cBox(0, 0, -4, -4, -4, 8, 8, 8);   // vidro externo
        rdScalef(.875f, .875f, .875f); rdRotatef(60.f, .7071f, 0.f, .7071f); rdRotatef(yaw, 0, 1, 0);
        cBox(0, 0, -4, -4, -4, 8, 8, 8);   // vidro interno
        rdScalef(.875f, .875f, .875f); rdRotatef(60.f, .7071f, 0.f, .7071f); rdRotatef(yaw, 0, 1, 0);
        cBox(32, 0, -4, -4, -4, 8, 8, 8);  // nucleo
        rdPopMatrix();
    }
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev); glColor4f(1.f, 1.f, 1.f, 1.f);
}

// ---- patchEnd.7: flecha (do jogador local) acerta cristal (esfera r=1) ou dragao (cabeca r=3 dano cheio; corpo r=4,5 dano/4+1) ----
static bool arrowHitEnd(const double* o, const double* d, double bt, int dmg) {
    auto sph = [&](double cx, double cy, double cz, double r) { const double ox = cx - o[0], oy = cy - o[1], oz = cz - o[2], t = ox * d[0] + oy * d[1] + oz * d[2];
        if (t < -r) return -1.0; const double q2 = ox * ox + oy * oy + oz * oz - t * t, r2 = r * r; if (q2 > r2) return -1.0; const double e = t - sqrt(r2 - q2); return e < 0 ? 0.0 : e; };
    double best = bt + 1e-9; int kind = -1, idx = -1;
    for (int i = 0; i < 10; i++) { if (!gCrystInit[i] || !gCryst[i].alive) continue; const double t = sph(gCryst[i].x, floor(gCryst[i].y + .01) + 2.0, gCryst[i].z, 1.0); if (t >= 0 && t < best) { best = t; kind = 0; idx = i; } }
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
