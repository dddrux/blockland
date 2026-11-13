// patchMP.3: NP_MOBUSE (convidado tosquia/alimenta; o host valida e aplica). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static NBuf mk(int uid, int act, int item) { NBuf b; b.put<int32_t>(uid); b.put<uint8_t>((uint8_t)act); b.put<int32_t>(item); return b; }
static void put(int uid, int type, double x, double z, bool baby = false) { MobEnt e; e.type = type; e.uid = uid; e.x = x; e.y = 70; e.z = z; e.baby = baby; MOBS.push_back(e); }
int main() {
    remotes.clear(); { RemotePl r; r.init = true; r.x = 0; r.y = 70; r.z = 0; remotes[2] = r; }
    MOBS.clear(); put(5, M_SHEEP, 3, 0); put(6, M_SHEEP, 30, 0); put(7, M_SHEEP, 2, 0, true); put(8, M_COW, 2, 1);
    const size_t d0 = DROPS.size();
    { NBuf b = mk(5, 2, shearsId()); mobUseRecv(b, 2); } CHECK(MOBS[0].sheared && DROPS.size() > d0);   // perto: tosquia e solta la
    { NBuf b = mk(6, 2, shearsId()); mobUseRecv(b, 2); } CHECK(!MOBS[1].sheared);                       // 30 blocos: pacote forjado
    { NBuf b = mk(7, 2, shearsId()); mobUseRecv(b, 2); } CHECK(!MOBS[2].sheared);                       // filhote nao
    { NBuf b = mk(8, 2, shearsId()); mobUseRecv(b, 2); } CHECK(!MOBS[3].sheared);                       // vaca nao
    const size_t d1 = DROPS.size(); { NBuf b = mk(5, 2, shearsId()); mobUseRecv(b, 2); } CHECK(DROPS.size() == d1);   // ja tosquiada: sem mais la
    { NBuf b = mk(5, 2, shearsId()); mobUseRecv(b, 9); } CHECK(true);   // remetente desconhecido: ignora sem quebrar
    remotes[2].gm = 3; MOBS[1].x = 2; { NBuf b = mk(6, 2, shearsId()); mobUseRecv(b, 2); } CHECK(!MOBS[1].sheared);   // espectador nao age
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
