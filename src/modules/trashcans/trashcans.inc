// ---------- modulo trashcans (port de Trash Cans 1.1.2 / SuperMartijn642, MC 1.12.2) ----------
// Modelos, texturas, receitas e drops dos 4 blocos vem de mods/trashcans (conversor). Aqui entra o comportamento:
//   item_trash_can     : botao direito abre a GUI de 1 slot (gTrashUi em inventory_ui.inc): tudo que entra no slot e destruido; shift-click na mochila tambem destroi
//   liquid_trash_can   : botao direito com balde cheio (agua/lava/leite) esvazia o balde
//   energy_trash_can   : o jogo nao tem energia (FE/EU): so avisa
//   ultimate_trash_can : item + liquido (+ energia, ignorada)
// Config: modulo_trashcans.cfg  trashcans_item=1 trashcans_liquid=1 trashcans_energy=1 (como a config do mod original)
// A GUI reaproveita a tela de bau (gChest, 1 linha) com um slot central. Funis/pipes: nao existem no jogo.
namespace mod_trashcans {
    static bool gItem = true, gLiquid = true, gSneakNow = false;
    static void load() { api::Cfg c("trashcans"); gItem = c.getInt("trashcans_item", 1); gLiquid = c.getInt("trashcans_liquid", 1); }
    static bool isFilledBucket(int id) { return id && (id == gBktWater || id == gBktLava || id == gBktMilk); }
    static void onTick(Phys&) { if (gTrashUi && gTrashSlot.id) { gTrashCnt += gTrashSlot.n; gTrashSlot = Slot(); } }   // o slot destroi tudo a cada tick
    static bool voidItem(bool one) {
        Slot& s = inv[gModHeldSel]; if (!s.id || s.n <= 0 || isFilledBucket(s.id)) return false;
        const int n = one ? 1 : s.n; const std::string nm = itemName(s.id);
        if (creative && gGm == 3) return false;
        if (n >= s.n) s = Slot(); else s.n -= n;
        api::chat::say("Trash Can: destruiu " + std::to_string(n) + "x " + nm, 0xAAAAAA); return true;
    }
    static bool voidLiquid() {
        Slot& s = inv[gModHeldSel]; if (!isFilledBucket(s.id) || !gBktEmpty) return false;
        s.id = gBktEmpty; api::chat::say("Trash Can: esvaziou o balde", 0xAAAAAA); return true;
    }
    static bool onPropUse(const std::string& key, int, int, int) {
        const bool ult = key == "trashcans_ultimate_trash_can";
        const bool item = ult || key == "trashcans_item_trash_can", liq = ult || key == "trashcans_liquid_trash_can", en = key == "trashcans_energy_trash_can";
        if (!item && !liq && !en) return false;
        if (gGm == 3) return true;
        if (en) { api::chat::say("Energy Trash Can: o jogo nao tem energia", 0xAAAAAA); return true; }
        bool did = false;
        if (liq && gLiquid) did = voidLiquid();
        if (did) return true;
        if (item && gItem) {   // abre a GUI (mesmo caminho do Container generico: gPropOpenChest e tratado em frame_begin.inc)
            gTrashUi = gChest = true; gFurn = gBrewUi = gAnvUi = false; gChEnder = false; gChShulker = false; gChWait = false; gChRows = 1; gChCnt = 1;
            gTrashSlot = Slot(); gTrashCnt = 0; gPropOpenChest = true;
        } else api::chat::say("Trash Can: segure um balde cheio", 0xAAAAAA);
        return true;
    }
    static GameModule mk() { GameModule m; m.name = "trashcans"; m.onInit = load; m.onTick = onTick; m.onWorldReset = []() { gTrashUi = false; }; m.onPropUse = onPropUse; return m; }
}
GAME_MODULE_REGISTER_FN(trashcans, mod_trashcans::mk);
