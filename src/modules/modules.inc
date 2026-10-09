// ---------- sistema de modulos (registro de ganchos) ----------
// Objetivo: dois projetos separados (o seu e o do seu amigo) que se juntam no fim SEM editar os mesmos arquivos.
// Cada modulo vive na propria pasta (src/modules/<nome>/<nome>.inc), registra seus ganchos aqui e so aparece em modules_list.inc (1 linha).
// Todos os ganchos sao OPCIONAIS (deixe nullptr o que nao usar). Ver src/modules/README.md.
struct GameModule {
    const char* name = "";
    void (*onInit)() = nullptr;                                              // 1x, depois de pack/modelos/mods carregados (antes do 1o mundo)
    void (*onTick)(Phys& P) = nullptr;                                       // 20x por segundo (so com o jogo rodando; congela na pausa)
    void (*onDrawWorld)(float pt) = nullptr;                                 // 3D, depois dos mobs/itens (atlas ja vinculado). pt = parcial do tick (0..1)
    void (*onDrawHud)(int w, int h) = nullptr;                               // 2D, depois do HUD/chat
    bool (*onCommand)(const std::string& cmd, const std::vector<std::string>& args, Phys& P) = nullptr;   // "/cmd args"; devolva true se tratou. cmd ja vem sem acento/minusculo
    void (*onShutdown)() = nullptr;                                          // ao fechar o jogo
};
static std::vector<GameModule>& gModules() { static std::vector<GameModule> v; return v; }   // funcao: a ordem de init estatica entre arquivos nao importa
struct ModuleReg { explicit ModuleReg(const GameModule& m) { gModules().push_back(m); } };
#define GAME_MODULE_REGISTER(ID, ...) static ModuleReg gModReg_##ID(GameModule{__VA_ARGS__})   // GAME_MODULE_REGISTER(meu, "meu", meuInit, meuTick, ...)

static void modulesInit()                            { for (auto& m : gModules()) if (m.onInit) m.onInit(); printf("modulos: %d registrado(s)\n", (int)gModules().size()); }
static void modulesTick(Phys& P)                     { for (auto& m : gModules()) if (m.onTick) m.onTick(P); }
static void modulesDrawWorld(float pt)               { for (auto& m : gModules()) if (m.onDrawWorld) m.onDrawWorld(pt); }
static void modulesDrawHud(int w, int h)             { for (auto& m : gModules()) if (m.onDrawHud) m.onDrawHud(w, h); }
static void modulesShutdown()                        { for (auto& m : gModules()) if (m.onShutdown) m.onShutdown(); }
static bool modulesCommand(const std::string& cmd, const std::vector<std::string>& args, Phys& P) {
    for (auto& m : gModules()) if (m.onCommand && m.onCommand(cmd, args, P)) return true;
    return false;
}

// Monitor Touch (modules/monitor/monitor.inc): pontes usadas pelo laco principal
static bool monIsOpen(); static bool monUse(int x, int y, int z); static bool monEvent(const SDL_Event& e, int w, int h);
