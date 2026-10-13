// Flecha cravada: recolher (survival ALLOWED / criativo CREATIVE_ONLY / esqueleto nao / inventario cheio / 1 min).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static int arrows() { int n = 0; for (const Slot& s : inv) if (s.id == IT_ARROW) n += s.n; return n; }
static ArrowEnt& mk(int owner, bool pick, bool cre) {
    ARROWS.clear(); arrowSpawn(.5, 70.5, .5, 0, 0, 0, false, owner, pick); ArrowEnt& a = ARROWS.back();
    a.cre = cre; a.stuck = true; a.shake = 0; a.age = 5; a.bx = 0; a.by = 70; a.bz = 0; return a;
}
int main() {
    if (chdir("..") != 0) return 99;
    setB(0, 70, 0, STONE); Phys P; P.x = .5; P.y = 69.5; P.z = .5; gDead = false; gGm = 0;
    for (Slot& s : inv) s = Slot();
    creative = false; mk(-1, true, false); CHECK(!arrowStep(ARROWS[0], P) && arrows() == 1);      // survival recolhe
    ARROWS.clear(); mk(-2, false, false); CHECK(arrowStep(ARROWS[0], P) && arrows() == 1);       // esqueleto: nao
    mk(-1, true, true); CHECK(arrowStep(ARROWS[0], P) && arrows() == 1);                         // CREATIVE_ONLY, jogador survival: nao
    mk(-1, true, false); ARROWS[0].shake = 3; CHECK(arrowStep(ARROWS[0], P) && arrows() == 1);   // ainda tremendo
    mk(-1, true, false); ARROWS[0].age = 1200; CHECK(!arrowStep(ARROWS[0], P) && arrows() == 1); // some em 1 min
    creative = true; mk(-1, true, true); CHECK(!arrowStep(ARROWS[0], P) && arrows() == 1);       // criativo recolhe sem item
    creative = false; for (Slot& s : inv) { s.id = STONE; s.n = 64; }
    mk(-1, true, false); CHECK(arrowStep(ARROWS[0], P));                                         // cheio: fica
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
