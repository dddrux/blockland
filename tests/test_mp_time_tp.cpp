// patch5.65: hora do mundo sincronizada (NP_TIME) com permissao de operador; NP_TPTO leva a dimensao. Roda o main.cpp REAL.
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
    Phys P; P.x = 8; P.y = 100; P.z = 8; netSock = -1;
    // host: so operador convidado muda a hora
    netOn = true; netHost = true; netSelf = 0; gOpIds.clear(); gOpIds.insert(0); gOpIds.insert(1); gWorldTime = 1000; gDayCycle = true;
    { NBuf b; b.put<int64_t>(5000); b.put<uint8_t>(3); netTimeRecv(b, 1); } CHECK(gWorldTime == 5000 && gDayCycle);   // convidado op, comando
    { NBuf b; b.put<int64_t>(9000); b.put<uint8_t>(2 | 1); netTimeRecv(b, 2); } CHECK(gWorldTime == 5000);   // convidado sem op: ignorado
    { NBuf b; b.put<int64_t>(9000); b.put<uint8_t>(0); netTimeRecv(b, 1); } CHECK(gWorldTime == 5000);   // sem flag de comando: ignorado
    { NBuf b; b.put<int64_t>(7000); b.put<uint8_t>(0); netTimeRecv(b, 1); } CHECK(gWorldTime == 5000);
    // convidado: aceita so do host
    netHost = false; netSelf = 2; gOpIds.clear(); gOpIds.insert(0);
    { NBuf b; b.put<int64_t>(12345); b.put<uint8_t>(0); netTimeRecv(b, 0); } CHECK(gWorldTime == 12345 && !gDayCycle);   // host manda: aplica hora e ciclo parado
    { NBuf b; b.put<int64_t>(1); b.put<uint8_t>(1); netTimeRecv(b, 1); } CHECK(gWorldTime == 12345);   // de outro convidado: ignora
    // comando /time por convidado sem op e com op
    gWorldTime = 100; gDayCycle = true; execCommand("/time set night", P); CHECK(gWorldTime == 100);   // negado
    execCommand("/time query", P);   // consulta e livre
    gOpIds.insert(2); execCommand("/time set night", P); CHECK(gWorldTime % DAY_TICKS == 13000);   // op: muda localmente (e pede ao host)
    execCommand("/time pause", P); CHECK(!gDayCycle);
    // host muda: aplica local sem exigir nada
    netHost = true; netSelf = 0; execCommand("/time resume", P); CHECK(gDayCycle);
    // offline: nada de rede, tudo livre
    netOn = false; netHost = false; gWorldTime = 0; execCommand("/time set noon", P); CHECK(gWorldTime % DAY_TICKS == 6000);
    // TPTO com dimensao
    netOn = true; netHost = false; netSelf = 2; gOpIds.clear(); gOpIds.insert(0);
    { NBuf b; b.put<uint8_t>(2); b.put<double>(1.5); b.put<double>(64.0); b.put<double>(-3.5); b.put<uint8_t>(1); netCmdRecv(NP_TPTO, 0, b); }
    CHECK(gNetTp.on && gNetTp.dim == 1 && gNetTp.x == 1.5 && gNetTp.z == -3.5);
    gNetTp.on = false;
    { NBuf b; b.put<uint8_t>(2); b.put<double>(1.0); b.put<double>(64.0); b.put<double>(1.0); netCmdRecv(NP_TPTO, 0, b); }   // pacote antigo, sem dim
    CHECK(gNetTp.on && gNetTp.dim == -1);
    CHECK(cmdPlayerDim(netName()) == gDim);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
