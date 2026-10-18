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
        {"pumpkin_face_off", "pumpkin_face_off", "pumpkin_side", "pumpkin_top", "pumpkin_top"},   // patchV.4: nomes do pack 1.12 (rosto so na frente; topo/base = pumpkin_top)
        {"pumpkin_face_on", "pumpkin_face_on", "pumpkin_side", "pumpkin_top", "pumpkin_top"},
        {"furnace", "furnace_front_off", "furnace_side", "furnace_top", "furnace_top"},
        {"dispenser", "dispenser_front_horizontal", "furnace_side", "furnace_top", "furnace_top"},   // patchRed.2: nomes do pack 1.12 (antes so achava dispenser_front e o icone ficava com a frente nas 6 faces)
        {"dropper", "dropper_front_horizontal", "furnace_side", "furnace_top", "furnace_top"},
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
        if (n.compare(0, 7, "flower_") == 0 || n.compare(0, 13, "double_plant_") == 0 || n.compare(0, 8, "sapling_") == 0 || n == "tallgrass") { const std::string c = "block/" + pk112(n); if (!gPkByFile.count(c)) gPkByFile[c] = PK0 + i; }   // patchV.4: flores/plantas/mudas do pack 1.12 -> nome canonico (itens ligados ao bloco: da para colocar)
        if (n == "anvil_top_damaged_0" && !gPkByFile.count("block/anvil_top")) gPkByFile["block/anvil_top"] = PK0 + i;   // topo da bigorna no pack 1.12
        if (n == "torch_on" || n == "redstone_torch_on" || n == "anvil_base") { const std::string c = "block/" + pk112(n); if (!gPkByFile.count(c)) gPkByFile[c] = PK0 + i; }   // patchInv.4: pack 1.12 -> nome canonico (tochas/bigorna sem textura nem drop)
        if (ends(n, "_top")) {   // X_top + X_side (+ X_bottom): abóbora, TNT, cacto, podzol... viram o bloco "X"
            const std::string base = n.substr(0, n.size() - 4); const int side = tileOf(base + "_side");
            if (side >= 0 && !by.count("block/" + base)) {
                e.cube = true; e.ft[0] = e.tile; e.ft[1] = side; const int bt = tileOf(base + "_bottom"); e.ft[2] = bt >= 0 ? bt : e.tile;
                e.name = pkTitle(pk112(base)); gPkByFile["block/" + base] = PK0 + i;
            }
        } else if (e.cube) {     // X (lado) + X_top: troncos, arenito...
            const int top = tileOf(n + "_top");
            if (top >= 0) { const int bt = tileOf(n + "_bottom"); e.ft[0] = top; e.ft[1] = e.tile; e.ft[2] = bt >= 0 ? bt : top; }
        }
        if (!e.cube) continue;
        auto has = [&](const char* s) { return n.find(s) != std::string::npos; };
        if (ends(pk112(n), "_leaves")) {   // patchV.8: pk112 (leaves_spruce -> spruce_leaves)   // folhagem: transparente a luz (opac 1), tinta de bioma (abeto/betula tem cor fixa no MC; azalea/cerejeira nenhuma)
            
#ifdef BL_RPI
            if (e.tile >= 0) {   // Raspberry: folha OPACA (sem furos de alpha): texels transparentes ficam com a media escurecida dos opacos
                uint8_t* t0 = &atlas[(((e.tile / ATL) * 16) * ATW + (e.tile % ATL) * 16) * 4]; int sr = 0, sg = 0, sb = 0, nO = 0;
                for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const uint8_t* p = t0 + ((size_t)y * ATW + x) * 4; if (p[3] > 25) { sr += p[0]; sg += p[1]; sb += p[2]; nO++; } }
                if (nO) for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { uint8_t* p = t0 + ((size_t)y * ATW + x) * 4; if (p[3] <= 25) { p[0] = sr / nO * 2 / 3; p[1] = sg / nO * 2 / 3; p[2] = sb / nO * 2 / 3; } p[3] = 255; }
            }
#endif
            gPkLeaf[i] = has("spruce") ? 2 : has("birch") ? 3 : (has("azalea") || has("cherry") || has("flowering")) ? 4 : 1; gPkOpac[i] = 1;
        }
        if (ends(pk112(n), "_log") || ends(pk112(n), "_wood")) gPkLog[i] = 1;
        // luz
        if (has("glass") || n == "slime_block") gPkOpac[i] = 0; else if (n == "ice") gPkOpac[i] = 3;
        { const std::string n12 = pk112(n); if (n12.find("stained_glass") != std::string::npos) gPkTrans[i] = 1; }   // patchV.12: pack 1.12 = glass_white (nome antigo) -> normaliza
        if (has("stained_glass") || has("tinted_glass") || n == "ice" || n == "slime_block" || n == "frosted_ice") gPkTrans[i] = 1; else if (n == "glass") gPkTrans[i] = 2;   // vidro colorido/gelo/slime: passada translucida (blend); vidro comum so corta faces internas
        if (has("glowstone") || has("sea_lantern") || has("shroomlight") || has("jack_o_lantern") || has("beacon") || has("redstone_lamp_on") || has("froglight")) gPkEmit[i] = 15;
        else if (has("magma")) gPkEmit[i] = 3;
        if (gPkEmit[i]) gPkTint[i] = (has("magma")) ? 1 : has("sea_lantern") || has("beacon") ? 5 : has("glowstone") || has("redstone_lamp") ? 4 : has("jack_o_lantern") || has("shroomlight") ? 6 : has("froglight") ? 7 : 0;   // cor da luz de bloco (paleta em biome_light.inc)
        // dureza (valores do Minecraft 1.8) e ferramenta ideal: 1 picareta, 2 machado, 3 pá, 0 qualquer
        float hd = 1.5f; int tool = 1, lvl = 0;
        const bool dirty = has("dirt") || has("grass") || has("sand") || has("gravel") || has("podzol") || has("mycelium") || has("snow") || has("mud") || has("farmland") || has("clay");
        const bool wood = has("planks") || has("log") || has("wood") || has("crafting") || has("bookshelf") || has("pumpkin") || has("melon") || has("chest") || has("jukebox") || has("note_block");
        if (has("tallgrass") || has("double_plant") || has("flower_") || has("sapling") || has("deadbush") || has("fern")) { hd = 0.f; tool = 0; }   // patchFix.6: mato/flores/mudas quebram na hora com qualquer coisa (como o Java)
        else if (has("obsidian")) { hd = 50.f; lvl = 3; }
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
    {   // patchHop.1: funil. O png do funil tem cantos TRANSPARENTES (so serve para o modelo vanilla): como cubo ficava furado. Preenche os texels vazios (media escurecida) e usa hopper_top na face de cima
        auto ho = gPkByFile.find("block/hopper_outside");
        if (ho != gPkByFile.end() && ho->second >= PK0 && ho->second - PK0 < (int)PK.size()) {
            PkItem& e = PK[ho->second - PK0]; auto ht = gPkByFile.find("block/hopper_top"); const int top = (ht != gPkByFile.end() && ht->second - PK0 < (int)PK.size()) ? PK[ht->second - PK0].tile : e.tile;
            for (int tl : {e.tile, top}) if (tl >= 0) {
                uint8_t* t0 = &atlas[(((tl / ATL) * 16) * ATW + (tl % ATL) * 16) * 4]; int sr = 0, sg = 0, sb = 0, nO = 0;
                for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { const uint8_t* q = t0 + ((size_t)y * ATW + x) * 4; if (q[3] > 25) { sr += q[0]; sg += q[1]; sb += q[2]; nO++; } }
                for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { uint8_t* q = t0 + ((size_t)y * ATW + x) * 4; if (q[3] <= 25 && nO) { q[0] = sr / nO * 2 / 3; q[1] = sg / nO * 2 / 3; q[2] = sb / nO * 2 / 3; } q[3] = 255; }
            }
            e.ft[0] = top; e.ft[1] = e.tile; e.ft[2] = e.tile; e.cube = true; gPkTrans[ho->second - PK0] = 0;
        }
    }
    for (const char* nmx : {"dropper", "dispenser"}) { const std::string key = std::string("block/") + nmx; if (!gPkByFile.count(key)) for (int k = 0; k < NFD; k++) if (!strcmp(FD[k].base, nmx) && anc[k] >= 0) { gPkByFile[key] = PK0 + anc[k]; break; } }   // patchLogic.2
    { if (!gPkByFile.count("block/furnace")) for (int k = 0; k < NFD; k++) if (!strcmp(FD[k].base, "furnace") && anc[k] >= 0) gPkByFile["block/furnace"] = PK0 + anc[k];   // pack 1.12: so ha furnace_top/_side/_front_off; a ancora vira "a fornalha" (funcional, igual ao item do criativo)
    }
    { auto fi = gPkByFile.find("block/furnace"); gFurnId = fi != gPkByFile.end() ? fi->second : 0; gFurnOnTile = -1;   // patchFur.1: id da fornalha + textura da frente acesa
      for (const char* n : {"block/furnace_front_on", "block/furnace_front_lit"}) { auto t = gPkByFile.find(n); if (t != gPkByFile.end()) { gFurnOnTile = PK[t->second - PK0].tile; break; } } }
    // blocos com EIXO (como o log do Minecraft: o jogador escolhe X/Y/Z pela face clicada). Por nome do arquivo.
    memset(&gBlkAxis[PK0], 0, 65536 - PK0);   // zera o intervalo do pack: troca de resource pack nao pode deixar flags do pack anterior em ids reaproveitados
    for (int i = 0; i < N; i++) {
        const PkItem& e = PK[i]; if (!e.cube || e.file.compare(0, 6, "block/") != 0) continue;
        const std::string nm = pk112(e.file.substr(6));   // patchBox.2: nome canonico (log_spruce -> spruce_log); antes so o carvalho (base) tinha eixo
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
    { auto it = gPkByFile.find("block/end_portal_frame"); if (it != gPkByFile.end()) gEndFrameId = it->second;
      else for (const char* nm : {"block/endframe", "block/endframe_top", "block/endframe_side", "block/end_portal_frame_top", "block/end_portal_frame_side"}) { auto i2 = gPkByFile.find(nm); if (i2 != gPkByFile.end()) { gEndFrameId = i2->second; break; } }
      if (!gEndFrameId) for (auto& kv : gPkByFile) if (kv.first.compare(0, 22, "block/end_portal_frame") == 0 || kv.first.compare(0, 14, "block/endframe") == 0) { gEndFrameId = kv.second; break; } }   // patchEnd.1: busca tolerante (nome do pack varia)
    if (gEndFrameId) { gPkOpac[gEndFrameId - PK0] = 0; static int fSh = 0; if (!fSh) fSh = shapeAdd({{0, 0, 0, 1, .8125, 1}}); blockShapeSet(gEndFrameId, fSh, fSh); }   // BlockEndPortalFrame: caixa 16x13x16
    gEnchId = 0; gEndTopF = tileOf("end_portal_frame_top_f"); gEndSideF = tileOf("end_portal_frame_side_f"); gEndEye = tileOf("end_portal_frame_eye"); if (gEndEye < 0) gEndEye = tileOf("endframe_eye"); if (gEndEye < 0) gEndEye = tileOf("ender_eye"); if (gEndEye < 0) gEndEye = tileOf("end_portal_frame_top");
    { auto it = gPkByFile.find("block/enchanting_table"); if (it != gPkByFile.end()) gEnchId = it->second; }
    if (gEnchId) { gPkOpac[gEnchId - PK0] = 0; static int eSh = 0; if (!eSh) eSh = shapeAdd({{0, 0, 0, 1, .75, 1}}); blockShapeSet(gEnchId, eSh, eSh); }   // BlockEnchantmentTable: caixa 16x12x16
    { static int cCol = 0, cSel = 0; if (!cCol) { cCol = shapeAdd({{.0625, 0, .0625, .9375, .9375, .9375}}); cSel = shapeAdd({{.0625, 0, .0625, .9375, 1, .9375}}); }   // ids criados 1 vez (o pool e uint8)
      if (gCactusId) blockShapeSet(gCactusId, cCol, cSel); }
    { auto it = gPkByFile.find("block/cobweb"); if (it != gPkByFile.end()) { gPkOpac[it->second - PK0] = 0; static int wSel = 0; if (!wSel) wSel = shapeAdd({{0, 0, 0, 1, 1, 1}}); blockShapeSet(it->second, SHAPE_NONE, wSel); } }   // patchMech.1: teia sem colisao (so freia), selecao cubo cheio
    { auto f = [&](const char* k) { for (int i = 0; i < N; i++) if (PK[i].file == k) { gPkByFile[k] = PK0 + i; gPkByHash[netFnvPk(k)] = PK0 + i; return PK0 + i; } return 0; };   // itens (item/...) nao entram no laco acima, so block/
      auto f2 = [&](const char* a, const char* b) { const int r = f(a); return r ? r : f(b); };   // patchBucket.4: pack 1.12 registra item/bucket_empty etc. (nome cru), nao item/bucket
      gBktEmpty = f2("item/bucket", "item/bucket_empty"); gBktWater = f2("item/water_bucket", "item/bucket_water"); gBktLava = f2("item/lava_bucket", "item/bucket_lava"); gBktMilk = f2("item/milk_bucket", "item/bucket_milk"); gFlintSteel = f("item/flint_and_steel"); }
}
