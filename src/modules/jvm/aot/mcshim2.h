// patchJVM.6: resto do shim Minecraft/Forge 1.12.2 (lista de mods/<mod>/aot_cobertura.txt). Semantica minima: o que o jogo nao modela vira no-op/valor neutro.
inline void nop(const char* k) { N_(k, [](V*) { return V(); }); }
inline void self(const char* k) { N_(k, [](V* a) { return a[0]; }); }
inline void initMc2() {
    for (auto p : {"net/minecraft/tileentity/TileEntityLockable:net/minecraft/tileentity/TileEntity", "net/minecraft/util/EnumFacing$Axis:java/lang/Enum", "net/minecraft/util/EnumFacing$AxisDirection:java/lang/Enum",
                   "net/minecraft/util/Rotation:java/lang/Object", "net/minecraft/util/Mirror:java/lang/Object", "net/minecraft/block/material/Material:java/lang/Object", "net/minecraft/inventory/Slot:java/lang/Object",
                   "net/minecraft/inventory/Container:java/lang/Object", "net/minecraft/util/math/AxisAlignedBB:java/lang/Object", "net/minecraftforge/registries/IForgeRegistry:java/lang/Object",
                   "net/minecraft/item/ItemTool:net/minecraft/item/Item", "net/minecraft/item/ItemSword:net/minecraft/item/Item", "net/minecraft/item/ItemHoe:net/minecraft/item/Item"}) {
        std::string s = p; size_t c = s.find(':'); declare(s.substr(0, c).c_str(), s.substr(c + 1).c_str()); }
    for (auto c : {"net/minecraft/util/EnumFacing$Axis", "net/minecraft/util/EnumFacing$AxisDirection"}) ci(c)->inited = true;
    const char* an[] = {"X", "Y", "Z"}; for (int i = 0; i < 3; i++) { Obj* o = mk("net/minecraft/util/EnumFacing$Axis"); o->f["name"].a = str(an[i]); o->f["ordinal"].i = i; ci("net/minecraft/util/EnumFacing$Axis")->statics[an[i]].a = o; }
    const char* dn[] = {"POSITIVE", "NEGATIVE"}; for (int i = 0; i < 2; i++) { Obj* o = mk("net/minecraft/util/EnumFacing$AxisDirection"); o->f["name"].a = str(dn[i]); o->f["ordinal"].i = i; ci("net/minecraft/util/EnumFacing$AxisDirection")->statics[dn[i]].a = o; }
    const std::string EF = "net/minecraft/util/EnumFacing.", EFD = "Lnet/minecraft/util/EnumFacing;";
    N_((EF + "values()[" + EFD).c_str(), [](V*) { Obj* r = newarray(6, 0); for (int i = 0; i < 6; i++) r->arr[i].a = facing(i); return va(r); });
    N_((EF + "func_82600_a(I)" + EFD).c_str(), [](V* a) { return va(facing(a[0].i)); });
    N_((EF + "func_176745_a()I").c_str(), [](V* a) { return a[0].a->f["ordinal"]; });
    N_((EF + "func_176740_k()Lnet/minecraft/util/EnumFacing$Axis;").c_str(), [](V* a) { static const char* ax[] = {"Y", "Y", "Z", "Z", "X", "X"}; return ci("net/minecraft/util/EnumFacing$Axis")->statics[ax[a[0].a->f["ordinal"].i]]; });
    N_((EF + "func_176743_c()Lnet/minecraft/util/EnumFacing$AxisDirection;").c_str(), [](V* a) { int o = a[0].a->f["ordinal"].i; return ci("net/minecraft/util/EnumFacing$AxisDirection")->statics[(o == 1 || o == 3 || o == 5) ? "POSITIVE" : "NEGATIVE"]; });
    N_("net/minecraft/util/EnumFacing$Axis.ordinal()I", [](V* a) { return a[0].a->f["ordinal"]; });
    N_("net/minecraft/util/Rotation.func_185831_a(Lnet/minecraft/util/EnumFacing;)Lnet/minecraft/util/EnumFacing;", [](V* a) { return a[1]; });
    N_("net/minecraft/util/Mirror.func_185800_a(Lnet/minecraft/util/EnumFacing;)Lnet/minecraft/util/Rotation;", [](V*) { return va(mk("net/minecraft/util/Rotation")); });
    N_("net/minecraft/block/state/IBlockState.func_185907_a(Lnet/minecraft/util/Rotation;)Lnet/minecraft/block/state/IBlockState;", [](V* a) { return a[0]; });
    N_("net/minecraft/block/state/IBlockState.func_185913_b()Z", [](V*) { return vi(1); });
    N_("net/minecraft/block/state/IBlockState.func_185904_a()Lnet/minecraft/block/material/Material;", [](V*) { return va(mk("net/minecraft/block/material/Material")); });
    N_("net/minecraft/block/Block.func_176223_P()Lnet/minecraft/block/state/IBlockState;", [](V* a) { Obj* s = mk("net/minecraft/block/state/BlockStateImpl"); s->f["block"].a = a[0].a; return va(s); });
    N_("net/minecraft/block/Block.func_149634_a(Lnet/minecraft/item/Item;)Lnet/minecraft/block/Block;", [](V* a) { return a[0].a ? a[0].a->f["block"] : V(); });
    N_("net/minecraft/item/ItemBlock.<init>(Lnet/minecraft/block/Block;)V", [](V* a) { a[0].a->f["block"] = a[1]; return V(); });
    self("net/minecraft/item/ItemBlock.setRegistryName(Lnet/minecraft/util/ResourceLocation;)Lnet/minecraftforge/registries/IForgeRegistryEntry;");
    N_("net/minecraft/item/ItemStack.func_82833_r()Ljava/lang/String;", [](V* a) { return a[0].a->f["name"].a ? a[0].a->f["name"] : va(str("")); });
    N_("net/minecraft/item/ItemStack.func_82837_s()Z", [](V* a) { return vi(a[0].a->f["name"].a != nullptr); });
    N_("net/minecraft/item/ItemStack.func_151001_c(Ljava/lang/String;)Lnet/minecraft/item/ItemStack;", [](V* a) { a[0].a->f["name"] = a[1]; return a[0]; });
    N_("net/minecraft/item/ItemStack.func_77970_a(Lnet/minecraft/item/ItemStack;Lnet/minecraft/item/ItemStack;)Z", [](V* a) { return vi(a[0].a->f["tag"].a == a[1].a->f["tag"].a); });
    N_("net/minecraft/item/Item.getContainerItem(Lnet/minecraft/item/ItemStack;)Lnet/minecraft/item/ItemStack;", [](V*) { return va(emptyStack()); });
    for (auto k : {"net/minecraft/item/ItemTool.func_77861_e()", "net/minecraft/item/ItemSword.func_150932_j()", "net/minecraft/item/ItemHoe.func_77842_f()"}) N_((std::string(k) + "Ljava/lang/String;").c_str(), [](V*) { return va(str("")); });
    // ItemStackHelper: a lista de itens fica guardada no proprio NBT (chave "Items"); sem serializar em bytes
    const std::string IH = "net/minecraft/inventory/ItemStackHelper.", NBT = "Lnet/minecraft/nbt/NBTTagCompound;", NNL = "Lnet/minecraft/util/NonNullList;";
    N_((IH + "func_191282_a(" + NBT + NNL + ")" + NBT).c_str(), [](V* a) { Obj* l = mk("net/minecraft/util/NonNullList"); for (auto& e : nn(a[1].a)->arr) { Obj* c = new Obj(*nn(e.a)); l->arr.push_back(va(c)); } a[0].a->f["Items"].a = l; return a[0]; });
    N_((IH + "func_191283_b(" + NBT + NNL + ")V").c_str(), [](V* a) { auto it = a[0].a->f.find("Items"); if (it == a[0].a->f.end() || !it->second.a) return V(); auto& src = it->second.a->arr; auto& d = nn(a[1].a)->arr; for (size_t i = 0; i < d.size() && i < src.size(); i++) d[i].a = new Obj(*src[i].a); return V(); });
    N_((IH + "func_188382_a(Ljava/util/List;II)Lnet/minecraft/item/ItemStack;").c_str(), [](V* a) { auto& L = nn(a[0].a)->arr; int i = a[1].i, n = a[2].i; if (i < 0 || i >= (int)L.size() || stackEmpty(L[i].a) || n <= 0) return va(emptyStack());
        Obj* s = L[i].a; int t = std::min(n, s->f["count"].i); Obj* r = new Obj(*s); r->f["count"].i = t; s->f["count"].i -= t; if (s->f["count"].i <= 0) L[i].a = emptyStack(); return va(r); });
    N_((IH + "func_188383_a(Ljava/util/List;I)Lnet/minecraft/item/ItemStack;").c_str(), [](V* a) { auto& L = nn(a[0].a)->arr; int i = a[1].i; if (i < 0 || i >= (int)L.size()) return va(emptyStack()); Obj* r = L[i].a; L[i].a = emptyStack(); return va(r); });
    N_("net/minecraft/util/math/MathHelper.func_76125_a(III)I", [](V* a) { return vi(a[0].i < a[1].i ? a[1].i : a[0].i > a[2].i ? a[2].i : a[0].i); });
    N_("net/minecraft/entity/EntityLivingBase.func_174811_aO()Lnet/minecraft/util/EnumFacing;", [](V*) { return va(facing(2)); });
    N_("net/minecraft/entity/player/EntityPlayer.func_70092_e(DDD)D", [](V*) { return vd(0); });
    N_("net/minecraft/entity/player/EntityPlayer.func_175149_v()Z", [](V*) { return vi(0); });
    N_("net/minecraft/inventory/IInventory.func_174887_a_(I)I", [](V* a) { auto it = a[0].a->f.find("field" + std::to_string(a[1].i)); return vi(it == a[0].a->f.end() ? 0 : it->second.i); });
    N_("net/minecraft/inventory/Slot.<init>(Lnet/minecraft/inventory/IInventory;III)V", [](V* a) { Obj* o = a[0].a; o->f["inv"] = a[1]; o->f["idx"] = a[2]; o->f["x"] = a[3]; o->f["y"] = a[4]; return V(); });
    N_("net/minecraft/inventory/SlotFurnaceFuel.func_178173_c_(Lnet/minecraft/item/ItemStack;)Z", [](V*) { return vi(0); });
    N_("net/minecraft/inventory/Container.func_178144_a(Lnet/minecraft/tileentity/TileEntity;)I", [](V*) { return vi(0); });
    N_("net/minecraft/inventory/Container.func_94526_b(Lnet/minecraft/inventory/IInventory;)I", [](V*) { return vi(0); });
    N_("net/minecraft/world/IBlockAccess.func_175625_s(Lnet/minecraft/util/math/BlockPos;)Lnet/minecraft/tileentity/TileEntity;", [](V* a) { Obj* p = nn(a[1].a); return va(host().getTile ? host().getTile(p->f["x"].i, p->f["y"].i, p->f["z"].i) : nullptr); });
    N_("net/minecraft/world/World.func_175690_a(Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/tileentity/TileEntity;)V", [](V* a) { if (host().setTile) { Obj* p = nn(a[1].a); host().setTile(p->f["x"].i, p->f["y"].i, p->f["z"].i, a[2].a); } return V(); });
    N_("net/minecraft/world/World.func_184143_b(Lnet/minecraft/util/math/AxisAlignedBB;)Z", [](V*) { return vi(1); });
    N_("net/minecraft/util/math/AxisAlignedBB.func_72321_a(DDD)Lnet/minecraft/util/math/AxisAlignedBB;", [](V* a) { return a[0]; });
    N_("net/minecraft/util/math/AxisAlignedBB.func_186670_a(Lnet/minecraft/util/math/BlockPos;)Lnet/minecraft/util/math/AxisAlignedBB;", [](V* a) { return a[0]; });
    N_("net/minecraft/util/math/AxisAlignedBB.func_191195_a(DDD)Lnet/minecraft/util/math/AxisAlignedBB;", [](V* a) { return a[0]; });
    N_("net/minecraft/util/text/translation/I18n.func_74838_a(Ljava/lang/String;)Ljava/lang/String;", [](V* a) { return a[0]; });
    N_("net/minecraft/tileentity/TileEntityLockable.func_189515_b(Lnet/minecraft/nbt/NBTTagCompound;)Lnet/minecraft/nbt/NBTTagCompound;", [](V* a) { return a[1]; });
    N_("net/minecraft/tileentity/TileEntityLockable.getCapability(Lnet/minecraftforge/common/capabilities/Capability;Lnet/minecraft/util/EnumFacing;)Ljava/lang/Object;", [](V*) { return V(); });
    N_("net/minecraftforge/event/ForgeEventFactory.getItemBurnTime(Lnet/minecraft/item/ItemStack;)I", [](V*) { return vi(-1); });
    N_("net/minecraftforge/event/RegistryEvent$Register.getRegistry()Lnet/minecraftforge/registries/IForgeRegistry;", [](V*) { return va(mk("net/minecraftforge/registries/IForgeRegistry")); });
    N_("net/minecraftforge/fml/client/event/ConfigChangedEvent$OnConfigChangedEvent.getModID()Ljava/lang/String;", [](V*) { return va(str("")); });
    // no-ops (sem equivalente no jogo)
    for (auto k : {"net/minecraft/block/BlockContainer.func_180663_b(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/state/IBlockState;)V", "net/minecraft/block/state/BlockStateContainer.<init>(Lnet/minecraft/block/Block;[Lnet/minecraft/block/properties/IProperty;)V",
        "net/minecraft/block/BlockContainer.<init>(Lnet/minecraft/block/material/Material;Lnet/minecraft/block/material/MapColor;)V", "net/minecraft/block/BlockContainer.func_190948_a(Lnet/minecraft/item/ItemStack;Lnet/minecraft/world/World;Ljava/util/List;Lnet/minecraft/client/util/ITooltipFlag;)V",
        "net/minecraft/world/World.func_184134_a(DDDLnet/minecraft/util/SoundEvent;Lnet/minecraft/util/SoundCategory;FFZ)V", "net/minecraft/world/World.func_175666_e(Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/Block;)V", "net/minecraft/world/World.func_175641_c(Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/block/Block;II)V",
        "net/minecraft/tileentity/TileEntity.func_145829_t()V", "net/minecraft/tileentity/TileEntityLockable.<init>()V", "net/minecraft/tileentity/TileEntityLockable.func_145839_a(Lnet/minecraft/nbt/NBTTagCompound;)V",
        "net/minecraft/inventory/InventoryHelper.func_180175_a(Lnet/minecraft/world/World;Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/inventory/IInventory;)V", "net/minecraft/inventory/IContainerListener.func_71112_a(Lnet/minecraft/inventory/Container;II)V",
        "net/minecraft/util/datafix/walkers/ItemStackDataLists.<init>(Ljava/lang/Class;[Ljava/lang/String;)V", "net/minecraft/util/datafix/DataFixer.func_188258_a(Lnet/minecraft/util/datafix/FixTypes;Lnet/minecraft/util/datafix/IDataWalker;)V",
        "net/minecraftforge/items/wrapper/SidedInvWrapper.<init>(Lnet/minecraft/inventory/ISidedInventory;Lnet/minecraft/util/EnumFacing;)V", "net/minecraftforge/fml/common/registry/GameRegistry.registerTileEntity(Ljava/lang/Class;Ljava/lang/String;)V",
        "net/minecraftforge/common/config/ConfigManager.sync(Ljava/lang/String;Lnet/minecraftforge/common/config/Config$Type;)V"}) nop(k);
    N_("net/minecraftforge/registries/IForgeRegistry.registerAll([Lnet/minecraftforge/registries/IForgeRegistryEntry;)V", [](V* a) { if (a[1].a) for (auto& e : a[1].a->arr) if (e.a) host().registered.push_back(e.a); return V(); });
}
