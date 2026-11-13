// patchJVM.10: ciclo de vida do mod Forge sobre o runtime AOT: preInit -> eventos de registro (blocos/itens) -> init -> postInit. Cada falha vira uma linha de log (nunca derruba o jogo).
inline void loadMods(std::vector<std::string>& log, const std::string& modId = "") {
    jdk(); if (!modId.empty()) host().modId = modId;
    std::vector<std::string> names; for (auto& kv : classes()) if (!kv.second.m.empty()) names.push_back(kv.first); std::sort(names.begin(), names.end());
    auto call = [&](ClassInfo* c, const MDef& d, Obj* ev) { V a[3]; int n = 0; if (!(d.acc & 8)) { Obj* o = mk(c->name.c_str()); if (const MDef* k = findM(c, "<init>", "()V")) { V b[1]; b[0].a = o; k->fn(b); } a[n++].a = o; } a[n++].a = ev;
        try { ensureInit(c); d.fn(a); return true; } catch (Thrown& t) { log.push_back("falhou " + c->name + "." + d.name + ": " + (t.ex ? toStr(t.ex) : "?")); return false; } catch (...) { log.push_back("falhou " + c->name + "." + d.name); return false; } };
    auto phase = [&](const char* tag, const char* evType, const char* ev) { int n = 0; for (auto& nm : names) { ClassInfo* c = ci(nm); for (auto& d : std::vector<MDef>(c->m)) if (strstr(d.desc, evType)) { Obj* e = mk(ev); if (call(c, d, e)) n++; } } log.push_back(std::string(tag) + ": " + std::to_string(n) + " metodo(s)"); };
    phase("preInit", "FMLPreInitializationEvent", "net/minecraftforge/fml/common/event/FMLPreInitializationEvent");
    phase("registro", "RegistryEvent$Register;", "net/minecraftforge/event/RegistryEvent$Register");
    phase("init", "FMLInitializationEvent", "net/minecraftforge/fml/common/event/FMLInitializationEvent");
    phase("postInit", "FMLPostInitializationEvent", "net/minecraftforge/fml/common/event/FMLPostInitializationEvent");
    log.push_back("objetos registrados: " + std::to_string(host().registered.size()));
}
