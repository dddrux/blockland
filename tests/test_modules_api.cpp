// patchMod.1: mini-APIs de modulos (api::arg, api::Cmd/dispatch, api::Cfg, api::chat) + gancho onEvent.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#include "../src/modules/exemplo/exemplo.inc"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
SDL_mutex* SDL_CreateMutex() { return (SDL_mutex*)1; } int SDL_LockMutex(SDL_mutex*) { return 0; } int SDL_UnlockMutex(SDL_mutex*) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
static int gHits = 0;
int main() {
    Phys P; memset(&P, 0, sizeof P);
    SECTION("A1 api::arg");
    const std::vector<std::string> a = {"12", "x", "3.5", "-7"};
    CHECK(api::arg::num(a, 0) == 12 && api::arg::num(a, 1, 99) == 99 && api::arg::num(a, 9, 5) == 5 && api::arg::num(a, 3) == -7);
    CHECK(api::arg::flt(a, 2) == 3.5 && api::arg::flt(a, 1, 1.5) == 1.5);
    CHECK(api::arg::str(a, 1) == "x" && api::arg::str(a, 8, "d") == "d" && api::arg::has(a, 4) && !api::arg::has(a, 5));

    SECTION("A2 api::dispatch + Cmd");
    static const api::Cmd T[] = { {"zz_ok", "/zz_ok", [](const std::vector<std::string>&, Phys&) { ++gHits; return true; }},
                                  {"zz_uso", "/zz_uso <n>", [](const std::vector<std::string>& v, Phys&) { return !v.empty(); }} };
    CHECK(api::dispatch(T, 2, "zz_ok", {}, P) && gHits == 1);
    CHECK(!api::dispatch(T, 2, "outro", {}, P));
    const size_t c0 = chatLog.size(); CHECK(api::dispatch(T, 2, "zz_uso", {}, P) && chatLog.size() == c0 + 1);   // uso errado: trata e mostra "Uso:"
    const size_t c1 = chatLog.size(); CHECK(api::dispatch(T, 2, "zz_uso", {"1"}, P) && chatLog.size() == c1);
    SECTION("A3 api::Cfg ida e volta");
    { api::Cfg c("zz_teste"); c.set("nome", "abc"); c.setInt("n", 42); c.set("nome", "xyz"); CHECK(c.save()); }
    { api::Cfg c("zz_teste"); CHECK(c.get("nome") == "xyz" && c.getInt("n") == 42 && c.getInt("falta", 7) == 7 && c.get("falta", "d") == "d"); }
    remove("modulo_zz_teste.cfg");
    SECTION("A4 api::chat e exemplo via execCommand");
    const size_t c2 = chatLog.size(); api::chat::ok("a"); api::chat::err("b"); api::chat::say("c"); CHECK(chatLog.size() == c2 + 3);
    const size_t c3 = chatLog.size(); execCommand("/exemplo", P); CHECK(chatLog.size() == c3 + 1);
    SECTION("A5 gancho onEvent: consumir e repassar");
    static GameModule M; M.name = "ev"; M.onEvent = [](const SDL_Event& e) { ++gHits; return e.type == SDL_KEYDOWN; }; ModuleReg r(M);
    SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; const int h0 = gHits;
    CHECK(modulesEvent(e) && gHits == h0 + 1 + 0);   // o exemplo devolve false e o "ev" consome: ambos chamados, resultado true
    e.type = SDL_MOUSEMOTION; CHECK(!modulesEvent(e));
    SECTION("A6 ganchos de bloco: placeBlock real dispara onBlockPlace (e so quando coloca)");
    static int pl = 0, plx, ply, plz, plid, plmeta; static GameModule B; B.name = "blk"; B.onBlockPlace = [](int x, int y, int z, int id, int meta) { ++pl; plx = x; ply = y; plz = z; plid = id; plmeta = meta; }; ModuleReg rb(B);
    const int AY = 150; for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 4; y++) setB(x, y, z, AIR); }
    PlaceCtx c{}; c.hx = 4; c.hy = AY - 1; c.hz = 4; c.face = 3; c.fx = .5; c.fy = 1; c.fz = .5; c.px = 40; c.py = AY; c.pz = 40;
    CHECK(placeBlock(STONE, c) && pl == 1 && plx == 4 && ply == AY && plz == 4 && plid == STONE && plmeta == 0);
    CHECK(!placeBlock(STONE, c) && pl == 1);   // celula ocupada: nao coloca, nao avisa
    SECTION("A7 ganchos de mundo + REGISTER_FN (so os campos usados)");
    static std::string lastLoad, lastSave; static int resets = 0;
    struct W { static GameModule mk() { GameModule m; m.name = "wld"; m.onWorldLoad = [](const std::string& d) { lastLoad = d; }; m.onWorldSave = [](const std::string& d) { lastSave = d; }; m.onWorldReset = [] { ++resets; }; return m; } };
    ModuleReg rw(W::mk());
    modulesWorldLoad("w1"); modulesWorldSave("w2"); modulesWorldReset(); modulesWorldReset();
    CHECK(lastLoad == "w1" && lastSave == "w2" && resets == 2);
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
