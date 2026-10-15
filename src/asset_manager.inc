// ================= assets separados =================
// Os arquivos PNG separados sao empacotados em um atlas somente em memoria.
// Isso mantem o renderer OpenGL 1.x simples, mas deixa cada textura editavel.
static int itemTile(int id);
static GLuint guiSlotTex = 0, guiSelectedTex = 0, guiHoverTex = 0, guiPanelTex = 0, guiCrosshairTex = 0, guiDimTex = 0;
static GLuint invSlotTex = 0, invSelectedTex = 0, invHoverTex = 0, invPanelTex = 0, invDimTex = 0, invHeaderTex = 0, invTooltipTex = 0;
static float guiScaleCfg = 0.f;   // 0 = automatico (como o Java)
static float guiSlotSize = 44.f, guiGap = 4.f, guiHotbarBottom = 12.f, guiPanelPad = 10.f, guiAlpha = .97f, guiDimAlpha = .45f;
static float inventorySlotMax = 52.f, inventoryTopMargin = 70.f, inventoryPanelAlpha = .97f, inventoryHeaderAlpha = .96f, inventoryTooltipAlpha = .90f;

// ---- Rethoughted GUI (assets/textures/gui/rt/): base visual de toda a interface. Se um arquivo faltar, cai no desenho em codigo ----
static GLuint rtIcons = 0;   // gui/icons.png do pack (coracao, coxa, bolha, barra de XP)
static GLuint rtBtn[3] = {0, 0, 0}, rtHotbar = 0, rtHotSel = 0, rtCross = 0, rtInv = 0, rtCraft = 0, rtFurn = 0, rtCrItems = 0, rtCrInv = 0, rtCrSearch = 0, rtSlot = 0, rtScr[2] = {0, 0}, rtTipBg = 0, rtTipFr = 0;
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
static int loadPackGUI() {   // devolve quantas texturas vieram do pack
    if (!guiFromPack || !gPack.ok) return 0; int n = 0;
    const std::string G = "gui/", C = "gui/container/";
    auto W = [&](int y, GLuint& t) { n += packGuiTex((G + "widgets.png").c_str(), 0, y, 200, 20, t); };
    W(66, rtBtn[0]); W(86, rtBtn[1]); W(46, rtBtn[2]);   // 1.12: normal y=66, destacado y=86, desativado y=46
    n += packGuiTex((G + "widgets.png").c_str(), 0, 0, 182, 22, rtHotbar);
    n += packGuiTex((G + "widgets.png").c_str(), 0, 22, 24, 23, rtHotSel);
    n += packGuiTex((C + "inventory.png").c_str(), 0, 0, 0, 0, rtInv, true);
    n += packGuiTex((C + "crafting_table.png").c_str(), 0, 0, 0, 0, rtCraft, true);
    n += packGuiTex((C + "furnace.png").c_str(), 0, 0, 0, 0, rtFurn, true);   // patchFur.4: fundo + chama (176,0 14x14) + seta (176,14 24x17) da fornalha do pack
    n += packGuiTex((C + "creative_inventory/tab_items.png").c_str(), 0, 0, 0, 0, rtCrItems, true);
    n += packGuiTex((C + "creative_inventory/tab_inventory.png").c_str(), 0, 0, 0, 0, rtCrInv, true);
    n += packGuiTex((C + "creative_inventory/tab_item_search.png").c_str(), 0, 0, 0, 0, rtCrSearch, true);
    n += packGuiTex((C + "creative_inventory/tabs.png").c_str(), 232, 0, 12, 15, rtScr[1]);
    n += packGuiTex((C + "creative_inventory/tabs.png").c_str(), 244, 0, 12, 15, rtScr[0]);
    for (int k = 0; k < 2; k++) for (int sl = 0; sl < 2; sl++) for (int c = 0; c < 7; c++) {   // abas do criativo: tabs.png, celulas 28x32 (topo y=0/32, base y=64/96)
        const int y = (k ? 64 : 0) + (sl ? 32 : 0); if (packGuiTex((C + "creative_inventory/tabs.png").c_str(), c * 28, y, 28, 32, rtTab[k][sl][c])) { n++; gTabW = 28; } }
    n += packGuiTex((C + "inventory.png").c_str(), 7, 141, 18, 18, rtSlot);   // moldura de slot 18x18 (hotbar da inventory.png): troca o slot escuro do Rethoughted (bau, lixeira, criativo)
    n += packGuiTex((G + "icons.png").c_str(), 0, 0, 0, 0, rtIcons, true);   // patchHud.1
    gGuiLight = rtInv != 0 && n > 0;
    n += packGuiTex((G + "widgets.png").c_str(), 0, 0, 0, 0, guiWidgetsTex, true);   // menu de pausa/reserva usa o widgets.png inteiro
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
        L("hotbar", rtHotbar); L("hotbar_selection", rtHotSel); L("crosshair", rtCross);
        L("inventory", rtInv); L("crafting_table", rtCraft); L("creative_items", rtCrItems); L("creative_inventory", rtCrInv); L("creative_search", rtCrSearch);
        L("slot", rtSlot); L("scroller", rtScr[1]); L("scroller_disabled", rtScr[0]); L("tooltip_background", rtTipBg); L("tooltip_frame", rtTipFr);
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

static int itemTile(int id) { return isPk(id) ? PK[id - PK0].tile : id < IT_END ? 48 + (id - IT_STICK) : 120 + (id - IT_GOLD); }

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
    if (pick.empty()) for (auto& e : std::filesystem::directory_iterator(root, ec)) {
        std::string n = e.path().filename().string(), x = n.size() > 4 ? n.substr(n.size() - 4) : "";
        if (e.is_directory() || x == ".zip" || x == ".ZIP") { pick = n; break; }
    }
    if (pick.empty() || !gPack.open(root + pick)) { if (!pick.empty()) fprintf(stderr, "resource pack invalido: %s\n", pick.c_str()); return false; }
    printf("Resource pack ativo: %s\n", pick.c_str()); return true;
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
    if (!opq && !lv && !((n.find("glass") != std::string::npos && n.find("pane") == std::string::npos && n.find("top") == std::string::npos) || n == "ice" || n == "slime_block")) return false;   // vidro e gelo (translúcidos) também são blocos
    if (isdigit((unsigned char)n.back())) return false;
    static const char* SFX[] = {"_top", "_bottom", "_side", "_front", "_back", "_end", "_inner", "_outer", "_overlay", "_particle", "_stage", "_side1", "_side2"};
    for (const char* x : SFX) { const size_t l = strlen(x); if (n.size() > l && n.compare(n.size() - l, l, x) == 0) return false; }
    return true;
}
static void entLoadAll();   // entmodels.inc (incluido por redstone.inc)
// ---- Chisel 1.12.2: assets/chisel/chisel.cfg (tools/chisel_import.py). Blocos cubicos como itens do pack; tiles 16x16 deduplicados ----
static std::vector<std::pair<int, int>> gChiselLit, gChiselTr;   // (indice em PK, luz) / (indice, 1 = translucido c/ blend, 2 = alpha-test)
static void loadChisel() {
    gChiselItems = gChiselTiles = 0; gChiselLit.clear(); gChiselTr.clear();
    FILE* f = fopen("assets/chisel/chisel.cfg", "r"); if (!f) return;
    std::unordered_map<std::string, int> slot, acls; char line[700];   // acls: tile -> 0 opaco, 1 semi, 2 furos
    const int base = (int)PK.size(), capT = ATL * ATLR0 - PKTILE0 - 400 - base;   // 400 tiles de folga p/ entidades/modelos
    int nt = 0, ni = 0, drop = 0;
    auto getTile = [&](const std::string& h) -> int {
        auto it = slot.find(h); if (it != slot.end()) return it->second;
        if (nt >= capT) return -1;
        std::vector<uint8_t> px; int w = 0, hh = 0;
        if (!loadPNG(("assets/chisel/tiles/" + h + ".png").c_str(), px, w, hh) || w != 16 || hh != 16) return -1;
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
        for (const std::string& n : allN) {
            if ((int)PK.size() >= cap) break;
            {   // variantes numeradas (crafting_table_front1, end_stone2, compass_05...) so existem como textura alternativa do bloco/quadro: nao viram item (apareciam como "mesa de trabalho" repetida)
                size_t e = n.size(); while (e > 0 && isdigit((unsigned char)n[e - 1])) e--;
                if (e > 0 && n.size() - e == 2 && n.substr(e) != "00" && n[e - 1] == '_' && haveN.count(n.substr(0, e) + "00")) continue;   // quadros 01..NN de animacao (compass, clock...) nao sao itens
                if (e > 0 && e < n.size()) { std::string base = n.substr(0, e); if (!base.empty() && base.back() == '_') base.pop_back(); if (!base.empty() && haveN.count(base)) continue; }
            }
            if (gPackUsed.count(std::string(k) + "/" + n) || !packTileRel(std::string(k) + "/" + n + ".png", px)) continue;
            bool any = false; for (int i = 0; i < 256 && !any; ++i) any = px[i * 4 + 3] != 0;
            if (!any) continue;   // textura totalmente transparente (nao serve como item)
            PkItem it; it.file = std::string(k) + "/" + n; it.name = pkTitle(pk112(n)); it.cube = kind == 0 && pkIsCube(n, px); it.tile = PKTILE0 + (int)PK.size();
            if (kind == 0) blockNames.insert(it.name); else if (blockNames.count(it.name)) it.name += " (item)";   // mesmo nome em block/ e item/
            putTile16(PKTILE0 + (int)PK.size(), px); PK.push_back(std::move(it));
        }
    }
    loadChisel();
    entLoadAll();   // texturas de entidade (bau, bau do Ender, cama) em blocos 4x4 de tiles logo depois dos itens
    pkBuildProps(); chiselApplyLight();
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
