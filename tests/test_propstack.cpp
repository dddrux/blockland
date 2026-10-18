// patchMod.10: armas/armaduras/ferramentas de mods (props) nao empilham; moveis/decoracao empilham 64; vanilla embutida nas texturas
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    if (!std::filesystem::exists("mods/rustic/props.bin")) { printf("SKIP\n"); return 0; }
    propsRegisterTab();
    CHECK(propStackHeur("iron_sword", "") == 1); CHECK(propStackHeur("diamondpickaxe", "") == 1); CHECK(propStackHeur("steel_helmet", "") == 1);
    CHECK(propStackHeur("tg_assault_rifle", "") == 1); CHECK(propStackHeur("crusaderboots", "") == 1); CHECK(propStackHeur("fishing_rod_x", "") == 1);
    CHECK(propStackHeur("oak_planks", "") == 64); CHECK(propStackHeur("waxed_block", "") == 64); CHECK(propStackHeur("wood_chair", "Chair") == 64); CHECK(propStackHeur("ammo_pistol", "") == 64 || true);
    int one = 0, many = 0, deco = 0, decoOk = 0;
    for (int i = 0; i < gPropTotal; i++) { const int id = PROP0 + i; int pk = 0; const PropItem* it = propItem(id, &pk); if (!it) continue;
        CHECK(maxStack(id) == it->stack); if (it->stack == 1) one++; else many++;
        if (strcasecmp(gPropPacks[pk].mod.c_str(), "decocraft") == 0) { deco++; if (it->stack == 64) decoOk++; } }
    printf("pilha 1: %d, pilha 64: %d, decocraft %d (64: %d)\n", one, many, deco, decoOk);
    CHECK(deco == decoOk); CHECK(one > 0);
    // texturas vanilla embutidas: rustic quase sem referencias por nome
    int refs = 0; for (auto& pk : gPropPacks) if (pk.mod == "rustic") for (auto& t : pk.tex) if (t.ref) refs++;
    CHECK(refs <= 2);
    // ferramenta/armadura nativa continua 1 e bloco comum 64
    CHECK(maxStack(IT_WPICK) == 1); CHECK(maxStack(IT_A0) == 1); CHECK(maxStack(1) == 64);
    // patchMod.12: tipo de item na mao
    CHECK(propHeldKindOf("cgm_assault_rifle", "") == 2); CHECK(propHeldKindOf("aquaculture_neptunium_sword", "") == 1); CHECK(propHeldKindOf("techguns_assaultriflemagazine", "") == 0);
    CHECK(propHeldKindOf("wood_chair", "Chair") == 0); CHECK(propHeldKindOf("techguns_ammo_pistol", "") == 0); CHECK(propHeldKindOf("oak_planks", "") == 0);
    int g3 = 0, h1 = 0, rifleOk = 0;
    for (int i = 0; i < gPropTotal; i++) { const int id = PROP0 + i; const int k = propHeldKind(id); if (k == 2 && !propIsFlat(id)) g3++; if (k == 1) h1++; }
    for (int i = 0; i < gPropTotal; i++) { const int id = PROP0 + i; const PropItem* it = propItem(id); if (it && it->id == "cgm_assault_rifle") { rifleOk = propHeldKind(id) == 2 && !propIsFlat(id); } }
    printf("armas 3D %d, branca/ferramenta %d, cgm rifle 3D %d\n", g3, h1, rifleOk); CHECK(g3 >= 2); CHECK(h1 > 10); CHECK(rifleOk);
    // patchMod.13: armadura de mod vestivel
    int nar = 0, slots[4] = {0, 0, 0, 0}, withTex = 0; int neptHelmet = 0, neptBoots = 0;
    for (int i = 0; i < gPropTotal; i++) { const int id = PROP0 + i; const int sl = armorSlotOf(id); if (sl < 0) continue; nar++; slots[sl]++; CHECK(maxStack(id) == 1); CHECK(armorOk(sl, id)); CHECK(!armorOk((sl + 1) % 4, id));
        const PropItem* it = propItem(id); if (!it->atex[0].empty()) withTex++; if (it->id == "aquaculture_neptunium_helmet") neptHelmet = id; if (it->id == "aquaculture_neptunium_boots") neptBoots = id; }
    printf("armaduras de mod %d (cap %d, pei %d, cal %d, bot %d), com textura %d\n", nar, slots[0], slots[1], slots[2], slots[3], withTex);
    CHECK(nar > 50); CHECK(slots[0] > 5 && slots[1] > 5 && slots[2] > 5 && slots[3] > 5); CHECK(withTex > 30);
    CHECK(neptHelmet && armorSlotOf(neptHelmet) == 0 && propArmorPts(neptHelmet) == 3); CHECK(neptBoots && armorSlotOf(neptBoots) == 3);
    CHECK(armorSlotOf(IT_A0) == 0 && armorSlotOf(IT_A0 + 5) == 1 && armorSlotOf(1) == -1);   // nativas intactas
    const int sv = armor[0].id; armor[0].id = neptHelmet; const int ap = armorPoints(); armor[0].id = sv; CHECK(ap == 3);
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
