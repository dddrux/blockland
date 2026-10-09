// ---------- modulo de exemplo: molde para criar o seu ----------
// 1) Copie esta pasta (src/modules/exemplo -> src/modules/<seu_nome>) e renomeie o arquivo.
// 2) Tudo dentro de um namespace proprio (mod_<nome>): o jogo e UMA unidade de compilacao, nomes iguais em dois modulos colidiriam.
// 3) Registre os ganchos no fim e ligue o modulo em src/modules/modules_list.inc.
// Pode usar qualquer API do jogo (getB/setB, placeBlock, chatAdd, drawMesh, text/flat...). Os ganchos abaixo sao todos opcionais.
namespace mod_exemplo {
    static int ticks = 0;
    static void onInit() { ticks = 0; }
    static void onTick(Phys&) { ++ticks; }
    static void onDrawHud(int, int) { /* desenhe HUD aqui (text(), flat()) */ }
    static bool onCommand(const std::string& cmd, const std::vector<std::string>&, Phys&) {
        if (cmd != "exemplo") return false;
        chatAdd("Modulo de exemplo ativo (" + std::to_string(ticks) + " ticks)", 0xFFFFFF);
        return true;
    }
}
GAME_MODULE_REGISTER(exemplo, "exemplo", mod_exemplo::onInit, mod_exemplo::onTick, nullptr, mod_exemplo::onDrawHud, mod_exemplo::onCommand, nullptr);
