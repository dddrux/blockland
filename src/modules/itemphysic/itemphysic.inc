// ---------- modulo itemphysic (port de ItemPhysic Lite 1.3.7, MC 1.12.2) ----------
// Itens soltos: no chao ficam DEITADOS (itens planos) ou parados (blocos) com giro fixo; sem flutuar/girar.
// Caindo ou na agua: rolam (tombam) em vez de girar em pe. O desenho esta em game/drops.inc (drawDrops), este modulo liga/desliga.
// Config: modulo_itemphysic.cfg  itemphysic_on=1.  Comando: /itemphysic [0|1]
// Nao portado: pocas de lava/fogo queimando itens e o tempo de despawn configuravel do mod original.
namespace mod_itemphysic {
    static void onInit() { api::Cfg c("itemphysic"); gItemPhys = c.getInt("itemphysic_on", 1) != 0; }
    static bool cmd(const std::vector<std::string>& a, Phys&) {
        gItemPhys = a.empty() ? !gItemPhys : (a[0] == "1" || a[0] == "on");
        api::Cfg c("itemphysic"); c.setInt("itemphysic_on", gItemPhys ? 1 : 0); c.save();
        api::chat::ok(std::string("ItemPhysic: ") + (gItemPhys ? "ligado" : "desligado")); return true;
    }
    static const api::Cmd CMDS[] = { {"itemphysic", "/itemphysic [0|1]", cmd} };
    static bool onCommand(const std::string& c, const std::vector<std::string>& a, Phys& P) { return api::dispatch(CMDS, sizeof CMDS / sizeof *CMDS, c, a, P); }
}
GAME_MODULE_REGISTER(itemphysic, "itemphysic", mod_itemphysic::onInit, nullptr, nullptr, nullptr, mod_itemphysic::onCommand, nullptr, nullptr);
