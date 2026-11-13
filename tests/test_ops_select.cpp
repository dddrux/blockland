// patch5.58: /kill no criativo e em outros, operadores (host/ops, NP_OPS), seletores @s @a @e @r @p, FOV. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    Phys P; P.x = 8; P.y = 100; P.z = 8;
    // offline: sempre op; selecoes
    CHECK(isOp());
    bool mobs = true; auto r = cmdSelect("@s", P, mobs); CHECK(r.size() == 1 && !mobs);
    r = cmdSelect("@a", P, mobs); CHECK(!r.empty() && !mobs);
    r = cmdSelect("@e", P, mobs); CHECK(!r.empty() && mobs);
    r = cmdSelect("@r", P, mobs); CHECK(r.size() == 1);
    r = cmdSelect("@p", P, mobs); CHECK(r.size() == 1 && cmdIsSelf(r[0]));   // sem outro jogador: eu
    CHECK(!cmdOp("x", true).empty());   // offline: mensagem, nada muda
    // /kill no criativo (antes recusava)
    creative = true; gDead = false; gHP = 20; gInvul = 0;
    execCommand("/kill", P); CHECK(gDead && gHP == 0 && creative);
    gDead = false; gHP = 20; creative = false;
    execCommand("/kill @s", P); CHECK(gDead);
    gDead = false; gHP = 20;
    // FOV
    execCommand("/fov 90", P); CHECK(fabsf(gsf("fov", .5f) - .75f) < .001f);
    execCommand("/fov 500", P); CHECK(fabsf(gsf("fov", .5f) - 1.f) < .001f);
    execCommand("/fov 70", P); CHECK(fabsf(gsf("fov", 0.f) - .5f) < .001f);
    // multiplayer simulado (host): ops por nick
    netOn = true; netHost = true; netSelf = 0; netSock = -1;
    peers.clear(); peers.push_back({sockaddr_in{}, 0u, 1, {}, false, "Ana", -1}); peers.push_back({sockaddr_in{}, 0u, 2, {}, false, "Beto", -1});
    gOpNicks.clear(); gOpsLoaded = true; gOpIds.clear();
    CHECK(cmdOp("ana", true).empty()); opsSync();
    CHECK(gOpIds.count(0) && gOpIds.count(1) && !gOpIds.count(2));   // host sempre; Ana por nick (sem diferenciar caixa); Beto nao
    CHECK(cmdOp("ANA", false).empty()); opsSync(); CHECK(!gOpIds.count(1));
    // convidado: so op age sobre outros
    netHost = false; netSelf = 2; gOpIds.clear(); gOpIds.insert(0);
    CHECK(!isOp()); CHECK(!cmdSendKill("Ana").empty() || netFindNick("Ana") < 0);
    CHECK(!cmdOp("Ana", true).empty());   // convidado nao usa /op
    { NBuf b; b.put<uint8_t>(NP_OPS); b.put<uint8_t>(0); b.put<uint8_t>(2); b.put<uint8_t>(0); b.put<uint8_t>(2); b.n = b.n; b.r = 2; b.get<uint8_t>();   // NP_OPS: [n=2][0][2]
      gOpIds.clear(); int c = 2; b.r = 3; while (c-- > 0) gOpIds.insert(b.get<uint8_t>()); }
    CHECK(isOp());
    // alvo de NP_KILL: so vale vindo do host ou de op
    gDead = false; gHP = 20; creative = false; netSelf = 2; gOpIds.clear(); gOpIds.insert(0);
    { NBuf b; b.put<uint8_t>(2); netCmdRecv(NP_KILL, 1, b); CHECK(!gDead); }   // convidado 1 nao e op: ignorado
    { NBuf b; b.put<uint8_t>(2); netCmdRecv(NP_KILL, 0, b); CHECK(gDead); }    // host: vale
    gDead = false; gHP = 20; gOpIds.insert(1);
    { NBuf b; b.put<uint8_t>(2); netCmdRecv(NP_KILL, 1, b); CHECK(gDead); }    // convidado op: vale
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
