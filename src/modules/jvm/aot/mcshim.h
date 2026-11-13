// patchJVM.4: shim do Minecraft/Forge 1.12.2 para o runtime AOT. Chaves em SRG (jar de release). Ligacao com o jogo: rt::host() (callbacks); sem host, os mundos retornam ar/null.
struct Host {
    std::function<Obj*(int, int, int)> getState, getTile; std::function<bool(int, int, int, Obj*, int)> setState; std::function<void(int, int, int, Obj*)> setTile; std::string modId = "mod"; std::vector<Obj*> registered;
    std::function<void(Obj*, Obj*)> openInventory; std::function<Obj*(Obj*)> smelt; std::function<void(const std::string&, double, double, double)> particle; };
inline Host& host() { static Host h; return h; }
inline Obj* mkStack(Obj* item, int n, int meta) { Obj* s = mk("net/minecraft/item/ItemStack"); s->f["item"].a = item; s->f["count"].i = n; s->f["meta"].i = meta; return s; }
inline Obj* emptyStack() { static Obj* e = nullptr; if (!e) e = mkStack(nullptr, 0, 0); return e; }
inline bool stackEmpty(Obj* s) { return !s || !s->f["item"].a || s->f["count"].i <= 0; }
inline Obj* mkPos(int x, int y, int z) { Obj* p = mk("net/minecraft/util/math/BlockPos"); p->f["x"].i = x; p->f["y"].i = y; p->f["z"].i = z; return p; }
inline std::string propKey(Obj* p) { char b[40]; snprintf(b, 40, "p%p", (void*)p); return b; }
inline Obj* facing(int i) { static const char* nm[] = {"DOWN", "UP", "NORTH", "SOUTH", "WEST", "EAST"}; return ci("net/minecraft/util/EnumFacing")->statics[nm[((i % 6) + 6) % 6]].a; }
inline void initMc2(); inline void initMc3(); inline void initMc4(); inline void initMc5(); inline void initMc6();
inline void initMc() {
    for (auto p : {"net/minecraft/block/Block:java/lang/Object", "net/minecraft/block/BlockContainer:net/minecraft/block/Block", "net/minecraft/item/Item:java/lang/Object", "net/minecraft/item/ItemBlock:net/minecraft/item/Item",
                   "net/minecraft/tileentity/TileEntity:java/lang/Object", "net/minecraft/util/NonNullList:java/util/ArrayList", "net/minecraft/util/EnumFacing:java/lang/Enum", "net/minecraft/entity/player/EntityPlayer:java/lang/Object",
                   "net/minecraft/world/World:java/lang/Object", "net/minecraft/nbt/NBTTagCompound:java/lang/Object", "net/minecraft/block/state/BlockStateImpl:java/lang/Object"}) {
        std::string s = p; size_t c = s.find(':'); declare(s.substr(0, c).c_str(), s.substr(c + 1).c_str()); }
    ci("net/minecraft/block/state/BlockStateImpl")->ifs = {"net/minecraft/block/state/IBlockState"};
    ClassInfo* ef = ci("net/minecraft/util/EnumFacing"); ef->inited = true; const char* nm[] = {"DOWN", "UP", "NORTH", "SOUTH", "WEST", "EAST"};
    for (int i = 0; i < 6; i++) { Obj* o = mk("net/minecraft/util/EnumFacing"); o->f["name"].a = str(nm[i]); o->f["ordinal"].i = i; ef->statics[nm[i]].a = o; }
    ci("net/minecraft/item/ItemStack")->inited = true; ci("net/minecraft/item/ItemStack")->statics["field_190927_a"].a = emptyStack(); ci("net/minecraft/item/ItemStack")->statics["EMPTY"].a = emptyStack();
    const std::string IS = "net/minecraft/item/ItemStack.", ITM = "Lnet/minecraft/item/Item;", ISD = "Lnet/minecraft/item/ItemStack;";
    N_((IS + "<init>(" + ITM + ")V").c_str(), [](V* a) { a[0].a->f["item"] = a[1]; a[0].a->f["count"].i = 1; return V(); });
    N_((IS + "<init>(" + ITM + "I)V").c_str(), [](V* a) { a[0].a->f["item"] = a[1]; a[0].a->f["count"] = a[2]; return V(); });
    N_((IS + "<init>(" + ITM + "II)V").c_str(), [](V* a) { a[0].a->f["item"] = a[1]; a[0].a->f["count"] = a[2]; a[0].a->f["meta"] = a[3]; return V(); });
    N_((IS + "<init>(Lnet/minecraft/block/Block;)V").c_str(), [](V* a) { a[0].a->f["item"].a = nullptr; a[0].a->f["blk"] = a[1]; a[0].a->f["count"].i = 1; return V(); });
    N_((IS + "func_190926_b()Z").c_str(), [](V* a) { return vi(stackEmpty(a[0].a)); }); N_((IS + "func_190916_E()I").c_str(), [](V* a) { return a[0].a->f["count"]; });
    N_((IS + "func_190920_e(I)V").c_str(), [](V* a) { a[0].a->f["count"] = a[1]; return V(); }); N_((IS + "func_190918_g(I)V").c_str(), [](V* a) { a[0].a->f["count"].i -= a[1].i; return V(); });
    N_((IS + "func_190917_f(I)V").c_str(), [](V* a) { a[0].a->f["count"].i += a[1].i; return V(); }); N_((IS + "func_77973_b()" + ITM).c_str(), [](V* a) { return a[0].a->f["item"]; });
    N_((IS + "func_77960_j()I").c_str(), [](V* a) { return a[0].a->f["meta"]; });
    N_((IS + "func_77969_a(" + ISD + ")Z").c_str(), [](V* a) { Obj* y = a[1].a; return vi(y && a[0].a->f["item"].a == y->f["item"].a && a[0].a->f["meta"].i == y->f["meta"].i); });
    N_((IS + "func_77946_l()" + ISD).c_str(), [](V* a) { Obj* o = new Obj(*a[0].a); return va(o); });
    N_((IS + "func_77978_p()Lnet/minecraft/nbt/NBTTagCompound;").c_str(), [](V* a) { return a[0].a->f["tag"]; }); N_((IS + "func_77942_o()Z").c_str(), [](V* a) { return vi(a[0].a->f["tag"].a != nullptr); });
    N_((IS + "func_77982_d(Lnet/minecraft/nbt/NBTTagCompound;)V").c_str(), [](V* a) { a[0].a->f["tag"] = a[1]; return V(); }); N_((IS + "func_77976_d()I").c_str(), [](V* a) { return vi(64); });
    N_(("net/minecraft/item/Item.func_150898_a(Lnet/minecraft/block/Block;)" + ITM).c_str(), [](V* a) { Obj* b = nn(a[0].a); if (!b->f["$item"].a) { Obj* it = mk("net/minecraft/item/ItemBlock"); it->f["block"].a = b; b->f["$item"].a = it; } return b->f["$item"]; });
    N_("net/minecraft/util/NonNullList.func_191197_a(ILjava/lang/Object;)Lnet/minecraft/util/NonNullList;", [](V* a) { Obj* l = mk("net/minecraft/util/NonNullList"); l->arr.assign(a[0].i, a[1]); return va(l); });
    N_("net/minecraft/util/NonNullList.func_191196_a()Lnet/minecraft/util/NonNullList;", [](V*) { return va(mk("net/minecraft/util/NonNullList")); });
    const std::string BP = "net/minecraft/util/math/BlockPos.";
    N_((BP + "<init>(III)V").c_str(), [](V* a) { a[0].a->f["x"] = a[1]; a[0].a->f["y"] = a[2]; a[0].a->f["z"] = a[3]; return V(); });
    N_((BP + "func_177958_n()I").c_str(), [](V* a) { return a[0].a->f["x"]; }); N_((BP + "func_177956_o()I").c_str(), [](V* a) { return a[0].a->f["y"]; }); N_((BP + "func_177952_p()I").c_str(), [](V* a) { return a[0].a->f["z"]; });
    N_((BP + "func_177982_a(III)Lnet/minecraft/util/math/BlockPos;").c_str(), [](V* a) { Obj* p = a[0].a; return va(mkPos(p->f["x"].i + a[1].i, p->f["y"].i + a[2].i, p->f["z"].i + a[3].i)); });
    const char* dn[] = {"func_177977_b", "func_177984_a", "func_177978_c", "func_177968_d", "func_177976_e", "func_177974_f"}; static const int dx[] = {0, 0, 0, 0, -1, 1}, dy[] = {-1, 1, 0, 0, 0, 0}, dz[] = {0, 0, -1, 1, 0, 0};
    for (int i = 0; i < 6; i++) N_((BP + dn[i] + "()Lnet/minecraft/util/math/BlockPos;").c_str(), [i](V* a) { Obj* p = a[0].a; return va(mkPos(p->f["x"].i + dx[i], p->f["y"].i + dy[i], p->f["z"].i + dz[i])); });
    N_((BP + "func_177972_a(Lnet/minecraft/util/EnumFacing;)Lnet/minecraft/util/math/BlockPos;").c_str(), [](V* a) { Obj* p = a[0].a; int i = nn(a[1].a)->f["ordinal"].i; return va(mkPos(p->f["x"].i + dx[i], p->f["y"].i + dy[i], p->f["z"].i + dz[i])); });
    N_((BP + "hashCode()I").c_str(), [](V* a) { Obj* p = a[0].a; return vi((p->f["y"].i + p->f["z"].i * 31) * 31 + p->f["x"].i); });
    N_((BP + "equals(Ljava/lang/Object;)Z").c_str(), [](V* a) { Obj *p = a[0].a, *q = a[1].a; return vi(q && q->ci == p->ci && p->f["x"].i == q->f["x"].i && p->f["y"].i == q->f["y"].i && p->f["z"].i == q->f["z"].i); });
    const std::string EF = "net/minecraft/util/EnumFacing.";
    N_((EF + "func_176734_d()Lnet/minecraft/util/EnumFacing;").c_str(), [](V* a) { static const int op[] = {1, 0, 3, 2, 5, 4}; return va(facing(op[a[0].a->f["ordinal"].i])); });
    N_((EF + "func_82601_c()I").c_str(), [](V* a) { return vi(dx[a[0].a->f["ordinal"].i]); }); N_((EF + "func_96559_d()I").c_str(), [](V* a) { return vi(dy[a[0].a->f["ordinal"].i]); }); N_((EF + "func_82599_e()I").c_str(), [](V* a) { return vi(dz[a[0].a->f["ordinal"].i]); });
    const std::string ST = "net/minecraft/block/state/IBlockState.";
    N_((ST + "func_177229_b(Lnet/minecraft/block/properties/IProperty;)Ljava/lang/Comparable;").c_str(), [](V* a) { return a[0].a->f[propKey(a[1].a)]; });
    N_((ST + "func_177226_a(Lnet/minecraft/block/properties/IProperty;Ljava/lang/Comparable;)Lnet/minecraft/block/state/IBlockState;").c_str(), [](V* a) { Obj* o = new Obj(*a[0].a); o->f[propKey(a[1].a)] = a[2]; return va(o); });
    N_((ST + "func_177230_c()Lnet/minecraft/block/Block;").c_str(), [](V* a) { return a[0].a->f["block"]; });
    const std::string W = "net/minecraft/world/World.", BPD = "Lnet/minecraft/util/math/BlockPos;";
    auto xyz = [](Obj* p, int& x, int& y, int& z) { nn(p); x = p->f["x"].i; y = p->f["y"].i; z = p->f["z"].i; };
    N_((W + "func_180495_p(" + BPD + ")Lnet/minecraft/block/state/IBlockState;").c_str(), [xyz](V* a) { int x, y, z; xyz(a[1].a, x, y, z); return va(host().getState ? host().getState(x, y, z) : nullptr); });
    N_((W + "func_175625_s(" + BPD + ")Lnet/minecraft/tileentity/TileEntity;").c_str(), [xyz](V* a) { int x, y, z; xyz(a[1].a, x, y, z); return va(host().getTile ? host().getTile(x, y, z) : nullptr); });
    N_((W + "func_180501_a(" + BPD + "Lnet/minecraft/block/state/IBlockState;I)Z").c_str(), [xyz](V* a) { int x, y, z; xyz(a[1].a, x, y, z); return vi(host().setState ? host().setState(x, y, z, a[2].a, a[3].i) : true); });
    N_((W + "func_175688_a(Lnet/minecraft/util/EnumParticleTypes;DDDDDD[I)V").c_str(), [](V* a) { if (host().particle) host().particle("particle", a[2].d, a[3].d, a[4].d); return V(); });
    N_("net/minecraft/entity/player/EntityPlayer.func_71007_a(Lnet/minecraft/inventory/IInventory;)V", [](V* a) { if (host().openInventory) host().openInventory(a[0].a, a[1].a); return V(); });
    N_("net/minecraft/entity/player/EntityPlayer.func_71029_a(Lnet/minecraft/stats/StatBase;)V", [](V*) { return V(); });
    N_("net/minecraft/item/crafting/FurnaceRecipes.func_77602_a()Lnet/minecraft/item/crafting/FurnaceRecipes;", [](V*) { static Obj* f = mk("net/minecraft/item/crafting/FurnaceRecipes"); return va(f); });
    N_("net/minecraft/item/crafting/FurnaceRecipes.func_151395_a(Lnet/minecraft/item/ItemStack;)Lnet/minecraft/item/ItemStack;", [](V* a) { Obj* r = host().smelt ? host().smelt(a[1].a) : nullptr; return va(r ? r : emptyStack()); });
    for (auto c : {"net/minecraft/block/Block", "net/minecraft/block/BlockContainer"}) {
        N_((std::string(c) + ".func_149647_a(Lnet/minecraft/creativetab/CreativeTabs;)Lnet/minecraft/block/Block;").c_str(), [](V* a) { return a[0]; });
        N_((std::string(c) + ".<init>(Lnet/minecraft/block/material/Material;)V").c_str(), [](V*) { return V(); }); }
    const std::string NB = "net/minecraft/nbt/NBTTagCompound.", SD = "(Ljava/lang/String;";
    N_((NB + "<init>()V").c_str(), [](V*) { return V(); });
    auto setN = [&](const char* nm, const char* t) { N_((NB + nm + SD + t + ")V").c_str(), [](V* a) { a[0].a->f[nn(a[1].a)->s] = a[2]; return V(); }); };
    auto getN = [&](const char* nm, const char* t) { N_((NB + nm + SD + ")" + t).c_str(), [](V* a) { auto& m = a[0].a->f; auto it = m.find(nn(a[1].a)->s); return it == m.end() ? V() : it->second; }); };
    setN("func_74768_a", "I"); getN("func_74762_e", "I"); setN("func_74778_a", "Ljava/lang/String;"); getN("func_74779_i", "Ljava/lang/String;"); setN("func_74757_a", "Z"); getN("func_74767_n", "Z");
    setN("func_74772_a", "J"); getN("func_74763_f", "J"); setN("func_74776_a", "F"); getN("func_74760_g", "F"); setN("func_74780_a", "D"); getN("func_74769_h", "D");
    setN("func_74782_a", "Lnet/minecraft/nbt/NBTBase;"); getN("func_74781_a", "Lnet/minecraft/nbt/NBTBase;");
    N_((NB + "func_74779_i" + SD + ")Ljava/lang/String;").c_str(), [](V* a) { auto& m = a[0].a->f; auto it = m.find(nn(a[1].a)->s); return it == m.end() ? va(str("")) : it->second; });
    N_((NB + "func_74764_b" + SD + ")Z").c_str(), [](V* a) { return vi(a[0].a->f.count(nn(a[1].a)->s)); }); N_((NB + "func_150297_b" + SD + "I)Z").c_str(), [](V* a) { return vi(a[0].a->f.count(nn(a[1].a)->s)); });
    N_((NB + "func_82580_o" + SD + ")V").c_str(), [](V* a) { a[0].a->f.erase(nn(a[1].a)->s); return V(); });
    N_((NB + "func_74775_l" + SD + ")Lnet/minecraft/nbt/NBTTagCompound;").c_str(), [](V* a) { auto& m = a[0].a->f; auto it = m.find(nn(a[1].a)->s); return it == m.end() || !it->second.a ? va(mk("net/minecraft/nbt/NBTTagCompound")) : it->second; });
    initMc2(); initMc3(); initMc4(); initMc5(); initMc6();
}
#include "mcshim2.h"
#include "mcshim3.h"
#include "mcshim4.h"
#include "mcshim5.h"
#include "mcshim6.h"
#include "modload.h"
