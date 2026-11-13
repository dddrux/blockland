// patchJVM.8: subconjunto da corelib do supermartijn642 (JDK + Minecraft + Forge) que o AOT precisa; render do cliente vira no-op. Streams/regex/reflection/javax.lang.model: ainda nao.
inline Obj* nstr3(const std::string& s) { return str(s); }
inline std::string dots(std::string n) { for (auto& c : n) if (c == '/') c = '.'; return n; }
inline void initMc3() {
    for (auto p : {"java/lang/Class:java/lang/Object", "net/minecraft/util/ResourceLocation:java/lang/Object", "net/minecraft/util/text/TextComponentString:java/lang/Object", "net/minecraft/util/text/TextComponentTranslation:java/lang/Object",
                   "net/minecraft/util/math/Vec3d:java/lang/Object", "net/minecraftforge/fml/common/Loader:java/lang/Object", "net/minecraftforge/fml/common/ModContainer:java/lang/Object", "net/minecraft/client/renderer/BufferBuilder:java/lang/Object",
                   "net/minecraft/client/renderer/Tessellator:java/lang/Object", "net/minecraftforge/fml/common/eventhandler/EventBus:java/lang/Object"}) {
        std::string s = p; size_t c = s.find(':'); declare(s.substr(0, c).c_str(), s.substr(c + 1).c_str()); }
    // ---- java.lang ----
    N_("java/lang/Object.getClass()Ljava/lang/Class;", [](V* a) { static std::unordered_map<std::string, Obj*> m; const std::string& n = nn(a[0].a)->ci->name; Obj*& c = m[n]; if (!c) { c = mk("java/lang/Class"); c->f["name"].a = nstr3(dots(n)); } return va(c); });
    N_("java/lang/Class.getName()Ljava/lang/String;", [](V* a) { return a[0].a->f["name"]; });
    N_("java/lang/Class.getCanonicalName()Ljava/lang/String;", [](V* a) { std::string n = a[0].a->f["name"].a->s; for (auto& c : n) if (c == '$') c = '.'; return va(nstr3(n)); });
    N_("java/lang/Class.getSimpleName()Ljava/lang/String;", [](V* a) { const std::string& n = a[0].a->f["name"].a->s; size_t p = n.find_last_of(".$"); return va(nstr3(p == std::string::npos ? n : n.substr(p + 1))); });
    N_("java/lang/Throwable.addSuppressed(Ljava/lang/Throwable;)V", [](V*) { return V(); });
    N_("java/lang/Throwable.<init>(Ljava/lang/Throwable;)V", [](V* a) { a[0].a->f["cause"] = a[1]; if (a[1].a) a[0].a->f["message"].a = nstr3(toStr(a[1].a)); return V(); });
    N_("java/lang/Throwable.<init>(Ljava/lang/String;Ljava/lang/Throwable;)V", [](V* a) { a[0].a->f["message"] = a[1]; a[0].a->f["cause"] = a[2]; return V(); });
    N_("java/lang/Throwable.getCause()Ljava/lang/Throwable;", [](V* a) { return a[0].a->f["cause"]; });
    N_("java/lang/Throwable.printStackTrace()V", [](V* a) { fprintf(stderr, "%s\n", toStr(a[0].a).c_str()); return V(); });
    N_("java/lang/String.replace(CC)Ljava/lang/String;", [](V* a) { std::string s = a[0].a->s; for (auto& c : s) if ((unsigned char)c == (unsigned)a[1].i) c = (char)a[2].i; return va(nstr3(s)); });
    N_("java/lang/String.replace(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Ljava/lang/String;", [](V* a) { std::string s = a[0].a->s, f = toStr(nn(a[1].a)), t = toStr(nn(a[2].a)); if (f.empty()) return va(nstr3(s)); std::string r; size_t i = 0, p; while ((p = s.find(f, i)) != std::string::npos) { r += s.substr(i, p - i) + t; i = p + f.size(); } return va(nstr3(r + s.substr(i))); });
    N_("java/lang/String.lastIndexOf(I)I", [](V* a) { size_t p = a[0].a->s.rfind((char)a[1].i); return vi(p == std::string::npos ? -1 : (int32_t)p); });
    N_("java/lang/String.indexOf(I)I", [](V* a) { size_t p = a[0].a->s.find((char)a[1].i); return vi(p == std::string::npos ? -1 : (int32_t)p); });
    N_("java/lang/String.endsWith(Ljava/lang/String;)Z", [](V* a) { const std::string &s = a[0].a->s, &t = nn(a[1].a)->s; return vi(s.size() >= t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0); });
    N_("java/lang/String.trim()Ljava/lang/String;", [](V* a) { const std::string& s = a[0].a->s; size_t b = 0, e = s.size(); while (b < e && (unsigned char)s[b] <= ' ') b++; while (e > b && (unsigned char)s[e - 1] <= ' ') e--; return va(nstr3(s.substr(b, e - b))); });
    N_("java/lang/String.toLowerCase(Ljava/util/Locale;)Ljava/lang/String;", [](V* a) { std::string s = a[0].a->s; for (auto& c : s) c = (char)tolower((unsigned char)c); return va(nstr3(s)); });
    N_("java/lang/String.equalsIgnoreCase(Ljava/lang/String;)Z", [](V* a) { if (!a[1].a) return vi(0); std::string x = a[0].a->s, y = a[1].a->s; for (auto& c : x) c = (char)tolower((unsigned char)c); for (auto& c : y) c = (char)tolower((unsigned char)c); return vi(x == y); });
    N_("java/lang/String.split(Ljava/lang/String;)[Ljava/lang/String;", [](V* a) { const std::string &s = a[0].a->s, &d = nn(a[1].a)->s; std::vector<std::string> p; size_t i = 0, q; while (!d.empty() && (q = s.find(d, i)) != std::string::npos) { p.push_back(s.substr(i, q - i)); i = q + d.size(); } p.push_back(s.substr(i)); while (p.size() > 1 && p.back().empty()) p.pop_back();
        Obj* r = newarray((int)p.size(), 0); for (size_t k = 0; k < p.size(); k++) r->arr[k].a = nstr3(p[k]); return va(r); });   // so delimitador literal (regex: ainda nao)
    N_("java/lang/Float.valueOf(F)Ljava/lang/Float;", [](V* a) { Obj* o = mk("java/lang/Float"); o->f["value"] = a[0]; return va(o); });
    // ---- java.util ----
    N_("java/util/Collections.emptyList()Ljava/util/List;", [](V*) { return va(mk("java/util/ArrayList")); });
    N_("java/util/Collections.unmodifiableList(Ljava/util/List;)Ljava/util/List;", [](V* a) { return a[0]; });
    N_("java/util/Collections.unmodifiableSet(Ljava/util/Set;)Ljava/util/Set;", [](V* a) { return a[0]; });
    // ---- Minecraft ----
    const std::string RL = "net/minecraft/util/ResourceLocation.";
    N_((RL + "<init>(Ljava/lang/String;)V").c_str(), [](V* a) { std::string s = nn(a[1].a)->s; size_t c = s.find(':'); a[0].a->f["ns"].a = nstr3(c == std::string::npos ? "minecraft" : s.substr(0, c)); a[0].a->f["path"].a = nstr3(c == std::string::npos ? s : s.substr(c + 1)); return V(); });
    N_((RL + "<init>(Ljava/lang/String;Ljava/lang/String;)V").c_str(), [](V* a) { a[0].a->f["ns"] = a[1]; a[0].a->f["path"] = a[2]; return V(); });
    N_((RL + "func_110624_b()Ljava/lang/String;").c_str(), [](V* a) { return a[0].a->f["ns"]; }); N_((RL + "func_110623_a()Ljava/lang/String;").c_str(), [](V* a) { return a[0].a->f["path"]; });
    N_((RL + "toString()Ljava/lang/String;").c_str(), [](V* a) { return va(nstr3(a[0].a->f["ns"].a->s + ":" + a[0].a->f["path"].a->s)); });
    N_((RL + "equals(Ljava/lang/Object;)Z").c_str(), [](V* a) { Obj* q = a[1].a; return vi(q && q->ci == a[0].a->ci && q->f["ns"].a->s == a[0].a->f["ns"].a->s && q->f["path"].a->s == a[0].a->f["path"].a->s); });
    N_((RL + "hashCode()I").c_str(), [](V* a) { return vi((int32_t)std::hash<std::string>()(a[0].a->f["ns"].a->s + ":" + a[0].a->f["path"].a->s)); });
    N_("net/minecraft/util/text/TextComponentString.<init>(Ljava/lang/String;)V", [](V* a) { a[0].a->f["text"] = a[1]; return V(); });
    N_("net/minecraft/util/text/TextComponentTranslation.<init>(Ljava/lang/String;[Ljava/lang/Object;)V", [](V* a) { a[0].a->f["text"] = a[1]; return V(); });
    N_("net/minecraft/util/text/ITextComponent.func_150254_d()Ljava/lang/String;", [](V* a) { return a[0].a->f["text"].a ? a[0].a->f["text"] : va(nstr3("")); });
    N_("net/minecraft/nbt/NBTTagCompound.func_82582_d()Z", [](V* a) { return vi(a[0].a->f.empty()); });
    N_("net/minecraft/item/ItemStack.func_77981_g()Z", [](V*) { return vi(0); });
    N_("net/minecraft/item/Item.func_77614_k()Z", [](V*) { return vi(0); }); N_("net/minecraft/item/Item.func_77645_m()Z", [](V*) { return vi(0); });
    N_("net/minecraft/item/Item.getHarvestLevel(Lnet/minecraft/item/ItemStack;Ljava/lang/String;Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/block/state/IBlockState;)I", [](V*) { return vi(-1); });
    N_("net/minecraft/util/math/Vec3d.<init>(DDD)V", [](V* a) { a[0].a->f["x"] = a[1]; a[0].a->f["y"] = a[2]; a[0].a->f["z"] = a[3]; return V(); });
    N_("net/minecraft/util/math/AxisAlignedBB.<init>(DDDDDD)V", [](V* a) { const char* k[] = {"x0", "y0", "z0", "x1", "y1", "z1"}; for (int i = 0; i < 6; i++) a[0].a->f[k[i]] = a[i + 1]; return V(); });
    N_("net/minecraft/util/math/MathHelper.func_151237_a(DDD)D", [](V* a) { return vd(a[0].d < a[1].d ? a[1].d : a[0].d > a[2].d ? a[2].d : a[0].d); });
    N_("net/minecraft/entity/player/EntityPlayer.func_184586_b(Lnet/minecraft/util/EnumHand;)Lnet/minecraft/item/ItemStack;", [](V*) { return va(emptyStack()); });
    N_("net/minecraft/entity/player/InventoryPlayer.func_70301_a(I)Lnet/minecraft/item/ItemStack;", [](V*) { return va(emptyStack()); });
    N_("net/minecraft/world/IBlockAccess.func_180495_p(Lnet/minecraft/util/math/BlockPos;)Lnet/minecraft/block/state/IBlockState;", [](V* a) { Obj* p = nn(a[1].a); return va(host().getState ? host().getState(p->f["x"].i, p->f["y"].i, p->f["z"].i) : nullptr); });
    N_("net/minecraft/tileentity/TileEntity.func_145837_r()Z", [](V* a) { return vi(a[0].a->f["invalid"].i); });
    N_("net/minecraft/world/DimensionType.func_186068_a()I", [](V*) { return vi(0); });
    // render do cliente (BufferBuilder encadeia): no-op
    const std::string BB = "net/minecraft/client/renderer/BufferBuilder.", BBD = "Lnet/minecraft/client/renderer/BufferBuilder;";
    N_((BB + "func_181662_b(DDD)" + BBD).c_str(), [](V* a) { return a[0]; }); N_((BB + "func_181666_a(FFFF)" + BBD).c_str(), [](V* a) { return a[0]; }); N_((BB + "func_187315_a(DD)" + BBD).c_str(), [](V* a) { return a[0]; });
    N_("net/minecraft/client/renderer/Tessellator.func_178181_a()Lnet/minecraft/client/renderer/Tessellator;", [](V*) { static Obj* t = mk("net/minecraft/client/renderer/Tessellator"); return va(t); });
    N_("net/minecraft/client/renderer/Tessellator.func_178180_c()Lnet/minecraft/client/renderer/BufferBuilder;", [](V*) { static Obj* b = mk("net/minecraft/client/renderer/BufferBuilder"); return va(b); });
    for (auto k : {"func_181675_d()V", "func_181668_a(ILnet/minecraft/client/renderer/vertex/VertexFormat;)V"}) nop((BB + k).c_str());
    nop("net/minecraft/client/renderer/Tessellator.func_78381_a()V");
    for (auto k : {"func_179135_a(ZZZZ)V", "func_179094_E()V", "func_179109_b(FFF)V", "func_179121_F()V", "func_179097_i()V", "func_179126_j()V", "func_187441_d(F)V", "func_179147_l()V", "func_179090_x()V", "func_179098_w()V"}) nop(("net/minecraft/client/renderer/GlStateManager." + std::string(k)).c_str());
    N_("net/minecraft/client/gui/FontRenderer.func_78256_a(Ljava/lang/String;)I", [](V* a) { return vi(6 * (int)nn(a[1].a)->s.size()); });
    N_("net/minecraft/client/gui/FontRenderer.func_175063_a(Ljava/lang/String;FFI)I", [](V* a) { return vi(0); });
    // ---- Forge: Loader/ModContainer (modid fixo do mod em execucao: rt::host().modId) ----
    N_("net/minecraftforge/fml/common/Loader.instance()Lnet/minecraftforge/fml/common/Loader;", [](V*) { static Obj* l = mk("net/minecraftforge/fml/common/Loader"); return va(l); });
    N_("net/minecraftforge/fml/common/Loader.activeModContainer()Lnet/minecraftforge/fml/common/ModContainer;", [](V*) { static Obj* c = mk("net/minecraftforge/fml/common/ModContainer"); return va(c); });
    N_("net/minecraftforge/fml/common/ModContainer.getModId()Ljava/lang/String;", [](V*) { return va(nstr3(host().modId)); }); N_("net/minecraftforge/fml/common/ModContainer.getName()Ljava/lang/String;", [](V*) { return va(nstr3(host().modId)); });
    N_("net/minecraftforge/fml/common/eventhandler/EventBus.post(Lnet/minecraftforge/fml/common/eventhandler/Event;)Z", [](V*) { return vi(0); });
}
