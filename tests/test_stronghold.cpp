// patchStronghold.1: posicoes em aneis, layout (pecas sem sobreposicao, 1 sala do portal), construcao da sala do portal no mundo.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) { printf("SKIP\n"); return 0; }
    gCurDir = "sh_test"; WORLD_SEED = 12345; gEndFrameId = 1100; gSpawnerId = 1101; netherInit();
    shInit(); CHECK(gShPos.size() == 128);
    for (int i = 0; i < 3; i++) { const double d = hypot(gShPos[i].first, gShPos[i].second); CHECK(d > 80 && d < 180); }
    for (int i = 3; i < 9; i++) { const double d = hypot(gShPos[i].first, gShPos[i].second); CHECK(d > 250 && d < 400); }
    int okAll = 0, n = 0;
    for (int i = 0; i < 12; i++) {
        SHS* S = shGet(i); n++; if (!S->ok) continue; okAll++;
        int portals = 0, ov = 0, lib = 0, ps = 0; for (size_t a = 0; a < S->ps.size(); a++) { const SHP& p = S->ps[a]; portals += p.t == SH_PORTAL; lib += p.t == SH_LIB; ps += p.t == SH_PRISON; CHECK(p.depth <= 51);
            for (size_t b = a + 1; b < S->ps.size(); b++) ov += p.t != SH_CORR && S->ps[b].t != SH_CORR && shInter(p.bb, S->ps[b].bb); }
        CHECK(portals == 1); CHECK(ov == 0); CHECK(lib <= 2); CHECK(ps <= 5); CHECK(S->ps[0].t == SH_START); CHECK(S->ps.size() > 8);
        if (i == 0) printf("  stronghold 0: chunk (%d,%d), %zu pecas, y %d\n", S->cx, S->cz, S->ps.size(), S->ps[0].bb[1]);
    }
    CHECK(okAll == n);
    SHS* S = shGet(0); const SHP* pr = nullptr; for (auto& p : S->ps) if (p.t == SH_PORTAL) pr = &p; CHECK(pr != nullptr);
    if (pr) {
        for (int cz = fdiv(pr->bb[2], 16); cz <= fdiv(pr->bb[5], 16); cz++) for (int cx = fdiv(pr->bb[0], 16); cx <= fdiv(pr->bb[3], 16); cx++) gen(cx, cz);
        int fr = 0, lava = 0, sb = 0, sp = 0;
        for (int y = pr->bb[1]; y <= pr->bb[4]; y++) for (int z = pr->bb[2]; z <= pr->bb[5]; z++) for (int x = pr->bb[0]; x <= pr->bb[3]; x++) { const int b = getB(x, y, z); fr += b == gEndFrameId; lava += isLava(b); sb += b == STONEBRICK || b >= PK0; sp += b == gSpawnerId; }
        printf("  sala do portal: molduras=%d lava=%d tijolos=%d spawner=%d\n", fr, lava, sb, sp);
        CHECK(fr == 12); CHECK(lava == 15); CHECK(sb > 100); CHECK(sp == 1);
    }
    printf("%d checks, %d falhas\n", checks, fails); return fails != 0;
}
