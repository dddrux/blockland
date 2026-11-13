// ---------- modulo de exemplo: molde para criar o seu (usa as mini-APIs de src/modules/api.inc) ----------
// Novo modulo: tools/novo_modulo.sh <nome>  (copia este molde, troca os nomes e liga em modules_list.inc).
// Tudo dentro de namespace proprio (mod_<nome>): o jogo e UMA unidade de compilacao, nomes iguais em dois modulos colidiriam.
// Todos os ganchos sao opcionais (nullptr). Prefixo proprio em comandos/arquivos (ex.: exemplo_...).
namespace mod_exemplo {
    static int ticks = 0;
    static void onInit() { ticks = 0; }
    static void onTick(Phys&) { ++ticks; }
    static void onDrawHud(int, int) { /* api::hud::begin(w, h); api::hud::rect(...); api::hud::text(...); api::hud::end(); */ }
    static bool cmdPrincipal(const std::vector<std::string>&, Phys&) { api::chat::say("Modulo de exemplo ativo (" + std::to_string(ticks) + " ticks)"); return true; }
    static const api::Cmd CMDS[] = { {"exemplo", "/exemplo", cmdPrincipal} };
    static bool onCommand(const std::string& cmd, const std::vector<std::string>& args, Phys& P) { return api::dispatch(CMDS, sizeof CMDS / sizeof *CMDS, cmd, args, P); }
    static bool onEvent(const SDL_Event&) { return false; }   // true = consome o evento (o jogo nao ve)
}
GAME_MODULE_REGISTER(exemplo, "exemplo", mod_exemplo::onInit, mod_exemplo::onTick, nullptr, mod_exemplo::onDrawHud, mod_exemplo::onCommand, nullptr, mod_exemplo::onEvent);
