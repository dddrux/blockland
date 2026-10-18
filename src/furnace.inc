// ---------- FORNALHA (patchFur.1): fundir, combustivel, tela, luz, salvar ----------
// A fornalha e o bloco do pack "block/furnace" (gFurnId, ver pkblocks.inc). Meta: bits 0-1 = frente; bit 2 = ACESA (textura furnace_front_on + luz 13).
// Conteudo por posicao em gFurns (entrada, combustivel, resultado + tempos), como o TileEntityFurnace do 1.12.2: 200 ticks por item,
// combustivel gasta-se por tempo, sem fogo o progresso volta 2 por tick. Solta o conteudo ao quebrar. Salva em saves/<mundo>/furnaces.dat.
// Multiplayer: host manda; convidado abre via NP_CHESTREQ/NP_CHEST (mesmo formato do bau, sem mudar NET_VER). Limites: XP (patchFur.3); sem funil; jogador 2 do splitscreen nao abre.
struct FurnData { Slot s[3]; int burn = 0, burnMax = 0, cook = 0; float xp = 0.f; };   // s[0] entrada, s[1] combustivel, s[2] resultado
static std::map<ChestKey, FurnData> gFurns;
static int gFurnPos[3] = {0, 0, 0};
static const int FURN_COOK = 200;
static int fidv(const char* names) {   // "a|b|c": primeiro nome que existe como item/ ou block/ do pack
    std::string t = names; size_t p = 0;
    while (p <= t.size()) { size_t e = t.find('|', p); if (e == std::string::npos) e = t.size(); const std::string n = t.substr(p, e - p); p = e + 1;
        if (n.empty()) continue; for (const char* pre : {"item/", "block/"}) { auto it = gPkByFile.find(pre + n); if (it != gPkByFile.end()) return it->second; } }
    return 0;
}
static std::string furnBase(int id) { if (!isPk(id)) return std::string(); std::string f = PK[id - PK0].file; const size_t s = f.rfind('/'); return s == std::string::npos ? f : f.substr(s + 1); }
static const std::map<int, int>& furnRecipes() {   // entrada -> resultado (SMELTING 1.12.2); refeita se o pack mudou
    static std::map<int, int> M; static size_t sz = (size_t)-1;
    if (sz == PK.size() && !M.empty()) return M;
    sz = PK.size(); M.clear();
    auto add = [&](int a, int b) { if (a > 0 && b > 0) M[a] = b; };
    add(IRONORE, IT_IRON); add(GOLDORE, IT_GOLD); add(DIAMONDORE, IT_DIAMOND); add(COALORE, IT_COAL); add(REDSTONEORE, IT_REDSTONE); add(LAPISORE, IT_LAPIS);
    add(COBBLE, STONE); add(SAND, fidv("glass")); add(LOG, fidv("charcoal|coal_charcoal")); add(CLAY, fidv("hardened_clay|terracotta"));
    add(IT_BEEF, IT_STEAK); add(IT_PORK, IT_CPORK); add(IT_CHICKEN, IT_CCHICKEN); add(IT_MUTTON, IT_CMUTTON);
    static const char* P[][2] = {{"iron_ore", "iron_ingot"}, {"gold_ore", "gold_ingot"}, {"emerald_ore", "emerald"}, {"diamond_ore", "diamond"}, {"coal_ore", "coal"},
        {"redstone_ore", "redstone"}, {"lapis_ore", "dye_powder_blue|lapis_lazuli"}, {"quartz_ore|nether_quartz_ore", "quartz"}, {"cobblestone", "stone"}, {"stonebrick|stone_bricks", "stonebrick_cracked|cracked_stone_bricks"},
        {"potato", "potato_baked|baked_potato"}, {"beef_raw|beef", "beef_cooked|cooked_beef"}, {"porkchop_raw|porkchop", "porkchop_cooked|cooked_porkchop"}, {"chicken_raw|chicken", "chicken_cooked|cooked_chicken"},
        {"mutton_raw|mutton", "mutton_cooked|cooked_mutton"}, {"rabbit_raw|rabbit", "rabbit_cooked|cooked_rabbit"}, {"fish_cod_raw|cod", "fish_cod_cooked|cooked_cod"},
        {"fish_salmon_raw|salmon", "fish_salmon_cooked|cooked_salmon"}, {"clay_ball", "brick"}, {"netherrack", "netherbrick|nether_brick"}, {"cactus", "dye_powder_green|green_dye"},
        {"sponge_wet|wet_sponge", "sponge"}, {"chorus_fruit", "chorus_fruit_popped|popped_chorus_fruit"}, {"sand", "glass"}, {"log_oak|oak_log", "charcoal|coal_charcoal"}, {"log_spruce|spruce_log", "charcoal|coal_charcoal"},
        {"log_birch|birch_log", "charcoal|coal_charcoal"}, {"log_jungle|jungle_log", "charcoal|coal_charcoal"}, {"log_acacia|acacia_log", "charcoal|coal_charcoal"}, {"log_big_oak|dark_oak_log", "charcoal|coal_charcoal"},
        {"clay", "hardened_clay|terracotta"}};
    for (auto& r : P) add(fidv(r[0]), fidv(r[1]));
    static const char* COL[16] = {"white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray", "silver|light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black"};
    for (const char* c : COL) { std::string a = std::string("hardened_clay_stained_") + c, b = std::string("glazed_terracotta_") + c;   // terracota colorida -> glazed (nomes 1.12 e 1.13)
        std::string t = c; const size_t bar = t.find('|'); const std::string c2 = bar == std::string::npos ? t : t.substr(0, bar), c3 = bar == std::string::npos ? t : t.substr(bar + 1);
        add(fidv((std::string("hardened_clay_stained_") + c2 + "|" + c3 + "_terracotta").c_str()), fidv((std::string("glazed_terracotta_") + c2 + "|" + c2 + "_glazed_terracotta|" + c3 + "_glazed_terracotta").c_str())); (void)a; (void)b; }
    return M;
}
static int furnResult(int id) { if (id <= 0) return 0; const auto& M = furnRecipes(); auto it = M.find(id); return it == M.end() ? 0 : it->second; }
static bool woodKind(const std::string& b) { for (const char* w : {"oak", "spruce", "birch", "jungle", "acacia", "planks", "wood"}) if (b.find(w) != std::string::npos) return true; return false; }
static int furnFuel(int id) {   // TileEntityFurnace.getItemBurnTime (ticks)
    if (id <= 0) return 0;
    if (id == gBktLava && id) return 20000;
    switch (id) { case LOG: case PLANKS: case BOOKSHELF: case CTABLE: return 300; case COALBLK: return 16000; case IT_COAL: return 1600; case IT_STICK: return 100; case WOOL: return 100;
        case IT_WPICK: case IT_WSWORD: case IT_WAXE: case IT_WSHOVEL: case IT_WHOE: return 200; case IT_BOW: return 300; default: break; }
    if (!isPk(id)) return 0;
    const std::string b = furnBase(id); auto has = [&](const char* s) { return b.find(s) != std::string::npos; };
    if (b == "blaze_rod") return 2400; if (b == "charcoal" || b == "coal_charcoal" || b == "coal") return 1600; if (b == "coal_block") return 16000; if (b == "lava_bucket") return 20000;
    if (b == "stick") return 100; if (b == "bowl") return 100; if (b == "dead_bush" || b == "deadbush") return 100; if (b == "bow" || b == "fishing_rod") return 300;
    if (b == "chest" || b == "trapped_chest" || b == "crafting_table" || b == "bookshelf" || b == "noteblock" || b == "jukebox" || b == "daylight_detector" || b == "ladder") return 300;
    if (has("sapling")) return 100; if (has("banner") && !has("pattern")) return 300; if (b.find("wool") != std::string::npos) return 100; if (has("carpet") && !has("moss")) return 67;
    if (has("wooden_")) return (has("pickaxe") || has("sword") || has("axe") || has("shovel") || has("hoe")) ? 200 : (has("door") ? 200 : 0);
    if (!woodKind(b) || has("nether") || has("iron") || has("stone")) return 0;
    if (has("planks")) return 300; if (has("slab") && !has("double")) return 150; if (has("stairs") || has("fence") || has("trapdoor") || has("pressure_plate")) return 300;
    if (has("button")) return 100; if (has("door")) return 200; if (has("sign")) return 200; if (has("boat")) return 1200;
    if (has("log") || has("wood")) return 300;
    return 0;
}
static FurnData* furnAt(int x, int y, int z, bool create = false) {
    if (!gFurnId || getB(x, y, z) != gFurnId) return nullptr;
    auto it = gFurns.find(ckey(x, y, z)); if (it != gFurns.end()) return &it->second;
    return create ? &gFurns[ckey(x, y, z)] : nullptr;
}
static float furnXp(int r) {   // XP por item fundido (FurnaceRecipes.getSmeltingExperience 1.12.2), pelo resultado
    if (r <= 0) return 0.f;
    if (r == IT_IRON || r == IT_REDSTONE) return 0.7f; if (r == IT_GOLD || r == IT_DIAMOND) return 1.0f; if (r == IT_COAL || r == STONE) return 0.1f; if (r == IT_LAPIS) return 0.2f;
    if (r == IT_STEAK || r == IT_CPORK || r == IT_CCHICKEN || r == IT_CMUTTON) return 0.35f;
    if (!isPk(r)) return 0.1f;
    const std::string b = furnBase(r); auto has = [&](const char* s) { return b.find(s) != std::string::npos; };
    if (has("emerald") || has("diamond") || has("gold_ingot")) return 1.0f; if (has("iron_ingot") || has("redstone")) return 0.7f; if (has("cooked") || has("baked") || has("_cooked")) return 0.35f;
    if (has("hardened_clay") || b == "terracotta") return 0.35f; if (b == "brick") return 0.3f; if (has("dye")) return 0.2f; if (has("quartz") || has("lapis")) return 0.2f;
    if (b == "charcoal" || has("coal_charcoal") || b == "sponge") return 0.15f; return 0.1f;
}
static void furnGiveXp(float share) {   // parte inteira + chance da fracao (como ExperienceOrb do TileEntityFurnace)
    if (share <= 0.f) return; int n = (int)share; if (frand() < share - n) n++; survAddXp(n);
}
static bool furnLit(const FurnData& f) { return f.burn > 0; }
static bool furnCanSmelt(const FurnData& f) {
    const int r = furnResult(f.s[0].id); if (!r || f.s[0].n <= 0) return false;
    const Slot& o = f.s[2]; if (!o.id || o.n <= 0) return true;
    return o.id == r && o.n < maxStack(r);
}
static void furnSetLit(int x, int y, int z, bool lit) {
    const int m = getM(x, y, z), nm = lit ? (m | 4) : (m & ~4); if (nm == m) return;
    const int sv = gRsLastMeta; setBM(x, y, z, gFurnId, nm); gRsLastMeta = sv; netSendBlock(x, y, z, gFurnId, nm);
}
// um tick de UMA fornalha (TileEntityFurnace.update). Devolve true se o estado aceso mudou.
static bool furnStep(FurnData& f) {
    const bool was = f.burn > 0; bool dirty = false;
    if (f.burn > 0) f.burn--;
    if (f.burn == 0 && furnCanSmelt(f) && f.s[1].id && f.s[1].n > 0) {   // acende: gasta 1 combustivel (balde de lava volta vazio)
        const int ft = furnFuel(f.s[1].id);
        if (ft > 0) { f.burn = f.burnMax = ft; dirty = true; Slot& fs = f.s[1]; if (fs.id == gBktLava && gBktEmpty) fs = {gBktEmpty, 1, 0}; else if (--fs.n <= 0) fs = Slot(); }
    }
    if (f.burn > 0 && furnCanSmelt(f)) {
        if (++f.cook >= FURN_COOK) {
            f.cook = 0; const int r = furnResult(f.s[0].id); Slot& o = f.s[2];
            if (!o.id || o.n <= 0) o = {r, 1, 0}; else o.n++;
            f.xp += furnXp(r);
            if (--f.s[0].n <= 0) f.s[0] = Slot(); dirty = true;
        }
    } else if (f.cook > 0) f.cook = std::max(0, f.cook - 2);
    (void)dirty; return was != (f.burn > 0);
}
static void furnWatchPush();
static void furnTick() {   // chamado por runWorldTick (so quem manda no mundo); so fornalhas da dimensao atual em chunks carregados
    if (gFurns.empty()) return;
    if (gWorldTick % 10 == 0) furnWatchPush();
    for (auto& kv : gFurns) {
        const int x = std::get<0>(kv.first), yy = std::get<1>(kv.first), z = std::get<2>(kv.first), y = yy & 4095;
        if ((yy >> 12) != gDim || !getC(fdiv(x, CW), fdiv(z, CW)) || getB(x, y, z) != gFurnId) continue;
        FurnData& f = kv.second;
        if (!f.burn && !f.cook && !(f.s[0].id && f.s[1].id)) { if (getM(x, y, z) & 4) furnSetLit(x, y, z, false); continue; }
        furnStep(f); furnSetLit(x, y, z, f.burn > 0); if (f.burn > 0 && rand() % 30 == 0) sndEvtAt("furnace.crackle", x + .5, y + .5, z + .5, .6f);   // patchAud.6: estalo do fogo
    }
}
static void furnOnRemoved(int x, int y, int z) {   // bloco saiu: solta o conteudo (so host/offline)
    auto it = gFurns.find(ckey(x, y, z)); if (it == gFurns.end()) return;
    if (!(netSock >= 0 && !netHost)) for (const Slot& s : it->second.s) if (s.id && s.n > 0) {
        int left = s.n; while (left > 0) { const int k = std::min(left, 1 + rand() % 21); left -= k;
            spawnDrop(x + .1 + frand() * .8, y + .1 + frand() * .8, z + .1 + frand() * .8, (frand() - .5) * .1, frand() * .1 + .2, (frand() - .5) * .1, s.id, k, 10, 0, s.nbt); } }
    gFurns.erase(it);
}
// ---- tela ----
static Slot* furnSlotRef(int k) {
    if (!gFurn || k < 0 || k > 2) return nullptr;
    FurnData* f = furnAt(gFurnPos[0], gFurnPos[1], gFurnPos[2], false); return f ? &f->s[k] : nullptr;
}
static bool furnGuest() { return netSock >= 0 && !netHost && netConn; }
// ---- multiplayer: o HOST manda; convidado pede (NP_CHESTREQ), recebe NP_CHEST e devolve o que mudou. Mesmo formato do bau: 27 pares
// (int32 id, u8 n); slots 0..2 = itens; slot 3 = chama 0..255, slot 4 = seta 0..255 (valores crus no campo id); cauda de NBT igual a do bau.
static std::map<ChestKey, Uint32> gFurnWatch;   // host: fornalhas que algum convidado tem aberta (empurra o progresso a cada 10 ticks por 30 s)
static FurnData gFurnSnap; static Uint32 gFurnReq = 0;
static bool furnSlotsEq(const FurnData& a, const FurnData& b) { for (int i = 0; i < 3; i++) if (a.s[i].id != b.s[i].id || a.s[i].n != b.s[i].n || a.s[i].nbt != b.s[i].nbt) return false; return true; }
static NBuf furnNetBuild(int x, int y, int z, const FurnData& f, bool progress) {
    ChestInv t; for (int i = 0; i < 3; i++) t.s[i] = f.s[i];
    for (int i = 0; i < 3; i++) if (t.s[i].id > 0 && t.s[i].n > 0) netSendNbt(t.s[i].nbt);
    NBuf b = netMk(NP_CHEST); b.put<int32_t>(x); b.put<int32_t>(y); b.put<int32_t>(z);
    const int flame = progress && f.burn > 0 ? std::max(1, std::min(255, f.burn * 255 / std::max(1, f.burnMax))) : 0, arrow = progress ? std::min(255, f.cook * 255 / FURN_COOK) : 0;
    for (int i = 0; i < CHEST_SLOTS; i++) {
        if (i < 3) { const Slot& s = t.s[i]; const bool ok = s.id > 0 && s.n > 0; b.put<int32_t>(ok ? netWireId(s.id) : 0); b.put<uint8_t>((uint8_t)(ok ? s.n : 0)); }
        else { b.put<int32_t>(i == 3 ? flame : i == 4 ? arrow : 0); b.put<uint8_t>(0); }
    }
    chestNbtTailPut(b, t); return b;
}
static void furnNetSend(int x, int y, int z, const FurnData& f, bool progress) { if (!netOn) return; NBuf b = furnNetBuild(x, y, z, f, progress); netBroadcast(b); }
static void furnNetReq(int x, int y, int z) {   // so o host responde
    if (!netHost) return; FurnData* f = furnAt(x, y, z, true); if (!f) return;
    gFurnWatch[ckey(x, y, z)] = SDL_GetTicks(); furnNetSend(x, y, z, *f, true);
}
static void furnNetRecv(NBuf& b, int x, int y, int z) {
    ChestInv t; int prog[2] = {0, 0};
    for (int i = 0; i < CHEST_SLOTS; i++) { const int32_t w = b.get<int32_t>(); const int n = b.get<uint8_t>();
        if (i < 3) { const int id = w ? netUnwireId(w) : 0; if (id > 0 && n > 0) { t.s[i].id = id; t.s[i].n = std::min(n, maxStack(id)); } } else if (i < 5) prog[i - 3] = std::max(0, std::min(255, (int)w)); }
    chestNbtTailGet(b, t);
    if (netHost) {   // convidado -> host: so os itens (progresso e do host); fornalha que o host nao tem nao e criada
        FurnData* f = furnAt(x, y, z, false); if (!f) return;
        for (int i = 0; i < 3; i++) f->s[i] = t.s[i];
        gFurnWatch[ckey(x, y, z)] = SDL_GetTicks(); return;
    }
    if (!gFurn || gFurnPos[0] != x || gFurnPos[1] != y || gFurnPos[2] != z) return;   // convidado: so a fornalha que ele abriu
    FurnData* f = furnAt(x, y, z, true); if (!f) return;
    for (int i = 0; i < 3; i++) f->s[i] = t.s[i];
    f->burnMax = 255; f->burn = prog[0]; f->cook = prog[1] * FURN_COOK / 255;
    gFurnSnap = *f; gChWait = false;
}
static void furnSyncNow() {   // convidado: manda o que mudou desde o ultimo envio; repete o pedido a cada 10 s (mantem o host empurrando o progresso)
    if (!furnGuest() || !gFurn || gChWait) return;
    FurnData* f = furnAt(gFurnPos[0], gFurnPos[1], gFurnPos[2], false); if (!f) return;
    if (!furnSlotsEq(*f, gFurnSnap)) { gFurnSnap = *f; furnNetSend(gFurnPos[0], gFurnPos[1], gFurnPos[2], *f, false); }
    const Uint32 now = SDL_GetTicks(); if (now - gFurnReq > 10000) { gFurnReq = now; NBuf r = netMk(NP_CHESTREQ); r.put<int32_t>(gFurnPos[0]); r.put<int32_t>(gFurnPos[1]); r.put<int32_t>(gFurnPos[2]); netBroadcast(r); }
}
static void furnWatchPush() {   // host: a cada 10 ticks, progresso para quem tem a fornalha aberta
    if (!netHost || !netOn || gFurnWatch.empty()) return;
    const Uint32 now = SDL_GetTicks();
    for (auto it = gFurnWatch.begin(); it != gFurnWatch.end();) {
        if (now - it->second > 30000) { it = gFurnWatch.erase(it); continue; }
        const int x = std::get<0>(it->first), yy = std::get<1>(it->first), z = std::get<2>(it->first);
        if ((yy >> 12) == gDim) if (FurnData* f = furnAt(x, yy & 4095, z, false)) furnNetSend(x, yy & 4095, z, *f, true);
        ++it;
    }
}
static int furnOpenAt(int x, int y, int z) {   // 0 = nao e fornalha; 1 = abriu (convidado: espera o host mandar o conteudo)
    if (!gFurnId || getB(x, y, z) != gFurnId) return 0;
    furnAt(x, y, z, true); gFurn = gChest = true; gChEnder = false; gChWait = false; gChRows = 1; gChCnt = 1; gFurnPos[0] = x; gFurnPos[1] = y; gFurnPos[2] = z;
    if (furnGuest()) {   // convidado nao tem o conteudo: pede ao host
        *furnAt(x, y, z, true) = FurnData(); gFurnSnap = FurnData(); gChWait = true; gChT0 = SDL_GetTicks(); gFurnReq = gChT0;
        NBuf r = netMk(NP_CHESTREQ); r.put<int32_t>(x); r.put<int32_t>(y); r.put<int32_t>(z); netBroadcast(r);
    }
    return 1;
}
static void furnClosed() {
    if (furnGuest() && gFurn && !gChWait) furnSyncNow();   // ultima edicao vai ao host
    if (furnGuest() && gFurn) gFurns.erase(ckey(gFurnPos[0], gFurnPos[1], gFurnPos[2]));   // o convidado nao guarda o conteudo
    gFurn = gChest = false; gChRows = 3; gChCnt = 0; gChWait = false;
}
static bool furnFrame(double px, double py, double pz) {   // true = fechar a tela
    (void)py; if (!furnAt(gFurnPos[0], gFurnPos[1], gFurnPos[2], false)) return true;
    const double dx = px - (gFurnPos[0] + .5), dz = pz - (gFurnPos[2] + .5); if (dx * dx + dz * dz > 64.0) return true;
    if (gChWait) return SDL_GetTicks() - gChT0 > 2500;   // convidado sem resposta do host
    furnSyncNow(); return false;
}
static bool furnProgress(float* flame, float* arrow) {
    *flame = *arrow = 0; if (!gFurn) return false; FurnData* f = furnAt(gFurnPos[0], gFurnPos[1], gFurnPos[2], false); if (!f) return false;
    if (f->burn > 0) *flame = f->burnMax > 0 ? std::min(1.f, f->burn / (float)f->burnMax) : 1.f;
    *arrow = std::min(1.f, f->cook / (float)FURN_COOK); return true;
}
static void furnShift(Slot& s) {   // shift-click da mochila: fundivel -> entrada, combustivel -> combustivel (como o ContainerFurnace)
    int k = furnResult(s.id) ? 0 : furnFuel(s.id) ? 1 : -1; if (k < 0) return;
    Slot* d = furnSlotRef(k); if (!d) return;
    if (d->id && !sameStack(*d, s)) return;
    const int put = std::min(s.n, maxStack(s.id) - (d->id ? d->n : 0)); if (put <= 0) return;
    if (!d->id) { d->id = s.id; d->nbt = s.nbt; d->d = s.d; d->n = 0; } d->n += put; s.n -= put; if (s.n <= 0) s = Slot();
}
static void furnTakeOut(int btn, bool shift) {   // slot de resultado: so retirar
    (void)btn; Slot* o = furnSlotRef(2); if (!o || !o->id || o->n <= 0) return;
    FurnData* fd = furnAt(gFurnPos[0], gFurnPos[1], gFurnPos[2], false); const int tot = o->n, rid = o->id;
    auto xpFor = [&](int took) {   // patchFur.3: XP guardado na fornalha (host/offline); convidado nao tem o valor, usa a tabela da receita
        if (took <= 0) return; if (furnGuest() || !fd) { furnGiveXp(furnXp(rid) * took); return; }
        const float sh = fd->xp * took / (float)tot; fd->xp = std::max(0.f, fd->xp - sh); if (took >= tot) fd->xp = 0.f; furnGiveXp(sh); };
    if (shift) { mergeInto(*o, ordMainHot(true)); xpFor(tot - (o->id ? o->n : 0)); return; }
    if (!cur.id) { cur = *o; *o = Slot(); xpFor(tot); return; }
    if (!sameStack(cur, *o)) return;
    const int take = std::min(o->n, maxStack(cur.id) - cur.n); if (take <= 0) return; cur.n += take; o->n -= take; if (o->n <= 0) *o = Slot(); xpFor(take);
}
// ---- salvar / carregar ----
static void furnSave(const std::string& dir) {
    const std::string path = "saves/" + dir + "/furnaces.dat"; bool any = false; for (auto& kv : gFurns) { const FurnData& f = kv.second; if (f.s[0].id || f.s[1].id || f.s[2].id || f.burn || f.cook) any = true; }
    if (!any) { remove(path.c_str()); return; }
    FILE* fp = fopen(path.c_str(), "wb"); if (!fp) return; fprintf(fp, "FURN1\n");
    for (auto& kv : gFurns) { const FurnData& f = kv.second; if (!(f.s[0].id || f.s[1].id || f.s[2].id || f.burn || f.cook)) continue;
        fprintf(fp, "f %d %d %d %d %d %d %.3f\n", std::get<0>(kv.first), std::get<1>(kv.first), std::get<2>(kv.first), f.burn, f.burnMax, f.cook, f.xp);
        for (int i = 0; i < 3; i++) { const Slot& s = f.s[i]; if (s.id && s.n > 0) { fprintf(fp, "s %d %d %d %s\n", i, s.n, s.d, chestItemKey(s.id).c_str()); } } }
    fclose(fp);
}
static void furnLoad(const std::string& dir) {   // chamar depois de resetWorldData()
    gFurns.clear(); FILE* fp = fopen(("saves/" + dir + "/furnaces.dat").c_str(), "rb"); if (!fp) return; static char ln[2048]; FurnData* cur = nullptr;
    while (fgets(ln, sizeof ln, fp)) {
        int x, y, z, b, bm, c, i, n, d; float xpv = 0.f; char key[900];
        if (ln[0] == 'f' && sscanf(ln, "f %d %d %d %d %d %d %f", &x, &y, &z, &b, &bm, &c, &xpv) >= 6) { cur = &gFurns[ChestKey(x, y, z)]; cur->burn = std::max(0, b); cur->burnMax = std::max(0, bm); cur->cook = std::max(0, c); cur->xp = std::max(0.f, xpv); }
        else if (ln[0] == 's' && cur && sscanf(ln, "s %d %d %d %899s", &i, &n, &d, key) == 4 && i >= 0 && i < 3 && n > 0) { const int id = chestItemFromKey(key); if (id) { cur->s[i].id = id; cur->s[i].n = std::min(n, maxStack(id)); cur->s[i].d = d; } }
    }
    fclose(fp);
}
