// patchJVM.5: ponte do "pseudo Forge" (runtime AOT em src/modules/jvm/aot) com o jogo: rt::host() passa a ler/gravar o mundo e a fornalha do Blockland.
// Sem mod gerado (mods/<mod>/aot vazio) nada muda no jogo.
#include <map>
#include <tuple>
#include "aot/jvmrt.h"
namespace aotbr {
    // patchJVM.7: tile entities do pseudo Forge, por posicao (o jogo nao tem TileEntity): vivem aqui, somem ao sair do mundo ou quando o bloco da posicao muda
    static std::map<std::tuple<int, int, int>, rt::Obj*>& tiles() { static std::map<std::tuple<int, int, int>, rt::Obj*> t; return t; }
    static rt::Obj* mkState(int id, int meta) { rt::Obj* s = rt::mk("net/minecraft/block/state/BlockStateImpl"); s->f["id"].i = id; s->f["meta"].i = meta; return s; }
    static bool& opened() { static bool o = false; return o; }
    // patchJVM.11: clique direito num prop de mod -> onBlockActivated. Se o mod abriu um inventario devolve false: o jogo abre o Container generico do prop (a GUI de fornalha ainda nao existe)
    static bool propUse(const std::string& key, int x, int y, int z) {
        if (rt::host().registered.empty()) return false;
        opened() = false; std::string err; const int r = rt::useBlock(key, x, y, z, &err); if (r == -1) return false;
        printf("aot: clique %s (%d,%d,%d) -> %s%s inv=%d\n", key.c_str(), x, y, z, r == -2 ? "falhou: " : (r ? "true" : "false"), err.c_str(), (int)opened());
        return r == 1 && !opened();
    }
    static void bind() {
        rt::Host& h = rt::host();
        h.getState = [](int x, int y, int z) -> rt::Obj* { return mkState(getB(x, y, z), getM(x, y, z)); };
        h.setState = [](int x, int y, int z, rt::Obj* st, int) -> bool { if (!st) return false; if (getB(x, y, z) != st->f["id"].i) tiles().erase({x, y, z}); setBM(x, y, z, st->f["id"].i, st->f["meta"].i); return true; };
        h.smelt = [](rt::Obj* stack) -> rt::Obj* { if (!stack || !stack->f["item"].a) return nullptr; int r = furnResult(stack->f["item"].a->f["id"].i); return r ? rt::mkStack(stack->f["item"].a, 1, 0) : nullptr; };
        h.getTile = [](int x, int y, int z) -> rt::Obj* { auto it = tiles().find({x, y, z}); return it == tiles().end() ? nullptr : it->second; };
        h.setTile = [](int x, int y, int z, rt::Obj* t) { if (t) { t->f["x"].i = x; t->f["y"].i = y; t->f["z"].i = z; tiles()[{x, y, z}] = t; } else tiles().erase({x, y, z}); };
        h.openInventory = [](rt::Obj*, rt::Obj*) { opened() = true; };
        h.particle = [](const std::string&, double, double, double) {};
    }
    static void init() { rt::jdk(); bind(); std::vector<std::string> lg; rt::loadMods(lg); bool any = !rt::host().registered.empty(); for (auto& s : lg) if (s.rfind("falhou", 0) == 0) any = true; if (any) for (auto& s : lg) printf("aot: %s\n", s.c_str()); }
}
static GameModule aotbrMake() { GameModule m; m.name = "aotbridge"; m.onInit = aotbr::init; m.onPropUse = aotbr::propUse; m.onWorldReset = []() { aotbr::tiles().clear(); };
    m.onBlockBreak = [](int x, int y, int z, int) { aotbr::tiles().erase({x, y, z}); }; return m; }
GAME_MODULE_REGISTER_FN(aotbridge, aotbrMake);
