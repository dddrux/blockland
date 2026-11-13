// patchLogic.1: lampada e bloco de notas (base: patch5.64: TNT (acender com fogo/isqueiro, pavio de 80 ticks, explosao forca 4, cadeia, dano) + explosao do creeper preservada. Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 120, T = 1300;
static void arena() { for (int z = -12; z < 20; z++) for (int x = -12; x < 20; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 12; y++) setB(x, y, z, AIR); } }
static void block(int r, int cx, int cy, int cz) { for (int y = cy - r; y <= cy + r; y++) for (int z = cz - r; z <= cz + r; z++) for (int x = cx - r; x <= cx + r; x++) if (y >= AY) setB(x, y, z, STONE); }
int main() {
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    const char* nm[7] = {"block/redstone_lamp", "block/redstone_lamp_on", "block/note_block", "block/redstone_block", "block/dropper", "block/dispenser", "block/hopper_outside"}; const int base = (int)PK.size();
    for (int i = 0; i < 7; i++) { PkItem a; a.file = nm[i]; PK.push_back(a); gPkByFile[nm[i]] = PK0 + base + i; }
    logicInit(); gRsRB = PK0 + base + 3; const int OFF = PK0 + base, ON = PK0 + base + 1, NOTE = PK0 + base + 2, RB = gRsRB; gSndOn = false; netOn = false;
    CHECK(gLampOff == OFF && gLampOn == ON && gNoteId == NOTE);
    arena(); setB(5, AY, 5, OFF); lampNeighbor(5, AY, 5, 0, 0, 0); CHECK(getB(5, AY, 5) == OFF);          // sem energia: apagada
    setB(6, AY, 5, RB); lampNeighbor(5, AY, 5, 0, 0, 0); CHECK(getB(5, AY, 5) == ON);                      // bloco de redstone ao lado: acende
    setB(6, AY, 5, AIR); lampNeighbor(5, AY, 5, 0, 0, 0); CHECK(getB(5, AY, 5) == OFF);                    // tirou: apaga
    // lampada acesa solta a apagada
    size_t d0 = DROPS.size(); setB(8, AY, 5, ON); blockDrops(8, AY, 5, ON); CHECK(DROPS.size() == d0 + 1 && DROPS.back().id == OFF);
    // bloco de notas: clique cicla 0..24 e volta
    setB(10, AY, 5, NOTE); for (int i = 0; i < 25; i++) CHECK(noteUse(10, AY, 5)); CHECK((getM(10, AY, 5) & 31) == 0);
    CHECK(!noteUse(5, AY, 5));
    // energia toca uma vez (bit 32) e solta ao desligar
    setB(11, AY, 5, RB); noteNeighbor(10, AY, 5, 0, 0, 0); CHECK(getM(10, AY, 5) & 32);
    setB(11, AY, 5, AIR); noteNeighbor(10, AY, 5, 0, 0, 0); CHECK(!(getM(10, AY, 5) & 32));
    // dropper/dispenser: energia solta 1 item na frente (meta&3: 1 = +X), esvazia aos poucos, quebrar solta o resto
    const int DRP = PK0 + base + 4, DSP = PK0 + base + 5; CHECK(gDropId == DRP && gDispId == DSP);
    setBM(14, AY, 5, DRP, 1); ChestInv* ci = chestAt(14, AY, 5, true); CHECK(ci != nullptr); ci->s[2].id = STONE; ci->s[2].n = 3;
    size_t d1 = DROPS.size(); setB(13, AY, 5, RB); dispNeighbor(14, AY, 5, 0, 0, 0);
    CHECK(DROPS.size() == d1 + 1 && DROPS.back().x > 14.9 && ci->s[2].n == 2 && (getM(14, AY, 5) & 8));
    dispNeighbor(14, AY, 5, 0, 0, 0); CHECK(DROPS.size() == d1 + 1);                       // ainda energizado: nao repete
    setB(13, AY, 5, AIR); dispNeighbor(14, AY, 5, 0, 0, 0); CHECK(!(getM(14, AY, 5) & 8)); setB(13, AY, 5, RB); dispNeighbor(14, AY, 5, 0, 0, 0); CHECK(DROPS.size() == d1 + 2 && ci->s[2].n == 1);
    setB(13, AY, 5, AIR); dispNeighbor(14, AY, 5, 0, 0, 0); setB(13, AY, 5, RB); dispNeighbor(14, AY, 5, 0, 0, 0); CHECK(ci->s[2].n == 0 && ci->s[2].id == 0);
    setB(13, AY, 5, AIR); dispNeighbor(14, AY, 5, 0, 0, 0); setB(13, AY, 5, RB); d1 = DROPS.size(); dispNeighbor(14, AY, 5, 0, 0, 0); CHECK(DROPS.size() == d1);   // vazio: nada cai
    ChestInv* c2 = chestAt(14, AY, 5, false); c2->s[0].id = COBBLE; c2->s[0].n = 5; d1 = DROPS.size(); setB(14, AY, 5, AIR); CHECK(DROPS.size() > d1 && !chestAt(14, AY, 5, false));   // quebrar solta o conteudo
    // dropper empurra para bau a frente
    setBM(20, AY, 5, DRP, 1); setBM(21, AY, 5, XB_CHEST, 0); ChestInv* d3 = chestAt(20, AY, 5, true); d3->s[0].id = DIRT; d3->s[0].n = 2; ChestInv* ch = chestAt(21, AY, 5, true); CHECK(ch != nullptr);
    setB(19, AY, 5, RB); d1 = DROPS.size(); dispNeighbor(20, AY, 5, 0, 0, 0); CHECK(ch->s[0].id == DIRT && ch->s[0].n == 1 && DROPS.size() == d1);
    // funil: puxa do bau de cima, empurra para o bau de baixo (1 item a cada 8 ticks), pega item solto, desliga com redstone, quebrar solta
    const int HOP = PK0 + base + 6; CHECK(gHopId == HOP);
    setBM(28, AY + 1, 5, XB_CHEST, 0); setBM(28, AY, 5, HOP, 0); setBM(28, AY - 1, 5, XB_CHEST, 0);
    ChestInv* up = chestAt(28, AY + 1, 5, true); up->s[0].id = STONE; up->s[0].n = 3; ChestInv* dn = chestAt(28, AY - 1, 5, true);
    hopperOne(28, AY, 5); CHECK(up->s[0].n == 2 && chestAt(28, AY, 5, false)->s[0].n == 1);                 // puxou 1 (nao empurrou: recem chegou? empurra no mesmo ciclo)
    for (int i = 0; i < 6; i++) hopperOne(28, AY, 5); int tot = 0; for (const Slot& q : dn->s) if (q.id == STONE) tot += q.n; CHECK(tot == 3 && up->empty());
     setBM(24, AY + 1, 5, XB_CHEST, 0); setBM(24, AY, 5, HOP, 0); setB(25, AY, 5, RB); ChestInv* u2 = chestAt(24, AY + 1, 5, true); u2->s[0].id = DIRT; u2->s[0].n = 2; hopperOne(24, AY, 5); CHECK(u2->s[0].n == 2);   // energizado: funil desligado
    // patchLogic.4: dispenser dispara flecha e acende TNT
    setBM(30, AY, 5, DSP, 1); ChestInv* ds = chestAt(30, AY, 5, true); ds->s[0].id = IT_ARROW; ds->s[0].n = 2; const size_t na = ARROWS.size(), d4 = DROPS.size();
    setB(29, AY, 5, RB); dispNeighbor(30, AY, 5, 0, 0, 0); CHECK(ARROWS.size() == na + 1 && ARROWS.back().vx > 0.5 && ds->s[0].n == 1 && DROPS.size() == d4);
    setB(29, AY, 5, AIR); dispNeighbor(30, AY, 5, 0, 0, 0); gTntId = COBBLE; ds->s[0].id = COBBLE; ds->s[0].n = 1; const size_t nt = TNTS.size(); setB(29, AY, 5, RB); dispNeighbor(30, AY, 5, 0, 0, 0); CHECK(TNTS.size() == nt + 1 && ds->empty());
    // patchLogic.5: dispenser com balde
    gBktEmpty = 9001; gBktWater = 9002; gBktLava = 9003; setB(29, AY, 5, AIR); setB(31, AY, 5, AIR);
    ds->s[0].id = gBktWater; ds->s[0].n = 1; setB(29, AY, 5, AIR); dispNeighbor(30, AY, 5, 0, 0, 0); setB(29, AY, 5, RB); const size_t d5 = DROPS.size(); dispNeighbor(30, AY, 5, 0, 0, 0);
    CHECK(getB(31, AY, 5) == WATER && ds->s[0].id == gBktEmpty && DROPS.size() == d5);
    setB(29, AY, 5, AIR); dispNeighbor(30, AY, 5, 0, 0, 0); setB(29, AY, 5, RB); dispNeighbor(30, AY, 5, 0, 0, 0);
    CHECK(getB(31, AY, 5) == AIR && ds->s[0].id == gBktWater);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
