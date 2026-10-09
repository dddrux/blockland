// ---------- blocos do resource pack colocáveis (v32) ----------
// Todo item "cubo" do pack (block/xxx opaco, vidro, gelo, e blocos com _top/_side/_bottom) vira bloco de verdade: os chunks guardam 16 bits.
// Aqui: faces por bloco, opacidade/emissão de luz, dureza/ferramenta (por nome) e as tabelas nome->id (save e rede).
static void pkBuildProps() {
    const int N = (int)PK.size();
    gPkOpac.assign(N, 15); gPkEmit.assign(N, 0); gPkTint.assign(N, 0); gPkTool.assign(N, 1); gPkLevel.assign(N, 0); gPkHard.assign(N, 1.5f);
    gPkLeaf.assign(N, 0); gPkLog.assign(N, 0); gPkTrans.assign(N, 0);
    gPkByFile.clear(); gPkByHash.clear();
    std::unordered_map<std::string, int> by; for (int i = 0; i < N; i++) by[PK[i].file] = i;
    auto tileOf = [&](const std::string& s) { auto it = by.find("block/" + s); return it == by.end() ? -1 : PK[it->second].tile; };
    auto ends = [](const std::string& n, const char* x) { const size_t l = strlen(x); return n.size() > l && n.compare(n.size() - l, l, x) == 0; };
    // blocos com FRENTE: o pack so traz a textura do rosto (carved_pumpkin, jack_o_lantern) ou so a frente (dispenser_front); topo/lado/base vem de outras texturas.
    // Antes disso viravam cubo com o rosto nas 6 faces (ou nem eram bloco, como o dispenser).
    struct FaceDef { const char *base, *front, *side, *top, *bottom; };
    static const FaceDef FD[] = {
        {"carved_pumpkin", "carved_pumpkin", "pumpkin_side", "pumpkin_top", "pumpkin_bottom"},
        {"jack_o_lantern", "jack_o_lantern", "pumpkin_side", "pumpkin_top", "pumpkin_bottom"},
        {"furnace", "furnace_front", "furnace_side", "furnace_top", "furnace_bottom"},
        {"dispenser", "dispenser_front", "furnace_side", "furnace_top", "furnace_top"},   // como no 1.12: lados/topo/base da fornalha
        {"dropper", "dropper_front", "furnace_side", "furnace_top", "furnace_top"},
    };
    const int NFD = (int)(sizeof FD / sizeof FD[0]); int anc[NFD];
    for (int k = 0; k < NFD; k++) {
        const FaceDef& d = FD[k]; anc[k] = -1;
        auto a = by.find(std::string("block/") + d.base), t = by.find(std::string("block/") + d.base + "_top"), f = by.find(std::string("block/") + d.front);
        if (a != by.end()) anc[k] = a->second;                       // a textura do bloco E a frente (abobora entalhada, jack-o'-lantern)
        else if (t != by.end()) anc[k] = t->second;                  // a regra X_top + X_side ja cria o bloco (fornalha)
        else if (f != by.end()) { anc[k] = f->second; PK[f->second].cube = true; PK[f->second].name = pkTitle(d.base); gPkByFile[std::string("block/") + d.base] = PK0 + f->second; }   // so existe a frente (dispenser, dropper): ancora nela
    }
    for (int i = 0; i < N; i++) {
        PkItem& e = PK[i]; if (e.file.compare(0, 6, "block/") != 0) continue;
        const std::string n = e.file.substr(6);
        gPkByFile[e.file] = PK0 + i; gPkByHash[netFnvPk(e.file)] = PK0 + i;
        if (ends(n, "_top")) {   // X_top + X_side (+ X_bottom): abóbora, TNT, cacto, podzol... viram o bloco "X"
            const std::string base = n.substr(0, n.size() - 4); const int side = tileOf(base + "_side");
            if (side >= 0 && !by.count("block/" + base)) {
                e.cube = true; e.ft[0] = e.tile; e.ft[1] = side; const int bt = tileOf(base + "_bottom"); e.ft[2] = bt >= 0 ? bt : e.tile;
                e.name = pkTitle(base); gPkByFile["block/" + base] = PK0 + i;
            }
        } else if (e.cube) {     // X (lado) + X_top: troncos, arenito...
            const int top = tileOf(n + "_top");
            if (top >= 0) { const int bt = tileOf(n + "_bottom"); e.ft[0] = top; e.ft[1] = e.tile; e.ft[2] = bt >= 0 ? bt : top; }
        }
        if (!e.cube) continue;
        auto has = [&](const char* s) { return n.find(s) != std::string::npos; };
        if (ends(n, "_leaves")) {   // folhagem: transparente a luz (opac 1), tinta de bioma (abeto/betula tem cor fixa no MC; azalea/cerejeira nenhuma)
            gPkLeaf[i] = has("spruce") ? 2 : has("birch") ? 3 : (has("azalea") || has("cherry") || has("flowering")) ? 4 : 1; gPkOpac[i] = 1;
        }
        if (ends(n, "_log") || ends(n, "_wood")) gPkLog[i] = 1;
        // luz
        if (has("glass") || n == "slime_block") gPkOpac[i] = 0; else if (n == "ice") gPkOpac[i] = 3;
        if (has("stained_glass") || has("tinted_glass") || n == "ice" || n == "slime_block" || n == "frosted_ice") gPkTrans[i] = 1; else if (n == "glass") gPkTrans[i] = 2;   // vidro colorido/gelo/slime: passada translucida (blend); vidro comum so corta faces internas
        if (has("glowstone") || has("sea_lantern") || has("shroomlight") || has("jack_o_lantern") || has("beacon") || has("redstone_lamp_on") || has("froglight")) gPkEmit[i] = 15;
        else if (has("magma")) gPkEmit[i] = 3;
        if (gPkEmit[i]) gPkTint[i] = (has("magma")) ? 1 : has("sea_lantern") || has("beacon") ? 5 : has("glowstone") || has("redstone_lamp") ? 4 : has("jack_o_lantern") || has("shroomlight") ? 6 : has("froglight") ? 7 : 0;   // cor da luz de bloco (paleta em biome_light.inc)
        // dureza (valores do Minecraft 1.8) e ferramenta ideal: 1 picareta, 2 machado, 3 pá, 0 qualquer
        float hd = 1.5f; int tool = 1, lvl = 0;
        const bool dirty = has("dirt") || has("grass") || has("sand") || has("gravel") || has("podzol") || has("mycelium") || has("snow") || has("mud") || has("farmland") || has("clay");
        const bool wood = has("planks") || has("log") || has("wood") || has("crafting") || has("bookshelf") || has("pumpkin") || has("melon") || has("chest") || has("jukebox") || has("note_block");
        if (has("obsidian")) { hd = 50.f; lvl = 3; }
        else if (has("_ore") || has("ancient_debris")) { hd = 3.f; lvl = (has("diamond") || has("gold") || has("redstone") || has("emerald")) ? 2 : (has("iron") || has("lapis") || has("copper")) ? 1 : 0; }
        else if (has("iron_block") || has("diamond_block") || has("gold_block") || has("emerald_block") || has("lapis_block") || has("redstone_block") || has("netherite_block") || has("coal_block")) { hd = 5.f; lvl = (has("diamond") || has("gold") || has("emerald") || has("redstone")) ? 2 : has("iron") || has("lapis") ? 1 : 0; }
        else if (has("wool") || has("carpet")) { hd = .8f; tool = 0; }
        else if (has("glass") || n == "ice") { hd = .3f; tool = 0; }
        else if (has("leaves") || has("sponge") || has("slime") || has("honey") || has("cactus") || has("hay")) { hd = has("leaves") ? .2f : .6f; tool = 0; }
        else if (wood) { hd = has("pumpkin") || has("melon") ? 1.f : 2.f; tool = 2; }
        else if (dirty) { hd = has("clay") ? .6f : .5f; tool = 3; }
        else if (has("dispenser") || has("dropper") || has("furnace")) hd = 3.5f;
        else if (has("netherrack")) hd = .4f;
        else if (has("terracotta")) hd = 1.25f;
        else if (has("sandstone")) hd = .8f;
        else if (has("concrete")) hd = 1.8f;
        else if (has("bedrock")) hd = 1e6f;
        gPkHard[i] = hd; gPkTool[i] = (uint8_t)tool; gPkLevel[i] = (int8_t)(tool == 1 ? lvl : -1);
    }
    memset(&gBlkFacing[PK0], 0, 65536 - PK0);
    for (int k = 0; k < NFD; k++) {   // define as 6 faces dos blocos com frente (precisa do loop acima ja ter criado o bloco)
        if (anc[k] < 0) continue;
        const FaceDef& d = FD[k]; const int fr = tileOf(d.front), sd = tileOf(d.side), tp = tileOf(d.top), bt = tileOf(d.bottom);
        if (fr < 0 || sd < 0 || tp < 0) continue;
        PkItem& e = PK[anc[k]]; e.ft[0] = tp; e.ft[1] = sd; e.ft[2] = bt >= 0 ? bt : tp; e.fr = fr; e.cube = true; gBlkFacing[PK0 + anc[k]] = 1;
    }
    // blocos com EIXO (como o log do Minecraft: o jogador escolhe X/Y/Z pela face clicada). Por nome do arquivo.
    memset(&gBlkAxis[PK0], 0, 65536 - PK0);   // zera o intervalo do pack: troca de resource pack nao pode deixar flags do pack anterior em ids reaproveitados
    for (int i = 0; i < N; i++) {
        const PkItem& e = PK[i]; if (!e.cube || e.file.compare(0, 6, "block/") != 0) continue;
        const std::string nm = e.file.substr(6);
        bool ax = ends(nm, "_log") || ends(nm, "_wood") || ends(nm, "_stem") || ends(nm, "_hyphae") || ends(nm, "basalt")
               || nm == "hay_block" || nm == "bone_block" || nm == "quartz_pillar" || nm == "purpur_pillar";
        gBlkAxis[PK0 + i] = ax ? 1 : 0;
    }
    // cacto (BlockCactus do 1.12): a textura do pack tem borda transparente (corpo 14x14 + espinhos) e o modelo e RECUADO 1/16 nos lados.
    // Como cubo cheio os cantos ficavam furados. O mesher recua as 4 faces laterais (gCactusId); colisao 15/16 de altura, selecao ate o topo.
    gCactusId = 0;
    { auto it = gPkByFile.find("block/cactus"); if (it != gPkByFile.end()) gCactusId = it->second; }   // no pack o cacto entra pela ancora "cactus_top" (sem cactus.png)
    // O cacto NAO e opaco (BlockCactus 1.12: nao e cubo cheio, opacidade 0). Como opaco (15) ele escondia a face de cima da areia embaixo dele,
    // e como os lados sao recuados 1/16 aparecia um anel vazado ("bloco fantasma") em volta da base do cacto. Agora a areia desenha o topo e a luz passa.
    if (gCactusId) gPkOpac[gCactusId - PK0] = 0;
    gEndFrameId = 0;
    { auto it = gPkByFile.find("block/end_portal_frame"); if (it != gPkByFile.end()) gEndFrameId = it->second; }
    if (gEndFrameId) { gPkOpac[gEndFrameId - PK0] = 0; static int fSh = 0; if (!fSh) fSh = shapeAdd({{0, 0, 0, 1, .8125, 1}}); blockShapeSet(gEndFrameId, fSh, fSh); }   // BlockEndPortalFrame: caixa 16x13x16
    gEnchId = 0; gEndTopF = tileOf("end_portal_frame_top_f"); gEndSideF = tileOf("end_portal_frame_side_f"); gEndEye = tileOf("end_portal_frame_eye");
    { auto it = gPkByFile.find("block/enchanting_table"); if (it != gPkByFile.end()) gEnchId = it->second; }
    if (gEnchId) { gPkOpac[gEnchId - PK0] = 0; static int eSh = 0; if (!eSh) eSh = shapeAdd({{0, 0, 0, 1, .75, 1}}); blockShapeSet(gEnchId, eSh, eSh); }   // BlockEnchantmentTable: caixa 16x12x16
    { static int cCol = 0, cSel = 0; if (!cCol) { cCol = shapeAdd({{.0625, 0, .0625, .9375, .9375, .9375}}); cSel = shapeAdd({{.0625, 0, .0625, .9375, 1, .9375}}); }   // ids criados 1 vez (o pool e uint8)
      if (gCactusId) blockShapeSet(gCactusId, cCol, cSel); }
    { auto f = [&](const char* k) { for (int i = 0; i < N; i++) if (PK[i].file == k) { gPkByFile[k] = PK0 + i; gPkByHash[netFnvPk(k)] = PK0 + i; return PK0 + i; } return 0; };   // itens (item/...) nao entram no laco acima, so block/
      gBktEmpty = f("item/bucket"); gBktWater = f("item/water_bucket"); gBktLava = f("item/lava_bucket"); }
}
