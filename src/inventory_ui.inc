// ================= inventário (reimplementação do Minecraft Java 1.8.9) =================
// - GUI em "unidades" de 1 pixel do Java, ampliadas por uma ESCALA INTEIRA (gs), igual ao Java (auto scale).
//   Tudo (painel, slots, ícones 16x16, texto) fica alinhado a pixels => pixel arts sempre iguais, sem borrar.
// - Slots: 0-8 hotbar | 9-35 mochila | 100+k itens do criativo | 200-203 armadura | 210-213 crafting 2x2 | 220 resultado
// - Cliques: pegar/soltar (esq/dir), shift-click (mesmos destinos do Java), tecla 1-9, arrastar para dividir
//   (esq = divide igualmente, dir = 1 por slot), duplo clique (juntar tudo), botão do meio no criativo.
struct Slot { int id = 0, n = 0, d = 0; std::string nbt; };   // d = dano (durabilidade gasta; usado pela elitra); nbt = dados extras da pilha (BlockEntityTag de bau com conteudo; "" = nenhum)
static inline bool sameStack(const Slot& a, const Slot& b) { return a.id == b.id && a.nbt == b.nbt; }   // so empilha se o item E o NBT forem iguais (como no Java)
static std::string foodTip(int id);   // survival.inc
static int gHeldSel = 0;      // slot da hotbar selecionado (mostrado na mão do boneco)
static Slot inv[36], cur;      // 0-8 = hotbar, 9-35 = mochila; cur = item preso ao mouse
static int gGm = 1; static bool gHard = false;   // modo: 0 sobrevivência, 1 criativo, 2 aventura, 3 espectador; hardcore = sobrevivência com morte definitiva
static bool creative = true;   // true em criativo e espectador (voo, sem dano, sem consumir itens)
// GUI do bau (ContainerChest do Java): 27 slots (simples) ou 54 (duplo). Conteudo em chest.inc; aqui so o estado e os slots SL_CHEST+k.
static bool gChest = false, gChWait = false, gChEnder = false; static int gChRows = 3;   // gChWait: convidado esperando o host mandar o conteudo
static const int SL_CHEST = 300;
static Slot* chestSlotRef(int k); static void chestClosed();   // chest.inc
static bool gTable = false;   // GUI da mesa de trabalho aberta (grade 3x3); no criativo também mostra o painel de sobrevivência, como no Java
// ---- nomes ----
static std::string NAMES[256];
static std::string prettyName(const std::string& up) {   // "MINERIO DE CARVAO" -> "Minério de carvão"
    static const std::unordered_map<std::string, std::string> M = {{"maca","maçã"},{"tabuas","tábuas"},{"minerio","minério"},{"carvao","carvão"},{"agua","água"},{"la","lã"},{"lapis","lápis"},{"silex","sílex"},{"pao","pão"},{"calcas","calças"},{"pa","pá"}};
    std::string out, w; const std::string s = up + " ";
    for (char c : s) { if (c == ' ') { std::string l; for (char x : w) l += (char)tolower((unsigned char)x); auto it = M.find(l); if (!out.empty()) out += ' '; out += it != M.end() ? it->second : l; w.clear(); } else w += c; }
    if (out.size() >= 2 && (unsigned char)out[0] == 0xC3 && (unsigned char)out[1] >= 0xA0) out[1] = (char)((unsigned char)out[1] - 0x20); else if (!out.empty()) out[0] = (char)toupper((unsigned char)out[0]);
    return out;
}
static void initNames() {
    static const char* B[] = {"", "GRAMA", "TERRA", "PEDRA", "AREIA", "TRONCO", "FOLHAS", "TABUAS", "PEDREGULHO", "AGUA", "CASCALHO", "TIJOLOS", "ROCHA BASE", "MINERIO DE CARVAO", "MINERIO DE DIAMANTE",
        "MINERIO DE OURO", "MINERIO DE FERRO", "BLOCO DE OURO", "BLOCO DE FERRO", "BLOCO DE DIAMANTE", "BLOCO DE CARVAO", "ARENITO", "TIJOLOS DE PEDRA", "PEDREGULHO MUSGOSO", "ARGILA", "NEVE", "OBSIDIANA", "PEDRA LUMINOSA", "ESTANTE DE LIVROS", "LA", "MINERIO DE REDSTONE", "MINERIO DE LAPIS"};
    for (int i = 0; i < 32; i++) NAMES[i] = B[i];
    static const char* I[] = {"GRAVETO", "CARVAO", "BARRA DE FERRO", "DIAMANTE", "MACA", "PICARETA DE MADEIRA", "PICARETA DE PEDRA", "ESPADA DE MADEIRA"};
    for (int i = 0; i < 8; i++) NAMES[IT_STICK + i] = I[i];
    static const char* M[] = {"BARRA DE OURO", "REDSTONE", "SILEX", "PENA", "OSSO", "PAPEL", "LIVRO", "LINHA", "COURO", "LAPIS-LAZULI", "PAO", "BISCOITO", "CARNE CRUA", "BIFE", "CENOURA", "MACA DOURADA", "FLECHA"};
    for (int i = 0; i < 17; i++) NAMES[IT_GOLD + i] = M[i];
    static const char* T[] = {"PICARETA DE FERRO", "PICARETA DE OURO", "PICARETA DE DIAMANTE", "ESPADA DE PEDRA", "ESPADA DE FERRO", "ESPADA DE OURO", "ESPADA DE DIAMANTE"};
    for (int i = 0; i < 7; i++) NAMES[IT_T0 + i] = T[i];
    static const char* MT[] = {"MADEIRA", "PEDRA", "FERRO", "OURO", "DIAMANTE"};
    static const char* TK[] = {"MACHADO", "PA", "ENXADA"};
    for (int k = 0; k < 3; k++) for (int m = 0; m < 5; m++) NAMES[IT_WAXE + k * 5 + m] = std::string(TK[k]) + " DE " + MT[m];
    static const char* MN[] = {"COSTELETA CRUA", "COSTELETA ASSADA", "FRANGO CRU", "FRANGO ASSADO", "CARNE DE CARNEIRO CRUA", "CARNE DE CARNEIRO ASSADA", "OVO",
        "OVO DE PORCO", "OVO DE GALINHA", "OVO DE VACA", "OVO DE OVELHA"};
    for (int i = 0; i < 11; i++) NAMES[IT_PORK + i] = MN[i];
    NAMES[IT_BOW] = "ARCO"; NAMES[CTABLE] = "MESA DE TRABALHO";
    static const char* AM[] = {"COURO", "FERRO", "OURO", "DIAMANTE"}; static const char* AP[] = {"CAPACETE", "PEITORAL", "CALCAS", "BOTAS"};
    for (int m = 0; m < 4; m++) for (int a = 0; a < 4; a++) NAMES[IT_A0 + m * 4 + a] = std::string(AP[a]) + " DE " + AM[m];
    for (int i = 0; i < 256; i++) if (!NAMES[i].empty()) NAMES[i] = prettyName(NAMES[i]);
}
static const char* itemName(int id) { if (isPk(id)) return PK[id - PK0].name.c_str(); if (id >= MOD0 && id < 256 && !modName[id].empty()) return modName[id].c_str(); static bool init = false; if (!init) { initNames(); init = true; } return id >= 0 && id < 256 ? NAMES[id].c_str() : ""; }

// ---- categorias do criativo (abas, como o Java) ----
// ordem e posicao das abas = Minecraft 1.12.2 (em cima: construcao, decoracao, redstone, transporte, diversos, pesquisa; embaixo: comida, ferramentas, combate, pocoes, materiais, inventario)
enum { TAB_BUILD, TAB_DECO, TAB_REDSTONE, TAB_TRANSPORT, TAB_MISC, TAB_SEARCH, TAB_FOOD, TAB_TOOLS, TAB_COMBAT, TAB_BREW, TAB_MATERIALS, TAB_INV, NTAB };
struct TabInfo { const char* name; int icon; bool bottom, right; int col; };
static std::vector<TabInfo> TABS = {{"Blocos de construção", BRICKS, false, false, 0}, {"Decoração", LEAVES, false, false, 1}, {"Redstone", IT_REDSTONE, false, false, 2},
    {"Transporte", IT_STICK, false, false, 3}, {"Diversos", IT_EGG, false, false, 4}, {"Pesquisar", IT_BOOK, false, true, 6}, {"Comida", IT_APPLE, true, false, 0},
    {"Ferramentas", IT_IPICK, true, false, 1}, {"Combate", IT_ISWORD, true, false, 2}, {"Poções", IT_FLINT, true, false, 3}, {"Materiais", IT_STICK, true, false, 4},
    {"Inventário de sobrevivência", IT_A0 + 4, true, true, 6}};   // ícones trocados pelos do cfg; posição real (linha/coluna) vem de tabPos()
static int ctab = TAB_BUILD, cscroll = 0; static bool scrollDrag = false; static std::string searchQ;
static bool invSurv() { return !creative || gTable || gChest; }                       // painel 176x166 com crafting 2x2 (só sobrevivência)
static bool invCInv() { return creative && !gTable && !gChest && ctab == TAB_INV; }     // aba "Inventário" do criativo: painel 195x136 com lixeira, igual ao Java
static bool invMain() { return invSurv() || invCInv(); }          // mochila (9-35) acessível
static bool invSearching() { return creative && !gTable && !gChest && ctab == TAB_SEARCH; }
static std::vector<int> gCreative[NTAB];   // conteudo das abas (assets/creative/mc-1.12.2.cfg); vazio = lista antiga abaixo
static void tabItems(int t, std::vector<int>& v) {
    v.clear();
    if (t >= 0 && t < NTAB && !gCreative[t].empty()) { v = gCreative[t]; return; }
    auto add = [&](std::initializer_list<int> l) { for (int x : l) v.push_back(x); };
    switch (t) {
    case TAB_BUILD: add({GRASS, DIRT, STONE, COBBLE, MOSSY, STONEBRICK, SAND, SANDSTONE, GRAVEL, CLAY, PLANKS, LOG, BRICKS, SNOWBLK, OBSIDIAN, BEDROCK, GOLDBLK, IRONBLK, DIAMBLK, COALBLK, WOOL}); break;
    case TAB_DECO: add({LEAVES, BOOKSHELF, CTABLE, GLOWSTONE, WATER, COALORE, IRONORE, GOLDORE, DIAMONDORE, REDSTONEORE, LAPISORE}); break;
    case TAB_MISC: add({IT_STICK, IT_COAL, IT_IRON, IT_GOLD, IT_DIAMOND, IT_REDSTONE, IT_LAPIS, IT_FLINT, IT_FEATHER, IT_BONE, IT_PAPER, IT_BOOK, IT_STRING, IT_LEATHER, IT_EGG, IT_EGG_PIG, IT_EGG_CHICKEN, IT_EGG_COW, IT_EGG_SHEEP}); break;
    case TAB_FOOD: add({IT_APPLE, IT_GAPPLE, IT_BREAD, IT_COOKIE, IT_BEEF, IT_STEAK, IT_CARROT, IT_PORK, IT_CPORK, IT_CHICKEN, IT_CCHICKEN, IT_MUTTON, IT_CMUTTON}); break;
    case TAB_TOOLS: add({IT_WPICK, IT_SPICK, IT_IPICK, IT_GPICK, IT_DPICK}); for (int i = IT_WAXE; i < IT_BOW; i++) v.push_back(i); break;
    case TAB_COMBAT: add({IT_WSWORD, IT_SSWORD, IT_ISWORD, IT_GSWORD, IT_DSWORD, IT_BOW, IT_ARROW}); for (int i = IT_A0; i < IT_A1; i++) v.push_back(i); break;
    default: if (t >= NTAB && t - NTAB < (int)MODS.size()) v = MODS[t - NTAB].ids; break;
    case TAB_SEARCH: {   // com cache: a lista (milhares de itens do pack) so e refeita quando o texto muda
        static std::string cq = "\x01"; static std::vector<int> cv;
        if (cq != searchQ) {
            cq = searchQ; cv.clear();
            std::vector<int> t2; std::unordered_set<int> seen; const std::string q = foldAscii(searchQ.c_str());
            for (int k = 0; k < (int)TABS.size(); k++) { if (k == TAB_SEARCH || k == TAB_INV) continue; tabItems(k, t2);
                for (int x : t2) if (seen.insert(x).second && (q.empty() || (isPk(x) ? PK[x - PK0].fold : foldAscii(itemName(x))).find(q) != std::string::npos)) cv.push_back(x); }
        }
        v = cv;
        break; }
    }
}
static int maxScrollRows() { std::vector<int> v; tabItems(ctab, v); return std::max(0, ((int)v.size() + 8) / 9 - 5); }
static void setTab(int t) { ctab = t; cscroll = 0; scrollDrag = false; }
static bool invSearchText(const char* t) {
    if (!invSearching()) return false;
    for (; *t; t++) { char c = (char)tolower((unsigned char)*t); if ((isalnum((unsigned char)c) || c == ' ' || c == '-') && searchQ.size() < 20) searchQ += c; }
    cscroll = 0; return true;
}
static void invSearchBack() { if (!searchQ.empty()) searchQ.pop_back(); cscroll = 0; }
static void invWheel(int dy) { if (creative && !gTable && !gChest && ctab != TAB_INV) cscroll = std::max(0, std::min(maxScrollRows(), cscroll - dy)); }

static int addItem(int id, int n, const std::string& nbt = std::string()) {  // devolve o que não coube (hotbar primeiro, como o Java); nbt: so junta com pilha de mesmo NBT
    for (int pass = 0; pass < 2 && n > 0; pass++)
        for (int i = 0; i < 36 && n > 0; i++) {
            Slot& s = inv[i];
            if (pass == 0 ? (s.id == id && s.nbt == nbt && s.n < maxStack(id)) : s.id == 0) {
                int put = std::min(n, maxStack(id) - s.n); s.id = id; s.nbt = nbt; s.n += put; n -= put;
            }
        }
    return n;
}
static std::vector<Slot> throwQ;   // itens a jogar no chão (consumido no loop principal)
// pick block: (1) item (e NBT) ja no hotbar => seleciona; (2) hotbar tem vazio => move o atual para o vazio e traz o novo; (3) senao substitui o slot atual. make = cria se nao tem (criativo)
static bool invPickPut(int& sel, int id, bool make, const std::string& nbt = std::string()) {
    for (int i = 0; i < 9; i++) if (inv[i].id == id && inv[i].nbt == nbt) { sel = i; return true; }   // mesmo item E mesmo NBT (bau cheio != bau vazio)
    int src = -1; for (int i = 9; i < 36; i++) if (inv[i].id == id && inv[i].nbt == nbt) { src = i; break; }
    if (src < 0 && !make) return false;
    if (inv[sel].id) { int e = -1; for (int i = 0; i < 9; i++) if (!inv[i].id) { e = i; break; } if (e >= 0) { inv[e] = inv[sel]; inv[sel] = Slot(); } }
    if (src >= 0) std::swap(inv[src], inv[sel]); else { inv[sel] = Slot(); inv[sel].id = id; inv[sel].n = 1; inv[sel].nbt = nbt; }
    return true;
}
static void returnCursor() { if (cur.id) { int left = addItem(cur.id, cur.n, cur.nbt); if (left > 0) throwQ.push_back({cur.id, left, 0, cur.nbt}); cur = Slot(); } }

static Slot armor[4], craftG[9], craftOut, offh;   // offh = mão secundária (aba Inventário do criativo)
static bool gInvTouch = false;   // modo toque (main_loop): inventário maior, seleção por moldura e arrastar como no Bedrock
static int gSelSlot = -1; static bool gSelHalf = false;   // slot selecionado (moldura da hotbar); gSelHalf = move metade da pilha (toque longo)
struct TchFinger { bool on = false, drag = false, scroll = false; int slot = -1, x0 = 0, y0 = 0, lx = 0, ly = 0, w = 0, h = 0, cs0 = 0; Uint32 t0 = 0; };
static TchFinger gTch;
static const int SL_PAL = 100, SL_ARMOR = 200, SL_CRAFT = 210, SL_OUT = 220, SL_OFF = 230, SL_TRASH = 240;
static bool armorOk(int slot, int id) { return isArmor(id) && (id - IT_A0) % 4 == slot; }   // 0 capacete, 1 peitoral, 2 calcas, 3 botas
static int elytraId() { static size_t n = (size_t)-1; static int id = 0; if (n != PK.size()) { n = PK.size(); id = 0; for (size_t i = 0; i < PK.size(); i++) if (PK[i].file == "item/elytra") { id = PK0 + (int)i; break; } } return id; }
static inline bool isElytraItem(int id) { return id && id == elytraId(); }
static inline bool elytraUsable(const Slot& s) { return isElytraItem(s.id) && s.d < 431; }   // ElytraItem.isUsable: dano < durabilidade maxima (432) - 1
static bool armorSlotOk(int slot, int id) { return armorOk(slot, id) || (slot == 1 && isElytraItem(id)); }   // a elitra ocupa o slot do peitoral
static Slot* slotRef(int id) {
    if (id >= SL_CHEST && id < SL_CHEST + 54) return chestSlotRef(id - SL_CHEST);
    if (id >= 0 && id < 36) return &inv[id];
    if (id >= SL_ARMOR && id < SL_ARMOR + 4) return &armor[id - SL_ARMOR];
    if (id >= SL_CRAFT && id < SL_CRAFT + 9) return &craftG[id - SL_CRAFT];
    if (id == SL_OUT) return &craftOut;
    if (id == SL_OFF) return &offh;
    return nullptr;
}
static Slot slotView(int id) {
    if (id >= SL_PAL && id < SL_PAL + 45) { std::vector<int> v; tabItems(ctab, v); int k = cscroll * 9 + id - SL_PAL; return k < (int)v.size() ? Slot{v[k], 1} : Slot(); }
    Slot* s = slotRef(id); return s ? *s : Slot();
}
static bool isCraft(int id) { return id >= SL_CRAFT && id < SL_CRAFT + 9; }
static int gridW() { return gTable ? 3 : 2; }
static bool isArmorSlot(int id) { return id >= SL_ARMOR && id < SL_ARMOR + 4; }

static void updateCraft() {   // procura a receita na grade atual (2x2 no inventário, 3x3 na mesa)
    craftOut = Slot();
    const int w = gridW(); int g[9] = {0}, n = 0;
    for (int k = 0; k < w * w; k++) g[k] = craftG[k].id;
    const int out = craftResult(g, w, w, n);
    if (out) craftOut = {out, n};
}
static void consumeCraft() { for (int k = 0; k < 9; k++) if (craftG[k].id && --craftG[k].n <= 0) craftG[k] = Slot(); }
static int roomFor(int id) {
    int r = 0;
    for (int i = 0; i < 36; i++) r += inv[i].id == 0 ? maxStack(id) : (inv[i].id == id ? maxStack(id) - inv[i].n : 0);
    return r;
}
// ordens do mergeItemStack do Java (índices do container 9..44 = mochila 9..35 + hotbar 0..8)
static std::vector<int> ordRange(int a, int b, bool rev) { std::vector<int> v; for (int i = a; i < b; i++) v.push_back(i); if (rev) std::reverse(v.begin(), v.end()); return v; }
static std::vector<int> ordMainHot(bool rev) {
    std::vector<int> v = ordRange(9, 36, false), h = ordRange(0, 9, false); v.insert(v.end(), h.begin(), h.end());
    if (rev) std::reverse(v.begin(), v.end());
    return v;
}
static void mergeInto(Slot& s, const std::vector<int>& order) {   // 1º empilha nos iguais, 2º ocupa vazios
    for (int pass = 0; pass < 2 && s.n > 0; pass++)
        for (int j : order) {
            if (s.n <= 0) break;
            Slot& d = inv[j];
            if (pass == 0 ? (sameStack(d, s) && d.n < maxStack(s.id)) : d.id == 0) {
                int put = std::min(s.n, maxStack(s.id) - d.n); d.id = s.id; d.nbt = s.nbt; d.n += put; s.n -= put;
            }
        }
    if (s.n <= 0) s = Slot();
}
static void slotClick(Slot& s, int btn) {   // modo 0 do Java: pegar/soltar
    if (btn == SDL_BUTTON_LEFT) {
        if (!cur.id) { cur = s; s = Slot(); }
        else if (!s.id) { s = cur; cur = Slot(); }
        else if (sameStack(s, cur)) { int put = std::min(cur.n, maxStack(s.id) - s.n); s.n += put; cur.n -= put; if (!cur.n) cur = Slot(); }
        else std::swap(cur, s);
    } else if (btn == SDL_BUTTON_RIGHT) {
        if (!cur.id) { if (s.id) { int take = (s.n + 1) / 2; cur = s; cur.n = take; s.n -= take; if (!s.n) s = Slot(); } }
        else if (!s.id) { s = cur; s.n = 1; if (!--cur.n) cur = Slot(); }
        else if (sameStack(s, cur)) { if (s.n < maxStack(s.id)) { s.n++; if (!--cur.n) cur = Slot(); } }
        else std::swap(cur, s);
    }
}
static void takeResult(bool shift) {
    if (!craftOut.id) return;
    if (!shift) {
        if (cur.id && (cur.id != craftOut.id || cur.n + craftOut.n > maxStack(cur.id))) return;
        if (cur.id) cur.n += craftOut.n; else cur = craftOut;
        consumeCraft(); updateCraft();
    } else {
        for (int guard = 0; craftOut.id && guard < 64; guard++) {
            if (roomFor(craftOut.id) < craftOut.n) break;
            Slot t = craftOut; mergeInto(t, ordMainHot(true));   // resultado vai de trás para frente (hotbar primeiro)
            consumeCraft(); updateCraft();
        }
    }
}
static void mergeChest(Slot& s) {   // shift-click da mochila: empilha nos iguais e depois ocupa vazios, de cima para baixo (mergeItemStack(0, linhas*9, false))
    for (int pass = 0; pass < 2 && s.n > 0; pass++) for (int k = 0; k < gChRows * 9 && s.n > 0; k++) {
        Slot* d = chestSlotRef(k); if (!d) continue;
        if (pass == 0 ? (sameStack(*d, s) && d->n < maxStack(s.id)) : d->id == 0) { const int put = std::min(s.n, maxStack(s.id) - d->n); d->id = s.id; d->nbt = s.nbt; d->n += put; s.n -= put; }
    }
    if (s.n <= 0) s = Slot();
}
static void invClick(int id, int btn, bool shift) {   // clique normal / shift em slot de armazenamento
    if (id == SL_OUT) { takeResult(shift); return; }
    if (id == SL_TRASH) {   // Java: clique apaga o item da mão; Shift+clique limpa o inventário inteiro
        if (shift) { for (auto& x : inv) x = Slot(); for (auto& x : armor) x = Slot(); offh = Slot(); } else cur = Slot();
        return;
    }
    if (gChest) {   // bau aberto: clique normal = pegar/soltar; shift: bau -> mochila (de tras para frente, hotbar primeiro) ou mochila -> bau (de cima para baixo)
        if (gChWait) return; Slot* cs = slotRef(id); if (!cs) return;
        if (shift) { if (!cs->id) return; if (id >= SL_CHEST) mergeInto(*cs, ordMainHot(true)); else mergeChest(*cs); return; }
        slotClick(*cs, btn); return;
    }
    Slot* s = slotRef(id); if (!s) return;
    if (shift) {
        if (!invMain() || !s->id) return;
        if (!gTable && isElytraItem(s->id) && !isArmorSlot(id) && !armor[1].id) { armor[1] = {s->id, 1, s->d}; *s = Slot(); if (isCraft(id)) updateCraft(); return; }
        if (!gTable && isArmor(s->id) && !isArmorSlot(id) && !armor[(s->id - IT_A0) % 4].id) { armor[(s->id - IT_A0) % 4] = {s->id, 1}; *s = Slot(); if (isCraft(id)) updateCraft(); return; }
        if (isArmorSlot(id) || isCraft(id)) mergeInto(*s, ordMainHot(false));
        else if (id < 9) mergeInto(*s, ordRange(9, 36, false));
        else mergeInto(*s, ordRange(0, 9, false));
        if (isCraft(id)) updateCraft();
        return;
    }
    if (isArmorSlot(id) && cur.id && !armorSlotOk(id - SL_ARMOR, cur.id)) return;
    slotClick(*s, btn);
    if (isCraft(id)) updateCraft();
}
static void collectAll() {   // duplo clique (modo 6): junta todos os iguais no cursor
    if (!cur.id) return;
    const int mx = maxStack(cur.id);
    for (int pass = 0; pass < 2; pass++)
        for (int id = 0; id < 36 + 9; id++) {
            int sid = id < 36 ? id : SL_CRAFT + id - 36;
            if (!invMain() && sid >= 9) continue;
            if (sid >= SL_ARMOR) continue;
            Slot& s = *slotRef(sid);
            if (sameStack(s, cur) && cur.n < mx && (pass == 0 ? s.n < mx : s.n == mx)) {
                int take = std::min(mx - cur.n, s.n); cur.n += take; s.n -= take; if (!s.n) s = Slot();
            }
        }
    if (gChest) for (int pass = 0; pass < 2; pass++) for (int k = 0; k < gChRows * 9; k++) {
        Slot* s = chestSlotRef(k);
        if (s && sameStack(*s, cur) && cur.n < mx && (pass == 0 ? s->n < mx : s->n == mx)) { int take = std::min(mx - cur.n, s->n); cur.n += take; s->n -= take; if (!s->n) *s = Slot(); }
    }
    updateCraft();
}
static void numberSwap(int n, int id) {   // tecla 1-9 sobre um slot
    if (id == SL_OUT) { if (craftOut.id && !inv[n].id) { inv[n] = craftOut; consumeCraft(); updateCraft(); } return; }
    Slot* s = slotRef(id); if (!s || s == &inv[n]) return;
    if (isArmorSlot(id) && inv[n].id && !armorSlotOk(id - SL_ARMOR, inv[n].id)) return;
    std::swap(inv[n], *s);
    if (isCraft(id)) updateCraft();
}
static int palItem(int id) { std::vector<int> v; tabItems(ctab, v); int k = cscroll * 9 + id - SL_PAL; return k >= 0 && k < (int)v.size() ? v[k] : 0; }
static void paletteClick(int id, int btn, bool shift) {   // lista de itens do criativo
    int it = palItem(id); if (!it) { if (cur.id) cur = Slot(); return; }
    if (shift) { addItem(it, maxStack(it)); return; }
    if (cur.id) {                       // com item na mão: esquerdo apaga a pilha, direito tira 1
        if (btn == SDL_BUTTON_LEFT || cur.id != it) cur = Slot(); else if (!--cur.n) cur = Slot();
        return;
    }
    cur = {it, btn == SDL_BUTTON_RIGHT ? 1 : maxStack(it)};
}
static void closeInv() {   // fechar: item da mão e da grade de crafting voltam para a mochila (criativo descarta)
    const bool wasChest = gChest; chestClosed();
    if (creative && !wasChest) cur = Slot(); else returnCursor();
    for (int k = 0; k < 9; k++) if (craftG[k].id) { int left = addItem(craftG[k].id, craftG[k].n); if (left > 0 && !creative) throwQ.push_back({craftG[k].id, left}); craftG[k] = Slot(); }
    gTable = false; updateCraft();
}
static int dropOf(int b) {
    switch (b) { case GRASS: return DIRT; case STONE: return COBBLE; case LEAVES: { const int r = rand() % 100; if (r < 5) { auto it = gPkByFile.find("block/oak_sapling"); if (it != gPkByFile.end()) return it->second; } return r < 8 ? IT_APPLE : 0; }
                 case COALORE: return IT_COAL; case REDSTONEORE: return IT_REDSTONE; case LAPISORE: return IT_LAPIS; case DIAMONDORE: return IT_DIAMOND; case WATER: return 0; default: return b; }
}

// ---------- primitivas ----------
static void quad(float x, float y, float w, float h, float u0, float v0, float u1, float v1) {
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x, y); glTexCoord2f(u1, v0); glVertex2f(x + w, y);
    glTexCoord2f(u1, v1); glVertex2f(x + w, y + h); glTexCoord2f(u0, v1); glVertex2f(x, y + h);
    glEnd();
}
static void flat(float x, float y, float w, float h, float r, float g, float b, float a) { glDisable(GL_TEXTURE_2D); glColor4f(r, g, b, a); quad(x, y, w, h, 0, 0, 0, 0); glEnable(GL_TEXTURE_2D); }
static void face(int tile, float sh, const float* a, const float* b, const float* c, const float* d, float vTop = 0.f) {   // vTop: fracao cortada em cima (blocos baixos)
    float u0, v0, u1, v1; tileUV(tile, u0, v0, u1, v1);
    const float eu = .02f / ATW, ev = .02f / (16.f * ATLR); u0 += eu; v0 += ev; u1 -= eu; v1 -= ev; v0 += vTop * (v1 - v0);   // evita vazar texel do tile vizinho
    glColor4f(sh * gIconTint[0], sh * gIconTint[1], sh * gIconTint[2], 1);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(a[0], a[1]); glTexCoord2f(u1, v0); glVertex2f(b[0], b[1]);
    glTexCoord2f(u1, v1); glVertex2f(c[0], c[1]); glTexCoord2f(u0, v1); glVertex2f(d[0], d[1]);
    glEnd();
}
static void guiImage(GLuint tex, float x, float y, float w, float h, float alpha = 1.f) {
    if (!tex) { flat(x, y, w, h, .12f, .12f, .12f, alpha); return; }
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); glColor4f(1, 1, 1, alpha);
    quad(x, y, w, h, 0, 0, 1, 1);
}
// Tocha 3D no icone: mesma projecao isometrica do cubo (cx0 + hw*(x-z), y + 4u*(x+z) + 8u*(1-y)); modelo centralizado e ampliado para ocupar o slot.
// Faces ordenadas de tras para frente (x+y+z crescente) e sombreadas como o cubo (topo 1, lado esquerdo .78, direito .55).
static void drawTorchIcon3D(const std::vector<XQuad>& qs, float x, float y, float s, bool fit = false) {   // fit: escala/centra pela area projetada (cama: baixa e comprida)
    float mn[3] = {1e9f, 1e9f, 1e9f}, mx[3] = {-1e9f, -1e9f, -1e9f};
    for (const XQuad& q : qs) for (int k = 0; k < 4; k++) for (int i = 0; i < 3; i++) { mn[i] = std::min(mn[i], q.p[k][i]); mx[i] = std::max(mx[i], q.p[k][i]); }
    const float ext = std::max(mx[1] - mn[1], std::max(mx[0] - mn[0], mx[2] - mn[2]));
    const float k = ext > 1e-4f ? .95f / ext : 1.f, c[3] = {(mn[0] + mx[0]) * .5f, (mn[1] + mx[1]) * .5f, (mn[2] + mx[2]) * .5f};
    const float u = s / 16.f, hw = 6.93f * u, cx0 = x + 8 * u;
    std::vector<std::pair<float, int>> ord; ord.reserve(qs.size());
    for (int i = 0; i < (int)qs.size(); i++) { float d = 0; for (int v = 0; v < 4; v++) d += qs[i].p[v][0] + qs[i].p[v][1] + qs[i].p[v][2]; ord.push_back({d, i}); }
    std::sort(ord.begin(), ord.end());
    float fk = 0.f, fa = 0.f, fd = 0.f;   // fit: caixa projetada (a = px-pz, d = parte vertical) para k=1
    if (fit) {
        float a0 = 1e9f, a1 = -1e9f, d0 = 1e9f, d1 = -1e9f;
        for (const XQuad& q : qs) for (int v = 0; v < 4; v++) {
            const float dx = q.p[v][0] - c[0], dy = q.p[v][1] - c[1], dz = q.p[v][2] - c[2], a = hw * (dx - dz), d = 4 * u * (dx + dz) - 8 * u * dy;
            a0 = std::min(a0, a); a1 = std::max(a1, a); d0 = std::min(d0, d); d1 = std::max(d1, d);
        }
        fk = std::min(15.f * u / std::max(a1 - a0, 1e-3f), 15.f * u / std::max(d1 - d0, 1e-3f)); fa = (a0 + a1) * .5f; fd = (d0 + d1) * .5f;
    }
    for (const auto& o : ord) {
        const XQuad& q = qs[o.second]; float sp[4][2];
        for (int v = 0; v < 4; v++) {
            if (fit) {
                const float dx = q.p[v][0] - c[0], dy = q.p[v][1] - c[1], dz = q.p[v][2] - c[2];
                sp[v][0] = cx0 + fk * (hw * (dx - dz) - fa); sp[v][1] = y + 8 * u + fk * (4 * u * (dx + dz) - 8 * u * dy - fd); continue;
            }
            const float px = .5f + (q.p[v][0] - c[0]) * k, py = .5f + (q.p[v][1] - c[1]) * k, pz = .5f + (q.p[v][2] - c[2]) * k;
            sp[v][0] = cx0 + hw * (px - pz); sp[v][1] = y + 4 * u * (px + pz) + 8 * u * (1.f - py);
        }
        float e1[3], e2[3], n[3]; for (int i = 0; i < 3; i++) { e1[i] = q.p[1][i] - q.p[0][i]; e2[i] = q.p[3][i] - q.p[0][i]; }
        n[0] = e1[1] * e2[2] - e1[2] * e2[1]; n[1] = e1[2] * e2[0] - e1[0] * e2[2]; n[2] = e1[0] * e2[1] - e1[1] * e2[0];
        const float sh = fabsf(n[1]) >= std::max(fabsf(n[0]), fabsf(n[2])) ? 1.f : (fabsf(n[2]) >= fabsf(n[0]) ? .78f : .55f);
        const int tx = q.tile % ATL, ty = q.tile / ATL; const float ex = .02f / ATW, ey = .02f / (16.f * ATLR);
        bool big = false; for (int v = 0; v < 4; v++) if (q.u[v] > 1.001f || q.v[v] > 1.001f) big = true;   // modelos de entidade (bau, cama, placa, shulker): u/v ate 4 sobre 4x4 tiles
        glColor4f(sh, sh, sh, 1); glBegin(GL_QUADS);
        for (int v = 0; v < 4; v++) {
            if (big) { glTexCoord2f((tx + q.u[v]) / (float)ATL, (ty + q.v[v]) / (float)ATLR); glVertex2f(sp[v][0], sp[v][1]); continue; }
            // u/v do XQuad: 0..1 dentro do tile (v = 0 em cima); ordem BL,BR,TR,TL igual aos vertices p
            glTexCoord2f((tx + std::min(std::max(q.u[v], 0.f), 1.f)) / (float)ATL + (q.u[v] < .5f ? ex : -ex), (ty + std::min(std::max(q.v[v], 0.f), 1.f)) / (float)ATLR + (q.v[v] < .5f ? ey : -ey));
            glVertex2f(sp[v][0], sp[v][1]);
        }
        glEnd();
    }
}
// Ícone 16x16 unidades (s = 16*gs, sempre inteiro). Bloco = cubo isométrico 2:1 em coordenadas inteiras; item = sprite.
static const std::vector<XQuad>* rsIconQuads(int id);   // redstone.inc
static void drawIcon(int id, float x, float y, float s) {
    if (!id) return;
    glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D);
    // NAO mexer no filtro do atlas aqui: o filtro (NEAREST_MIPMAP_LINEAR) e definido UMA vez em uploadAtlasMips(). Icones sao >= 1:1, usam o nivel 0.
    glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .02f);
    if (isCubeIcon(id)) {
        const float u = s / 16.f;
        const float hw = 6.93f * u, cx0 = x + 8 * u;   // isométrico real: meia-largura = 0,866 * aresta (aresta 8) => sem esticar na horizontal
        float A[2] = {cx0, y}, B[2] = {cx0 + hw, y + 4 * u}, C[2] = {cx0, y + 8 * u}, D[2] = {cx0 - hw, y + 4 * u};
        float E[2] = {cx0 + hw, y + 12 * u}, F[2] = {cx0, y + 16 * u}, G[2] = {cx0 - hw, y + 12 * u};
        const float hh = pkHeight(id), dh = (1.f - hh) * 8 * u;   // mesa de encantamento 12/16, moldura do End 13/16: topo desce, laterais cortam o alto da textura
        A[1] += dh; B[1] += dh; C[1] += dh; D[1] += dh;
        float tc[3]; const bool tinted = iconTintRGB(id, tc), topOnly = tinted && isPk(id) && PK[id - PK0].file.find("grass_block") != std::string::npos;
        if (tinted) { gIconTint[0] = tc[0]; gIconTint[1] = tc[1]; gIconTint[2] = tc[2]; }
        face(iconTile(id, 0), 1.f, A, B, C, D);
        if (topOnly) gIconTint[0] = gIconTint[1] = gIconTint[2] = 1.f;
        face(pkFront(id) >= 0 ? pkFront(id) : iconTile(id, 1), .78f, D, C, F, G, 1.f - hh); face(iconTile(id, 1), .55f, C, B, E, F, 1.f - hh);
        gIconTint[0] = gIconTint[1] = gIconTint[2] = 1.f;
    } else if (const std::vector<XQuad>* rq = rsIconQuads(id)) {
        { auto gi = gXbItem.find(id); drawTorchIcon3D(*rq, x, y, s, gi != gXbItem.end() && gi->second == XB_BED); }   // placa/botao/portao/alcapao/sensor/bau: malha do proprio bloco
    } else if (const int xbt = xbItemBlock(id); (xbt == XB_TORCH || xbt == XB_RTORCH) && !gXM[xbt - XB0].empty()) {
        drawTorchIcon3D(gXM[xbt - XB0], x, y, s);   // tocha em 3D (modelo do pack), como os blocos
    } else {
        float u0, v0, u1, v1; tileUV(iconSpriteTile(id), u0, v0, u1, v1);
        const float eu = .02f / ATW, ev = .02f / (16.f * ATLR); u0 += eu; v0 += ev; u1 -= eu; v1 -= ev;
        { float tc[3] = {1, 1, 1}; iconTintRGB(id, tc); glColor4f(tc[0], tc[1], tc[2], 1); }
        quad(x, y, s, s, u0, v0, u1, v1);
    }
    glDisable(GL_ALPHA_TEST);
}

// ---------- layout em unidades do Java ----------
static int G = 1; static float OX = 0, OY = 0; static int gVpX = 0, gVpY = 0;   // origem do viewport atual (split screen): overlays do HUD usam glViewport(gVpX, gVpY, w, h)      // escala atual e origem (px) usada pelos helpers U/uText
static int guiScaleFor(int w, int h) {               // "Auto" do Java: maior escala em que 320x240 ainda cabe
    if (guiScaleCfg >= 1) return (int)guiScaleCfg;
    int s = 1; while (w / (s + 1) >= 320 && h / (s + 1) >= 240) s++;
    return s;
}
static void U(float x, float y, float w, float h, unsigned c, float a = 1.f) {
    flat(OX + x * G, OY + y * G, w * G, h * G, ((c >> 16) & 255) / 255.f, ((c >> 8) & 255) / 255.f, (c & 255) / 255.f, a);
}
static float tsc() { return (float)G; }   // fonte 5x7: 1 pixel da fonte = 1 unidade da GUI (como o Java)
static float gUiA = 1.f;   // opacidade global do texto de UI (fade das telas)
static void uText(float ux, float uy, const char* s, unsigned c, bool shadow = false) {
    float sc = tsc();
    text(OX + ux * G, OY + uy * G + floorf((8 * G - 7 * sc) / 2), sc, s, ((c >> 16) & 255) / 255.f, ((c >> 8) & 255) / 255.f, (c & 255) / 255.f, shadow ? (float)G : 0.f, gUiA);
}
static void uPanel(float x, float y, float w, float h) {   // mesmas cores do original; arte com cantos recortados e rebites
    U(x + 1, y, w - 2, h, 0x000000); U(x, y + 1, w, h - 2, 0x000000);
    U(x + 1, y + 1, w - 2, h - 2, 0xFFFFFF); U(x + 3, y + 3, w - 4, h - 4, 0x555555); U(x + 3, y + 3, w - 6, h - 6, 0xC6C6C6);
    U(x + 1, y + h - 2, w - 2, 1, 0x555555); U(x + w - 2, y + 1, 1, h - 2, 0x555555);
    U(x + w - 3, y + 1, 2, 2, 0xC6C6C6); U(x + 1, y + h - 3, 2, 2, 0xC6C6C6);
    U(x + 1, y + 1, 1, 1, 0x000000); U(x + w - 2, y + 1, 1, 1, 0x000000); U(x + 1, y + h - 2, 1, 1, 0x000000); U(x + w - 2, y + h - 2, 1, 1, 0x000000);
}
static void uBox(float x, float y, float w, float h, float a = 1.f, unsigned fill = 0x8B8B8B) {   // rebaixado: escuro em cima/esq, claro embaixo/dir
    U(x, y, w, h, fill, a); U(x, y, w - 1, 1, 0x373737, a); U(x, y, 1, h - 1, 0x373737, a);
    U(x + 1, y + h - 1, w - 1, 1, 0xFFFFFF, a); U(x + w - 1, y + 1, 1, h - 1, 0xFFFFFF, a);
}
static void uRing(float x, float y, float w, float h, float t, unsigned c, float a = 1.f) {
    U(x, y, w, t, c, a); U(x, y + h - t, w, t, c, a); U(x, y + t, t, h - 2 * t, c, a); U(x + w - t, y + t, t, h - 2 * t, c, a);
}
// ---- sprites do Rethoughted GUI em unidades da GUI (1 pixel da textura = 1 unidade, ampliado pela escala inteira) ----
static void uTexRegion(GLuint tex, float ux, float uy, float uw, float uh, float su, float sv, float sw_, float sh_, float TW, float TH, float a = 1.f) {
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); glColor4f(1, 1, 1, a);
    quad(OX + ux * G, OY + uy * G, uw * G, uh * G, su / TW, sv / TH, (su + sw_) / TW, (sv + sh_) / TH);
}
static void uSpr(GLuint tex, float ux, float uy, float w, float h, float a = 1.f) { uTexRegion(tex, ux, uy, w, h, 0, 0, 1, 1, 1, 1, a); }
// nove fatias (como o nine_slice dos .mcmeta): bordas l,t,r,b em pixels da textura tw x th; o meio estica
static void uNine(GLuint tex, int tw, int th, float bl, float bt, float br, float bb, float ux, float uy, float w, float h, float a = 1.f) {
    // Como o GuiButton do Java: se o destino for menor que as duas bordas juntas (ex.: botao 20x20), cada lado leva so metade
    // (recorte da textura, sem esticar) em vez de a borda direita ser desenhada por cima da esquerda.
    if (w < bl + br) { bl = floorf(w / 2.f); br = w - bl; }
    if (h < bt + bb) { bt = floorf(h / 2.f); bb = h - bt; }
    const float xs[4] = {0, bl, (float)tw - br, (float)tw}, ys[4] = {0, bt, (float)th - bb, (float)th};
    const float dx[4] = {0, bl, w - br, w}, dy[4] = {0, bt, h - bb, h};
    for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++) {
        if (dx[i + 1] <= dx[i] || dy[j + 1] <= dy[j]) continue;
        uTexRegion(tex, ux + dx[i], uy + dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j], xs[i], ys[j], xs[i + 1] - xs[i], ys[j + 1] - ys[j], (float)tw, (float)th, a);
    }
}
static const unsigned UI_TXT = 0xE0E0E0;   // texto de titulos sobre os paineis escuros do Rethoughted GUI (o 0x404040 do Java some no fundo escuro)
static void drawArmorSilhouette(int k, float ux, float uy) {   // silhueta 16x16 do slot vazio (capacete, peitoral, calça, botas), no estilo do Minecraft
    static const char* const S[4][16] = {
        {"................", "................", "....########....", "...#oooooooo#...", "...#oooooooo#...", "...#oooooooo#...", "...#oooooooo#...", "...#oo####oo#...", "...#o#....#o#...", "...###....###...", "................", "................", "................", "................", "................", "................"},
        {"................", "...###....###...", "..#ooo#..#ooo#..", "..#oooo##oooo#..", "..#oooooooooo#..", "..#oo#oooo#oo#..", "..#o#.#oo#.#o#..", "..#o#.#oo#.#o#..", "..###.#oo#.###..", "......#oo#......", "......#oo#......", "......#oo#......", "......####......", "................", "................", "................"},
        {"................", "................", "...##########...", "...#oooooooo#...", "...#oooooooo#...", "...#ooo##ooo#...", "...#oo#..#oo#...", "...#oo#..#oo#...", "...#oo#..#oo#...", "...#oo#..#oo#...", "...#oo#..#oo#...", "...#oo#..#oo#...", "...####..####...", "................", "................", "................"},
        {"................", "................", "................", "................", "................", "..####...####...", "..#oo#...#oo#...", "..#oo#...#oo#...", "..#oo#...#oo#...", ".#ooo#..#ooo#...", ".#oooo#.#oooo#..", ".######.######..", "................", "................", "................", "................"}};
    uy += 2;   // chamadores passam y-2
    if (emptySlotTex[k]) { guiImage(emptySlotTex[k], OX + ux * G, OY + uy * G, 16.f * G, 16.f * G); return; }   // PNG editavel; abaixo: reserva em codigo
    for (int r = 0; r < 16; r++) for (int c = 0; c < 16; c++) { char ch = S[k][r][c]; if (ch == '#') U(ux + c, uy + r, 1, 1, 0x373737); else if (ch == 'o') U(ux + c, uy + r, 1, 1, 0x7B7B7B, .55f); }
}
static void drawStack(float px, float py, int id, int n) {   // px/py = canto do ícone 16x16 (px)
    if (!id) return;
    drawIcon(id, px, py, 16.f * G);
    if (n > 1) {
        char b[8]; snprintf(b, sizeof b, "%d", n); float sc = tsc();
        text(px + 17 * G - textWidth(b, sc), py + 9 * G, sc, b, 1, 1, 1, (float)G);
    }
}

struct InvUI { int gs; float ox, oy; int pw, ph; bool surv, cinv; };
static InvUI invLayout(int w, int h) {
    InvUI u; u.gs = guiScaleFor(w, h); u.surv = invSurv(); u.cinv = invCInv(); u.pw = u.surv ? 176 : 195; u.ph = u.surv ? 166 : 136;   // Java: sobrevivência 176x166, criativo 195x136
    if (gChest) { u.pw = 176; u.ph = 114 + gChRows * 18; }   // GuiChest: 176 x (114 + 18 * linhas)
    if (gInvTouch) { const bool tabs = creative && !gTable && !gChest; const int s = std::min((w - 8) / u.pw, (h - 4) / (u.ph + (tabs ? 60 : 10))); u.gs = std::max(u.gs, std::min(12, s)); }   // toque: maior escala inteira em que painel + abas cabem
    int sw = (w + u.gs - 1) / u.gs, sh = (h + u.gs - 1) / u.gs;
    u.ox = (float)(((sw - u.pw) / 2) * u.gs); u.oy = (float)(((sh - u.ph) / 2) * u.gs);
    return u;
}
static bool slotUnits(const InvUI& u, int id, int* x, int* y) {   // posição do ITEM (16x16) em unidades do painel
    if (gChest) {   // ContainerChest: bau em (8,18) + 18/linha; mochila em y = 31 + 18*linhas; hotbar em 89 + 18*linhas
        if (id >= SL_CHEST && id < SL_CHEST + gChRows * 9) { const int k = id - SL_CHEST; *x = 8 + 18 * (k % 9); *y = 18 + 18 * (k / 9); return true; }
        if (id >= 0 && id < 9) { *x = 8 + 18 * id; *y = 89 + gChRows * 18; return true; }
        if (id >= 9 && id < 36) { *x = 8 + 18 * ((id - 9) % 9); *y = 31 + gChRows * 18 + 18 * ((id - 9) / 9); return true; }
        return false;
    }
    if (u.surv) {
        if (gTable) {   // CraftingTable do Java: grade em (30,17), resultado em (124,35)
            if (id == SL_OUT) { *x = 124; *y = 35; }
            else if (isCraft(id)) { int k = id - SL_CRAFT; *x = 30 + 18 * (k % 3); *y = 17 + 18 * (k / 3); }
            else if (id >= 0 && id < 9) { *x = 8 + 18 * id; *y = 142; }
            else if (id >= 9 && id < 36) { *x = 8 + 18 * ((id - 9) % 9); *y = 84 + 18 * ((id - 9) / 9); }
            else return false;
            return true;
        }
        if (id == SL_OUT) { *x = rtInv ? 154 : 144; *y = rtInv ? 28 : 36; }
        else if (id == SL_OFF && rtInv) { *x = 77; *y = 62; }
        else if (isCraft(id) && id - SL_CRAFT < 4) { int k = id - SL_CRAFT; *x = (rtInv ? 98 : 88) + 18 * (k % 2); *y = (rtInv ? 18 : 26) + 18 * (k / 2); }
        else if (isArmorSlot(id)) { *x = 8; *y = 8 + 18 * (id - SL_ARMOR); }
        else if (id >= 0 && id < 9) { *x = 8 + 18 * id; *y = 142; }
        else if (id >= 9 && id < 36) { *x = 8 + 18 * ((id - 9) % 9); *y = 84 + 18 * ((id - 9) / 9); }
        else return false;
        return true;
    }
    if (id >= SL_PAL && id < SL_PAL + 45 && !u.cinv) { int k = id - SL_PAL; *x = 9 + 18 * (k % 9); *y = 18 + 18 * (k / 9); return true; }   // grade 9x5 em (9,18)
    if (id >= 0 && id < 9) { *x = 9 + 18 * id; *y = 112; return true; }                                                                   // hotbar em (9,112)
    if (u.cinv) {   // aba Inventário do criativo (ContainerCreative do Java)
        if (id >= 9 && id < 36) { *x = 9 + 18 * ((id - 9) % 9); *y = 54 + 18 * ((id - 9) / 9); return true; }
        if (isArmorSlot(id)) { int i = id - SL_ARMOR; *x = 54 + (i / 2) * 54; *y = 6 + (i % 2) * 27; return true; }   // capacete/peitoral à esquerda, calças/botas à direita
        if (id == SL_OFF) { *x = 35; *y = 20; return true; }
        if (id == SL_TRASH) { *x = 173; *y = 112; return true; }
    }
    return false;
}
static std::vector<int> activeSlots(const InvUI& u) {
    std::vector<int> v;
    if (gChest) { for (int i = 0; i < 36; i++) v.push_back(i); for (int k = 0; k < gChRows * 9; k++) v.push_back(SL_CHEST + k); return v; }
    if (u.surv) {
        for (int i = 0; i < 36; i++) v.push_back(i);
        if (gTable) for (int i = 0; i < 9; i++) v.push_back(SL_CRAFT + i);
        else { for (int i = 0; i < 4; i++) { v.push_back(SL_ARMOR + i); v.push_back(SL_CRAFT + i); } if (rtInv) v.push_back(SL_OFF); }
        v.push_back(SL_OUT);
    }
    else if (u.cinv) { for (int i = 0; i < 36; i++) v.push_back(i); for (int i = 0; i < 4; i++) v.push_back(SL_ARMOR + i); v.push_back(SL_OFF); v.push_back(SL_TRASH); }
    else { for (int i = 0; i < 45; i++) v.push_back(SL_PAL + i); for (int i = 0; i < 9; i++) v.push_back(i); }
    return v;
}
static int hitSlot(int w, int h, int mx, int my) {
    InvUI u = invLayout(w, h);
    for (int id : activeSlots(u)) {
        int x, y; if (!slotUnits(u, id, &x, &y)) continue;
        float px = u.ox + x * u.gs, py = u.oy + y * u.gs;
        if (mx >= px && mx < px + 16 * u.gs && my >= py && my < py + 16 * u.gs) return id;
    }
    return -1;
}
static bool inPanel(int w, int h, int mx, int my) {
    InvUI u = invLayout(w, h);
    return mx >= u.ox && my >= u.oy && mx < u.ox + u.pw * u.gs && my < u.oy + u.ph * u.gs;
}

// ---------- entrada do inventário ----------
static int dragBtn = -1, lastClickSlot = -1; static Uint32 lastClickT = 0; static std::vector<int> dragSlots;
static void invReset() { gSelSlot = -1; gSelHalf = false; gTch = TchFinger(); scrollDrag = false; dragBtn = -1; dragSlots.clear(); lastClickSlot = -1; }
static bool dragOk(int id) { return id >= 0 && id < 36 ? true : (isCraft(id) || id >= SL_CHEST); }
// ---- abas: 28x32, até 5 por linha por página (como o Forge), pesquisa/inventário fixos à direita ----
static int tabPage = 0;   // página atual das abas; 0 = abas do jogo, 1+ = abas dos mods (10 por página)
static int tabPages() { int m = (int)TABS.size() - NTAB; return 1 + (m > 0 ? (m + 9) / 10 : 0); }
static bool tabPos(int t, int* row, int* col) {   // row 0 = em cima, 1 = embaixo; col 6 = alinhada à direita
    if (t == TAB_SEARCH) { *row = 0; *col = 6; return true; }
    if (t == TAB_INV) { *row = 1; *col = 6; return true; }
    if (t >= NTAB) { int m = t - NTAB; if (1 + m / 10 != tabPage) return false; *row = (m % 10) / 5; *col = m % 5; return true; }
    if (tabPage != 0) return false;
    static const int R[NTAB] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, C[NTAB] = {0, 1, 2, 3, 4, 6, 0, 1, 2, 3, 4, 6};
    *row = R[t]; *col = C[t]; return true;
}
static bool tabRect(const InvUI& u, int t, float* x, float* y) {   // canto da aba em px
    if (!creative || gTable || gChest) return false;
    int row, col; if (!tabPos(t, &row, &col)) return false;
    const float ux = col == 6 ? u.pw - 28.f : 28.f * col + col, uy = row == 0 ? -28.f : u.ph - 4.f;   // Java: x = 28*col + col, y = -28 / altura-4
    *x = u.ox + ux * u.gs; *y = u.oy + uy * u.gs; return true;
}
static bool pageBtnRect(const InvUI& u, int a, float* x, float* y) {   // botões "<" ">" 20x20 acima das abas (guiTop-50)
    if (!creative || gTable || gChest || tabPages() < 2) return false;
    *x = u.ox + (a ? u.pw - 20.f : 0.f) * u.gs; *y = u.oy - 50.f * u.gs; return true;
}
static int hitTabArrow(int w, int h, int mx, int my) {   // 0 = esquerda, 1 = direita
    InvUI u = invLayout(w, h);
    for (int a = 0; a < 2; a++) { float x, y; if (pageBtnRect(u, a, &x, &y) && mx >= x && mx < x + 20 * u.gs && my >= y && my < y + 20 * u.gs) return a; }
    return -1;
}
static int hitTab(int w, int h, int mx, int my) {
    InvUI u = invLayout(w, h);
    for (int t = 0; t < (int)TABS.size(); t++) { float x, y; if (tabRect(u, t, &x, &y) && mx >= x && mx < x + 28 * u.gs && my >= y && my < y + 32 * u.gs) return t; }
    return -1;
}
static bool scrollRect(const InvUI& u, float* x, float* y, float* w, float* h) {   // trilho do Java: x=175, y=18, 14x112
    if (!creative || gTable || gChest || ctab == TAB_INV) return false;
    *x = u.ox + 175 * u.gs; *y = u.oy + 18 * u.gs; *w = 14.f * u.gs; *h = 112.f * u.gs; return true;
}
static void scrollTo(int w, int h, int my) {
    InvUI u = invLayout(w, h); float x, y, ww, hh; if (!scrollRect(u, &x, &y, &ww, &hh)) return;
    int mr = maxScrollRows(); if (mr <= 0) { cscroll = 0; return; }
    float f = (my - y - 7.5f * u.gs) / (97.f * u.gs); cscroll = std::max(0, std::min(mr, (int)floorf(f * mr + .5f)));   // (altura - 15), igual ao Java
}
// ---- toque (Bedrock): tocar seleciona (moldura), tocar noutro slot move; arrastar leva a pilha; nada fica "solto" no cursor ----
static bool tchPal(int id) { return id >= SL_PAL && id < SL_PAL + 45; }
static bool tchHas(int id) { return tchPal(id) ? palItem(id) != 0 : slotView(id).id != 0; }
static void tchPick(int id, bool half) { const int b = half ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT; if (tchPal(id)) paletteClick(id, b, false); else invClick(id, b, false); }
static void tchPlace(int dest, int origin) {   // solta a pilha da mão em dest; o que sobrar volta à origem
    if (cur.id && dest >= 0 && dest != SL_OUT && !tchPal(dest)) invClick(dest, SDL_BUTTON_LEFT, false);
    if (cur.id) { if (tchPal(origin)) cur = Slot(); else invClick(origin, SDL_BUTTON_LEFT, false); }
    if (cur.id) returnCursor();
}
static bool tchDown(int w, int h, int mx, int my, Uint32 now) {   // true = tocou num slot
    const int hs = hitSlot(w, h, mx, my);
    if (hs < 0) { if (!inPanel(w, h, mx, my)) gSelSlot = -1; return false; }
    gTch = TchFinger(); gTch.on = true; gTch.slot = hs; gTch.x0 = gTch.lx = mx; gTch.y0 = gTch.ly = my; gTch.w = w; gTch.h = h; gTch.t0 = now; gTch.cs0 = cscroll;
    return true;
}
static void tchMove(int mx, int my) {
    gTch.lx = mx; gTch.ly = my; if (gTch.drag || gTch.scroll) { if (gTch.scroll) { const InvUI u = invLayout(gTch.w, gTch.h); cscroll = std::max(0, std::min(maxScrollRows(), gTch.cs0 - (int)lroundf((my - gTch.y0) / (18.f * u.gs)))); } return; }
    const InvUI u = invLayout(gTch.w, gTch.h); const float dx = (float)(mx - gTch.x0), dy = (float)(my - gTch.y0), th = 6.f * u.gs;
    if (hypotf(dx, dy) < th) return;
    const Uint32 el = SDL_GetTicks() - gTch.t0;
    if (tchPal(gTch.slot) && el < 250 && fabsf(dy) > fabsf(dx)) { gTch.scroll = true; return; }   // paleta: arrastar na vertical rola a lista
    if (!tchHas(gTch.slot) || cur.id) { gTch.on = false; return; }
    gSelSlot = -1; tchPick(gTch.slot, false); if (cur.id) gTch.drag = true; else gTch.on = false;
}
static void tchUp() {
    if (!gTch.on) return; gTch.on = false;
    const int dest = hitSlot(gTch.w, gTch.h, gTch.lx, gTch.ly);
    if (gTch.drag) { tchPlace(dest, gTch.slot); gTch.drag = false; return; }
    if (gTch.scroll || dest != gTch.slot) return;
    if (dest == SL_OUT) { invClick(SL_OUT, SDL_BUTTON_LEFT, true); gSelSlot = -1; return; }   // tocar no resultado = fabricar para a mochila
    if (gSelSlot < 0 || tchPal(dest) || !tchHas(gSelSlot)) {
        gSelSlot = tchHas(dest) ? dest : -1; gSelHalf = gSelSlot >= 0 && !tchPal(dest) && slotView(dest).n > 1 && SDL_GetTicks() - gTch.t0 > 450;   // toque longo = metade
        return;
    }
    if (dest == gSelSlot) { gSelSlot = -1; gSelHalf = false; return; }
    tchPick(gSelSlot, gSelHalf); tchPlace(dest, gSelSlot); gSelSlot = -1; gSelHalf = false;
}
static void invMouseDown(int btn, int w, int h, int mx, int my, bool shift, Uint32 now) {
    if (gChest && gChWait) return;
    if (gInvTouch && btn == SDL_BUTTON_LEFT && tchDown(w, h, mx, my, now)) return;
    if (btn == SDL_BUTTON_LEFT) {
        { int ar = hitTabArrow(w, h, mx, my); if (ar >= 0) { tabPage = std::max(0, std::min(tabPages() - 1, tabPage + (ar ? 1 : -1))); return; } }
        int t = hitTab(w, h, mx, my); if (t >= 0) { if (t != ctab) { returnCursor(); setTab(t); } return; }
        InvUI u = invLayout(w, h); float sx, sy, sw, sh;
        if (scrollRect(u, &sx, &sy, &sw, &sh) && mx >= sx && mx < sx + sw && my >= sy && my < sy + sh) { scrollDrag = true; scrollTo(w, h, my); return; }
    }
    int hs = hitSlot(w, h, mx, my);
    if (hs < 0) {
        if (!inPanel(w, h, mx, my) && cur.id) {   // fora do painel: solta o item no chão (esquerdo = pilha, direito = 1)
            if (btn == SDL_BUTTON_RIGHT && cur.n > 1) { throwQ.push_back({cur.id, 1}); cur.n--; } else { throwQ.push_back(cur); cur = Slot(); } }
        return;
    }
    if (hs >= SL_PAL && hs < SL_PAL + 45) {
        if (btn == SDL_BUTTON_MIDDLE) { int it = palItem(hs); if (!cur.id && it) cur = {it, maxStack(it)}; }
        else if (btn == SDL_BUTTON_LEFT || btn == SDL_BUTTON_RIGHT) paletteClick(hs, btn, shift);
        return;
    }
    if (btn == SDL_BUTTON_MIDDLE) { if (creative && !cur.id && slotRef(hs) && slotRef(hs)->id) { cur = *slotRef(hs); cur.n = maxStack(cur.id); } return; }
    if (btn != SDL_BUTTON_LEFT && btn != SDL_BUTTON_RIGHT) return;
    if (btn == SDL_BUTTON_LEFT && shift && hs != SL_OUT && hs != SL_TRASH) {   // shift+duplo clique (GuiContainer.shiftClickedSlot): move todos os iguais do mesmo inventario
        static Slot shiftItem; static bool shiftOk = false;
        const auto grp = [](int i) { return i >= 0 && i < 36 ? 0 : i >= SL_CHEST && i < SL_CHEST + 54 ? 1 : i >= SL_CRAFT && i < SL_CRAFT + 9 ? 2 : i >= SL_ARMOR && i < SL_ARMOR + 4 ? 3 : -1; };
        if (shiftOk && lastClickSlot == hs && now - lastClickT < 250 && grp(hs) >= 0) {
            std::vector<int> ids; for (int i = 0; i < 36; i++) ids.push_back(i); for (int i = 0; i < 54; i++) ids.push_back(SL_CHEST + i); for (int i = 0; i < 9; i++) ids.push_back(SL_CRAFT + i); for (int i = 0; i < 4; i++) ids.push_back(SL_ARMOR + i);
            for (int i : ids) { Slot* x = slotRef(i); if (x && grp(i) == grp(hs) && x->id == shiftItem.id && x->d == shiftItem.d) invClick(i, SDL_BUTTON_LEFT, true); }
            shiftOk = false; lastClickSlot = -1; return;
        }
        Slot* ss = slotRef(hs); shiftOk = ss && ss->id; if (shiftOk) shiftItem = *ss;
    }
    if (btn == SDL_BUTTON_LEFT && !shift && cur.id && hs != SL_OUT && lastClickSlot == hs && now - lastClickT < 250) {   // duplo clique
        collectAll(); lastClickSlot = -1; return;
    }
    lastClickSlot = hs; lastClickT = now;
    if (shift) { invClick(hs, btn, true); return; }
    if (cur.id && dragOk(hs)) { dragBtn = btn; dragSlots.assign(1, hs); return; }   // pode virar arrasto; decide ao soltar
    invClick(hs, btn, false);
}
static void invMouseMotion(int w, int h, int mx, int my) {
    if (gInvTouch && gTch.on) { tchMove(mx, my); return; }
    if (scrollDrag) { scrollTo(w, h, my); return; }
    if (dragBtn < 0 || !cur.id) return;
    int hs = hitSlot(w, h, mx, my);
    if (hs < 0 || !dragOk(hs) || std::find(dragSlots.begin(), dragSlots.end(), hs) != dragSlots.end()) return;
    Slot* s = slotRef(hs);
    if (s && (!s->id || (sameStack(*s, cur) && s->n < maxStack(cur.id))) && (int)dragSlots.size() < cur.n) dragSlots.push_back(hs);
}
static int dragPer() { int c = (int)dragSlots.size(); return c < 2 ? 0 : dragBtn == SDL_BUTTON_LEFT ? cur.n / c : 1; }
static void invMouseUp(int btn, int, int, int, int) {
    if (gInvTouch && btn == SDL_BUTTON_LEFT && gTch.on) { tchUp(); return; }
    if (btn == SDL_BUTTON_LEFT) scrollDrag = false;
    if (dragBtn < 0 || btn != dragBtn) return;
    if (dragSlots.size() == 1) invClick(dragSlots[0], dragBtn, false);
    else if (dragSlots.size() > 1 && cur.id) {
        int per = dragPer();
        for (int id : dragSlots) {
            Slot* dp = slotRef(id); if (!dp) continue; Slot& d = *dp;
            int put = std::min(std::min(per, maxStack(cur.id) - (sameStack(d, cur) ? d.n : 0)), cur.n);
            if (put > 0) { d.id = cur.id; d.nbt = cur.nbt; d.n += put; cur.n -= put; }
        }
        if (cur.n <= 0) cur = Slot();
        updateCraft();
    }
    dragBtn = -1; dragSlots.clear();
}
static void invDropKey(int hs, bool all) {   // Q sobre um slot (GuiContainer: THROW): 1 item, Ctrl = a pilha; so com o cursor vazio. Resultado do craft: so Ctrl (1 fabricacao)
    if (cur.id || (gChest && gChWait) || hs < 0) return;
    if (hs >= SL_PAL && hs < SL_PAL + 45) { if (!creative) return; const int it = palItem(hs); if (it) throwQ.push_back({it, all ? maxStack(it) : 1}); return; }   // paleta do criativo: Q solta 1, Ctrl+Q uma pilha (GuiContainerCreative THROW)
    Slot* s = slotRef(hs); if (!s || !s->id) return;
    if (hs == SL_OUT) { if (!all) return; throwQ.push_back(craftOut); consumeCraft(); updateCraft(); return; }
    Slot t = *s; t.n = all ? s->n : 1; throwQ.push_back(t); s->n -= t.n; if (s->n <= 0) *s = Slot();
    if (isCraft(hs)) updateCraft();
}
static void invOffhandKey(int hs) {   // F sobre um slot: troca com a mao secundaria (SWAP 40); so com o cursor vazio
    if (cur.id || (gChest && gChWait) || hs < 0 || (hs >= SL_PAL && hs < SL_PAL + 45) || hs == SL_OFF) return;
    Slot* s = slotRef(hs); if (!s) return;
    if (hs == SL_OUT) { if (craftOut.id && !offh.id) { offh = craftOut; consumeCraft(); updateCraft(); } return; }
    if (isArmorSlot(hs) && offh.id && !armorSlotOk(hs - SL_ARMOR, offh.id)) return;
    std::swap(offh, *s); if (isCraft(hs)) updateCraft();
}
static void invNumberKey(int n, int hs) {
    if (gChest && gChWait) return;
    if (cur.id) return;   // GuiContainer.checkHotbarKeys: so com o cursor vazio
    if (hs >= SL_PAL && hs < SL_PAL + 45) { int it = palItem(hs); if (it) inv[n] = {it, maxStack(it)}; }
    else if (hs >= 0) numberSwap(n, hs);
}

static void drawTabs(const InvUI& u, bool selectedPass) {
    // Aba 28x32 (como o Java). Topo: y = -28..4 ; base: y = ph-4..ph+28. 4 unidades ficam sob/sobre o painel. Sem destaque ao passar o mouse (o Java só mostra o nome).
    for (int t = 0; t < (int)TABS.size(); t++) {
        const bool sel = t == ctab; if (sel != selectedPass) continue;
        float px, py; if (!tabRect(u, t, &px, &py)) continue;
        int row, col; tabPos(t, &row, &col); const bool bottomTab = row == 1;
        const float ux = (px - u.ox) / u.gs, uy = (py - u.oy) / u.gs;
        if (GLuint ts = rtTab[bottomTab ? 1 : 0][sel ? 1 : 0][std::max(0, std::min(6, col))]) {   // sprite 26x32 do Rethoughted GUI dentro da celula de 28
            uSpr(ts, ux + 1, uy, 26, 32);
            drawIcon(TABS[t].icon, OX + (ux + 6) * G, OY + (uy + (bottomTab ? 7 : 9)) * G, 16.f * G);
            continue;
        }
        const unsigned fill = sel ? 0xC6C6C6 : 0x8B8B8B, hi = 0xFFFFFF, sh = sel ? 0x555555 : 0x373737;
        if (!bottomTab) {
            U(ux + 1, uy, 26, 1, 0x000000);                    // topo
            U(ux, uy + 1, 1, 27, 0x000000); U(ux + 27, uy + 1, 1, 27, 0x000000);   // laterais (so ate a borda do painel)
            U(ux + 1, uy + 1, 26, sel ? 31 : 27, fill);        // selecionada desce 4 e apaga a borda do painel
            U(ux + 1, uy + 1, 26, 1, hi); U(ux + 1, uy + 2, 1, 26, hi);
            U(ux + 26, uy + 2, 1, 26, sh);
            if (!sel) U(ux + 1, uy + 28, 26, 1, 0x000000);       // base da aba fechada
        } else {
            U(ux + 1, uy + 31, 26, 1, 0x000000);
            U(ux, uy + 4, 1, 27, 0x000000); U(ux + 27, uy + 4, 1, 27, 0x000000);
            U(ux + 1, sel ? uy : uy + 4, 26, sel ? 31 : 27, fill);
            U(ux + 1, uy + 4, 1, 26, hi); U(ux + 26, uy + 4, 1, 26, sh); U(ux + 1, uy + 30, 26, 1, sh);
            if (!sel) U(ux + 1, uy + 3, 26, 1, 0x000000);
        }
        drawIcon(TABS[t].icon, OX + (ux + 6) * G, OY + (uy + (bottomTab ? 7 : 9)) * G, 16.f * G);   // Java: ícone em (x+6, y+9) em cima / (x+6, y+7) embaixo
    }
}
static void uButton(float x, float y, float w, float h, bool on, bool hov, const char* label) {   // botão padrão do Java 20x20
    if (rtBtn[0]) { uNine(rtBtn[!on ? 2 : hov ? 1 : 0], 200, 20, 20, 4, 20, 4, x, y, w, h); uText(x + (w - textWidth(label, 1.f)) / 2, y + (h - 8) / 2, label, on ? 0xFFFFFF : 0xA0A0A0, true); return; }
    U(x, y, w, h, 0x000000);
    U(x + 1, y + 1, w - 2, h - 2, !on ? 0x4A4A4A : hov ? 0x7A7AA6 : 0x707070);
    if (on) { U(x + 1, y + 1, w - 2, 1, 0xA8A8A8); U(x + 1, y + 1, 1, h - 2, 0xA8A8A8); U(x + 1, y + h - 3, w - 2, 2, 0x3C3C3C); U(x + w - 2, y + 2, 1, h - 4, 0x3C3C3C); }
    uText(x + (w - textWidth(label, 1.f)) / 2, y + (h - 8) / 2, label, on ? 0xE0E0E0 : 0xA0A0A0, true);
}
static void uArrowBtn(float x, float y, bool on, bool hov, bool right) {   // botão 20x20 com seta em pixel-art (mesmo desenho nos dois lados, só espelhado)
    uButton(x, y, 20, 20, on, hov, "");
    const unsigned fg = !on ? 0x707070 : hov ? 0xFFFFA0 : 0xFFFFFF, sh = !on ? 0x2A2A2A : 0x3F3F3F;
    const float ax = x + (right ? 7.f : 8.f), ay = y + 6.f;   // seta 5x7 centrada (a de baixo-direita do botão compensa o relevo)
    for (int pass = 0; pass < 2; pass++) {
        const float o = pass == 0 ? 1.f : 0.f; const unsigned c = pass == 0 ? sh : fg;
        for (int k = 0; k < 4; k++) {
            const float cx = right ? ax + k : ax + 3 - k;
            U(cx + o, ay + k + o, 2, 1, c);        // metade de cima
            if (k < 3) U(cx + o, ay + 6 - k + o, 2, 1, c);   // metade de baixo
        }
    }
}
static void drawPager(const InvUI& u, int w, int h, int mx, int my) {   // "<  1 / 2  >" acima das abas, só quando há mais de uma página (mods)
    if (tabPages() < 2) return;
    const int hov = hitTabArrow(w, h, mx, my);
    uArrowBtn(0, -50, tabPage > 0, hov == 0, false);
    uArrowBtn(u.pw - 20.f, -50, tabPage < tabPages() - 1, hov == 1, true);
    char b[24]; snprintf(b, sizeof b, "%d / %d", tabPage + 1, tabPages());
    uText((u.pw - textWidth(b, 1.f)) / 2, -44, b, 0xFFFFFF, true);
}
static void drawShieldSilhouette(float ux, float uy) {   // silhueta 16x16 do escudo no slot vazio da mão secundária
    if (emptySlotTex[4]) { guiImage(emptySlotTex[4], OX + ux * G, OY + uy * G, 16.f * G, 16.f * G); return; }
    static const char* const S[16] = {"................", "...##########...", "...#oooooooo#...", "...#oooooooo#...", "...#oooooooo#...", "...#oooooooo#...", "...#oooooooo#...", "....#oooooo#....", "....#oooooo#....", ".....#oooo#.....", "......#oo#......", ".......##.......", "................", "................", "................", "................"};
    for (int r = 0; r < 16; r++) for (int c = 0; c < 16; c++) { char ch = S[r][c]; if (ch == '#') U(ux + c, uy + r, 1, 1, 0x373737); else if (ch == 'o') U(ux + c, uy + r, 1, 1, 0x7B7B7B, .55f); }
}
static void drawPlayerPreview(float ox, float oy, float gs, float mx, float my, float fxU = 51.f, float fyU = 75.f, float scU = 30.f, float cx = 26.f, float cy = 8.f, float cw = 50.f, float ch = 70.f);   // pés/escala/recorte em unidades da GUI

static void survDrawHud(int w, int h);   // survival.inc
static bool gNoCross = false;   // modo toque: sem a mira "+" (como o MCPE)
static void drawHud(int w, int h, int sel, bool invOpen, int mx, int my, const char* nameTxt, float nameA, const char* msgTxt, float msgA) {
    auto ortho = [&]() { glViewport(gVpX, gVpY, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity(); };
    ortho();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    G = guiScaleFor(w, h);
    const int sw = (w + G - 1) / G, sh = (h + G - 1) / G;
    if (!invOpen) {
        if (!gNoCross) {
            if (rtCross) {   // mira 15x15 com mistura invertida (como o Java)
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
                guiImage(rtCross, floorf((w - 15.f * G) / 2.f), floorf((h - 15.f * G) / 2.f), 15.f * G, 15.f * G);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            } else guiImage(guiCrosshairTex, floorf(w / 2.f) - 10.f, floorf(h / 2.f) - 10.f, 21, 21, .92f);
        }
        // ---- hotbar: 182x22 unidades ----
        OX = (float)(((sw - 182) / 2) * G); OY = (float)((sh - 22) * G);
        if (gGm == 3) {} else if (rtHotbar && rtHotSel) { uSpr(rtHotbar, 0, 0, 182, 22); uSpr(rtHotSel, -1.f + 20 * sel, -1, 24, 23); }   // hotbar 182x22 + seletor 24x23 do Rethoughted GUI
        else {
        U(0, 0, 182, 22, 0x000000, .6f);
        for (int i = 0; i < 9; i++) uBox(2 + 20 * i, 2, 18, 18, .9f, 0x6E6E6E);
        const float sx = -1 + 20 * sel;   // seletor 24x24
        uRing(sx, -1, 24, 24, 1, 0x000000); uRing(sx + 1, 0, 22, 22, 2, 0xFFFFFF);
        }
        if (gGm != 3 && offh.id) {   // slot da mao esquerda ao lado da hotbar (como o Minecraft)
            U(-29, 0, 22, 22, 0x000000, .6f); uBox(-27, 2, 18, 18, .9f, 0x6E6E6E);
            drawStack(OX + (-26) * G, OY + 3 * G, offh.id, offh.n);
        }
        if (gGm != 3) for (int i = 0; i < 9; i++) drawStack(OX + (3 + 20 * i) * G, OY + 3 * G, inv[i].id, inv[i].n);
        survDrawHud(w, h);   // corações, fome, flash de dano e tela de morte (só sobrevivência)
        if (nameA > 0 && *nameTxt) { float sc = tsc(); text(floorf((w - textWidth(nameTxt, sc)) / 2), (float)((sh - 48) * G), sc, nameTxt, 1, 1, 1, (float)G); }
    }
    (void)msgTxt; (void)msgA;
    if (invOpen) {
        InvUI u = invLayout(w, h); OX = u.ox; OY = u.oy; G = u.gs;   // toque: escala própria do inventário (maior)
        flat(0, 0, (float)w, (float)h, 16 / 255.f, 16 / 255.f, 16 / 255.f, .77f);   // fundo escurecido (0xC0101010..0xD0101010)
        std::vector<int> ids = activeSlots(u);
        int hov = hitSlot(w, h, mx, my);
        if (gInvTouch) { if (gSelSlot >= 0 && !tchHas(gSelSlot)) gSelSlot = -1; hov = gSelSlot; int tx, ty; if (hov >= 0 && !cur.id && slotUnits(u, hov, &tx, &ty)) { mx = (int)(u.ox + (tx + 8) * G); my = (int)(u.oy + (ty + 8) * G); } }   // toque: sem "hover"; o tooltip é do slot selecionado
        const int hovTab = hitTab(w, h, mx, my);
        if (creative && !gTable && !gChest) drawTabs(u, false);
        if (u.surv && gChest) {   // GUI do bau: cabecalho + miolo liso + mochila recortados do Rethoughted GUI (mesa de trabalho); sem ele, painel padrao
            const int ph = u.ph;
            if (rtCraft) {
                uTexRegion(rtCraft, 0, 0, 176, 16, 0, 0, 176, 16, 256, 256);
                uTexRegion(rtCraft, 0, 16, 176, (float)(ph - 95 - 16), 0, 73, 176, 8, 256, 256);
                uTexRegion(rtCraft, 0, (float)(ph - 95), 176, 94, 0, 72, 176, 94, 256, 256);   // 1 px acima: no baú a mochila começa em y=85 (na mesa, 84), senão a moldura dos slots ficava 1 px abaixo da seleção
                uTexRegion(rtCraft, 0, (float)(ph - 1), 176, 1, 0, 165, 176, 1, 256, 256);   // borda inferior
                for (int k = 0; k < gChRows * 9; k++) { const int sx = 8 + 18 * (k % 9), sy = 18 + 18 * (k / 9); if (rtSlot) uSpr(rtSlot, sx - 1, sy - 1, 18, 18); else uBox(sx - 1, sy - 1, 18, 18); }
            } else { uPanel(0, 0, 176, (float)ph); for (int id : ids) { int x, y; slotUnits(u, id, &x, &y); uBox(x - 1, y - 1, 18, 18); } }
            uText(8, 6, gChEnder ? "Baú do Ender" : gChRows == 6 ? "Baú grande" : "Baú", rtCraft ? UI_TXT : 0x404040); uText(8, (float)(ph - 94), "Inventário", rtCraft ? UI_TXT : 0x404040);
        } else if (u.surv && gTable && rtCraft) {
            uTexRegion(rtCraft, 0, 0, 176, 166, 0, 0, 176, 166, 256, 256);   // fundo, slots, seta e resultado ja estao na textura
            uText(29, 6, "Fabricação", UI_TXT); uText(8, 72, "Inventário", UI_TXT);
        } else if (u.surv && gTable) {
            uPanel(0, 0, 176, 166);
            uText(28, 6, "Fabricação", 0x404040); uText(8, 72, "Inventário", 0x404040);
            for (int id : ids) { int x, y; slotUnits(u, id, &x, &y); if (id != SL_OUT) uBox(x - 1, y - 1, 18, 18); }
            {   // seta (90..111, 35..53) e slot de resultado grande 26x26 em (118,31)
                U(90, 41, 12, 5, 0x8B8B8B);   // seta centrada na linha y=43,5 do slot de resultado
                static const int hh[6] = {11, 9, 7, 5, 3, 1};
                for (int k = 0; k < 6; k++) U(102 + k, 43.5f - hh[k] / 2.f, 1, (float)hh[k], 0x8B8B8B);
                uBox(119, 30, 26, 26);
            }
        } else if (u.surv && rtInv) {
            uTexRegion(rtInv, 0, 0, 176, 166, 0, 0, 176, 166, 256, 256);   // moldura do boneco, armaduras, 2x2, seta, resultado, mao secundaria, mochila e hotbar
            { drawPlayerPreview(OX, OY, (float)G, (float)mx, (float)my); ortho(); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); }
            for (int id : ids) if (isArmorSlot(id) && !slotView(id).id) { int x, y; slotUnits(u, id, &x, &y); drawArmorSilhouette(id - SL_ARMOR, (float)x, (float)y - 2); }
            if (!offh.id) drawShieldSilhouette(77, 62);
        } else if (u.surv) {
            uPanel(0, 0, 176, 166);
            uBox(25, 7, 51, 72, 1.f, 0x000000);   // moldura do boneco
            { drawPlayerPreview(OX, OY, (float)G, (float)mx, (float)my); ortho(); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); }
            {   // seta centrada entre a grade e o resultado (linha central y=44,5, a mesma do slot de saída)
                U(127, 42, 7, 5, 0x8B8B8B);
                static const int hh[6] = {11, 9, 7, 5, 3, 1};
                for (int k = 0; k < 6; k++) U(134 + k, 44.5f - hh[k] / 2.f, 1, (float)hh[k], 0x8B8B8B);
            }
            for (int id : ids) { int x, y; slotUnits(u, id, &x, &y); if (id != SL_OUT) uBox(x - 1, y - 1, 18, 18); }
            for (int id : ids) if (isArmorSlot(id) && !slotView(id).id) { int x, y; slotUnits(u, id, &x, &y); drawArmorSilhouette(id - SL_ARMOR, (float)x, (float)y - 2); }
            uBox(143, 35, 18, 18);   // mesmo tamanho dos outros slots; item em (144,36) = área de clique
        } else {
            const GLuint cbg = u.cinv ? rtCrInv : ctab == TAB_SEARCH ? rtCrSearch : rtCrItems;
            if (cbg) uTexRegion(cbg, 0, 0, (float)u.pw, (float)u.ph, 0, 0, (float)u.pw, (float)u.ph, 256, 256); else uPanel(0, 0, u.pw, u.ph);
            if (u.cinv) {   // aba "Inventário de sobrevivência" do criativo: boneco, 4 armaduras, mão secundária, mochila, hotbar e lixeira
                if (!cbg) uBox(73, 6, 32, 43, 1.f, 0x000000);
                { drawPlayerPreview(OX, OY, (float)G, (float)mx, (float)my, 89.f, 45.f, 20.f, 74.f, 7.f, 30.f, 41.f); ortho(); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); }
                for (int id : ids) {
                    int x, y; slotUnits(u, id, &x, &y);
                    if (id == SL_TRASH) {   // slot de destruir item (X sobre fundo rosado)
                        if (!cbg) uBox(x - 1, y - 1, 18, 18, 1.f, 0xB89494); else if (rtSlot) uSpr(rtSlot, x - 1, y - 1, 18, 18);
                        for (int k = 0; k < 12; k++) { U(x + 2 + k, y + 2 + k, 2, 1, 0x202020); U(x + 12 - k, y + 2 + k, 2, 1, 0x202020); }
                    } else if (!cbg) uBox(x - 1, y - 1, 18, 18);
                }
                for (int id : ids) if (isArmorSlot(id) && !slotView(id).id) { int x, y; slotUnits(u, id, &x, &y); drawArmorSilhouette(id - SL_ARMOR, (float)x, (float)y - 2); }
                if (!offh.id) drawShieldSilhouette(35, 20);
            } else {
                uText(8, 6, TABS[ctab].name, cbg ? UI_TXT : 0x404040);
                if (ctab == TAB_SEARCH) {   // caixa de texto (Java: x=80, y=4)
                    if (!cbg) { U(80, 4, 91, 12, 0xA0A0A0); U(81, 5, 89, 10, 0x000000); }
                    std::string q = searchQ + ((SDL_GetTicks() / 400) % 2 ? "_" : " ");
                    uText(83, 6, q.c_str(), 0xFFFFFF);
                }
                if (!cbg) for (int id : ids) { int x, y; slotUnits(u, id, &x, &y); uBox(x - 1, y - 1, 18, 18); }
                if (cbg && rtScr[0]) { const int mr = maxScrollRows(); uSpr(rtScr[mr ? 1 : 0], 175, 18 + (mr ? 95.f * cscroll / mr : 0), 12, 15); } else
                { const int mr = maxScrollRows(); const float hy = 18 + (mr ? 95.f * cscroll / mr : 0);   // trilho rebaixado + cursor 12x15 (cinza escuro quando não há o que rolar)
                  uBox(174, 17, 14, 114);
                  U(175, hy, 12, 15, mr ? 0xC6C6C6 : 0x9A9A9A);
                  U(175, hy, 12, 1, mr ? 0xFFFFFF : 0xB8B8B8); U(175, hy, 1, 15, mr ? 0xFFFFFF : 0xB8B8B8);
                  U(175, hy + 14, 12, 1, mr ? 0x555555 : 0x6B6B6B); U(186, hy, 1, 15, mr ? 0x555555 : 0x6B6B6B); }
            }
        }
        if (creative && !gTable && !gChest) { drawTabs(u, true); drawPager(u, w, h, mx, my); }
        const int per = dragPer();
        if (gInvTouch && gSelSlot >= 0) {   // moldura de seleção (a mesma da hotbar) 24x23 em volta do slot
            int sx, sy; if (slotUnits(u, gSelSlot, &sx, &sy)) {
                if (rtHotSel) uSpr(rtHotSel, sx - 4.f, sy - 4.f, 24, 23); else { uRing(sx - 4.f, sy - 4.f, 24, 24, 1, 0x000000); uRing(sx - 3.f, sy - 3.f, 22, 22, 2, 0xFFFFFF); }
                if (gSelHalf) uText((float)sx, sy - 12.f, "1/2", 0xFFFF55, true);
            }
        }
        for (int id : ids) {
            int x, y; slotUnits(u, id, &x, &y);
            float px = OX + x * G, py = OY + y * G;
            Slot s = slotView(id); int n = s.n;
            bool inDrag = std::find(dragSlots.begin(), dragSlots.end(), id) != dragSlots.end() && dragSlots.size() > 1;
            if (inDrag) { s.id = cur.id; n = (slotRef(id)->id == cur.id ? slotRef(id)->n : 0) + per; }
            drawStack(px, py, s.id, n);
            if ((id == hov && !gInvTouch) || inDrag) { glDisable(GL_TEXTURE_2D); glColor4f(1, 1, 1, inDrag ? .35f : .5f); quad(px, py, 16.f * G, 16.f * G, 0, 0, 0, 0); glEnable(GL_TEXTURE_2D); }
        }
        if (cur.id) {
            int rem = cur.n - (dragSlots.size() > 1 ? per * (int)dragSlots.size() : 0);
            drawStack(floorf((float)mx / G) * G - 8 * G, floorf((float)my / G) * G - 8 * G, cur.id, rem);
        } else if ((hov >= 0 && !(hov == SL_OUT && !craftOut.id)) || hovTab >= 0) {
            int tid = hovTab >= 0 || hov == SL_TRASH ? 1 : slotView(hov).id;
            if (tid) {
                const char* nm = hovTab >= 0 ? TABS[hovTab].name : hov == SL_TRASH ? "Destruir item" : itemName(tid); float sc = tsc(); const std::string l2 = (hovTab >= 0 || hov == SL_TRASH) ? std::string() : foodTip(tid); float tw = std::max(textWidth(nm, sc), l2.empty() ? 0.f : textWidth(l2.c_str(), sc)), th = (l2.empty() ? 8.f : 17.f) * G;   // 2a linha: fome/saturacao da comida
                float x = floorf((float)mx / G) * G + 12 * G, y = floorf((float)my / G) * G - 12 * G;
                if (rtTipBg && rtTipFr) {   // tooltip do Rethoughted GUI: fundo (borda 9) + moldura (borda 10), 12 unidades alem do texto
                    const float ux = (x - OX) / G - 12, uy = (y - OY) / G - 12, uw = tw / G + 24, uh = th / G + 24;
                    uNine(rtTipBg, 100, 100, 9, 9, 9, 9, ux, uy, uw, uh); uNine(rtTipFr, 100, 100, 10, 10, 10, 10, ux, uy, uw, uh);
                } else {
                flat(x - 3 * G, y - 4 * G, tw + 6 * G, th + 8 * G, 16 / 255.f, 0, 16 / 255.f, .94f);
                flat(x - 4 * G, y - 3 * G, tw + 8 * G, th + 6 * G, 16 / 255.f, 0, 16 / 255.f, .94f);
                float bx = x - 3 * G, by = y - 3 * G, bw = tw + 6 * G, bh = th + 6 * G;
                flat(bx, by, bw, G, .15f, 0, .42f, 1); flat(bx, by + bh - G, bw, G, .15f, 0, .42f, 1);
                flat(bx, by + G, G, bh - 2 * G, .15f, 0, .42f, 1); flat(bx + bw - G, by + G, G, bh - 2 * G, .15f, 0, .42f, 1);
                }
                if (l2.empty()) text(x, y + floorf((th - 7 * sc) / 2), sc, nm, 1, 1, 1, (float)G);
                else { text(x, y + floorf((8.f * G - 7 * sc) / 2), sc, nm, 1, 1, 1, (float)G); text(x, y + 9.f * G + floorf((8.f * G - 7 * sc) / 2), sc, l2.c_str(), 1.f, .85f, .3f, (float)G); }
            }
        }
    }
    glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
