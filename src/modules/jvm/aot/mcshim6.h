// patchJVM.12: BlockStateContainer.Builder, PropertyBool/Enum, estado padrao, BlockHorizontal/Directional, AABB.union, EnumFacing.rotateY, EnumHelper.addArt, Model* (cliente: no-op). Para mcw-paintings e mcw-lights.
inline void initMc6() {
    for (auto p : {"net/minecraft/block/properties/PropertyBool:java/lang/Object", "net/minecraft/block/properties/PropertyEnum:java/lang/Object", "net/minecraft/block/state/BlockStateContainer$Builder:java/lang/Object", "net/minecraft/block/state/BlockStateContainer:java/lang/Object",
                   "net/minecraft/block/BlockHorizontal:net/minecraft/block/Block", "net/minecraft/block/BlockDirectional:net/minecraft/block/Block", "net/minecraft/block/material/MapColor:java/lang/Object", "net/minecraft/block/state/BlockFaceShape:java/lang/Object",
                   "net/minecraft/entity/item/EntityPainting$EnumArt:java/lang/Object", "net/minecraftforge/common/util/EnumHelper:java/lang/Object", "net/minecraftforge/client/model/ModelLoader:java/lang/Object"}) { std::string s = p; size_t c = s.find(':'); declare(s.substr(0, c).c_str(), s.substr(c + 1).c_str()); }
    auto prop = [](const char* cls, Obj* name) { Obj* p = mk(cls); p->f["name"].a = name; return va(p); };
    N_("net/minecraft/block/properties/PropertyBool.func_177716_a(Ljava/lang/String;)Lnet/minecraft/block/properties/PropertyBool;", [prop](V* a) { return prop("net/minecraft/block/properties/PropertyBool", a[0].a); });
    N_("net/minecraft/block/properties/PropertyEnum.func_177709_a(Ljava/lang/String;Ljava/lang/Class;)Lnet/minecraft/block/properties/PropertyEnum;", [prop](V* a) { return prop("net/minecraft/block/properties/PropertyEnum", a[0].a); });
    N_("net/minecraft/block/state/BlockStateContainer$Builder.<init>(Lnet/minecraft/block/Block;)V", [](V* a) { a[0].a->f["block"] = a[1]; return V(); });
    N_("net/minecraft/block/state/BlockStateContainer$Builder.add([Lnet/minecraft/block/properties/IProperty;)Lnet/minecraft/block/state/BlockStateContainer$Builder;", [](V* a) { return a[0]; });
    N_("net/minecraft/block/state/BlockStateContainer$Builder.build()Lnet/minecraft/block/state/BlockStateContainer;", [](V* a) { Obj* c = mk("net/minecraft/block/state/BlockStateContainer"); c->f["block"] = a[0].a->f["block"]; return va(c); });
    N_("net/minecraft/block/state/BlockStateContainer.func_177621_b()Lnet/minecraft/block/state/IBlockState;", [](V* a) { Obj* s = mk("net/minecraft/block/state/BlockStateImpl"); s->f["block"] = a[0].a->f["block"]; return va(s); });
    // estado padrao: setDefaultState guarda; getDefaultState devolve copia (antes: estado vazio)
    N_("net/minecraft/block/Block.func_180632_j(Lnet/minecraft/block/state/IBlockState;)V", [](V* a) { a[0].a->f["defstate"] = a[1]; return V(); });
    N_("net/minecraft/block/Block.func_176223_P()Lnet/minecraft/block/state/IBlockState;", [](V* a) { auto it = a[0].a->f.find("defstate"); Obj* s = (it != a[0].a->f.end() && it->second.a) ? new Obj(*it->second.a) : mk("net/minecraft/block/state/BlockStateImpl"); s->f["block"].a = a[0].a; return va(s); });
    for (const char* o : {"net/minecraft/block/Block", "net/minecraft/block/BlockHorizontal"}) {
        N_((std::string(o) + ".getStateForPlacement(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/util/EnumFacing;FFFILnet/minecraft/entity/EntityLivingBase;Lnet/minecraft/util/EnumHand;)Lnet/minecraft/block/state/IBlockState;").c_str(), [](V* a) { V b[1]; b[0] = a[0]; return dispatch(a[0].a->ci, MRef{"net/minecraft/block/Block", "func_176223_P", "()Lnet/minecraft/block/state/IBlockState;"}, b, 1); });
        N_((std::string(o) + ".func_189540_a(Lnet/minecraft/block/state/IBlockState;Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/Block;Lnet/minecraft/util/math/BlockPos;)V").c_str(), [](V*) { return V(); });
    }
    N_("net/minecraft/block/state/IBlockState.func_177231_a(Lnet/minecraft/block/properties/IProperty;)Lnet/minecraft/block/state/IBlockState;", [](V* a) { Obj* o = new Obj(*a[0].a); V& v = o->f[propKey(a[1].a)]; if (a[1].a->ci->name == "net/minecraft/block/properties/PropertyBool") v.i = !v.i; return va(o); });   // enum: mantem (nao verificado)
    N_("net/minecraft/block/state/IBlockState.func_193401_d(Lnet/minecraft/world/IBlockAccess;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/util/EnumFacing;)Lnet/minecraft/block/state/BlockFaceShape;", [](V*) { return va(mk("net/minecraft/block/state/BlockFaceShape")); });
    N_("net/minecraft/block/state/IBlockState.func_185896_q()Z", [](V*) { return vi(1); });   // valor fixo, nao verificado
    N_("net/minecraft/block/state/IBlockState.func_185917_h()Z", [](V*) { return vi(0); });   // valor fixo, nao verificado
    N_("net/minecraft/block/material/Material.func_151565_r()Lnet/minecraft/block/material/MapColor;", [](V*) { return va(mk("net/minecraft/block/material/MapColor")); });
    N_("net/minecraft/entity/player/EntityPlayer.func_184614_ca()Lnet/minecraft/item/ItemStack;", [](V*) { return va(emptyStack()); });
    N_("net/minecraft/util/EnumFacing.func_176746_e()Lnet/minecraft/util/EnumFacing;", [](V* a) { static const int r[] = {0, 1, 5, 4, 2, 3}; return va(facing(r[a[0].a->f["ordinal"].i])); });   // rotateY: N->E->S->W
    N_("net/minecraft/util/math/AxisAlignedBB.func_111270_a(Lnet/minecraft/util/math/AxisAlignedBB;)Lnet/minecraft/util/math/AxisAlignedBB;", [](V* a) { Obj *p = a[0].a, *q = nn(a[1].a), *r = mk("net/minecraft/util/math/AxisAlignedBB"); const char* k[] = {"x0", "y0", "z0", "x1", "y1", "z1"};
        for (int i = 0; i < 6; i++) r->f[k[i]].d = i < 3 ? std::min(p->f[k[i]].d, q->f[k[i]].d) : std::max(p->f[k[i]].d, q->f[k[i]].d); return va(r); });
    N_("java/util/List.toArray([Ljava/lang/Object;)[Ljava/lang/Object;", [](V* a) { Obj* o = mk("[array"); o->arr = nn(a[0].a)->arr; o->at = 0; return va(o); });
    N_("[array.clone()Ljava/lang/Object;", [](V* a) { return va(new Obj(*a[0].a)); });
    N_("net/minecraftforge/common/util/EnumHelper.addArt(Ljava/lang/String;Ljava/lang/String;IIII)Lnet/minecraft/entity/item/EntityPainting$EnumArt;", [](V*) { return va(mk("net/minecraft/entity/item/EntityPainting$EnumArt")); });
    N_("net/minecraft/client/renderer/block/model/ModelResourceLocation.<init>(Lnet/minecraft/util/ResourceLocation;Ljava/lang/String;)V", [](V*) { return V(); });
    N_("net/minecraftforge/client/model/ModelLoader.setCustomModelResourceLocation(Lnet/minecraft/item/Item;ILnet/minecraft/client/renderer/block/model/ModelResourceLocation;)V", [](V*) { return V(); });
    self("net/minecraft/block/Block.func_149672_a(Lnet/minecraft/block/SoundType;)Lnet/minecraft/block/Block;");
    self("net/minecraft/creativetab/CreativeTabs.func_78025_a(Ljava/lang/String;)Lnet/minecraft/creativetab/CreativeTabs;");
    // Block.<init>: o jogo real chama createBlockState (func_180661_e) no construtor e guarda em blockState (field_176227_L) + estado padrao
    N_("net/minecraft/block/state/BlockStateContainer.<init>(Lnet/minecraft/block/Block;[Lnet/minecraft/block/properties/IProperty;)V", [](V* a) { a[0].a->f["block"] = a[1]; return V(); });
    auto binit = [](V* a) { Obj* b = a[0].a; const char* d = "()Lnet/minecraft/block/state/BlockStateContainer;"; if (b->f.count("field_176227_L") || !findM(b->ci, "func_180661_e", d)) return V();
        V c[1]; c[0].a = b; V r = dispatch(b->ci, MRef{b->ci->name.c_str(), "func_180661_e", d}, c, 1);
        if (r.a) { b->f["field_176227_L"] = r; Obj* s = mk("net/minecraft/block/state/BlockStateImpl"); s->f["block"].a = b; b->f["defstate"].a = s; } return V(); };
    for (const char* o : {"net/minecraft/block/Block", "net/minecraft/block/BlockContainer", "net/minecraft/block/BlockHorizontal", "net/minecraft/block/BlockDirectional"}) for (const char* d : {"(Lnet/minecraft/block/material/Material;)V", "(Lnet/minecraft/block/material/Material;Lnet/minecraft/block/material/MapColor;)V"}) N_((std::string(o) + ".<init>" + d).c_str(), binit);
}
