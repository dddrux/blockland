// patch5.57a: NBT por pilha (Slot.nbt) + pick block de bau com conteudo (Ctrl + botao do meio).
// Testes: slot_serialize_nbt, slot_packet_nbt, slot_stack_nbt, pick_chest, pick_chest_empty. Esperado: 0 falhas.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static const int AY = 200;
static std::string readAll(const std::string& p) { FILE* f = fopen(p.c_str(), "rb"); if (!f) return ""; std::string s; char b[4096]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) s.append(b, n); fclose(f); return s; }
static void clearInv() { for (Slot& s : inv) s = Slot(); cur = Slot(); offh = Slot(); for (Slot& a : armor) a = Slot(); }
static void feed(const NBuf& src, void (*fn)(NBuf&)) { NBuf b = src; b.r = 2; fn(b); }   // pula tipo+remetente, como o despachante de rede
int main() {
    if (chdir("..") != 0) return 99;
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    gNoBlockHooks = true; for (int z = -4; z < 20; z++) for (int x = -4; x < 20; x++) for (int y = AY - 1; y < AY + 6; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR); gNoBlockHooks = false;
    const int CH = XB_CHEST, CHEST_ITEM = STONE;   // id do "item bau" no teste (o jogo usa xbDropItem; aqui so importa que e o mesmo id nos dois lados)
    auto putChest = [&](int x, int z, int type) { setBM(x, AY, z, CH, (type << 2) | 0); };

    SECTION("slot_serialize_nbt: texto do conteudo (BlockEntityTag:{Items})");
    {
        ChestInv c; c.s[0] = {STONE, 5}; c.s[3] = {DIRT, 64}; c.s[26] = {GRAVEL, 1};
        const std::string t = chestNbtPack(c); CHECK(!t.empty() && t.compare(0, 5, "BET1|") == 0);
        ChestInv u; CHECK(chestNbtUnpack(t, u)); CHECK(chestEq(c, u) && u.s[0].n == 5 && u.s[3].id == DIRT && u.s[26].id == GRAVEL && u.s[1].id == 0);
        // bau dentro de bau (NBT recursivo): a pilha interna guarda o texto da interna
        ChestInv inner; inner.s[7] = {DIRT, 9}; ChestInv outer; outer.s[2] = {CHEST_ITEM, 1, 0, chestNbtPack(inner)}; outer.s[2].nbt += "";
        const std::string t2 = chestNbtPack(outer); ChestInv o2; CHECK(chestNbtUnpack(t2, o2) && o2.s[2].id == CHEST_ITEM && o2.s[2].nbt == outer.s[2].nbt);
        ChestInv in2; CHECK(chestNbtUnpack(o2.s[2].nbt, in2) && in2.s[7].id == DIRT && in2.s[7].n == 9);
        CHECK(chestNbtPack(ChestInv()).empty());                                                   // vazio => sem NBT
        ChestInv bad; CHECK(!chestNbtUnpack("", bad)); CHECK(!chestNbtUnpack("?12345", bad)); CHECK(!chestNbtUnpack("BET1|0,5,3:#1", bad)); CHECK(!chestNbtUnpack("BET1|x", bad));
        CHECK(!chestNbtUnpack("BET1|0,5,99:#1", bad)); CHECK(bad.empty());                         // tamanho da chave maior que o texto
    }
    SECTION("slot_serialize_nbt: player.dat com e sem NBT (e save antigo)");
    {
        gCurDir = "_t_nbt"; fs::create_directories("saves/_t_nbt"); clearInv();
        ChestInv big; for (int i = 0; i < CHEST_SLOTS; i++) big.s[i] = {DIRT, 1 + i};
        ChestInv huge; for (int i = 0; i < CHEST_SLOTS; i++) huge.s[i] = {CHEST_ITEM, 1, 0, chestNbtPack(big)};   // 27 baus cheios dentro de um bau
        const std::string nb = chestNbtPack(huge); CHECK(nb.size() > 5000);                          // bem maior que os antigos buffers de 600/1024 bytes
        inv[0] = {STONE, 5}; inv[1] = {CHEST_ITEM, 1, 0, nb}; inv[2] = {CHEST_ITEM, 1}; offh = {CHEST_ITEM, 1, 0, "BET1|sentinela"}; cur = {DIRT, 3, 0, "BET1|na-mao"}; armor[0] = {STONE, 1};
        savePlayer();
        { const std::string raw = readAll(playerFile()); CHECK(raw.find("\nx BET1|") != std::string::npos || raw.rfind("x BET1|", 0) == 0); }
        clearInv(); CHECK(loadPlayer());
        CHECK(inv[0].id == STONE && inv[0].n == 5 && inv[0].nbt.empty()); CHECK(inv[1].id == CHEST_ITEM && inv[1].nbt == nb); CHECK(inv[2].id == CHEST_ITEM && inv[2].nbt.empty());
        CHECK(offh.nbt == "BET1|sentinela"); CHECK(cur.id == DIRT && cur.nbt == "BET1|na-mao"); CHECK(armor[0].id == STONE && armor[0].nbt.empty());
        // marcador de rede "?hash" nunca vai para o disco
        clearInv(); inv[0] = {CHEST_ITEM, 1, 0, "?777"}; savePlayer(); clearInv(); loadPlayer(); CHECK(inv[0].id == CHEST_ITEM && inv[0].nbt.empty());
        // save antigo (sem linhas x): carrega igual
        { FILE* f = fopen(playerFile().c_str(), "wb"); fprintf(f, "i 0 7 %d\ni 4 2 %d\nhp 18 15\n", (int)STONE, (int)DIRT); fclose(f); }
        clearInv(); CHECK(loadPlayer()); CHECK(inv[0].id == STONE && inv[0].n == 7 && inv[4].id == DIRT && inv[4].n == 2 && inv[0].nbt.empty() && gHP == 18);
        // linha x solta (sem pilha antes) e x depois de pilha invalida: ignoradas
        { FILE* f = fopen(playerFile().c_str(), "wb"); fprintf(f, "x BET1|solto\ni 0 7 %d\nx BET1|a\nhp 20 20\nx BET1|depois-do-hp\n", (int)STONE); fclose(f); }
        clearInv(); CHECK(loadPlayer()); CHECK(inv[0].nbt == "BET1|a");
        fs::remove_all("saves/_t_nbt"); gCurDir.clear();
    }
    SECTION("slot_serialize_nbt: chests.dat com NBT interno");
    {
        fs::create_directories("saves/_t_nbt2"); gChests.clear();
        putChest(2, 2, 0); ChestInv* c = chestAt(2, AY, 2, true); ChestInv inner; inner.s[0] = {DIRT, 2};
        c->s[0] = {STONE, 4}; c->s[5] = {CHEST_ITEM, 1, 0, chestNbtPack(inner)}; c->s[6] = {CHEST_ITEM, 1};
        const ChestInv orig = *c; chestsSave("_t_nbt2"); gChests.clear(); chestsLoad("_t_nbt2");
        ChestInv* d = chestAt(2, AY, 2, false); CHECK(d != nullptr); if (d) { CHECK(chestEq(orig, *d)); CHECK(d->s[0].id == STONE && d->s[0].nbt.empty()); CHECK(d->s[5].nbt == chestNbtPack(inner)); CHECK(d->s[6].id == CHEST_ITEM && d->s[6].nbt.empty()); }
        fs::remove_all("saves/_t_nbt2"); gChests.clear();
    }

    SECTION("slot_packet_nbt: NP_NBT em fragmentos + cauda de NP_CHEST + NP_DROP");
    {
        std::string big = "BET1|"; for (int i = 0; big.size() < 2500; i++) big += std::to_string(i) + ",";
        const uint32_t h = nbtHash(big); CHECK(h != 0); CHECK(nbtHash("") == 0);
        auto fr = nbtFragments(big); CHECK(fr.size() == 3);
        for (auto& b : fr) CHECK(b.n <= 1400 && b.n < 1100);
        gNbtCache.clear(); gNbtAsm.clear();
        CHECK(nbtResolve(h) == "?" + std::to_string(h)); CHECK(nbtHash(nbtResolve(h)) == h);   // desconhecido: marcador que carrega o hash
        feed(fr[2], nbtNetRecv); feed(fr[0], nbtNetRecv); feed(fr[0], nbtNetRecv);               // fora de ordem + duplicado
        CHECK(!gNbtCache.count(h));
        feed(fr[1], nbtNetRecv); CHECK(gNbtCache.count(h) && gNbtCache[h] == big); CHECK(gNbtAsm.empty());
        CHECK(nbtResolve(h) == big);
        // pedaco adulterado: o hash nao confere => nao entra no cache
        gNbtCache.clear(); gNbtAsm.clear(); { NBuf bad = fr[1]; bad.d[bad.n - 1] ^= 0x55; feed(fr[0], nbtNetRecv); feed(bad, nbtNetRecv); feed(fr[2], nbtNetRecv); CHECK(!gNbtCache.count(h)); }
        // pacote truncado / sem dados: ignorado sem travar
        { NBuf t = fr[0]; t.n = 12; feed(t, nbtNetRecv); NBuf z; z.n = 2; feed(z, nbtNetRecv); CHECK(!gNbtCache.count(h)); }
        // cauda de NP_CHEST/NP_ENDER
        ChestInv c; c.s[0] = {STONE, 3}; c.s[4] = {CHEST_ITEM, 1, 0, big}; c.s[9] = {CHEST_ITEM, 1, 0, "BET1|curto"}; c.s[10] = {DIRT, 2};
        NBuf pk = netMk(NP_CHEST); chestNbtTailPut(pk, c); CHECK(pk.n == 2 + 1 + 2 * 5);
        gNbtCache.clear(); gNbtCache[nbtHash("BET1|curto")] = "BET1|curto"; gNbtCache[h] = big;
        ChestInv t; for (int i = 0; i < CHEST_SLOTS; i++) { t.s[i].id = c.s[i].id; t.s[i].n = c.s[i].n; }
        { NBuf r = pk; r.r = 2; chestNbtTailGet(r, t); }
        CHECK(chestEq(c, t));
        { gNbtCache.erase(h); ChestInv t2; for (int i = 0; i < CHEST_SLOTS; i++) { t2.s[i].id = c.s[i].id; t2.s[i].n = c.s[i].n; } NBuf r = pk; r.r = 2; chestNbtTailGet(r, t2);
          CHECK(t2.s[4].nbt == "?" + std::to_string(h)); CHECK(nbtHash(t2.s[4].nbt) == h); CHECK(t2.s[9].nbt == "BET1|curto");   // pacote NBT perdido: marcador, nao apaga o item
          NBuf back = netMk(NP_CHEST); chestNbtTailPut(back, t2); NBuf rr = back; rr.r = 2; ChestInv t3; t3.s[4].id = CHEST_ITEM; t3.s[4].n = 1; t3.s[9].id = CHEST_ITEM; t3.s[9].n = 1; gNbtCache[h] = big; chestNbtTailGet(rr, t3); CHECK(t3.s[4].nbt == big); }   // ao voltar, o dono do texto reconhece pelo hash
        // pacote antigo (sem cauda): nada muda; cauda com slot invalido: ignorada
        { NBuf old = netMk(NP_CHEST); NBuf r = old; r.r = 2; ChestInv q; q.s[0] = {STONE, 1}; chestNbtTailGet(r, q); CHECK(q.s[0].nbt.empty()); }
        { NBuf w = netMk(NP_CHEST); w.put<uint8_t>(1); w.put<uint8_t>(200); w.put<uint32_t>(5); NBuf r = w; r.r = 2; ChestInv q; chestNbtTailGet(r, q); CHECK(q.empty()); }
        // NP_DROP: hash no fim; pacote sem o campo continua legivel (hash 0)
        DropEnt d{}; d.id = STONE; d.n = 3; d.nid = 77; d.nbt = big; NBuf dp = netDropPacket(d); CHECK(nbtHash(d.nbt) == h);
        uint32_t last; memcpy(&last, dp.d + dp.n - 4, 4); CHECK(last == h);
        DropEnt d0{}; d0.id = STONE; d0.n = 1; NBuf dp0 = netDropPacket(d0); memcpy(&last, dp0.d + dp0.n - 4, 4); CHECK(last == 0 && dp0.n == dp.n);
        // spawnDrop leva o NBT
        DROPS.clear(); spawnDrop(1, AY, 1, 0, 0, 0, CHEST_ITEM, 1, 10, 0, "BET1|drop"); CHECK(DROPS.size() == 1 && DROPS[0].nbt == "BET1|drop"); DROPS.clear();
    }

    SECTION("slot_stack_nbt: so empilha item + NBT iguais");
    {
        clearInv(); CHECK(addItem(CHEST_ITEM, 1, "A") == 0); CHECK(addItem(CHEST_ITEM, 1, "A") == 0); CHECK(addItem(CHEST_ITEM, 1, "B") == 0); CHECK(addItem(CHEST_ITEM, 1) == 0);
        CHECK(inv[0].n == 2 && inv[0].nbt == "A" && inv[1].n == 1 && inv[1].nbt == "B" && inv[2].n == 1 && inv[2].nbt.empty() && inv[3].id == 0);
        // clique: soltar pilha com NBT diferente troca (nao junta); igual junta; metade carrega o NBT
        clearInv(); inv[0] = {CHEST_ITEM, 2, 0, "A"}; cur = {CHEST_ITEM, 1, 0, "B"}; slotClick(inv[0], SDL_BUTTON_LEFT);
        CHECK(cur.nbt == "A" && cur.n == 2 && inv[0].nbt == "B" && inv[0].n == 1);
        clearInv(); inv[0] = {CHEST_ITEM, 4, 0, "A"}; slotClick(inv[0], SDL_BUTTON_RIGHT); CHECK(cur.n == 2 && cur.nbt == "A" && inv[0].n == 2 && inv[0].nbt == "A");
        slotClick(inv[1], SDL_BUTTON_RIGHT); CHECK(inv[1].id == CHEST_ITEM && inv[1].n == 1 && inv[1].nbt == "A" && cur.n == 1);
        clearInv(); inv[0] = {CHEST_ITEM, 1, 0, "A"}; cur = {CHEST_ITEM, 1, 0, "A"}; slotClick(inv[0], SDL_BUTTON_LEFT); CHECK(inv[0].n == 2 && cur.id == 0);
        // shift-click (mergeInto) respeita o NBT
        clearInv(); inv[9] = {CHEST_ITEM, 1, 0, "A"}; inv[0] = {CHEST_ITEM, 1, 0, "B"}; Slot t = {CHEST_ITEM, 1, 0, "A"}; mergeInto(t, ordRange(0, 9, false)); CHECK(t.id == 0 && inv[0].n == 1 && inv[0].nbt == "B" && inv[1].nbt == "A");
        // cursor devolvido ao inventario/chao leva o NBT
        clearInv(); cur = {CHEST_ITEM, 1, 0, "Z"}; returnCursor(); CHECK(inv[0].nbt == "Z");
        // chestAdd e chestEq
        ChestInv c; CHECK(chestAdd(c, CHEST_ITEM, 1, "A") == 0 && chestAdd(c, CHEST_ITEM, 1, "A") == 0 && chestAdd(c, CHEST_ITEM, 1, "B") == 0); CHECK(c.s[0].n == 2 && c.s[1].nbt == "B");
        ChestInv e = c; CHECK(chestEq(c, e)); e.s[1].nbt = "C"; CHECK(!chestEq(c, e));
        // quebrar bau solta as pilhas com NBT
        DROPS.clear(); gChests.clear(); putChest(4, 4, 0); ChestInv* k = chestAt(4, AY, 4, true); k->s[0] = {CHEST_ITEM, 1, 0, "BET1|x"}; chestOnRemoved(4, AY, 4);
        bool found = false; for (auto& d : DROPS) if (d.id == CHEST_ITEM && d.nbt == "BET1|x") found = true; CHECK(found); DROPS.clear(); gChests.clear();
    }

    SECTION("pick_chest: copiar o conteudo e colocar de volta");
    {
        gChests.clear(); clearInv(); putChest(6, 6, 0); ChestInv* c = chestAt(6, AY, 6, true); c->s[0] = {STONE, 10}; c->s[13] = {DIRT, 64}; c->s[26] = {GRAVEL, 3};
        const std::string nb = chestPickNbt(6, AY, 6); CHECK(!nb.empty());
        int sel = 0; CHECK(invPickPut(sel, CHEST_ITEM, true, nb)); CHECK(inv[0].id == CHEST_ITEM && inv[0].n == 1 && inv[0].nbt == nb);   // criativo cria com o NBT
        sel = 0; CHECK(invPickPut(sel, CHEST_ITEM, true, nb) && sel == 0 && inv[1].id == 0);                                                 // ja no hotbar com o mesmo NBT: so seleciona
        sel = 0; CHECK(invPickPut(sel, CHEST_ITEM, true, std::string())); CHECK(inv[1].nbt == nb && inv[0].nbt.empty() && inv[0].id == CHEST_ITEM);   // bau vazio != bau cheio: cria outro e guarda o cheio no slot vazio
        // sobrevivencia (make = false): so acha o que ja tem
        clearInv(); inv[12] = {CHEST_ITEM, 1, 0, nb}; sel = 2; CHECK(!invPickPut(sel, CHEST_ITEM, false, std::string())); CHECK(invPickPut(sel, CHEST_ITEM, false, nb) && inv[2].nbt == nb && inv[12].id == 0);
        // colocar pelo caminho real (rsPlace + gPlaceNbt) recria o conteudo no bau novo
        int hh[3] = {8, AY - 1, 8}, pp[3] = {8, AY, 8}; gPlaceNbt = nb; const bool ok = rsPlace(XB_CHEST, 0, hh, pp, 0.f, 8.5, AY + 2.0, 12.0); gPlaceNbt.clear();
        CHECK(ok); ChestInv* d = chestAt(8, AY, 8, false); CHECK(d != nullptr); if (d) { CHECK(chestEq(*c, *d)); CHECK(d->s[13].id == DIRT && d->s[13].n == 64 && d->s[26].n == 3); }
        // sem NBT: bau novo vazio
        int hh2[3] = {10, AY - 1, 8}, pp2[3] = {10, AY, 8}; CHECK(rsPlace(XB_CHEST, 0, hh2, pp2, 0.f, 10.5, AY + 2.0, 12.0)); CHECK(chestAt(10, AY, 8, false) == nullptr || chestAt(10, AY, 8, false)->empty());
        // item com NBT invalido/marcador de rede coloca bau vazio, sem travar
        int hh3[3] = {12, AY - 1, 8}, pp3[3] = {12, AY, 8}; gPlaceNbt = "?999"; CHECK(rsPlace(XB_CHEST, 0, hh3, pp3, 0.f, 12.5, AY + 2.0, 12.0)); gPlaceNbt.clear(); CHECK(chestAt(12, AY, 8, false) == nullptr || chestAt(12, AY, 8, false)->empty());
        // baus armadilhado: tambem copia; Ender (2) nao; shulker (3) copia so com conteudo
        putChest(14, 6, 1); chestAt(14, AY, 6, true)->s[0] = {STONE, 1}; CHECK(!chestPickNbt(14, AY, 6).empty());
        putChest(16, 6, 2); CHECK(chestPickNbt(16, AY, 6).empty()); putChest(18, 6, 3); CHECK(chestPickNbt(18, AY, 6).empty()); chestAt(18, AY, 6, true)->s[0] = {STONE, 1}; CHECK(!chestPickNbt(18, AY, 6).empty());   // patchChest.2: shulker vazio nao gera NBT; COM conteudo copia como o bau
        // bau duplo: cada metade copia so o proprio conteudo
        gChests.clear(); setBM(2, AY, 12, AIR, 0); setBM(3, AY, 12, AIR, 0);
        int ha[3] = {2, AY - 1, 12}, pa[3] = {2, AY, 12}, hb[3] = {3, AY - 1, 12}, pb[3] = {3, AY, 12};
        CHECK(rsPlace(XB_CHEST, 0, ha, pa, 0.f, 2.5, AY + 2.0, 16.0)); CHECK(rsPlace(XB_CHEST, 0, hb, pb, 0.f, 3.5, AY + 2.0, 16.0));
        if (chestIsDbl(getM(2, AY, 12))) { chestAt(2, AY, 12, true)->s[0] = {STONE, 1}; chestAt(3, AY, 12, true)->s[0] = {DIRT, 1};
            const std::string n1 = chestPickNbt(2, AY, 12), n2 = chestPickNbt(3, AY, 12); CHECK(!n1.empty() && !n2.empty() && n1 != n2); }
        else { printf("  (aviso: baus nao formaram duplo no teste)\n"); }
        // nao-bau
        CHECK(chestPickNbt(1, AY - 1, 1).empty()); CHECK(chestPickNbt(1, AY, 1).empty());
        gChests.clear();
    }

    SECTION("pick_chest_empty: bau vazio nao gera NBT");
    {
        gChests.clear(); clearInv(); putChest(6, 14, 0);
        CHECK(chestPickNbt(6, AY, 14).empty());                                       // sem conteudo registrado
        chestAt(6, AY, 14, true); CHECK(chestPickNbt(6, AY, 14).empty());             // conteudo registrado mas vazio
        chestAt(6, AY, 14, true)->s[3] = {STONE, 1}; chestAt(6, AY, 14, true)->s[3] = Slot(); CHECK(chestPickNbt(6, AY, 14).empty());   // esvaziado depois
        int sel = 0; CHECK(invPickPut(sel, CHEST_ITEM, true, chestPickNbt(6, AY, 14))); CHECK(inv[0].nbt.empty());
        addItem(CHEST_ITEM, 1); CHECK(inv[0].n == 2);                                 // bau vazio empilha com bau comum
        gChests.clear();
    }
    printf("test_pick_nbt: %d verificacoes, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
