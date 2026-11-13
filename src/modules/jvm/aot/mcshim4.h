// patchJVM.9: fecha o que faltava do ironfurnaces + streams (avaliacao imediata), Collectors.toList/toSet, regex (std::regex), LinkedHashMap, Map.computeIfAbsent, addAll.
inline std::vector<V> elems(Obj* c) { nn(c); if (c->ci->name == "java/util/HashSet") { std::vector<V> k; for (auto& e : mapOrder(c)) k.push_back(e.a->f["key"]); return k; } return c->arr; }
inline Obj* mkStream(const std::vector<V>& v) { Obj* s = mk("java/util/stream/Stream"); s->arr = v; return s; }
inline V call1(Obj* f, const char* n, const char* d, V x) { V a[1] = {x}; return callFn(f, n, d, a, 1); }
inline void initMc4() {
    for (auto p : {"java/util/stream/Stream:java/lang/Object", "java/util/stream/Collectors:java/lang/Object", "java/util/stream/Collector:java/lang/Object", "java/util/LinkedHashMap:java/util/HashMap", "java/util/regex/Pattern:java/lang/Object", "java/util/regex/Matcher:java/lang/Object",
                   "net/minecraft/block/properties/PropertyDirection:java/lang/Object", "net/minecraft/inventory/SlotFurnaceFuel:net/minecraft/inventory/Slot", "net/minecraft/inventory/SlotFurnaceOutput:net/minecraft/inventory/Slot", "net/minecraft/tileentity/TileEntityLockableLoot:net/minecraft/tileentity/TileEntityLockable",
                   "net/minecraft/creativetab/CreativeTabs:java/lang/Object", "net/minecraft/client/renderer/block/model/ModelResourceLocation:net/minecraft/util/ResourceLocation"}) { std::string s = p; size_t c = s.find(':'); declare(s.substr(0, c).c_str(), s.substr(c + 1).c_str()); }
    ci("java/util/stream/Stream")->ifs = {"java/util/stream/BaseStream"};
    // ---- streams ----
    for (auto o : {"java/util/List", "java/util/Set", "java/util/Collection"}) N_((std::string(o) + ".stream()Ljava/util/stream/Stream;").c_str(), [](V* a) { return va(mkStream(elems(a[0].a))); });
    N_("java/util/Arrays.stream([Ljava/lang/Object;)Ljava/util/stream/Stream;", [](V* a) { return va(mkStream(nn(a[0].a)->arr)); });
    const std::string S = "java/util/stream/Stream.";
    N_((S + "map(Ljava/util/function/Function;)Ljava/util/stream/Stream;").c_str(), [](V* a) { std::vector<V> r; for (auto& e : a[0].a->arr) r.push_back(call1(a[1].a, "apply", "(Ljava/lang/Object;)Ljava/lang/Object;", e)); return va(mkStream(r)); });
    N_((S + "filter(Ljava/util/function/Predicate;)Ljava/util/stream/Stream;").c_str(), [](V* a) { std::vector<V> r; for (auto& e : a[0].a->arr) if (call1(a[1].a, "test", "(Ljava/lang/Object;)Z", e).i) r.push_back(e); return va(mkStream(r)); });
    N_((S + "forEach(Ljava/util/function/Consumer;)V").c_str(), [](V* a) { for (auto e : std::vector<V>(a[0].a->arr)) call1(a[1].a, "accept", "(Ljava/lang/Object;)V", e); return V(); });
    N_((S + "count()J").c_str(), [](V* a) { return vj((int64_t)a[0].a->arr.size()); });
    N_((S + "close()V").c_str(), [](V*) { return V(); });
    N_((S + "collect(Ljava/util/stream/Collector;)Ljava/lang/Object;").c_str(), [](V* a) { Obj* c = nn(a[1].a); if (c->f["kind"].i == 1) { Obj* s = mk("java/util/HashSet"); mapNew(s); for (auto& e : a[0].a->arr) mapPut(s, e, e); return va(s); } Obj* l = newList(); l->arr = a[0].a->arr; return va(l); });
    N_("java/util/stream/Collectors.toList()Ljava/util/stream/Collector;", [](V*) { Obj* c = mk("java/util/stream/Collector"); c->f["kind"].i = 0; return va(c); });
    N_("java/util/stream/Collectors.toSet()Ljava/util/stream/Collector;", [](V*) { Obj* c = mk("java/util/stream/Collector"); c->f["kind"].i = 1; return va(c); });
    // ---- colecoes extras ----
    N_("java/util/LinkedHashMap.<init>()V", [](V* a) { mapNew(a[0].a); a[0].a->f["linked"].i = 1; return V(); });
    for (auto o : {"java/util/List", "java/util/ArrayList"}) N_((std::string(o) + ".addAll(Ljava/util/Collection;)Z").c_str(), [](V* a) { auto v = elems(a[1].a); a[0].a->arr.insert(a[0].a->arr.end(), v.begin(), v.end()); return vi(!v.empty()); });
    for (auto o : {"java/util/Set", "java/util/HashSet"}) N_((std::string(o) + ".addAll(Ljava/util/Collection;)Z").c_str(), [](V* a) { bool ch = false; for (auto& e : elems(a[1].a)) ch |= mapPut(a[0].a, e, e).a == nullptr; return vi(ch); });
    for (auto o : {"java/util/Map", "java/util/HashMap"}) N_((std::string(o) + ".computeIfAbsent(Ljava/lang/Object;Ljava/util/function/Function;)Ljava/lang/Object;").c_str(), [](V* a) { int i = findKey(a[0].a, a[1].a); if (i >= 0 && a[0].a->arr[i].a->f["value"].a) return a[0].a->arr[i].a->f["value"];
        V v = call1(a[2].a, "apply", "(Ljava/lang/Object;)Ljava/lang/Object;", a[1]); if (v.a) mapPut(a[0].a, a[1], v); return v; });
    // ---- regex ----
    auto rx = [](Obj* p) { return std::regex(p->s); };
    N_("java/util/regex/Pattern.compile(Ljava/lang/String;)Ljava/util/regex/Pattern;", [](V* a) { Obj* p = mk("java/util/regex/Pattern"); p->s = nn(a[0].a)->s; return va(p); });
    N_("java/util/regex/Pattern.matcher(Ljava/lang/CharSequence;)Ljava/util/regex/Matcher;", [](V* a) { Obj* m = mk("java/util/regex/Matcher"); m->s = toStr(nn(a[1].a)); m->f["rx"].a = a[0].a; m->f["pos"].i = 0; return va(m); });
    auto fill = [](Obj* m, const std::smatch& r, size_t base) { m->arr.clear(); for (size_t i = 0; i < r.size(); i++) { V v; v.a = r[i].matched ? str(r[i].str()) : nullptr; m->arr.push_back(v); } m->f["pos"].i = (int)(base + r.position(0) + r.length(0)); if (r.length(0) == 0) m->f["pos"].i++; };
    N_("java/util/regex/Matcher.matches()Z", [rx, fill](V* a) { Obj* m = a[0].a; std::smatch r; bool ok = std::regex_match(m->s, r, rx(m->f["rx"].a)); if (ok) fill(m, r, 0); return vi(ok); });
    N_("java/util/regex/Matcher.find()Z", [rx, fill](V* a) { Obj* m = a[0].a; size_t p = (size_t)m->f["pos"].i; if (p > m->s.size()) return vi(0); std::string t = m->s.substr(p); std::smatch r; bool ok = std::regex_search(t, r, rx(m->f["rx"].a)); if (ok) fill(m, r, p); return vi(ok); });
    N_("java/util/regex/Matcher.group(I)Ljava/lang/String;", [](V* a) { Obj* m = a[0].a; if (a[1].i < 0 || a[1].i >= (int)m->arr.size()) throwNew("java/lang/IndexOutOfBoundsException", "No group " + std::to_string(a[1].i)); return m->arr[a[1].i]; });
    N_("java/lang/String.matches(Ljava/lang/String;)Z", [](V* a) { return vi(std::regex_match(a[0].a->s, std::regex(nn(a[1].a)->s))); });
    // ---- Minecraft: sobras do ironfurnaces ----
    for (auto k : {"net/minecraft/world/World.func_184133_a(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/util/SoundEvent;Lnet/minecraft/util/SoundCategory;FF)V", "net/minecraft/inventory/Container.<init>()V", "net/minecraft/inventory/Container.func_75132_a(Lnet/minecraft/inventory/IContainerListener;)V",
        "net/minecraft/inventory/Container.func_75142_b()V", "net/minecraft/inventory/IContainerListener.func_175173_a(Lnet/minecraft/inventory/Container;Lnet/minecraft/inventory/IInventory;)V", "net/minecraft/inventory/IInventory.func_174885_b(II)V", "net/minecraft/inventory/Slot.func_75220_a(Lnet/minecraft/item/ItemStack;Lnet/minecraft/item/ItemStack;)V",
        "net/minecraft/inventory/Slot.func_75218_e()V", "net/minecraft/creativetab/CreativeTabs.<init>(Ljava/lang/String;)V", "net/minecraft/client/renderer/block/model/ModelResourceLocation.<init>(Ljava/lang/String;Ljava/lang/String;)V", "net/minecraft/tileentity/TileEntityLockableLoot.<init>()V",
        "net/minecraft/entity/Entity.func_70091_d(Lnet/minecraft/entity/MoverType;DDD)V"}) nop(k);
    N_("net/minecraft/block/BlockContainer.func_185473_a(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/state/IBlockState;)Lnet/minecraft/item/ItemStack;", [](V*) { return va(emptyStack()); });
    N_("net/minecraft/item/ItemStack.func_77983_a(Ljava/lang/String;Lnet/minecraft/nbt/NBTBase;)V", [](V* a) { Obj*& t = a[0].a->f["tag"].a; if (!t) t = mk("net/minecraft/nbt/NBTTagCompound"); t->f[nn(a[1].a)->s] = a[2]; return V(); });
    N_("net/minecraft/block/properties/PropertyDirection.func_177714_a(Ljava/lang/String;)Lnet/minecraft/block/properties/PropertyDirection;", [](V* a) { Obj* p = mk("net/minecraft/block/properties/PropertyDirection"); p->f["name"] = a[0]; return va(p); });
    N_("net/minecraft/inventory/SlotFurnaceFuel.<init>(Lnet/minecraft/inventory/IInventory;III)V", [](V* a) { a[0].a->f["inv"] = a[1]; a[0].a->f["idx"] = a[2]; return V(); });
    N_("net/minecraft/inventory/SlotFurnaceOutput.<init>(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/inventory/IInventory;III)V", [](V* a) { a[0].a->f["inv"] = a[2]; a[0].a->f["idx"] = a[3]; return V(); });
    N_("net/minecraft/inventory/Slot.func_75211_c()Lnet/minecraft/item/ItemStack;", [](V* a) { Obj* inv = a[0].a->f["inv"].a; if (!inv) return va(emptyStack()); V r[1]; r[0].i = a[0].a->f["idx"].i; V b[2] = {va(inv), r[0]}; return invokevirtual({"net/minecraft/inventory/IInventory", "func_70301_a", "(I)Lnet/minecraft/item/ItemStack;"}, b, 2); });
    N_("net/minecraft/inventory/Slot.func_75216_d()Z", [](V* a) { Obj* inv = a[0].a->f["inv"].a; if (!inv) return vi(0); V b[2] = {va(inv), vi(a[0].a->f["idx"].i)}; V r = invokevirtual({"net/minecraft/inventory/IInventory", "func_70301_a", "(I)Lnet/minecraft/item/ItemStack;"}, b, 2); return vi(!stackEmpty(r.a)); });
    N_("net/minecraft/inventory/Slot.func_75215_d(Lnet/minecraft/item/ItemStack;)V", [](V* a) { Obj* inv = a[0].a->f["inv"].a; if (!inv) return V(); V b[3] = {va(inv), vi(a[0].a->f["idx"].i), a[1]}; return invokevirtual({"net/minecraft/inventory/IInventory", "func_70299_a", "(ILnet/minecraft/item/ItemStack;)V"}, b, 3); });
    N_("net/minecraft/inventory/Slot.func_190901_a(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/item/ItemStack;)Lnet/minecraft/item/ItemStack;", [](V* a) { return a[2]; });
    N_("net/minecraft/util/EnumFacing$Axis.values()[Lnet/minecraft/util/EnumFacing$Axis;", [](V*) { Obj* r = newarray(3, 0); const char* n[] = {"X", "Y", "Z"}; for (int i = 0; i < 3; i++) r->arr[i] = ci("net/minecraft/util/EnumFacing$Axis")->statics[n[i]]; return va(r); });
    N_("net/minecraft/entity/Entity.func_174813_aQ()Lnet/minecraft/util/math/AxisAlignedBB;", [](V*) { return va(mk("net/minecraft/util/math/AxisAlignedBB")); });
    N_("net/minecraft/world/World.func_72839_b(Lnet/minecraft/entity/Entity;Lnet/minecraft/util/math/AxisAlignedBB;)Ljava/util/List;", [](V*) { return va(newList()); });
    N_("net/minecraft/inventory/IInventory.func_70300_a(Lnet/minecraft/entity/player/EntityPlayer;)Z", [](V*) { return vi(1); });
    N_("net/minecraft/tileentity/TileEntityLockableLoot.func_145842_c(II)Z", [](V*) { return vi(0); });
    // ironfurnaces: ultimas 7
    declare("net/minecraft/world/LockCode", "java/lang/Object"); declare("net/minecraft/network/play/server/SPacketUpdateTileEntity", "java/lang/Object");
    N_("net/minecraft/entity/Entity.func_184192_z()Lnet/minecraft/block/material/EnumPushReaction;", [](V*) { return V(); });
    N_("net/minecraft/tileentity/TileEntityLockableLoot.getCapability(Lnet/minecraftforge/common/capabilities/Capability;Lnet/minecraft/util/EnumFacing;)Ljava/lang/Object;", [](V*) { return V(); });
    nop("net/minecraft/tileentity/TileEntityLockableLoot.func_145839_a(Lnet/minecraft/nbt/NBTTagCompound;)V");
    N_("net/minecraft/tileentity/TileEntityLockableLoot.func_189515_b(Lnet/minecraft/nbt/NBTTagCompound;)Lnet/minecraft/nbt/NBTTagCompound;", [](V* a) { return a[1]; });
    N_("net/minecraft/inventory/ItemStackHelper.func_191281_a(Lnet/minecraft/nbt/NBTTagCompound;Lnet/minecraft/util/NonNullList;Z)Lnet/minecraft/nbt/NBTTagCompound;", [](V* a) { Obj* l = mk("net/minecraft/util/NonNullList"); for (auto& e : nn(a[1].a)->arr) l->arr.push_back(va(new Obj(*nn(e.a)))); a[0].a->f["Items"].a = l; return a[0]; });
    nop("net/minecraft/world/LockCode.func_180157_a(Lnet/minecraft/nbt/NBTTagCompound;)V");
    N_("net/minecraft/network/play/server/SPacketUpdateTileEntity.<init>(Lnet/minecraft/util/math/BlockPos;ILnet/minecraft/nbt/NBTTagCompound;)V", [](V* a) { a[0].a->f["pos"] = a[1]; a[0].a->f["tag"] = a[3]; return V(); });
}
