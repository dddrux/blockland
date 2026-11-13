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
    bool (*onEvent)(const SDL_Event& e) = nullptr;                           // patchMod.1: evento SDL bruto (teclas, mouse...); devolva true para CONSUMIR (o jogo nao ve). Nunca recebe SDL_QUIT. Campo no fim: registros antigos (posicionais) continuam validos
    // patchMod.2: ganchos de mundo e blocos (todos no fim do struct; para usa-los monte o GameModule por campo: GAME_MODULE_REGISTER_FN)
    void (*onWorldLoad)(const std::string& dir) = nullptr;                   // depois de carregar chunks/baus/etc. de saves/<dir> (leia seus arquivos aqui)
    void (*onWorldSave)(const std::string& dir) = nullptr;                   // ao salvar o mundo (so host/solo; convidado nao salva o mundo). Grave em saves/<dir>/<prefixo>_...
    void (*onWorldReset)() = nullptr;                                        // ao descartar o mundo (sair para o menu): limpe o estado do modulo
    void (*onBlockBreak)(int x, int y, int z, int id) = nullptr;             // jogador local quebrou um bloco (antes de virar ar)
    void (*onBlockPlace)(int x, int y, int z, int id, int meta) = nullptr;   // jogador local colocou um bloco (depois de colocado)
    bool (*onPropUse)(const std::string& key, int x, int y, int z) = nullptr;   // patchJVM.11: botao direito num prop de mod (key = `ns_nome`); true = tratou (o jogo nao abre a tela dele)
};
static int gModHeldSel = 0;   // patchTreeChop.1: slot da hotbar na hora de quebrar (lido por modulos em onBlockBreak)
static bool gModHudBlocked = false;   // patchJade.2: true com inventario/pausa/chat abertos (modulos de HUD passivo se escondem)
static std::vector<GameModule>& gModules() { static std::vector<GameModule> v; return v; }   // funcao: a ordem de init estatica entre arquivos nao importa
struct ModuleReg { explicit ModuleReg(const GameModule& m) { gModules().push_back(m); } };
#define GAME_MODULE_REGISTER_FN(ID, MAKE) static ModuleReg gModReg_##ID(MAKE())   // MAKE = funcao "static GameModule mk() { GameModule m; m.name = \"x\"; m.onWorldSave = ...; return m; }" (so os campos que usar)
#define GAME_MODULE_REGISTER(ID, ...) static ModuleReg gModReg_##ID(GameModule{__VA_ARGS__})   // GAME_MODULE_REGISTER(meu, "meu", meuInit, meuTick, ...)

static void modulesInit()                            { for (auto& m : gModules()) if (m.onInit) m.onInit(); printf("modulos: %d registrado(s)\n", (int)gModules().size()); }
static void modulesTick(Phys& P)                     { for (auto& m : gModules()) if (m.onTick) m.onTick(P); }
static void modulesDrawWorld(float pt)               { for (auto& m : gModules()) if (m.onDrawWorld) m.onDrawWorld(pt); }
static void modulesDrawHud(int w, int h)             { for (auto& m : gModules()) if (m.onDrawHud) m.onDrawHud(w, h); }
static bool modulesEvent(const SDL_Event& e)         { for (auto& m : gModules()) if (m.onEvent && m.onEvent(e)) return true; return false; }
static void modulesWorldLoad(const std::string& d)   { for (auto& m : gModules()) if (m.onWorldLoad) m.onWorldLoad(d); }
static void modulesWorldSave(const std::string& d)   { for (auto& m : gModules()) if (m.onWorldSave) m.onWorldSave(d); }
static void modulesWorldReset()                      { for (auto& m : gModules()) if (m.onWorldReset) m.onWorldReset(); }
static void modulesBlockBreak(int x, int y, int z, int id)            { for (auto& m : gModules()) if (m.onBlockBreak) m.onBlockBreak(x, y, z, id); }
static void modulesBlockPlace(int x, int y, int z, int id, int meta)  { for (auto& m : gModules()) if (m.onBlockPlace) m.onBlockPlace(x, y, z, id, meta); }
static bool modulesPropUse(const std::string& key, int x, int y, int z) { for (auto& m : gModules()) if (m.onPropUse && m.onPropUse(key, x, y, z)) return true; return false; }
static void modulesShutdown()                        { for (auto& m : gModules()) if (m.onShutdown) m.onShutdown(); }
static bool modulesCommand(const std::string& cmd, const std::vector<std::string>& args, Phys& P) {
    for (auto& m : gModules()) if (m.onCommand && m.onCommand(cmd, args, P)) return true;
    return false;
}

// Monitor Touch (modules/monitor/monitor.inc): pontes usadas pelo laco principal
static bool monIsOpen(); static bool monUse(int x, int y, int z); static bool monEvent(const SDL_Event& e, int w, int h);
