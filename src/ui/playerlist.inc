// Lista de jogadores (Tab), como o Minecraft Java 1.8.9: cabeca da skin (rosto + camada do chapeu), nick e barras de ping.
// Logica pura (ordem, colunas, nivel de ping) fica separada do desenho para ser testada sem GL (tests/test_playerlist.cpp).
struct TabEntry { int id; std::string name; int ping; std::string team = ""; unsigned rgb = 0xFFFFFF; int plat = -1; };   // team/rgb: time e cor do nome
static bool gTabHeld = false, gTabTouch = false;   // gTabTouch: botao de toque (Android) segurado   // Tab segurado (teclado) ou botao de toque

static std::string tabKey(const std::string& s) { std::string k = s; for (char& c : k) c = (char)tolower((unsigned char)c); return k; }
static void tabBuild(std::vector<TabEntry>& out) {   // todos os jogadores conectados, ordem alfabetica sem diferenciar maiuscula (sem times ainda); maximo 80, como o oficial
    out.clear(); out.push_back({netSelf, netName(), netPingOf(netSelf)});
    std::vector<int> ids;
    if (netHost) { for (const NetPeer& p : peers) ids.push_back(p.id); }
    else { if (netConn) ids.push_back(0); for (auto& kv : gNetNames) ids.push_back(kv.first); for (auto& kv : remotes) ids.push_back(kv.first); }
    for (int id : ids) { if (id == netSelf) continue; bool dup = false; for (const TabEntry& e : out) if (e.id == id) dup = true; if (!dup) out.push_back({id, netLabel(id), netPingOf(id)}); }
    for (TabEntry& e : out) { e.team = teamStrOf(e.id); e.rgb = teamRgbOf(e.id); e.plat = platOf(e.id); }
    std::stable_sort(out.begin(), out.end(), [](const TabEntry& a, const TabEntry& b) { const std::string tx = tabKey(a.team), ty = tabKey(b.team); if (tx != ty) return tx < ty;   // por time (sem time primeiro), depois por nick
        const std::string x = tabKey(a.name), y = tabKey(b.name); return x != y ? x < y : a.id < b.id; });
    if (out.size() > 80) out.resize(80);
}
static void tabLayout(int n, int& rows, int& cols) {   // mesma regra do oficial: ate 20 linhas por coluna
    rows = std::max(n, 1); cols = 1; while (rows > 20) { cols++; rows = (n + cols - 1) / cols; }
}
static int tabPingLevel(int ms) {   // 0 = melhor (5 barras) .. 4 = pior (1 barra); 5 = desconhecido (X), igual aos icones do oficial
    if (ms < 0) return 5; if (ms < 150) return 0; if (ms < 300) return 1; if (ms < 600) return 2; if (ms < 1000) return 3; return 4;
}
static GLuint netSkinTexFor(int id);   // net.inc
// patchTab.1: formato do GuiPlayerTabOverlay do Java 1.12: celula 8px de altura (passo 9), cabeca 8x8 (rosto + chapeu no mesmo tamanho) e nome a +9, icone de ping 10x8 do gui/icons.png
// (u=0, v=176+nivel*8) a 11 px do fim da celula, fundo 0x80000000 com 1 px de folga e celulas 0x20FFFFFF, 5 px entre colunas. Sem icons.png no pack cai nas barras desenhadas.
static void tabHead(float x, float y, float s, GLuint tex) {   // rosto 8x8 em (8,8) + chapeu em (40,8) da skin 64x64, ambos 8x8 (igual ao oficial)
    if (!tex) return; glBindTexture(GL_TEXTURE_2D, tex); glColor4f(1, 1, 1, 1);
    quad(x, y, s, s, 8.f / 64, 8.f / 64, 16.f / 64, 16.f / 64);
    quad(x, y, s, s, 40.f / 64, 8.f / 64, 48.f / 64, 16.f / 64);
}
static void tabPingBars(float x, float y, float g, int level) {   // reserva (sem icons.png): 5 barras; level 5 = cinza
    for (int i = 0; i < 5; i++) {
        const float bh = (2 + i * 2) * g * .9f, bx = x + i * 2.f * g;
        if (level >= 5) flat(bx, y + 10.f * g - bh, 1.5f * g, bh, .5f, .5f, .5f, .8f);
        else { const bool on = i < 5 - level; const float r = level == 0 ? 0.f : level == 1 ? .4f : level == 2 ? .9f : level == 3 ? .9f : .8f, gg = level <= 1 ? .85f : level == 2 ? .8f : level == 3 ? .4f : .1f;
            flat(bx, y + 10.f * g - bh, 1.5f * g, bh, on ? r : .15f, on ? gg : .15f, on ? 0.f : .15f, on ? 1.f : .6f); }
    }
}
static void tabPingIcon(float x, float y, float g, int level) {   // x,y = canto do icone 10x8 (px de tela); g = escala da GUI
    if (!rtIcons) { tabPingBars(x, y - 1.f * g, g * .8f, level); return; }
    rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, rtIcons); glColor4f(1, 1, 1, 1);
    const float v = 176.f + 8.f * (float)std::max(0, std::min(5, level));
    quad(x, y, 10.f * g, 8.f * g, 0.f, v / 256.f, 10.f / 256.f, (v + 8.f) / 256.f);
}
static bool tabVisible() { return gTabHeld && netOn; }   // como o Java: so aparece em partida com mais de um jogador
static void drawTabList(int w, int h) {
    if (!tabVisible()) return;
    std::vector<TabEntry> es; tabBuild(es); if (es.size() < 2) return;
    uiOrtho(w, h); const int g = guiScaleFor(w, h); const float G = (float)g;
    int rows, cols; tabLayout((int)es.size(), rows, cols);
    float nameW = 0; bool anyPlat = false; for (const TabEntry& e : es) { nameW = std::max(nameW, textWidth(e.name.c_str(), G) / G); if (e.plat >= 0 && platTex(e.plat)) anyPlat = true; }
    const float plW = anyPlat ? 9.f : 0.f;   // icone da plataforma (extra do Blockland) entre o nome e o ping
    float cellW = std::min((float)cols * (9.f + nameW + plW + 13.f), (float)(w / G - 50.f)) / (float)cols;   // em px de GUI, como o oficial
    cellW = std::floor(cellW);
    const float totW = cellW * cols + 5.f * (cols - 1), x0 = std::floor((w / G - totW) / 2.f), y0 = 10.f;
    flat((x0 - 1.f) * G, (y0 - 1.f) * G, (totW + 2.f) * G, (rows * 9.f + 1.f) * G, 0, 0, 0, .5f);   // Integer.MIN_VALUE = 0x80000000
    for (size_t i = 0; i < es.size(); i++) {
        const int c = (int)i / rows, r = (int)i % rows; const float x = (x0 + c * cellW + c * 5.f) * G, y = (y0 + r * 9.f) * G;
        flat(x, y, cellW * G, 8.f * G, 1, 1, 1, 32.f / 255.f);   // 553648127 = 0x20FFFFFF
        tabHead(x, y, 8.f * G, es[i].id == netSelf ? skinTex : netSkinTexFor(es[i].id));
        text(x + 9.f * G, y, G, es[i].name.c_str(), ((es[i].rgb >> 16) & 255) / 255.f, ((es[i].rgb >> 8) & 255) / 255.f, (es[i].rgb & 255) / 255.f, G);
        if (GLuint pt = platTex(es[i].plat)) { rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, pt); glColor4f(1, 1, 1, 1); quad(x + (cellW - 11.f - 9.f) * G, y, 8.f * G, 8.f * G, 0.f, 0.f, 1.f, 1.f); }   // patchPlat.1
        tabPingIcon(x + (cellW - 11.f) * G, y, G, tabPingLevel(es[i].ping));
        rdEnable(GL_TEXTURE_2D);
    }
    uiEnd();
}
static void drawSplitNick(int w, int h, const std::string& nm) {   // split: nick do dono do viewport, canto superior esquerdo (texto branco com sombra sobre fundo preto 50%)
    uiOrtho(w, h); const int g = guiScaleFor(w, h); const float sc = (float)g, tw = textWidth(nm.c_str(), sc);
    flat(2.f * g, 2.f * g, tw + 4.f * g, 10.f * g, 0, 0, 0, .5f); text(4.f * g, 3.f * g, sc, nm.c_str(), 1, 1, 1, sc); uiEnd();
}

// F3+F4: seletor de modo de jogo igual ao Java 1.16+ (GameModeSwitcherScreen). Segurando F3, cada F4 avanca Criativo > Sobrevivencia > Aventura > Espectador; soltar F3 aplica.
// Layout do Mojang (px de GUI): painel 125x75 em (w/2-62, h/2-58); slots 26x26 a cada 31 px em y = h/2-31; icone 16x16 em +5; nome centrado em y = h/2-51; dica em y = h/2+5.
// Icones: bloco de grama, espada de ferro, mapa, olho do Ender (os mesmos do Minecraft), desenhados pelo atlas do pack. patchGm.1: antes o nome saia trocado (indexava a lista por id do modo).
static bool gGmSw = false, gF3Used = false; static int gGmSelI = 0;
static const int GM_ORDER[4] = {1, 0, 2, 3};   // posicao no menu -> id do modo (0 sobrevivencia, 1 criativo, 2 aventura, 3 espectador)
static int cxId(const char* names);   // game/craft_extra.inc
static void drawGmSwitcher(int w, int h) {
    if (!gGmSw) return;
    uiOrtho(w, h); const int g = guiScaleFor(w, h); const float G = (float)g;
    static const char* const NM[4] = {"Modo Sobrevivência", "Modo Criativo", "Modo Aventura", "Modo Espectador"};   // patchGm.2: nomes como lang do Java; indexado pelo ID do modo
    static int ico[4] = {0, 0, 0, 0}; static bool icoOk = false;   // icone por POSICAO do menu
    if (!icoOk) { icoOk = true; ico[0] = GRASS; ico[1] = IT_ISWORD;   /* patchGm.2: espada de ferro e item nativo (nao do pack): cxId dava 0 e saia "S" */ ico[2] = cxId("map|filled_map|compass"); ico[3] = cxId("ender_eye|eye_of_ender"); }
    const float cx = w / 2.f, cy = h / 2.f, px = cx - 62.f * G, py = cy - 58.f * G;
    auto spr = [&](float u, float v, float sw, float sh, float x, float y) { glBindTexture(GL_TEXTURE_2D, rtGmSw); rdEnable(GL_TEXTURE_2D); glColor4f(1, 1, 1, 1); quad(x, y, sw * G, sh * G, u / 128.f, v / 128.f, (u + sw) / 128.f, (v + sh) / 128.f); };
    if (rtGmSw) spr(0, 0, 125, 75, px, py); else flat(px, py, 125.f * G, 75.f * G, .06f, .06f, .06f, .8f);
    const char* nome = NM[GM_ORDER[gGmSelI]];
    text(cx - textWidth(nome, G) / 2.f, cy - 51.f * G, G, nome, 1, 1, 1, G);
    for (int i = 0; i < 4; i++) {
        const float x = cx - 59.f * G + i * 31.f * G, y = cy - 31.f * G;
        if (rtGmSw) spr(0, 75, 26, 26, x, y); else flat(x, y, 26.f * G, 26.f * G, .14f, .14f, .14f, .95f);
        if (ico[i]) drawIcon(ico[i], x + 5.f * G, y + 5.f * G, 16.f * G);
        else { const char* ab[4] = {"C", "S", "A", "E"}; text(x + (26.f * G - textWidth(ab[i], G)) / 2.f, y + 9.f * G, G, ab[i], 1, 1, 1, G); }
        if (i == gGmSelI) { if (rtGmSw) spr(26, 75, 26, 26, x, y); else uRing(x, y, 26.f * G, 26.f * G, 2.f * G, 0xECB81C); }
    }
    const char* k = "[ F4 ]"; const char* rest = " Próximo"; const float kw = textWidth(k, G), pw = 7.f * G, tw = kw + pw + textWidth(rest, G), hx = cx - tw / 2.f, hy = cy + 5.f * G;   // patchGm.2: "[ F4 ] + Próximo" como no Java (+ cinza entre a tecla e o texto)
    text(hx, hy, G, k, .333f, 1.f, 1.f, G);
    flat(hx + kw + 2.f * G, hy + 3.f * G, 5.f * G, G, .63f, .63f, .63f, 1.f); flat(hx + kw + 4.f * G, hy + 1.f * G, G, 5.f * G, .63f, .63f, .63f, 1.f);
    text(hx + kw + pw, hy, G, rest, .63f, .63f, .63f, G);
    uiEnd();
}
