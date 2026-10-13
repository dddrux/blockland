// Lista de jogadores (Tab), como o Minecraft Java 1.8.9: cabeca da skin (rosto + camada do chapeu), nick e barras de ping.
// Logica pura (ordem, colunas, nivel de ping) fica separada do desenho para ser testada sem GL (tests/test_playerlist.cpp).
struct TabEntry { int id; std::string name; int ping; std::string team = ""; unsigned rgb = 0xFFFFFF; };   // team/rgb: time e cor do nome
static bool gTabHeld = false, gTabTouch = false;   // gTabTouch: botao de toque (Android) segurado   // Tab segurado (teclado) ou botao de toque

static std::string tabKey(const std::string& s) { std::string k = s; for (char& c : k) c = (char)tolower((unsigned char)c); return k; }
static void tabBuild(std::vector<TabEntry>& out) {   // todos os jogadores conectados, ordem alfabetica sem diferenciar maiuscula (sem times ainda); maximo 80, como o oficial
    out.clear(); out.push_back({netSelf, netName(), netPingOf(netSelf)});
    std::vector<int> ids;
    if (netHost) { for (const NetPeer& p : peers) ids.push_back(p.id); }
    else { if (netConn) ids.push_back(0); for (auto& kv : gNetNames) ids.push_back(kv.first); for (auto& kv : remotes) ids.push_back(kv.first); }
    for (int id : ids) { if (id == netSelf) continue; bool dup = false; for (const TabEntry& e : out) if (e.id == id) dup = true; if (!dup) out.push_back({id, netLabel(id), netPingOf(id)}); }
    for (TabEntry& e : out) { e.team = teamStrOf(e.id); e.rgb = teamRgbOf(e.id); }
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
static void tabHead(float x, float y, float s, GLuint tex) {   // rosto 8x8 em (8,8) + chapeu em (40,8) da skin 64x64
    if (!tex) return; glBindTexture(GL_TEXTURE_2D, tex); glColor4f(1, 1, 1, 1);
    quad(x, y, s, s, 8.f / 64, 8.f / 64, 16.f / 64, 16.f / 64);
    const float o = s / 8.f * .5f; quad(x - o, y - o, s + 2 * o, s + 2 * o, 40.f / 64, 8.f / 64, 48.f / 64, 16.f / 64);
}
static void tabPingBars(float x, float y, float g, int level) {   // 5 barras de 2x(2..10) px; level 5 = X vermelho
    for (int i = 0; i < 5; i++) {
        const float bh = (2 + i * 2) * g * .9f, bx = x + i * 2.f * g;
        if (level >= 5) flat(bx, y + 10.f * g - bh, 1.5f * g, bh, .5f, .5f, .5f, .8f);
        else { const bool on = i < 5 - level; const float r = level == 0 ? 0.f : level == 1 ? .4f : level == 2 ? .9f : level == 3 ? .9f : .8f, gg = level <= 1 ? .85f : level == 2 ? .8f : level == 3 ? .4f : .1f;
            flat(bx, y + 10.f * g - bh, 1.5f * g, bh, on ? r : .15f, on ? gg : .15f, on ? 0.f : .15f, on ? 1.f : .6f); }
    }
}
static bool tabVisible() { return gTabHeld && netOn; }   // como o Java: so aparece em partida com mais de um jogador
static void drawTabList(int w, int h) {
    if (!tabVisible()) return;
    std::vector<TabEntry> es; tabBuild(es); if (es.size() < 2) return;
    uiOrtho(w, h); const int g = guiScaleFor(w, h); const float sc = (float)g;
    int rows, cols; tabLayout((int)es.size(), rows, cols);
    float colW = 0; for (const TabEntry& e : es) colW = std::max(colW, textWidth(e.name.c_str(), sc));
    colW += 9.f * g + 14.f * g + 6.f * g;   // cabeca + nome + barras
    colW = std::min(colW, (float)(w - 20 * g) / cols);
    const float totW = colW * cols, x0 = (w - totW) / 2, y0 = 10.f * g;
    flat(x0 - 2 * g, y0 - 2 * g, totW + 4 * g, rows * 9.f * g + 4 * g, 0, 0, 0, .5f);
    for (size_t i = 0; i < es.size(); i++) {
        const int c = (int)i / rows, r = (int)i % rows; const float x = x0 + c * colW, y = y0 + r * 9.f * g;
        flat(x, y, colW - 1.f * g, 8.f * g, 1, 1, 1, .13f);
        tabHead(x + 1.f * g, y, 8.f * g, es[i].id == netSelf ? skinTex : netSkinTexFor(es[i].id));
        text(x + 11.f * g, y, sc, es[i].name.c_str(), ((es[i].rgb >> 16) & 255) / 255.f, ((es[i].rgb >> 8) & 255) / 255.f, (es[i].rgb & 255) / 255.f, sc);
        tabPingBars(x + colW - 13.f * g, y - 1.f * g, g * .8f, tabPingLevel(es[i].ping));
        glEnable(GL_TEXTURE_2D);
    }
    uiEnd();
}
static void drawSplitNick(int w, int h, const std::string& nm) {   // split: nick do dono do viewport, canto superior esquerdo (texto branco com sombra sobre fundo preto 50%)
    uiOrtho(w, h); const int g = guiScaleFor(w, h); const float sc = (float)g, tw = textWidth(nm.c_str(), sc);
    flat(2.f * g, 2.f * g, tw + 4.f * g, 10.f * g, 0, 0, 0, .5f); text(4.f * g, 3.f * g, sc, nm.c_str(), 1, 1, 1, sc); uiEnd();
}

// F3+F4: seletor de modo de jogo como no Java 1.16+ (F4 repetido, com F3 segurado, passa Criativo > Sobrevivencia > Aventura > Espectador; soltar F3 aplica)
static bool gGmSw = false, gF3Used = false; static int gGmSelI = 0;
static const int GM_ORDER[4] = {1, 0, 2, 3};
static void drawGmSwitcher(int w, int h) {
    if (!gGmSw) return;
    uiOrtho(w, h); const int g = guiScaleFor(w, h); const float sc = (float)g;
    static const char* const N[4] = {"Criativo", "Sobrevivência", "Aventura", "Espectador"};
    const float bw = 26.f * g, gap = 5.f * g, tot = 4 * bw + 3 * gap, x0 = (w - tot) / 2.f, y0 = h / 2.f - 40.f * g;
    const char* hint = "[ F4 ] Próximo";
    text((w - textWidth(N[GM_ORDER[gGmSelI]], sc)) / 2.f, y0 - 14.f * g, sc, N[GM_ORDER[gGmSelI]], 1, 1, 1, sc);
    for (int i = 0; i < 4; i++) {
        const float x = x0 + i * (bw + gap); const bool on = i == gGmSelI;
        flat(x - (on ? 2.f : 0.f) * g, y0 - (on ? 2.f : 0.f) * g, bw + (on ? 4.f : 0.f) * g, bw + (on ? 4.f : 0.f) * g, on ? 1.f : 0.f, on ? 1.f : 0.f, on ? 1.f : 0.f, on ? .9f : .6f);
        flat(x, y0, bw, bw, 0.12f, 0.12f, 0.12f, .85f);
        const char* ab[4] = {"C", "S", "A", "E"};
        text(x + (bw - textWidth(ab[i], sc)) / 2.f, y0 + (bw - 8.f * g) / 2.f, sc, ab[i], 1, 1, 1, sc);
    }
    text((w - textWidth(hint, sc)) / 2.f, y0 + bw + 8.f * g, sc, hint, .7f, .7f, .7f, sc);
    uiEnd();
}
