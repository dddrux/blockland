// patchJVM.11: nome de registro (Block/Item) e clique direito do jogo -> Block.onBlockActivated do mod.
inline std::string regNameOf(Obj* o) { return o && o->f.count("rn") && o->f["rn"].a ? o->f["rn"].a->s : std::string(); }
inline void initMc5() {
    const char* own[] = {"net/minecraft/block/Block", "net/minecraft/block/BlockContainer", "net/minecraft/item/Item", "net/minecraft/item/ItemBlock"};
    for (const char* m : {"func_149663_c(Ljava/lang/String;)", "func_149711_c(F)", "func_149752_b(F)", "func_149715_a(F)"}) self((std::string("net/minecraft/block/Block.") + m + "Lnet/minecraft/block/Block;").c_str());
    N_("net/minecraft/tileentity/TileEntity.func_70296_d()V", [](V*) { return V(); });
    N_("net/minecraft/block/Block.setHarvestLevel(Ljava/lang/String;I)V", [](V*) { return V(); });
    const char* R = ")Lnet/minecraftforge/registries/IForgeRegistryEntry;";
    for (const char* o : own) {
        std::string p = o;
        N_((p + ".setRegistryName(Ljava/lang/String;" + R).c_str(), [](V* a) { if (a[1].a) { std::string s = a[1].a->s; if (s.find(':') == std::string::npos) s = host().modId + ":" + s; a[0].a->f["rn"].a = str(s); } return a[0]; });
        N_((p + ".setRegistryName(Lnet/minecraft/util/ResourceLocation;" + R).c_str(), [](V* a) { if (a[1].a && a[1].a->f["ns"].a) a[0].a->f["rn"].a = str(a[1].a->f["ns"].a->s + ":" + a[1].a->f["path"].a->s); return a[0]; });
        N_((p + ".getRegistryName()Lnet/minecraft/util/ResourceLocation;").c_str(), [](V* a) { std::string s = regNameOf(a[0].a); if (s.empty()) return va(nullptr); size_t c = s.find(':'); Obj* r = mk("net/minecraft/util/ResourceLocation"); r->f["ns"].a = str(s.substr(0, c)); r->f["path"].a = str(s.substr(c + 1)); return va(r); });
    }
}
// key do jogo ("ironfurnaces_diamond_furnace_idle") -> Block registrado ("ironfurnaces:diamond_furnace"); nullptr se nao houver
inline Obj* findBlockByKey(std::string key) {
    { const std::string a = "_active"; if (key.size() > a.size() && key.compare(key.size() - a.size(), a.size(), a) == 0) key = key.substr(0, key.size() - a.size()) + "_idle"; }
    for (Obj* o : host().registered) { if (!o || !o->ci || !findM(o->ci, "func_180639_a", "(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/state/IBlockState;Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/util/EnumHand;Lnet/minecraft/util/EnumFacing;FFF)Z")) continue;
        std::string r = regNameOf(o); size_t c = r.find(':'); std::string ns = r.substr(0, c), path = r.substr(c + 1); if (ns + "_" + path == key || (ns == "mod" && key.size() > path.size() && key.compare(key.size() - path.size() - 1, path.size() + 1, "_" + path) == 0)) return o; }
    return nullptr;
}
// clique direito em (x,y,z): cria o tile entity se faltar (func_149915_a) e chama onBlockActivated. -1 = sem Block; 0/1 = retorno do mod; -2 = o mod lancou excecao
inline int useBlock(const std::string& key, int x, int y, int z, std::string* err = nullptr) {
    Obj* b = findBlockByKey(key); if (!b) return -1;
    try {
        Obj* w = mk("net/minecraft/world/World"); Obj* pl = mk("net/minecraft/entity/player/EntityPlayer"); Obj* pos = mkPos(x, y, z);
        if (host().getTile && !host().getTile(x, y, z) && host().setTile) { V c[3]; c[0].a = b; c[1].a = w; c[2].i = 0; V t = dispatch(b->ci, MRef{b->ci->name.c_str(), "func_149915_a", "(Lnet/minecraft/world/World;I)Lnet/minecraft/tileentity/TileEntity;"}, c, 3); if (t.a) host().setTile(x, y, z, t.a); }
        V s0[1]; s0[0].a = b; Obj* st = dispatch(b->ci, MRef{b->ci->name.c_str(), "func_176223_P", "()Lnet/minecraft/block/state/IBlockState;"}, s0, 1).a;
        V a[10]; a[0].a = b; a[1].a = w; a[2].a = pos; a[3].a = st; a[4].a = pl; a[5].a = mk("net/minecraft/util/EnumHand"); a[6].a = facing(1); a[7].f = .5f; a[8].f = .5f; a[9].f = .5f;
        return dispatch(b->ci, MRef{b->ci->name.c_str(), "func_180639_a", "(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/state/IBlockState;Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/util/EnumHand;Lnet/minecraft/util/EnumFacing;FFF)Z"}, a, 10).i ? 1 : 0;
    } catch (Thrown& t) { if (err) *err = t.ex ? toStr(t.ex) : "?"; return -2; } catch (...) { if (err) *err = "?"; return -2; }
}
