// patchJVM.4: testa o shim do Minecraft chamando como o codigo gerado chama (rt::invoke*)
#include "jvmrt.h"
using namespace rt;
static int bad = 0;
#define CK(c) do { if (!(c)) { printf("FALHOU: %s\n", #c); bad++; } } while (0)
static V call(const char* o, const char* n, const char* d, std::initializer_list<V> a, bool st = false) { std::vector<V> v(a); MRef m{o, n, d}; return st ? invokestatic(m, v.data(), (int)v.size()) : invokevirtual(m, v.data(), (int)v.size()); }
int main() {
    jdk(); Obj* blk = newobj("net/minecraft/block/Block");
    V it = call("net/minecraft/item/Item", "func_150898_a", "(Lnet/minecraft/block/Block;)Lnet/minecraft/item/Item;", {va(blk)}, true);
    V it2 = call("net/minecraft/item/Item", "func_150898_a", "(Lnet/minecraft/block/Block;)Lnet/minecraft/item/Item;", {va(blk)}, true); CK(it.a == it2.a && it.a);
    Obj* st = newobj("net/minecraft/item/ItemStack"); { V a[3] = {va(st), it, vi(5)}; invokespecial({"net/minecraft/item/ItemStack", "<init>", "(Lnet/minecraft/item/Item;I)V"}, a, 3); }
    CK(call("net/minecraft/item/ItemStack", "func_190916_E", "()I", {va(st)}).i == 5); CK(!call("net/minecraft/item/ItemStack", "func_190926_b", "()Z", {va(st)}).i);
    call("net/minecraft/item/ItemStack", "func_190918_g", "(I)V", {va(st), vi(5)}); CK(call("net/minecraft/item/ItemStack", "func_190926_b", "()Z", {va(st)}).i);
    Obj* cp = call("net/minecraft/item/ItemStack", "func_77946_l", "()Lnet/minecraft/item/ItemStack;", {va(st)}).a; CK(cp != st && cp->f["count"].i == 0);
    Obj* l = call("net/minecraft/util/NonNullList", "func_191197_a", "(ILjava/lang/Object;)Lnet/minecraft/util/NonNullList;", {vi(3), va(emptyStack())}, true).a;
    CK(call("net/minecraft/util/NonNullList", "get", "(I)Ljava/lang/Object;", {va(l), vi(2)}).a == emptyStack());
    call("net/minecraft/util/NonNullList", "set", "(ILjava/lang/Object;)Ljava/lang/Object;", {va(l), vi(1), va(st)}); CK(call("net/minecraft/util/NonNullList", "get", "(I)Ljava/lang/Object;", {va(l), vi(1)}).a == st);
    bool ex = false; try { call("net/minecraft/util/NonNullList", "get", "(I)Ljava/lang/Object;", {va(l), vi(9)}); } catch (Thrown&) { ex = true; } CK(ex);
    Obj* p = mkPos(1, 64, -3); Obj* up = call("net/minecraft/util/math/BlockPos", "func_177984_a", "()Lnet/minecraft/util/math/BlockPos;", {va(p)}).a;
    CK(call("net/minecraft/util/math/BlockPos", "func_177956_o", "()I", {va(up)}).i == 65); CK(call("net/minecraft/util/math/BlockPos", "func_177952_p", "()I", {va(up)}).i == -3);
    Obj* e = call("net/minecraft/util/math/BlockPos", "func_177972_a", "(Lnet/minecraft/util/EnumFacing;)Lnet/minecraft/util/math/BlockPos;", {va(p), va(facing(5))}).a; CK(e->f["x"].i == 2);
    Obj* op = call("net/minecraft/util/EnumFacing", "func_176734_d", "()Lnet/minecraft/util/EnumFacing;", {va(facing(2))}).a; CK(op == facing(3)); CK(call("java/lang/Enum", "ordinal", "()I", {va(op)}).i == 3);
    Obj* n = newobj("net/minecraft/nbt/NBTTagCompound"); call("net/minecraft/nbt/NBTTagCompound", "func_74768_a", "(Ljava/lang/String;I)V", {va(str("burn")), vi(200)});
    call("net/minecraft/nbt/NBTTagCompound", "func_74768_a", "(Ljava/lang/String;I)V", {va(n), va(str("burn")), vi(200)}) ; // (args deslocados de proposito: nao deve quebrar)
    CK(call("net/minecraft/nbt/NBTTagCompound", "func_74762_e", "(Ljava/lang/String;)I", {va(n), va(str("burn"))}).i == 200); CK(call("net/minecraft/nbt/NBTTagCompound", "func_74762_e", "(Ljava/lang/String;)I", {va(n), va(str("x"))}).i == 0);
    CK(call("net/minecraft/nbt/NBTTagCompound", "func_74764_b", "(Ljava/lang/String;)Z", {va(n), va(str("burn"))}).i == 1);
    CK(call("net/minecraft/nbt/NBTTagCompound", "func_74779_i", "(Ljava/lang/String;)Ljava/lang/String;", {va(n), va(str("nome"))}).a->s == "");
    Obj* s0 = newobj("net/minecraft/block/state/BlockStateImpl"); Obj* prop = newobj("java/lang/Object"); V pv = va(box("java/lang/Integer", vi(7)));
    Obj* s1 = call("net/minecraft/block/state/IBlockState", "func_177226_a", "(Lnet/minecraft/block/properties/IProperty;Ljava/lang/Comparable;)Lnet/minecraft/block/state/IBlockState;", {va(s0), va(prop), pv}).a;
    CK(s1 != s0 && call("net/minecraft/block/state/IBlockState", "func_177229_b", "(Lnet/minecraft/block/properties/IProperty;)Ljava/lang/Comparable;", {va(s1), va(prop)}).a == pv.a); CK(call("net/minecraft/block/state/IBlockState", "func_177229_b", "(Lnet/minecraft/block/properties/IProperty;)Ljava/lang/Comparable;", {va(s0), va(prop)}).a == nullptr);
    Obj* w = newobj("net/minecraft/world/World"); CK(call("net/minecraft/world/World", "func_180495_p", "(Lnet/minecraft/util/math/BlockPos;)Lnet/minecraft/block/state/IBlockState;", {va(w), va(p)}).a == nullptr);
    host().getState = [&](int x, int y, int z) { return x == 1 && y == 64 && z == -3 ? s1 : nullptr; }; CK(call("net/minecraft/world/World", "func_180495_p", "(Lnet/minecraft/util/math/BlockPos;)Lnet/minecraft/block/state/IBlockState;", {va(w), va(p)}).a == s1);
    host().smelt = [&](Obj* in) { return mkStack(it.a, 1, 0); }; Obj* fr = call("net/minecraft/item/crafting/FurnaceRecipes", "func_77602_a", "()Lnet/minecraft/item/crafting/FurnaceRecipes;", {}, true).a;
    CK(!stackEmpty(call("net/minecraft/item/crafting/FurnaceRecipes", "func_151395_a", "(Lnet/minecraft/item/ItemStack;)Lnet/minecraft/item/ItemStack;", {va(fr), va(st)}).a));
    { // patchJVM.6: shim2
        V v = call("net/minecraft/util/EnumFacing", "values", "()[Lnet/minecraft/util/EnumFacing;", {}, true); CK(v.a && v.a->arr.size() == 6);
        V f = call("net/minecraft/util/EnumFacing", "func_82600_a", "(I)Lnet/minecraft/util/EnumFacing;", {vi(5)}, true); CK(f.a && f.a->f["ordinal"].i == 5);
        V ax = call("net/minecraft/util/EnumFacing", "func_176740_k", "()Lnet/minecraft/util/EnumFacing$Axis;", {f}); CK(call("net/minecraft/util/EnumFacing$Axis", "ordinal", "()I", {ax}).i == 0);
        CK(call("net/minecraft/util/math/MathHelper", "func_76125_a", "(III)I", {vi(9), vi(0), vi(4)}, true).i == 4);
        Obj* l = newobj("net/minecraft/util/NonNullList"); Obj* s1 = mkStack(blk, 7, 0); l->arr.push_back(va(s1)); l->arr.push_back(va(emptyStack()));
        V r = call("net/minecraft/inventory/ItemStackHelper", "func_188382_a", "(Ljava/util/List;II)Lnet/minecraft/item/ItemStack;", {va(l), vi(0), vi(3)}, true); CK(r.a->f["count"].i == 3 && s1->f["count"].i == 4);
        Obj* nbt = newobj("net/minecraft/nbt/NBTTagCompound"); call("net/minecraft/inventory/ItemStackHelper", "func_191282_a", "(Lnet/minecraft/nbt/NBTTagCompound;Lnet/minecraft/util/NonNullList;)Lnet/minecraft/nbt/NBTTagCompound;", {va(nbt), va(l)}, true);
        Obj* l2 = newobj("net/minecraft/util/NonNullList"); l2->arr.assign(2, va(emptyStack())); call("net/minecraft/inventory/ItemStackHelper", "func_191283_b", "(Lnet/minecraft/nbt/NBTTagCompound;Lnet/minecraft/util/NonNullList;)V", {va(nbt), va(l2)}, true); CK(l2->arr[0].a->f["count"].i == 4);
    }
    { // patchJVM.8: shim3 (corelib)
        Obj* rl = newobj("net/minecraft/util/ResourceLocation"); { V a[2] = {va(rl), va(str("foo:bar/baz"))}; invokespecial({"net/minecraft/util/ResourceLocation", "<init>", "(Ljava/lang/String;)V"}, a, 2); }
        CK(toStr(call("net/minecraft/util/ResourceLocation", "toString", "()Ljava/lang/String;", {va(rl)}).a) == "foo:bar/baz");
        Obj* rl2 = newobj("net/minecraft/util/ResourceLocation"); { V a[2] = {va(rl2), va(str("x"))}; invokespecial({"net/minecraft/util/ResourceLocation", "<init>", "(Ljava/lang/String;)V"}, a, 2); }
        CK(toStr(call("net/minecraft/util/ResourceLocation", "func_110624_b", "()Ljava/lang/String;", {va(rl2)}).a) == "minecraft");
        V c = call("java/lang/Object", "getClass", "()Ljava/lang/Class;", {va(rl)}); CK(toStr(call("java/lang/Class", "getSimpleName", "()Ljava/lang/String;", {c}).a) == "ResourceLocation");
        CK(toStr(call("java/lang/String", "replace", "(CC)Ljava/lang/String;", {va(str("a.b.c")), vi('.'), vi('/')}).a) == "a/b/c");
        CK(call("java/lang/String", "lastIndexOf", "(I)I", {va(str("a/b/c")), vi('/')}).i == 3);
        CK(toStr(call("java/lang/String", "trim", "()Ljava/lang/String;", {va(str("  x "))}).a) == "x");
        V sp = call("java/lang/String", "split", "(Ljava/lang/String;)[Ljava/lang/String;", {va(str("a,b,c")), va(str(","))}); CK(sp.a->arr.size() == 3);
    }
    { // patchJVM.9: streams, regex, LinkedHashMap
        Obj* l = newobj("java/util/ArrayList"); l->arr.push_back(va(str("a1"))); l->arr.push_back(va(str("b22"))); l->arr.push_back(va(str("c3")));
        V st = call("java/util/List", "stream", "()Ljava/util/stream/Stream;", {va(l)}); CK(st.a->arr.size() == 3);
        V cl = call("java/util/stream/Collectors", "toList", "()Ljava/util/stream/Collector;", {}, true); V r = call("java/util/stream/Stream", "collect", "(Ljava/util/stream/Collector;)Ljava/lang/Object;", {st, cl}); CK(r.a->arr.size() == 3);
        Obj* p = call("java/util/regex/Pattern", "compile", "(Ljava/lang/String;)Ljava/util/regex/Pattern;", {va(str("([a-z])(\\d+)"))}, true).a;
        V m = call("java/util/regex/Pattern", "matcher", "(Ljava/lang/CharSequence;)Ljava/util/regex/Matcher;", {va(p), va(str("xx b22 c3"))});
        CK(call("java/util/regex/Matcher", "find", "()Z", {m}).i); CK(toStr(call("java/util/regex/Matcher", "group", "(I)Ljava/lang/String;", {m, vi(2)}).a) == "22");
        CK(call("java/util/regex/Matcher", "find", "()Z", {m}).i); CK(toStr(call("java/util/regex/Matcher", "group", "(I)Ljava/lang/String;", {m, vi(1)}).a) == "c"); CK(!call("java/util/regex/Matcher", "find", "()Z", {m}).i);
        CK(call("java/lang/String", "matches", "(Ljava/lang/String;)Z", {va(str("abc123")), va(str("[a-z]+\\d+"))}).i);
        Obj* lm = newobj("java/util/LinkedHashMap"); { V a[1] = {va(lm)}; invokespecial({"java/util/LinkedHashMap", "<init>", "()V"}, a, 1); }
        for (const char* k : {"zz", "aa", "mm"}) { V a[3] = {va(lm), va(str(k)), vi(1)}; call("java/util/HashMap", "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", {va(lm), va(str(k)), va(str(k))}); }
        CK(mapOrder(lm)[0].a->f["key"].a->s == "zz");
    }
    printf(bad ? "shim: %d falhas\n" : "shim: tudo ok\n", bad); return bad ? 1 : 0; }
