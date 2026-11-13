// ================= assets separados =================
// Os arquivos PNG separados sao empacotados em um atlas somente em memoria.
// Isso mantem o renderer OpenGL 1.x simples, mas deixa cada textura editavel.
static int itemTile(int id);
static GLuint guiSlotTex = 0, guiSelectedTex = 0, guiHoverTex = 0, guiPanelTex = 0, guiCrosshairTex = 0, guiDimTex = 0;
static GLuint invSlotTex = 0, invSelectedTex = 0, invHoverTex = 0, invPanelTex = 0, invDimTex = 0, invHeaderTex = 0, invTooltipTex = 0;
static bool gChShulker = false;   // patchOpt.1: tela do bau aberta e de caixa de shulker
static bool gShadowMasterClick = false, gShadowClick = false, gShQClick = false, gGodClick = false, gFlClick = false;   // patchOpt.1: pedidos da tela de Shaders (tratados no loop)
static int gGuiScaleOpt = 0, gGuiScalePrev = 0; static bool gViewBobOn = true, gGfxFancy = false;   // patchOpt.1
static float guiScaleCfg = 0.f;   // 0 = automatico (como o Java)
static float guiSlotSize = 44.f, guiGap = 4.f, guiHotbarBottom = 12.f, guiPanelPad = 10.f, guiAlpha = .97f, guiDimAlpha = .45f;
static float inventorySlotMax = 52.f, inventoryTopMargin = 70.f, inventoryPanelAlpha = .97f, inventoryHeaderAlpha = .96f, inventoryTooltipAlpha = .90f;

// ---- Rethoughted GUI (assets/textures/gui/rt/): base visual de toda a interface. Se um arquivo faltar, cai no desenho em codigo ----
static GLuint rtGmSw = 0;   // patchGm.1: gui/rt/gamemode_switcher.png (128x128, layout do Java 1.16+: painel 125x75 em 0,0 | slot 26x26 em 0,75 | selecionado 26x26 em 26,75)
static GLuint rtIcons = 0;   // gui/icons.png do pack (coracao, coxa, bolha, barra de XP)
static GLuint rtBtn[3] = {0, 0, 0}, rtHotbar = 0, rtHotSel = 0, rtOffL = 0, rtCross = 0, rtInv = 0, rtCraft = 0, rtFurn = 0, rtCrItems = 0, rtCrInv = 0, rtCrSearch = 0, rtSlot = 0, rtScr[2] = {0, 0}, rtTipBg = 0, rtTipFr = 0;
static GLuint rtTab[2][2][7];   // [0 = topo, 1 = base][0 = nao selecionada, 1 = selecionada][coluna 0..6]
static bool gGuiLight = false;   // GUI do pack 1.12 (paineis claros): texto escuro nos paineis
static int gTabW = 26;            // largura do sprite da aba: 26 (Rethoughted) ou 28 (pack)
static bool guiFromPack = true;   // assets/ui/gui.cfg: gui_pack=0 mantem o GUI escuro do jogo (rt/)
static GLuint guiWidgetsTex = 0;   // widgets.png (botoes do menu, formato do Java: 256x256, normal v=46, destacado v=66, desativado v=86)
static GLuint emptySlotTex[5] = {0, 0, 0, 0, 0};   // icones dos slots vazios: capacete, peitoral, calcas, botas, escudo (assets/textures/gui/empty_*.png)
static bool loadTexturePNG(const char* path, GLuint& tex, bool linear = false) {
    std::vector<uint8_t> rgba; int w = 0, h = 0;
    if (!loadPNG(path, rgba, w, h)) { logWarn("Falha ao carregar textura: %s", path); return false; }
    if (!tex) glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    return true;
}

// Textura padrao dos botoes embutida no jogo: usada quando assets/textures/gui/widgets.png nao existe.
// Mesma regra do arquivo (borda preta, brilho em cima/esquerda, sombra embaixo, cantos vazados).
static void genWidgetsFallback() {
    static const unsigned ST[3][3] = {{0x6F6F6F, 0xA0A0A0, 0x555555}, {0x7B8BC6, 0xB0BCEC, 0x5767A0}, {0x484848, 0x666666, 0x353535}};   // preenchimento, brilho, sombra
    std::vector<uint8_t> px(256 * 256 * 4, 0);
    for (int s = 0; s < 3; ++s) for (int y = 0; y < 20; ++y) for (int x = 0; x < 200; ++x) {
        if ((x == 0 || x == 199) && (y == 0 || y == 19)) continue;
        unsigned c;
        if (x == 0 || x == 199 || y == 0 || y == 19) c = 0x000000;
        else if (y == 1 || x == 1) c = ST[s][1];
        else if (y >= 17) c = y == 18 ? ST[s][2] : ST[s][2] + 0x0B0B0B;
        else c = ST[s][0];
        uint8_t* q = &px[((size_t)(46 + 20 * s + y) * 256 + x) * 4];
        q[0] = (c >> 16) & 255; q[1] = (c >> 8) & 255; q[2] = c & 255; q[3] = 255;
    }
    if (!guiWidgetsTex) glGenTextures(1, &guiWidgetsTex);
    glBindTexture(GL_TEXTURE_2D, guiWidgetsTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
}

static void readGuiConfig() {
    FILE* f = fopen("assets/ui/gui.cfg", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char key[96]; float v;
        if (sscanf(line, " %95[^=]=%f", key, &v) != 2) continue;
        if (!strcmp(key, "gui_scale")) guiScaleCfg = v;
        else if (!strcmp(key, "slot_size")) guiSlotSize = v;
        else if (!strcmp(key, "slot_gap")) guiGap = v;
        else if (!strcmp(key, "gui_pack")) guiFromPack = v != 0;
        else if (!strcmp(key, "hotbar_bottom")) guiHotbarBottom = v;
        else if (!strcmp(key, "inventory_padding")) guiPanelPad = v;
        else if (!strcmp(key, "ui_alpha")) guiAlpha = v;
        else if (!strcmp(key, "dim_alpha")) guiDimAlpha = v;
        else if (!strcmp(key, "inventory_slot_max")) inventorySlotMax = v;
        else if (!strcmp(key, "inventory_top_margin")) inventoryTopMargin = v;
        else if (!strcmp(key, "inventory_panel_alpha")) inventoryPanelAlpha = v;
        else if (!strcmp(key, "inventory_header_alpha")) inventoryHeaderAlpha = v;
        else if (!strcmp(key, "inventory_tooltip_alpha")) inventoryTooltipAlpha = v;
    }
}

// ---- GUI do pack ativo (patchTex.2): recorta regioes de assets/minecraft/textures/gui/... e sobe como textura. Sem o arquivo no pack, mantem o GUI do jogo ----
static bool packGuiCrop(const char* rel, int sx, int sy, int sw, int sh, std::vector<uint8_t>& out) {   // pura (testavel): recorte RGBA de um PNG do pack
    std::vector<uint8_t> px; int w, h; if (!packRGBA(rel, px, w, h) || sx < 0 || sy < 0 || sx + sw > w || sy + sh > h) return false;
    out.resize((size_t)sw * sh * 4); for (int y = 0; y < sh; y++) memcpy(&out[(size_t)y * sw * 4], &px[((size_t)(sy + y) * w + sx) * 4], (size_t)sw * 4); return true;
}
static bool packGuiTex(const char* rel, int sx, int sy, int sw, int sh, GLuint& tex, bool full = false) {   // full = mantem o PNG inteiro (256x256 dos containers)
    std::vector<uint8_t> px; int w, h; if (full) { if (!packRGBA(rel, px, w, h)) return false; sw = w; sh = h; } else if (!packGuiCrop(rel, sx, sy, sw, sh, px)) return false;
    if (!tex) glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sw, sh, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data()); return true;
}
// patchRP.4: GUI POR CAMADA. Cada elemento e resolvido camada por camada (topo -> base). Em cada camada tenta, nesta ordem, o sprite moderno
// (gui/sprites/...) e o atlas antigo (widgets.png, tabs.png...). A 1a camada que tiver qualquer um vence: um pack de cima que so traz
// sprites modernos (ex.: mcglint-gui) nao e mais ignorado so porque o jogo procurava o widgets.png (que o pack de baixo tinha).
struct GuiSrc { const char* rel; int sx, sy, sw, sh; };   // sw == 0: PNG inteiro
static bool layerHas(const McPack& p, const std::string& rel) { const std::string f = "assets/minecraft/textures/" + rel; return p.zip ? p.ents.count(p.prefix + f) > 0 : std::filesystem::exists(p.dir + f); }
static bool layerGuiPx(const McPack& L, const GuiSrc& s, std::vector<uint8_t>& out, int& ow, int& oh) {
    std::vector<uint8_t> d, px; int w, h;
    if (!L.getOne(std::string("assets/minecraft/textures/") + s.rel, d) || !loadPNGMem(d, px, w, h)) return false;
    if (!s.sw) { out.swap(px); ow = w; oh = h; return true; }
    if (s.sx < 0 || s.sy < 0 || s.sx + s.sw > w || s.sy + s.sh > h) return false;
    out.resize((size_t)s.sw * s.sh * 4); for (int y = 0; y < s.sh; y++) memcpy(&out[(size_t)y * s.sw * 4], &px[((size_t)(s.sy + y) * w + s.sx) * 4], (size_t)s.sw * 4);
    ow = s.sw; oh = s.sh; return true;
}
static void guiUpload(const std::vector<uint8_t>& px, int w, int h, GLuint& tex) {
    if (!tex) glGenTextures(1, &tex);   // reaproveita o mesmo id: recarregar nao invalida quem ja guarda o GLuint
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
}
// onlyLayer >= 0: so essa camada (usado nas abas, que precisam vir todas da mesma). Devolve true se achou (tex so e tocada nesse caso)
static bool packGuiL(std::initializer_list<GuiSrc> srcs, GLuint& tex, int onlyLayer = -1) {
    std::vector<uint8_t> px; int w = 0, h = 0; const int nl = packLayerCount();
    for (int k = 0; k < nl; k++) {
        if (onlyLayer >= 0 && k != onlyLayer) continue;
        const McPack* L = packLayerAt(k); if (!L) continue;
        for (const GuiSrc& s : srcs) if (layerGuiPx(*L, s, px, w, h)) { guiUpload(px, w, h, tex); return true; }
    }
    return false;
}
// patchRP.5: icons.png composto por camada. Base = icons.png antigo (1.12) da camada mais alta que o tiver; por cima, os sprites modernos
// (gui/sprites/hud/...) de cada camada, colados nas coordenadas do icons.png que o HUD ja usa. Da base para o topo: o de cima vence elemento a elemento.
struct IconMap { const char* rel; int x, y, w, h; const char* alt = nullptr; };   // alt = 2o nome tentado na mesma camada   // x,y,w,h = onde fica no icons.png 256x256 (coordenadas do Java 1.12)
static const IconMap ICON_MAP[] = {
    {"gui/sprites/hud/heart/container.png", 16, 0, 9, 9}, {"gui/sprites/hud/heart/container_blinking.png", 25, 0, 9, 9},
    {"gui/sprites/hud/heart/full.png", 52, 0, 9, 9}, {"gui/sprites/hud/heart/half.png", 61, 0, 9, 9},
    {"gui/sprites/hud/heart/container_hardcore.png", 16, 45, 9, 9}, {"gui/sprites/hud/heart/container_hardcore_blinking.png", 25, 45, 9, 9},
    {"gui/sprites/hud/heart/hardcore_full.png", 52, 45, 9, 9}, {"gui/sprites/hud/heart/hardcore_half.png", 61, 45, 9, 9},
    {"gui/sprites/hud/food_empty.png", 16, 27, 9, 9}, {"gui/sprites/hud/food_full.png", 52, 27, 9, 9}, {"gui/sprites/hud/food_half.png", 61, 27, 9, 9},
    {"gui/sprites/hud/air.png", 16, 18, 9, 9}, {"gui/sprites/hud/air_bursting.png", 25, 18, 9, 9},
    {"gui/sprites/hud/experience_bar_background.png", 0, 64, 182, 5}, {"gui/sprites/hud/experience_bar_progress.png", 0, 69, 182, 5},
    // ping da lista de jogadores (10x8): linhas do icons.png antigo = 0 melhor (5 barras) ... 4 pior (1 barra), 5 desconhecido; modern: icon/ping_N (ou server_list/ping_N)
    {"gui/sprites/icon/ping_5.png", 0, 176, 10, 8, "gui/sprites/server_list/ping_5.png"}, {"gui/sprites/icon/ping_4.png", 0, 184, 10, 8, "gui/sprites/server_list/ping_4.png"},
    {"gui/sprites/icon/ping_3.png", 0, 192, 10, 8, "gui/sprites/server_list/ping_3.png"}, {"gui/sprites/icon/ping_2.png", 0, 200, 10, 8, "gui/sprites/server_list/ping_2.png"},
    {"gui/sprites/icon/ping_1.png", 0, 208, 10, 8, "gui/sprites/server_list/ping_1.png"}, {"gui/sprites/icon/ping_unknown.png", 0, 216, 10, 8, "gui/sprites/server_list/ping_unknown.png"},
};
static bool packIconsAtlas(GLuint& tex) {
    std::vector<uint8_t> at; int S = 0; bool any = false;   // atlas SxS (S = 256 * sc)
    auto upscale = [&](int ns) { if (ns <= S) return; if (S == 0) { at.assign((size_t)ns * ns * 4, 0); S = ns; return; }
        const int f = (ns + S - 1) / S; const int N = S * f; std::vector<uint8_t> t((size_t)N * N * 4);
        for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) memcpy(&t[((size_t)y * N + x) * 4], &at[((size_t)(y / f) * S + x / f) * 4], 4);
        at.swap(t); S = N; };
    for (int k = packLayerCount() - 1; k >= 0; k--) {   // base -> topo
        const McPack* L = packLayerAt(k); if (!L) continue;
        std::vector<uint8_t> px; int w = 0, h = 0;
        if (layerGuiPx(*L, GuiSrc{"gui/icons.png", 0, 0, 0, 0}, px, w, h) && w == h && w >= 256) { at = px; S = w; any = true; }   // icons.png inteiro: substitui tudo que veio de baixo
        for (const IconMap& m : ICON_MAP) {
            if (!(layerGuiPx(*L, GuiSrc{m.rel, 0, 0, 0, 0}, px, w, h) || (m.alt && layerGuiPx(*L, GuiSrc{m.alt, 0, 0, 0, 0}, px, w, h))) || w < 1 || h < 1) continue;
            const int need = 256 * std::max(1, (w + m.w - 1) / m.w); if (S < need) upscale(need);
            const int sc = S / 256, tw = m.w * sc, th = m.h * sc;   // destino em pixels do atlas; vizinho mais proximo
            for (int y = 0; y < th; y++) for (int x = 0; x < tw; x++) memcpy(&at[((size_t)(m.y * sc + y) * S + m.x * sc + x) * 4], &px[((size_t)(y * h / th) * w + x * w / tw) * 4], 4);
            any = true;
        }
    }
    if (!any) return false; guiUpload(at, S, S, tex); return true;
}
// patchRP.6: seletor de modo (F3+F4) composto por camada em um atlas 128x128: painel 125x75 em 0,0 | slot 26x26 em 0,75 | selecionado 26x26 em 26,75.
// Base = gui/rt/gamemode_switcher.png do jogo; da base para o topo cada camada troca so o que tiver: painel (gui/container/gamemode_switcher.png, 125x75),
// slot (gui/sprites/gamemode_switcher/slot.png) e realce (gui/sprites/gamemode_switcher/selected.png). O arquivo do pack NAO traz slot/realce no atlas: nunca copiar ele inteiro.
static bool packGmSwitcher(GLuint& tex) {
    std::vector<uint8_t> at; int aw = 0, ah = 0; bool any = false;
    if (!loadPNG("assets/textures/gui/rt/gamemode_switcher.png", at, aw, ah) || aw != 128 || ah != 128) { at.assign(128 * 128 * 4, 0); aw = ah = 128; }
    auto paste = [&](const std::vector<uint8_t>& px, int w, int h, int dx, int dy, int tw, int th) {   // vizinho mais proximo; apaga a regiao antes (transparencia do pack vale)
        for (int y = 0; y < th; y++) for (int x = 0; x < tw; x++) memcpy(&at[((size_t)(dy + y) * 128 + dx + x) * 4], &px[((size_t)(y * h / th) * w + x * w / tw) * 4], 4); };
    for (int k = packLayerCount() - 1; k >= 0; k--) {   // base -> topo
        const McPack* L = packLayerAt(k); if (!L) continue; std::vector<uint8_t> px; int w = 0, h = 0;
        if (layerGuiPx(*L, GuiSrc{"gui/container/gamemode_switcher.png", 0, 0, 0, 0}, px, w, h) && w >= 128 && h >= 128) {   // painel: recorte 125x75 (na escala do arquivo)
            const int f = w / 128, pw = 125 * f, ph = 75 * f; std::vector<uint8_t> c((size_t)pw * ph * 4); for (int y = 0; y < ph; y++) memcpy(&c[(size_t)y * pw * 4], &px[(size_t)y * w * 4], (size_t)pw * 4);
            paste(c, pw, ph, 0, 0, 125, 75); any = true; }
        if (layerGuiPx(*L, GuiSrc{"gui/sprites/gamemode_switcher/slot.png", 0, 0, 0, 0}, px, w, h)) { paste(px, w, h, 0, 75, 26, 26); any = true; }
        if (layerGuiPx(*L, GuiSrc{"gui/sprites/gamemode_switcher/selected.png", 0, 0, 0, 0}, px, w, h)) { paste(px, w, h, 26, 75, 26, 26); any = true; }
    }
    if (!any) return false; guiUpload(at, 128, 128, tex); return true;
}
static int loadPackGUI() {   // devolve quantas texturas vieram do pack
    if (!guiFromPack || !gPack.ok) return 0; int n = 0;
    const char* WG = "gui/widgets.png"; const char* IC = "gui/container/inventory.png"; const char* TB = "gui/container/creative_inventory/tabs.png";
    n += packGuiL({{"gui/sprites/widget/button.png", 0, 0, 0, 0}, {WG, 0, 66, 200, 20}}, rtBtn[0]);   // 1.12: normal y=66, destacado y=86, desativado y=46
    n += packGuiL({{"gui/sprites/widget/button_highlighted.png", 0, 0, 0, 0}, {WG, 0, 86, 200, 20}}, rtBtn[1]);
    n += packGuiL({{"gui/sprites/widget/button_disabled.png", 0, 0, 0, 0}, {WG, 0, 46, 200, 20}}, rtBtn[2]);
    n += packGuiL({{"gui/sprites/hud/hotbar.png", 0, 0, 0, 0}, {WG, 0, 0, 182, 22}}, rtHotbar);
    n += packGuiL({{"gui/sprites/hud/hotbar_selection.png", 0, 0, 0, 0}, {WG, 0, 22, 24, 23}}, rtHotSel);
    n += packGuiL({{"gui/sprites/hud/hotbar_offhand_left.png", 0, 0, 0, 0}, {WG, 24, 22, 29, 24}}, rtOffL);   // slot da mao esquerda (1.12: widgets.png u=24 v=22 29x24)
    n += packGuiL({{IC, 0, 0, 0, 0}}, rtInv);
    n += packGuiL({{"gui/container/crafting_table.png", 0, 0, 0, 0}}, rtCraft);
    n += packGuiL({{"gui/container/furnace.png", 0, 0, 0, 0}}, rtFurn);   // patchFur.4: fundo + chama (176,0 14x14) + seta (176,14 24x17)
    n += packGuiL({{"gui/container/creative_inventory/tab_items.png", 0, 0, 0, 0}}, rtCrItems);
    n += packGuiL({{"gui/container/creative_inventory/tab_inventory.png", 0, 0, 0, 0}}, rtCrInv);
    n += packGuiL({{"gui/container/creative_inventory/tab_item_search.png", 0, 0, 0, 0}}, rtCrSearch);
    n += packGuiL({{"gui/sprites/container/creative_inventory/scroller.png", 0, 0, 0, 0}, {TB, 232, 0, 12, 15}}, rtScr[1]);
    n += packGuiL({{"gui/sprites/container/creative_inventory/scroller_disabled.png", 0, 0, 0, 0}, {TB, 244, 0, 12, 15}}, rtScr[0]);
    {   // abas do criativo: todas da MESMA camada (a 1a que tiver sprites 26x32 modernos ou o tabs.png 28x32 antigo)
        const int nl = packLayerCount();
        for (int k = 0; k < nl; k++) {
            const McPack* L = packLayerAt(k); if (!L) continue;
            const bool modern = layerHas(*L, "gui/sprites/container/creative_inventory/tab_top_selected_1.png"), legacy = layerHas(*L, TB);
            if (!modern && !legacy) continue;
            gTabW = modern ? 26 : 28;
            for (int kk = 0; kk < 2; kk++) for (int sl = 0; sl < 2; sl++) for (int c = 0; c < 7; c++) {
                char nm[160]; snprintf(nm, sizeof nm, "gui/sprites/container/creative_inventory/tab_%s_%s_%d.png", kk ? "bottom" : "top", sl ? "selected" : "unselected", c + 1);
                const int y = (kk ? 64 : 0) + (sl ? 32 : 0);
                if (modern) n += packGuiL({{nm, 0, 0, 0, 0}}, rtTab[kk][sl][c], k); else n += packGuiL({{TB, c * 28, y, 28, 32}}, rtTab[kk][sl][c], k);
            }
            break;
        }
    }
    n += packGuiL({{"gui/sprites/container/slot.png", 0, 0, 0, 0}, {IC, 7, 141, 18, 18}}, rtSlot);   // moldura de slot 18x18: troca o slot escuro do Rethoughted (bau, lixeira, criativo)
    n += packGmSwitcher(rtGmSw);   // patchRP.6
    n += packIconsAtlas(rtIcons);   // patchHud.1 / patchRP.5: coracoes, fome, bolhas e barra de XP (icons.png antigo e/ou sprites modernos, por camada)
    n += packGuiL({{"gui/sprites/tooltip/background.png", 0, 0, 0, 0}}, rtTipBg);   // patchRP.5: 100x100, borda 9 (mesmo formato do Rethoughted)
    n += packGuiL({{"gui/sprites/tooltip/frame.png", 0, 0, 0, 0}}, rtTipFr);       // 100x100, borda 10
    gGuiLight = rtInv != 0 && n > 0;
    n += packGuiL({{WG, 0, 0, 0, 0}}, guiWidgetsTex);   // menu de pausa/reserva usa o widgets.png inteiro
    return n;
}
static void loadGUIAssets() {
    readGuiConfig();
    loadTexturePNG("assets/textures/gui/slot.png", guiSlotTex);
    loadTexturePNG("assets/textures/gui/slot_selected.png", guiSelectedTex);
    loadTexturePNG("assets/textures/gui/slot_hover.png", guiHoverTex);
    loadTexturePNG("assets/textures/gui/panel.png", guiPanelTex);
    loadTexturePNG("assets/textures/gui/crosshair.png", guiCrosshairTex);
    loadTexturePNG("assets/textures/gui/dim.png", guiDimTex);
    if (!loadTexturePNG("assets/textures/gui/widgets.png", guiWidgetsTex)) genWidgetsFallback();
    {   // Rethoughted GUI
        const char* R = "assets/textures/gui/rt/"; char p[160];
        auto L = [&](const char* n, GLuint& t) { snprintf(p, sizeof p, "%s%s.png", R, n); return loadTexturePNG(p, t); };
        L("button", rtBtn[0]); L("button_highlighted", rtBtn[1]); L("button_disabled", rtBtn[2]);
        L("hotbar", rtHotbar); L("hotbar_selection", rtHotSel); { snprintf(p, sizeof p, "%shotbar_offhand_left.png", R); if (FILE* fo = fopen(p, "rb")) { fclose(fo); if (!rtOffL) L("hotbar_offhand_left", rtOffL); } }   /* patchLog.1: arquivo opcional (o slot vem do widgets.png do pack): sem aviso */ L("crosshair", rtCross);
        L("inventory", rtInv); L("crafting_table", rtCraft); L("creative_items", rtCrItems); L("creative_inventory", rtCrInv); L("creative_search", rtCrSearch);
        L("gamemode_switcher", rtGmSw); L("slot", rtSlot); L("scroller", rtScr[1]); L("scroller_disabled", rtScr[0]); L("tooltip_background", rtTipBg); L("tooltip_frame", rtTipFr);
        for (int k = 0; k < 2; k++) for (int s = 0; s < 2; s++) for (int c = 0; c < 7; c++) {
            rtTab[k][s][c] = 0; char n[48]; snprintf(n, sizeof n, "tab_%s_%s_%d", k ? "bottom" : "top", s ? "selected" : "unselected", c + 1); L(n, rtTab[k][s][c]);
        }
        { static const char* EN[5] = {"helmet", "chestplate", "leggings", "boots", "shield"};   // icones dos slots vazios do pack
          for (int i = 0; i < 5; i++) { char n[32]; snprintf(n, sizeof n, "empty_%s", EN[i]); L(n, emptySlotTex[i]); } }
    }   // botoes do menu de pausa (padrao do jogo, integrado)
    { int k = loadPackGUI(); if (k) logInfo("GUI do pack: %d texturas (botoes, hotbar, inventario, bancada, criativo)", k); }
    { static const char* EN[5] = {"helmet", "chestplate", "leggings", "boots", "shield"};
      for (int i = 0; i < 5; i++) { char pth[128]; snprintf(pth, sizeof pth, "assets/textures/gui/empty_%s.png", EN[i]); loadTexturePNG(pth, emptySlotTex[i]); }
      if (gPack.ok) for (int i = 0; i < 5; i++) {   // textura PADRAO do Minecraft (1.12: items/empty_armor_slot_*; 1.13+: item/empty_slot_*) tem prioridade sobre a do jogo
          char rel[96]; GLuint t = 0; const char* nm = EN[i];
          snprintf(rel, sizeof rel, "items/empty_armor_slot_%s.png", nm); bool ok = packGuiTex(rel, 0, 0, 0, 0, t, true);
          if (!ok) { snprintf(rel, sizeof rel, "item/empty_armor_slot_%s.png", nm); ok = packGuiTex(rel, 0, 0, 0, 0, t, true); }
          if (!ok) { snprintf(rel, sizeof rel, "item/empty_slot_%s.png", i == 4 ? "shield" : nm); ok = packGuiTex(rel, 0, 0, 0, 0, t, true); }
          if (ok) emptySlotTex[i] = t; } }
    // O inventário possui seus próprios arquivos para poder ser redesenhado
    // sem alterar o HUD/hotbar.
    loadTexturePNG("assets/textures/inventory/slot.png", invSlotTex);
    loadTexturePNG("assets/textures/inventory/slot_selected.png", invSelectedTex);
    loadTexturePNG("assets/textures/inventory/slot_hover.png", invHoverTex);
    loadTexturePNG("assets/textures/inventory/panel.png", invPanelTex);
    loadTexturePNG("assets/textures/inventory/dim.png", invDimTex);
    loadTexturePNG("assets/textures/inventory/header.png", invHeaderTex);
    loadTexturePNG("assets/textures/inventory/tooltip.png", invTooltipTex);
}

static bool loadSpriteTile(const char* path, int tile) {
    std::vector<uint8_t> rgba; int w = 0, h = 0;
    if (!loadPNG(path, rgba, w, h) || w != 16 || h != 16) return false;
    int tx = (tile % ATL) * 16, ty = (tile / ATL) * 16;
    for (int y = 0; y < 16; ++y)
        memcpy(&atlas[((ty + y) * ATW + tx) * 4], &rgba[(size_t)y * 16 * 4], 16 * 4);
    return true;
}

static int itemTile(int id) { return isProp(id) ? 48 /* provisorio: graveto; icone do atlas proprio vem no Prop.2 */ : isPk(id) ? PK[id - PK0].tile : id < IT_END ? 48 + (id - IT_STICK) : 120 + (id - IT_GOLD); }

// ---- ponte com resource packs do Minecraft (ver mcpack.inc) ----
static void putTile16(int tile, const uint8_t* px) {
    if (tile < 0 || tile >= ATL * ATLR) return;
    int tx = (tile % ATL) * 16, ty = (tile / ATL) * 16;
    for (int y = 0; y < 16; ++y) memcpy(&atlas[((ty + y) * ATW + tx) * 4], px + y * 16 * 4, 16 * 4);
}
static bool openActivePack() {   // assets/resourcepacks/pack.cfg: "pack=Nome.zip" (ou pasta). Sem cfg: primeiro .zip/pasta que existir. Sem pack = texturas "ausentes" (xadrez), nao ha placeholder.
    const std::string root = "assets/resourcepacks/"; std::string pick;
    char line[256];
    if (FILE* f = fopen((root + "pack.cfg").c_str(), "r")) {
        while (fgets(line, sizeof line, f)) { char v[200]; if (sscanf(line, " pack = %199[^\r\n]", v) == 1) pick = v; }
        fclose(f);
    }
    std::error_code ec;
    if (!pick.empty() && !std::filesystem::exists(root + pick)) pick.clear();   /* patchRP.1 */
    if (pick.empty()) { std::vector<std::string> nm; for (auto& e : std::filesystem::directory_iterator(root, ec)) { const std::string n = e.path().filename().string(), x = n.size() > 4 ? n.substr(n.size() - 4) : ""; if (e.is_directory() || x == ".zip" || x == ".ZIP") nm.push_back(n); } std::sort(nm.begin(), nm.end()); if (!nm.empty()) pick = nm[0]; }
    if (false) for (auto& e : std::filesystem::directory_iterator(root, ec)) {
        std::string n = e.path().filename().string(), x = n.size() > 4 ? n.substr(n.size() - 4) : "";
        if (e.is_directory() || x == ".zip" || x == ".ZIP") { pick = n; break; }
    }
    { auto o = packNames(root, ""); if (!o.empty()) pick = o[0]; }   /* patchRP.3 */
    if (pick.empty() || !gPack.open(root + pick)) { if (!pick.empty()) fprintf(stderr, "resource pack invalido: %s\n", pick.c_str()); return false; }
    gTcKey = "v2|" + ([=] { std::string k = tcStat(root + pick); for (const auto& n : packNames(root, pick)) if (n != pick) k += "+" + tcStat(root + n); return k; }()) + "|" + tcStat("assets/chisel/chisel.cfg"); tcLoad();
    packLoadSnd(root, pick); printf("Resource pack ativo: %s\n", pick.c_str()); return true;
}
// ---- TODOS os blocos e itens do pack como itens do jogo ----
#include "pk112.inc"
static std::string pkTitle(const std::string& id) {   // "oak_planks" -> "Oak Planks"
    std::string o; bool up = true;
    for (size_t i = 0; i < id.size(); ++i) { const char c = id[i];
        if (c == '_') { o += ' '; up = true; continue; }
        if (isdigit((unsigned char)c) && i && !isdigit((unsigned char)id[i - 1]) && id[i - 1] != '_') o += ' ';   // "podzol2" -> "Podzol 2"
        o += up ? (char)toupper((unsigned char)c) : c; up = false; }
    return o;
}
static bool pkIsCube(const std::string& n, const uint8_t* px) {   // textura de bloco 100% opaca e que nao e so uma face/quadro solto -> icone em cubo
    bool opq = true; for (int i = 0; i < 256; ++i) if (px[i * 4 + 3] < 255) { opq = false; break; }
    const std::string n12 = pk112(n); const bool lv = n12.size() > 7 && n12.compare(n12.size() - 7, 7, "_leaves") == 0;   // patchV.8: pack 1.12 chama leaves_spruce (nome antigo) -> normaliza   // folhas tem furos de alpha mas sao bloco (folhagem)
    if (!opq && !lv && !((n.find("glass") != std::string::npos && n.find("pane") == std::string::npos && n.find("top") == std::string::npos) || n == "ice" || n12 == "slime_block"))   /* patchIcon.1: pack 1.12 chama "slime" */ return false;   // vidro e gelo (translúcidos) também são blocos
    if (isdigit((unsigned char)n.back())) return false;
    static const char* SFX[] = {"_top", "_bottom", "_side", "_front", "_back", "_end", "_inner", "_outer", "_overlay", "_particle", "_stage", "_side1", "_side2"};
    for (const char* x : SFX) { const size_t l = strlen(x); if (n.size() > l && n.compare(n.size() - l, l, x) == 0) return false; }
    return true;
}
static void potionRegister(int cap);   // potions.inc
static void headRegister(int cap);   // heads.inc
static void entLoadAll();   // entmodels.inc (incluido por redstone.inc)
// ---- Chisel 1.12.2: assets/chisel/chisel.cfg (tools/chisel_import.py). Blocos cubicos como itens do pack; tiles 16x16 deduplicados ----
static std::vector<std::pair<int, int>> gChiselLit, gChiselTr;   // (indice em PK, luz) / (indice, 1 = translucido c/ blend, 2 = alpha-test)
static void loadChisel() {
    gChiselItems = gChiselTiles = 0; gChiselLit.clear(); gChiselTr.clear();
#ifdef BL_RPI
    if (getenv("BLOCKLAND_NOCHISEL")) return;   // patchRpiOpt.7: Chisel agora aparece no Pi (BLOCKLAND_NOCHISEL=1 desliga: muita RAM e +21 s de carga)
    //   // patchRpi.1: sem Chisel no Raspberry (2042 blocos/1998 tiles = 21 s de carga e ~100 MB do atlas, com 869 MB de RAM no total)
#endif
    FILE* f = fopen("assets/chisel/chisel.cfg", "r"); if (!f) return;
    std::unordered_map<std::string, int> slot, acls; char line[700];   // acls: tile -> 0 opaco, 1 semi, 2 furos
    const int base = (int)PK.size(), capT = ATL * ATLR0 - PKTILE0 - 400 - base;   // 400 tiles de folga p/ entidades/modelos
    struct PreT { uint8_t d[1024]; bool ok = false; };
    std::unordered_map<std::string, PreT> pre;
    {   // pre-passada: nomes unicos de tiles -> decodifica em ate 4 threads (antes: 1998 PNGs em serie, ~12 s)
        std::vector<std::string> names; std::unordered_map<std::string, int> seen; char l2[700];
        while (fgets(l2, sizeof l2, f)) { if (l2[0] == '#') continue; int col = 0; const char* p = l2, *st = l2;
            for (;; ++p) { if (*p == '\t' || *p == '\n' || *p == '\r' || !*p) { if (col >= 2 && col <= 4 && p > st) { std::string h(st, p); if (!seen.count(h)) { seen[h] = 1; names.push_back(h); } } col++; st = p + 1; if (col > 4 || !*p) break; } } }
        rewind(f);
        for (const std::string& h : names) pre[h];   // reserva (sem rehash durante as threads)
        std::vector<PreT*> slots; for (const std::string& h : names) slots.push_back(&pre[h]);
        std::vector<uint8_t> hitC(names.size(), 0);
        for (size_t q = 0; q < names.size(); q++) if (auto* ce = tcGet("c/" + names[q])) { memcpy(slots[q]->d, ce->data(), 1024); slots[q]->ok = (*ce)[1024] != 0; hitC[q] = 1; }   // patchFast.3
        std::atomic<size_t> nx{0}; const unsigned nth = std::max(1u, std::min(4u, std::thread::hardware_concurrency()));
        auto work = [&]() { tlPngWorker = true; for (size_t q; (q = nx.fetch_add(1)) < names.size();) { if (hitC[q]) continue; std::vector<uint8_t> px; int w = 0, hh = 0;
            if (loadPNG(("assets/chisel/tiles/" + names[q] + ".png").c_str(), px, w, hh) && w == 16 && hh == 16) { memcpy(slots[q]->d, px.data(), 1024); slots[q]->ok = true; } } };
        std::vector<std::thread> th; for (unsigned t = 1; t < nth; t++) th.emplace_back(work);
        work(); tlPngWorker = false; for (auto& t : th) t.join();
        for (size_t q = 0; q < names.size(); q++) if (!hitC[q]) tcPut("c/" + names[q], slots[q]->ok, slots[q]->d);
    }
    int nt = 0, ni = 0, drop = 0;
    auto getTile = [&](const std::string& h) -> int {
        auto it = slot.find(h); if (it != slot.end()) return it->second;
        if (nt >= capT) return -1;
        std::vector<uint8_t> px; int w = 0, hh = 0;
        auto pc = pre.find(h);   // patchSeam.2: PNGs ja decodificados em paralelo
        if (pc != pre.end()) { if (!pc->second.ok) return -1; px.assign(pc->second.d, pc->second.d + 1024); }
        else if (!loadPNG(("assets/chisel/tiles/" + h + ".png").c_str(), px, w, hh) || w != 16 || hh != 16) return -1;
        const int t = PKTILE0 + base + nt++; putTile16(t, px.data()); slot[h] = t;
        int a = 0; for (int i = 0; i < 256; i++) { const uint8_t al = px[i * 4 + 3]; if (al == 0) a = std::max(a, 2); else if (al < 255) { a = 1; break; } }
        acls[h] = a; return t; };
    for (int k = 0; k < 2; k++) {   // cinzeis (itens, nao colocaveis)
        std::vector<uint8_t> px; int w = 0, hh = 0; const char* fn = k ? "chisel_diamond" : "chisel_iron";
        if (nt >= capT || !loadPNG((std::string("assets/chisel/items/") + fn + ".png").c_str(), px, w, hh) || w != 16 || hh != 16) continue;
        const int t = PKTILE0 + base + nt++; putTile16(t, px.data());
        PkItem it; it.file = std::string("chisel/") + fn; it.name = k ? "Cinzel de Diamante" : "Cinzel de Ferro"; it.cube = false; it.tile = t; it.stack = 1; PK.push_back(std::move(it)); ni++;
    }
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#') continue;
        std::vector<std::string> c; std::string cur; for (const char* p = line; *p; ++p) { if (*p == '\t' || *p == '\n' || *p == '\r') { c.push_back(cur); cur.clear(); } else cur += *p; } if (!cur.empty()) c.push_back(cur);
        if (c.size() < 6) continue;
        const int t0 = getTile(c[2]), ts = getTile(c[3]), tb = getTile(c[4]);
        if (t0 < 0 || ts < 0 || tb < 0) { drop++; continue; }
        PkItem it; it.file = c[0]; it.name = c[1]; it.cube = true; it.tile = t0; it.ft[0] = t0; it.ft[1] = ts; it.ft[2] = tb;
        const int lt = atoi(c[5].c_str()); if (lt > 0) gChiselLit.push_back({(int)PK.size(), lt});
        { int a = std::max(acls[c[2]], std::max(acls[c[3]], acls[c[4]])); if (a) gChiselTr.push_back({(int)PK.size(), a}); }
        PK.push_back(std::move(it)); ni++;
    }
    fclose(f); gChiselItems = ni; gChiselTiles = nt;
    printf("Chisel: %d blocos, %d tiles%s\n", ni, nt, drop ? " (alguns sem espaco no atlas)" : "");
}
static void chiselApplyLight() {
    gChiselIron = gChiselDia = 0;
    for (int i = (int)PK.size() - gChiselItems; i < (int)PK.size(); i++) { if (PK[i].file == "chisel/chisel_iron") gChiselIron = PK0 + i; else if (PK[i].file == "chisel/chisel_diamond") gChiselDia = PK0 + i; gPkByFile[PK[i].file] = PK0 + i; gPkByHash[netFnvPk(PK[i].file)] = PK0 + i; }   // salvar/carregar e rede por nome
    for (auto& pr : gChiselTr) if (pr.first < (int)gPkTrans.size()) { gPkTrans[pr.first] = (uint8_t)pr.second; if (PK[pr.first].file.rfind("chisel/glass", 0) == 0 && pr.first < (int)gPkOpac.size()) gPkOpac[pr.first] = 0; }
    for (auto& pr : gChiselLit) if (pr.first < (int)gPkEmit.size()) gPkEmit[pr.first] = (uint8_t)std::min(15, pr.second); }
static void loadPackItems() {
    PK.clear(); if (!gPack.ok) { loadChisel(); entLoadAll(); if (!PK.empty()) { pkBuildProps(); chiselApplyLight(); } return; }
    const std::string T = "assets/minecraft/textures/"; const int cap = ATL * ATLR0 - PKTILE0;
    std::unordered_set<std::string> blockNames; uint8_t px[1024];
    for (int kind = 0; kind < 2; ++kind) {
        const char* k = kind ? "item" : "block";
        const std::vector<std::string> allN = gPack.listPng(T + k + "/"); const std::unordered_set<std::string> haveN(allN.begin(), allN.end());
        // patchRpiOpt.7: fase 1 (serial) filtra os nomes; fase 2 decodifica as texturas em paralelo (PNG/inflate sao puros); fase 3 (serial) registra na MESMA ordem de antes
        std::vector<std::string> cand;
        for (const std::string& n : allN) {
            {   // variantes numeradas (crafting_table_front1, end_stone2, compass_05...) so existem como textura alternativa do bloco/quadro: nao viram item
                size_t e = n.size(); while (e > 0 && isdigit((unsigned char)n[e - 1])) e--;
                if (e > 0 && n.size() - e == 2 && n.substr(e) != "00" && n[e - 1] == '_' && haveN.count(n.substr(0, e) + "00")) continue;   // quadros 01..NN de animacao (compass, clock...) nao sao itens
                if (e > 0 && e < n.size()) { std::string base = n.substr(0, e); if (!base.empty() && base.back() == '_') base.pop_back(); if (!base.empty() && haveN.count(base)) continue; }
            }
            if (gPackUsed.count(std::string(k) + "/" + n)) continue;
            cand.push_back(n);
        }
        std::vector<std::array<uint8_t, 1024>> tiles(cand.size()); std::vector<uint8_t> okT(cand.size(), 0), hitT(cand.size(), 0);
        for (size_t q = 0; q < cand.size(); q++) if (auto* ce = tcGet(std::string(k) + "/" + cand[q])) { memcpy(tiles[q].data(), ce->data(), 1024); okT[q] = (*ce)[1024]; hitT[q] = 1; }   // patchFast.3: tiles ja em cache
        {
            std::atomic<size_t> nx{0}; const unsigned nth = std::max(1u, std::min(4u, std::thread::hardware_concurrency()));
            auto work = [&]() { tlPngWorker = true; for (size_t q; (q = nx.fetch_add(1)) < cand.size();) if (!hitT[q]) okT[q] = packTileRel(std::string(k) + "/" + cand[q] + ".png", tiles[q].data()) ? 1 : 0; };
            std::vector<std::thread> th; for (unsigned t = 1; t < nth; t++) th.emplace_back(work);
            work(); tlPngWorker = false; for (auto& t : th) t.join();
            for (size_t q = 0; q < cand.size(); q++) if (!hitT[q]) tcPut(std::string(k) + "/" + cand[q], okT[q] != 0, tiles[q].data());   // a thread principal tambem trabalha (tlPngWorker volta a false: splashPoll normal depois)
        }
        for (size_t q = 0; q < cand.size(); q++) {
            if ((int)PK.size() >= cap) break;
            if (!okT[q]) continue; const std::string& n = cand[q]; memcpy(px, tiles[q].data(), 1024);
            bool any = false; for (int i = 0; i < 256 && !any; ++i) any = px[i * 4 + 3] != 0;
            if (!any) continue;   // textura totalmente transparente (nao serve como item)
            PkItem it; it.file = std::string(k) + "/" + n; it.name = pkTitle(pk112(n)); it.cube = kind == 0 && pkIsCube(n, px); it.tile = PKTILE0 + (int)PK.size();
            if (kind == 0) blockNames.insert(it.name); else if (blockNames.count(it.name)) it.name += " (item)";   // mesmo nome em block/ e item/
            putTile16(PKTILE0 + (int)PK.size(), px); PK.push_back(std::move(it));
        }
    }
    {   // patchInv.5: ovos de spawn (o pack 1.12 so traz spawn_egg + overlay): um item por mob, textura gerada (base + manchas tingidas)
        static const char* EG[] = {"bat","blaze","cave_spider","creeper","donkey","elder_guardian","enderman","endermite","evoker","ghast","guardian","horse","husk","llama","magma_cube","mooshroom","mule","ocelot","parrot","polar_bear","rabbit","shulker","silverfish","skeleton","skeleton_horse","slime","spider","squid","stray","vex","villager","vindicator","witch","wither_skeleton","wolf","zombie","zombie_horse","zombie_villager","zombified_piglin"};
        for (const char* m : EG) { if ((int)PK.size() >= cap) break; const std::string n = std::string(m) + "_spawn_egg"; bool have = false; for (const PkItem& q : PK) if (q.file == "item/" + n) { have = true; break; } if (have || !packSpawnEgg("item", n, px)) continue;
            PkItem it; it.file = "item/" + n; it.name = pkTitle(n); it.cube = false; it.tile = PKTILE0 + (int)PK.size(); putTile16(PKTILE0 + (int)PK.size(), px); PK.push_back(std::move(it)); }
    }
    potionRegister(cap);   // patchBrew.1: pocoes (garrafa + overlay tingido)
    headRegister(cap);   // patchHead.1: cabecas de mob
    loadChisel();
    entLoadAll();   // texturas de entidade (bau, bau do Ender, cama) em blocos 4x4 de tiles logo depois dos itens
    pkBuildProps(); chiselApplyLight(); tcSave();
    printf("Resource pack: %d blocos/itens extras registrados (ids %d..%d)\n", (int)PK.size(), PK0, PK0 + (int)PK.size() - 1);
}
static void applyMcPack() {
    // tudo comeca como textura "ausente"; o que o pack tiver sobrescreve (nao existe mais sprite/atlas placeholder)
    for (int id = 1; id < NB; ++id) for (int f = 0; f < 3; ++f) fillMissing(TILES[id][f]);
    for (int f = 0; f < 3; ++f) fillMissing(TILES[CTABLE][f]);
    for (int id = IT_STICK; id < IT_END; ++id) fillMissing(itemTile(id));
    for (int id = IT_GOLD; id < IT_ALLEND; ++id) fillMissing(itemTile(id));
    for (int id = IT_PORK; id <= IT_EGG_SHEEP; ++id) fillMissing(itemTile(id));
    if (!openActivePack()) { fprintf(stderr, "ERRO: nenhum resource pack em assets/resourcepacks/ (padrao: Ashen_16x.zip); texturas ausentes\n"); return; }
    uint8_t px[1024]; int nb = 0, ni = 0;
    struct B { int id; const char *top, *side, *bot; };
    static const B BL[] = {
        {GRASS, "grass_block_top|grass_top", "grass_block_side|grass_side", "dirt"}, {DIRT, "dirt", 0, 0}, {STONE, "stone", 0, 0}, {SAND, "sand", 0, 0},
        {LOG, "oak_log_top|log_oak_top", "oak_log|log_oak", "oak_log_top|log_oak_top"}, {LEAVES, "oak_leaves|leaves_oak", 0, 0}, {PLANKS, "oak_planks|planks_oak", 0, 0},
        {COBBLE, "cobblestone", 0, 0}, {GRAVEL, "gravel", 0, 0}, {BRICKS, "bricks|brick", 0, 0}, {BEDROCK, "bedrock", 0, 0},
        {COALORE, "coal_ore", 0, 0}, {DIAMONDORE, "diamond_ore", 0, 0}, {GOLDORE, "gold_ore", 0, 0}, {IRONORE, "iron_ore", 0, 0},
        {GOLDBLK, "gold_block", 0, 0}, {IRONBLK, "iron_block", 0, 0}, {DIAMBLK, "diamond_block", 0, 0}, {COALBLK, "coal_block", 0, 0},
        {SANDSTONE, "sandstone_top", "sandstone|sandstone_normal", "sandstone_bottom"}, {STONEBRICK, "stone_bricks|stonebrick", 0, 0},
        {MOSSY, "mossy_cobblestone|cobblestone_mossy", 0, 0}, {CLAY, "clay", 0, 0}, {SNOWBLK, "snow", 0, 0}, {OBSIDIAN, "obsidian", 0, 0},
        {GLOWSTONE, "glowstone", 0, 0}, {BOOKSHELF, "oak_planks|planks_oak", "bookshelf", "oak_planks|planks_oak"}, {WOOL, "white_wool|wool_colored_white", 0, 0},
        {REDSTONEORE, "redstone_ore", 0, 0}, {LAPISORE, "lapis_ore", 0, 0}, {CTABLE, "crafting_table_top", "crafting_table_side|crafting_table_front", "oak_planks|planks_oak"}};
    for (const B& b : BL) for (int f = 0; f < 3; ++f) {
        const char* n = f == 0 ? b.top : f == 1 ? (b.side ? b.side : b.top) : (b.bot ? b.bot : (b.side ? b.side : b.top));
        if (!packTile16("block", n, px)) continue;
        if (b.id == GRASS && f == 1) {   // pack moderno: lateral = terra + "grass_block_side_overlay" cinza (tingida pelo bioma, como o MC)
            uint8_t ov[1024]; if (packTile16("block", "grass_block_side_overlay", ov)) for (int i = 0; i < 256; ++i) if (ov[i * 4 + 3] > 128) memcpy(px + i * 4, ov + i * 4, 3);
        }
        putTile16(TILES[b.id][f], px); nb++; gCoreTile[gLastPack] = TILES[b.id][f];
        if (b.id == SANDSTONE && f == 1) gCoreTile["block/sandstone"] = TILES[b.id][f];   // 1.12: lado = sandstone_normal
    }
    // agua: a textura do MC e cinza (tingida pelo bioma); aqui recebe o azul padrao e fica translucida
    if (packTile16("block", "water_still|water", px)) {
        for (int i = 0; i < 256; ++i) { uint8_t* q = px + i * 4; int lum = (q[0] * 77 + q[1] * 150 + q[2] * 29) >> 8; q[0] = std::min(255, lum * 60 / 128); q[1] = std::min(255, lum * 110 / 128); q[2] = std::min(255, lum * 220 / 128); q[3] = 190; }
        for (int f = 0; f < 3; ++f) putTile16(TILES[WATER][f], px);
        { packFrames16("blocks/water_still.png", gWaterFrames, gWaterNF); if (!gWaterNF) packFrames16("block/water_still.png", gWaterFrames, gWaterNF);
          for (int k = 0; k < gWaterNF; k++) for (int i = 0; i < 256; ++i) { uint8_t* q = &gWaterFrames[(size_t)k * 1024 + i * 4]; int lum = (q[0] * 77 + q[1] * 150 + q[2] * 29) >> 8; q[0] = std::min(255, lum * 60 / 128); q[1] = std::min(255, lum * 110 / 128); q[2] = std::min(255, lum * 220 / 128); q[3] = 190; } }   // patchOpt.2: mesma tintura, todos os frames
        nb++;
    }
    static const struct { const char* t; int id; } GID[] = {   // textura de ICONE de cada bloco real do jogo (usada pelo criativo 1.12.2)
        {"block/grass_block_top", GRASS}, {"block/dirt", DIRT}, {"block/stone", STONE}, {"block/sand", SAND}, {"block/oak_log", LOG}, {"block/oak_leaves", LEAVES},
        {"block/oak_planks", PLANKS}, {"block/cobblestone", COBBLE}, {"block/gravel", GRAVEL}, {"block/bricks", BRICKS}, {"block/bedrock", BEDROCK}, {"block/coal_ore", COALORE},
        {"block/diamond_ore", DIAMONDORE}, {"block/gold_ore", GOLDORE}, {"block/iron_ore", IRONORE}, {"block/gold_block", GOLDBLK}, {"block/iron_block", IRONBLK},
        {"block/diamond_block", DIAMBLK}, {"block/coal_block", COALBLK}, {"block/sandstone", SANDSTONE}, {"block/stone_bricks", STONEBRICK}, {"block/mossy_cobblestone", MOSSY},
        {"block/clay", CLAY}, {"block/snow", SNOWBLK}, {"block/obsidian", OBSIDIAN}, {"block/glowstone", GLOWSTONE}, {"block/bookshelf", BOOKSHELF}, {"block/white_wool", WOOL},
        {"block/redstone_ore", REDSTONEORE}, {"block/lapis_ore", LAPISORE}, {"block/crafting_table_front", CTABLE}};
    gPackId.clear(); for (const auto& g : GID) gPackId[g.t] = g.id;
    struct I { int id; const char* n; };
    static const I IL[] = {
        {IT_STICK, "stick"}, {IT_COAL, "coal"}, {IT_IRON, "iron_ingot"}, {IT_DIAMOND, "diamond"}, {IT_APPLE, "apple"}, {IT_GOLD, "gold_ingot"}, {IT_REDSTONE, "redstone"},
        {IT_FLINT, "flint"}, {IT_FEATHER, "feather"}, {IT_BONE, "bone"}, {IT_PAPER, "paper"}, {IT_BOOK, "book"}, {IT_STRING, "string"}, {IT_LEATHER, "leather"},
        {IT_LAPIS, "lapis_lazuli|dye_powder_blue"}, {IT_BREAD, "bread"}, {IT_COOKIE, "cookie"}, {IT_BEEF, "beef"}, {IT_STEAK, "cooked_beef|beef_cooked"}, {IT_CARROT, "carrot"},
        {IT_GAPPLE, "golden_apple|apple_golden"},
        {IT_PORK, "porkchop|porkchop_raw"}, {IT_CPORK, "cooked_porkchop|porkchop_cooked"}, {IT_CHICKEN, "chicken|chicken_raw"}, {IT_CCHICKEN, "cooked_chicken|chicken_cooked"},
        {IT_MUTTON, "mutton|mutton_raw"}, {IT_CMUTTON, "cooked_mutton|mutton_cooked"}, {IT_EGG, "egg"}, {IT_EGG_PIG, "pig_spawn_egg"}, {IT_EGG_CHICKEN, "chicken_spawn_egg"},
        {IT_EGG_COW, "cow_spawn_egg"}, {IT_EGG_SHEEP, "sheep_spawn_egg"}, {IT_ARROW, "arrow"}, {IT_BOW, "bow"}};
    for (const I& it : IL) if (packTile16("item", it.n, px)) { putTile16(itemTile(it.id), px); gPackId[gLastPack] = it.id; ni++; }
    static const char* MA[5] = {"wooden", "stone", "iron", "golden", "diamond"}; static const char* MB[5] = {"wood", "stone", "iron", "gold", "diamond"};
    static const char* KD[5] = {"pickaxe", "sword", "axe", "shovel", "hoe"};
    auto tool = [&](int id, int k, int m) { char n[96]; snprintf(n, sizeof n, "%s_%s|%s_%s", MA[m], KD[k], MB[m], KD[k]); if (packTile16("item", n, px)) { putTile16(itemTile(id), px); gPackId[gLastPack] = id; ni++; } };
    tool(IT_WPICK, 0, 0); tool(IT_SPICK, 0, 1); tool(IT_WSWORD, 1, 0);
    tool(IT_IPICK, 0, 2); tool(IT_GPICK, 0, 3); tool(IT_DPICK, 0, 4);
    for (int m = 1; m < 5; ++m) tool(IT_SSWORD + m - 1, 1, m);
    for (int m = 0; m < 5; ++m) { tool(IT_WAXE + m, 2, m); tool(IT_WSHOVEL + m, 3, m); tool(IT_WHOE + m, 4, m); }
    static const char* PA[4] = {"helmet", "chestplate", "leggings", "boots"}; static const char* AM[4] = {"leather", "iron", "golden|gold", "diamond"};
    for (int m = 0; m < 4; ++m) for (int a = 0; a < 4; ++a) {
        std::string n; std::string mm = AM[m]; size_t bar = mm.find('|');
        if (bar == std::string::npos) n = mm + "_" + PA[a]; else n = mm.substr(0, bar) + "_" + PA[a] + "|" + mm.substr(bar + 1) + "_" + PA[a];
        if (!packTile16("item", n.c_str(), px)) continue;
        const std::string nm0 = gLastPack;
        if (m == 0) {   // couro: packs modernos trazem o icone cinza (o jogo pinta); aplica a cor padrao 0xA06540 e o overlay sem tinta
            long sat = 0, cnt = 0; for (int i = 0; i < 256; ++i) if (px[i * 4 + 3]) { sat += std::max({px[i*4], px[i*4+1], px[i*4+2]}) - std::min({px[i*4], px[i*4+1], px[i*4+2]}); cnt++; }
            if (cnt && sat / cnt < 12) {
                static const int LC[3] = {160, 101, 64}; for (int i = 0; i < 256; ++i) for (int q = 0; q < 3; ++q) px[i * 4 + q] = px[i * 4 + q] * LC[q] / 255;
                uint8_t ov[1024]; std::string on = std::string("leather_") + PA[a] + "_overlay";
                if (packTile16("item", on.c_str(), ov)) for (int i = 0; i < 256; ++i) if (ov[i * 4 + 3]) { float al = ov[i * 4 + 3] / 255.f; for (int q = 0; q < 3; ++q) px[i * 4 + q] = (uint8_t)(ov[i * 4 + q] * al + px[i * 4 + q] * (1 - al)); px[i * 4 + 3] = std::max(px[i * 4 + 3], ov[i * 4 + 3]); }
            }
        }
        putTile16(itemTile(IT_A0 + m * 4 + a), px); gPackId[nm0] = IT_A0 + m * 4 + a; ni++;
    }
    printf("Resource pack: %d texturas de bloco e %d de item aplicadas\n", nb, ni);
    loadPackItems();
}
