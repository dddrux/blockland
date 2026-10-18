// patchProp.2: colocar/colidir/remover/salvar props do Decocraft real (sem GL: so logica e malha)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const int AY = 150;
static int DP = 0, DOFF = 0, NI = 0;   // patchMod.9: o pack do Decocraft nao e mais o indice 0 (Cgm/Techguns entram antes)
#define P0 (PROP0 + DOFF)
int main() {
    if (chdir("..") != 0) return 99;
    if (!std::filesystem::exists("mods/decocraft/props.bin")) { printf("SKIP\n"); return 0; }
    propsRegisterTab(); for (size_t k = 0, off = 0; k < gPropPacks.size(); k++) { if (gPropPacks[k].items.size() == 3119) { DP = (int)k; DOFF = (int)off; NI = (int)gPropPacks[k].items.size(); } off += gPropPacks[k].items.size(); }
    CHECK(NI == 3119); CHECK(isProp(P0) && isProp(P0 + 3118) && !isProp(P0 + 3119 + 100000));
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    for (int z = -8; z < 24; z++) for (int x = -8; x < 24; x++) { setB(x, AY - 1, z, STONE); for (int y = AY; y < AY + 4; y++) setB(x, y, z, AIR); }
    // malha e icone de TODOS os itens: sem quads invalidos, icone dentro do slot 16x16
    int semMalha = 0, semIcone = 0; bool dentro = true, finito = true;
    for (int i = 0; i < NI; i++) { PropMesh* m = propMesh(P0 + i); if (!m || m->q.empty()) { semMalha++; continue; }
        propBuildIcon(*m); if (m->icon.empty()) semIcone++;
        for (auto& q : m->icon) for (int k = 0; k < 4; k++) { if (!std::isfinite(q.sx[k]) || !std::isfinite(q.sy[k])) finito = false; if (q.sx[k] < -.5f || q.sx[k] > 16.5f || q.sy[k] < -.5f || q.sy[k] > 16.5f) dentro = false; } }
    printf("sem malha %d, sem icone %d\n", semMalha, semIcone); CHECK(semMalha < 30); CHECK(semIcone < 30); CHECK(finito); CHECK(dentro);
    int comPagina = 0, total = 0; { PropMesh* m = propMesh(P0); for (auto& q : m->q) { total++; if (q.page >= 0) comPagina++; } } CHECK(total > 0 && comPagina == total);
    { int gw = 0, other = 0; const auto& its = gPropPacks[DP].items; gPropGlass.clear(); gPropGlassLoaded = false; for (size_t i = 0; i < its.size(); i++) { PropMesh* m = propMesh(P0 + (int)i); if (its[i].name.rfind("Glass Block Window", 0) == 0) { if (m->alpha > .59f && m->alpha < .61f) gw++; } else if (m->alpha < 1.f) other++; } CHECK(gw >= 20); CHECK(other > 0 && other < 700); }   // translucent.txt: so as janelas de vidro
    // colocar: cadeira/estante qualquer com colisao
    int id = P0; for (int i = 0; i < NI; i++) { PropEnt e = propMake(P0 + i, 4.5f, AY, 4.5f, 0); if (e.coll && e.box.y1 - e.box.y0 > 1.f) { id = P0 + i; break; } }
    CHECK(propPlace(id, 4, AY - 1, 4, 3, 0, 40, AY, 40)); CHECK(gProps.size() == 1);
    CHECK(!propPlace(id, 4, AY - 1, 4, 3, 0, 40, AY, 40));   // celula ocupada
    std::vector<AABB> l; collBoxes(AABB{4.4, AY + .1, 4.4, 4.6, AY + .5, 4.6}, l); CHECK(!l.empty());   // colide
    collBoxes(AABB{10, AY, 10, 10.5, AY + 1, 10.5}, l); CHECK(l.empty());
    CHECK(!propPlace(id, 7, AY - 1, 7, 3, 0, 7.5, AY, 7.5));   // jogador em cima
    // rotacao: yaw 90 gira a caixa
    PropEnt a = propMake(id, 0, 0, 0, 0), b = propMake(id, 0, 0, 0, 4);
    CHECK(fabs((a.box.x1 - a.box.x0) - (b.box.z1 - b.box.z0)) < 1e-3 && fabs((a.box.z1 - a.box.z0) - (b.box.x1 - b.box.x0)) < 1e-3);
    // mirar e remover
    const float eye[3] = {4.5f, AY + .3f, 1.5f}, dir[3] = {0, 0, 1}; gRay = RayHit();
    printf("box %.2f %.2f %.2f / %.2f %.2f %.2f\n", gProps[0].box.x0, gProps[0].box.y0, gProps[0].box.z0, gProps[0].box.x1, gProps[0].box.y1, gProps[0].box.z1);
    CHECK(propPick(eye, dir, 5.f) == 0);
    CHECK(propHit(eye, dir, 5.f, true)); CHECK(gProps.empty());
    // salvar/carregar
    std::filesystem::create_directories("saves/_tprops");
    CHECK(propPlace(id, 4, AY - 1, 4, 3, 0, 40, AY, 40)); CHECK(propPlace(P0 + 5, 9, AY - 1, 9, 3, 90, 40, AY, 40));
    propsWorldSave("_tprops"); gProps.clear(); propsWorldLoad("_tprops"); CHECK(gProps.size() == 2); CHECK(gProps[0].id == id && gProps[1].id == P0 + 5 && gProps[1].rot == 4);
    gProps.clear(); propsWorldSave("_tprops"); CHECK(!std::filesystem::exists("saves/_tprops/props.dat")); std::filesystem::remove_all("saves/_tprops");
    // patchProp.3: empilhar, colisao por parte, posicao livre
    gProps.clear();
    { int tb = -1; for (int i = 0; i < NI && tb < 0; i++) { PropEnt e = propMake(P0 + i, 4.5f, AY, 4.5f, 0); const float w = e.box.x1 - e.box.x0, d = e.box.z1 - e.box.z0, hgt = e.box.y1 - e.box.y0; if (e.coll && w > .8f && d > .8f && w < 2.f && d < 2.f && hgt > .5f && hgt < 1.2f) tb = i; }
      CHECK(tb >= 0); const int tid = P0 + tb; CHECK(propPlace(tid, 4, AY - 1, 4, 3, 0, 40, AY, 40)); const AABB tb0 = gProps[0].box;
      const float eye[3] = {4.5f, (float)(tb0.y1 + 2.0), 4.5f}, dn[3] = {0, -1, 0};   // olhando de cima para a mesa
      float tt = 0; int ax = -1, sg = 0; CHECK(propRay(eye, dn, 5.f, &tt, &ax, &sg) == 0 && ax == 1 && sg > 0);
      CHECK(fabs(eye[1] - tt - tb0.y1) < .02);   // atinge o TOPO de alguma parte
      gRay.hit = false;
      const int r1 = propPlaceOnProp(P0 + 5, eye, dn, 6.f, 67.f, 40, AY, 40); CHECK(r1 == 2 && gProps.size() == 2);
      CHECK(gProps[1].rot == gProps[0].rot);   // patchMod.2: empilhado herda o giro do de baixo, mesmo com o jogador olhando noutra direcao
      CHECK(gProps[1].box.y0 >= tb0.y0 + .1 && fabs(gProps[1].y - (eye[1] - tt)) < .6);   // ficou em cima (y acima da base da mesa)
      CHECK(propPlaceOnProp(P0 + 5, eye, dn, 6.f, 0, 40, AY, 40) != 2 || gProps.size() == 3);   // mesmo ponto: nao duplica / sobrepoe
      { const size_t nf = gProps.size(); const int rf = propPlaceOnProp(P0 + 5, eye, dn, 6.f, 90.f, 40, AY, 40, true);   // patchMod.3: agachado = giro livre
        if (rf == 2) CHECK(gProps.size() == nf + 1 && gProps.back().rot == propRotFor(P0 + 5, 90.f)); else printf("  (giro livre: nao coube, rf=%d)\n", rf); }
      const size_t n3 = gProps.size();
      const float eye2[3] = {40.5f, (float)AY + .5f, 40.5f}, d2[3] = {0, 0, -1}; CHECK(propPlaceOnProp(P0 + 5, eye2, d2, 6.f, 0, 0, AY, 0) == 0 && gProps.size() == n3);   // nao mira prop: 0
    }
    gProps.clear();
    { PropEnt e = propMake(id, 4.5f, AY, 4.5f, 3); double soma = 0; for (const AABB& c : e.cb) soma += (c.x1 - c.x0) * (c.y1 - c.y0) * (c.z1 - c.z0);
      CHECK(!e.cb.empty() && e.cb.size() <= 24); CHECK(soma <= (e.box.x1 - e.box.x0) * (e.box.y1 - e.box.y0) * (e.box.z1 - e.box.z0) * 1.001 + 1e-6);   // caixas dentro da caixa total
      for (const AABB& c : e.cb) CHECK(c.x0 >= e.box.x0 - 1e-4 && c.x1 <= e.box.x1 + 1e-4 && c.y0 >= e.box.y0 - 1e-4 && c.y1 <= e.box.y1 + 1e-4 && c.z0 >= e.box.z0 - 1e-4 && c.z1 <= e.box.z1 + 1e-4); }
    { int ocos = 0, n = 0; for (int i = 0; i < NI; i += 7) { PropEnt e = propMake(P0 + i, 4.5f, AY, 4.5f, 0); n++; if (e.coll) { double a = 0, bb = 0; for (const AABB& c : e.cb) { a += (c.x1 - c.x0) * (c.y1 - c.y0) * (c.z1 - c.z0); } bb = (e.box.x1 - e.box.x0) * (e.box.y1 - e.box.y0) * (e.box.z1 - e.box.z0); if (a < bb * .999) ocos++; } }
      printf("props com colisao mais justa que a caixa total: %d de %d amostrados\n", ocos, n); CHECK(ocos > 0); }
    { gProps.clear(); CHECK(propPlace(id, 4, AY - 1, 4, 3, 0, 40, AY, 40, 4.25, 4.75)); CHECK(fabs(gProps[0].x - .25 - 4.0f) < 1.f); CHECK(fabs((gProps[0].x * 8) - floor(gProps[0].x * 8 + .5)) < 1e-4); }   // posicao livre em 1/8
    // patchProp.4: vidro por quad + culling entre vidros iguais
    gProps.clear();
    { int gid = -1; for (int i = 0; i < NI && gid < 0; i++) { PropMesh* m = propMesh(P0 + i); if (!m || m->alpha >= 1.f) continue; int g = 0; for (auto& q : m->q) if (q.glass) g++; if (g && g < (int)m->q.size()) gid = P0 + i; }
      CHECK(gid >= 0); if (gid >= 0) { PropMesh* m = propMesh(gid); int g = 0, o = 0; for (auto& q : m->q) (q.glass ? g : o)++; CHECK(g > 0 && o > 0);   // vidraca translucida + moldura opaca
        gProps.push_back(propMake(gid, 4.5f, AY, 4.5f, 0)); gProps.push_back(propMake(gid, 5.5f, AY, 4.5f, 0)); gProps.push_back(propMake(gid, 9.5f, AY, 4.5f, 0)); propNbAll();
        CHECK(gProps[0].nb == (1 << 1) && gProps[1].nb == (1 << 0) && gProps[2].nb == 0);   // vizinhos em +X / -X; o distante nao
        gProps[1].rot = 4; propNbAll(); CHECK(gProps[0].nb == 0);   // rotacao diferente: nao culla
        gProps.clear(); gProps.push_back(propMake(gid, 4.5f, AY, 4.5f, 4)); gProps.push_back(propMake(gid, 4.5f, AY, 3.5f, 4)); propNbAll(); CHECK(gProps[0].nb == (1 << 4) && gProps[1].nb == (1 << 5));   // rot 90: +X local vira -Z mundial; vizinho em -Z
      } }
    CHECK(propWorldFace(1, 4) == 4 && propWorldFace(5, 4) == 1 && propWorldFace(0, 8) == 1 && propWorldFace(1, 3) == -1);
    gProps.clear();
    // patchProp.4: lampada e porta
    gProps.clear();
    { int lamp = -1, door = -1, openId = 0; const auto& its = gPropPacks[DP].items;
      for (int i = 0; i < (int)its.size(); i++) { if (lamp < 0 && its[i].beh == "SwitchableLamp") lamp = i; if (door < 0 && its[i].beh.rfind("Door:", 0) == 0 && propFromKey(gPropPacks[DP].mod + "/deco_" + its[i].beh.substr(5))) door = i; }
      CHECK(lamp >= 0 && door >= 0);
      if (lamp >= 0) { gProps.push_back(propMake(P0 + lamp, 4.5f, AY, 4.5f, 0)); const AABB b = gProps[0].box; const float eye[3] = {(float)((b.x0 + b.x1) / 2), (float)((b.y0 + b.y1) / 2), (float)b.z1 + 2}, d[3] = {0, 0, -1}; gRay.hit = false;
        CHECK(gProps[0].on == 1 && propUse(eye, d, 5.f) == 1 && gProps[0].on == 0 && propUse(eye, d, 5.f) == 1 && gProps[0].on == 1);
        gProps[0].on = 0; std::filesystem::create_directories("saves/_tprops"); propsWorldSave("_tprops"); gProps.clear(); propsWorldLoad("_tprops"); CHECK(gProps.size() == 1 && gProps[0].on == 0); std::filesystem::remove_all("saves/_tprops"); }
      gProps.clear();
      if (door >= 0) { openId = propFromKey(gPropPacks[DP].mod + "/deco_" + its[door].beh.substr(5)); gProps.push_back(propMake(P0 + door, 4.5f, AY, 4.5f, 4)); const AABB b = gProps[0].box; const float eye[3] = {(float)b.x1 + 2, (float)((b.y0 + b.y1) / 2), (float)((b.z0 + b.z1) / 2)}, d[3] = {-1, 0, 0}; gRay.hit = false;
        CHECK(propUse(eye, d, 5.f) == 1 && gProps.size() == 1 && gProps[0].id == openId && gProps[0].rot == 4); }
    }
    gProps.clear();
    { int ch = -1, bd = -1; const auto& its = gPropPacks[DP].items;   // patchProp.5b: Chair senta, Bed define spawn e pula a noite
      for (int i = 0; i < (int)its.size(); i++) { if (ch < 0 && its[i].beh == "Chair") ch = i; if (bd < 0 && its[i].beh == "Bed") bd = i; }
      CHECK(ch >= 0 && bd >= 0);
      auto eyeOf = [&](const PropEnt& e, float* eye, float* d) { eye[0] = (float)((e.box.x0 + e.box.x1) / 2); eye[1] = (float)((e.box.y0 + e.box.y1) / 2); eye[2] = (float)e.box.z1 + 2; d[0] = 0; d[1] = 0; d[2] = -1; gRay.hit = false; };
      if (ch >= 0) { gProps.push_back(propMake(P0 + ch, 4.5f, AY, 4.5f, 0)); float eye[3], d[3]; eyeOf(gProps[0], eye, d); gSeated = false;
        CHECK(propUse(eye, d, 5.f) == 1 && gSeated);
        Phys P; P.x = 20; P.y = AY + 5; P.z = 20; P.my = -.5; CHECK(propSeatTick(P, false));
        double o[3]; propSeatFor(gProps[0], o); CHECK(std::fabs(P.x - o[0]) < 1e-9 && std::fabs(P.z - o[2]) < 1e-9 && std::fabs(P.y - o[1]) < 1e-9 && P.my == 0 && P.y + 1.62 > gProps[0].y + .8 && P.y < gProps[0].box.y1);   // patchProp.6: pes abaixo do tampo (olho = tampo + ~1,0)
        CHECK(propSeatTick(P, false) && gSeated);                       // continua sentado
        CHECK(!propSeatTick(P, true) && !gSeated); { std::vector<AABB> lb; collBoxes(AABB{P.x - .3, P.y, P.z - .3, P.x + .3, P.y + 1.8, P.z + .3}, lb); CHECK(lb.empty()); }   // pular/agachar levanta ao lado/na frente, sem ficar preso na cadeira
        gSeated = true; gSeatPx = gProps[0].x; gSeatPy = gProps[0].y; gSeatPz = gProps[0].z; gProps.clear(); CHECK(!propSeatTick(P, false) && !gSeated); }   // prop removido: levanta
      if (bd >= 0) { gProps.clear(); gProps.push_back(propMake(P0 + bd, 6.5f, AY, 6.5f, 0)); float eye[3], d[3]; eyeOf(gProps[0], eye, d);
        const long long wt = gWorldTime; gWorldTime = 14000;
        CHECK(propUse(eye, d, 5.f) == 1 && gSleepT == 1 && std::fabs(gSpX - gProps[0].x) < 1e-9 && gSpY > AY);   // patchProp.11b: de noite deita (dormir pula a noite apos 100 ticks)
        { Phys sp; sp.x = gProps[0].x; sp.y = AY; sp.z = gProps[0].z; for (int k = 0; k < 105; k++) propSleepTick(sp, false); CHECK(gSleepT == 0 && gWorldTime == DAY_TICKS); }
        gSleepT = 0; gWorldTime = 3000; propUse(eye, d, 5.f); CHECK(gWorldTime == 3000 && gSleepT == 0);   // de dia: so define o spawn
        gWorldTime = wt; }
    }
    { int wp = -1, wn = -1; for (int i = 0; i < NI && (wp < 0 || wn < 0); i++) { const int sd = propWallSide(P0 + i); if (sd > 0 && wp < 0) wp = i; if (sd < 0 && wn < 0) wn = i; }   // patchProp.5b3: props de parede encostam no plano do bloco mirado
      CHECK(wp >= 0 && wn >= 0);
      setB(12, AY, 12, STONE);
      for (int k = 0; k < 2; k++) { const int idx = k ? wn : wp; if (idx < 0) continue; const int id = P0 + idx;
        for (int face : {0, 1, 4, 5}) for (float yaw : {0.f, 90.f, 200.f}) {
            gProps.clear(); const bool ok = propPlace(id, 12, AY, 12, face, yaw, 40, AY, 40); CHECK(ok && gProps.size() == 1); if (!ok) continue;
            const PropEnt& e = gProps[0]; const double plane = (face == 1 || face == 5) ? 13 : 12;
            const double edge = face == 1 ? e.box.x0 : face == 0 ? e.box.x1 : face == 5 ? e.box.z0 : e.box.z1;
            CHECK(std::fabs(edge - plane) < .02 && e.rot % 4 == 0);
        } }
      gProps.clear(); setB(12, AY, 12, AIR); }
    gProps.clear();
    { int ct = -1; const auto& its = gPropPacks[DP].items; for (int i = 0; i < (int)its.size() && ct < 0; i++) if (its[i].beh == "Container") ct = i;   // patchProp.5c: Container guarda itens (tela do bau), salva e solta ao remover
      CHECK(ct >= 0);
      if (ct >= 0) { gProps.clear(); gChests.clear(); gProps.push_back(propMake(P0 + ct, 4.5f, AY, 4.5f, 0)); const AABB b = gProps[0].box;
        const float eye[3] = {(float)((b.x0 + b.x1) / 2), (float)((b.y0 + b.y1) / 2), (float)b.z1 + 2}, d[3] = {0, 0, -1}; gRay.hit = false; gPropOpenChest = false;
        CHECK(propUse(eye, d, 5.f) == 1 && gPropOpenChest && gChest && gChCnt == 1 && gChRows == 3);
        Slot* sl = chestSlotRef(5); CHECK(sl != nullptr); if (sl) { sl->id = STONE; sl->n = 7; }
        CHECK(!chestFrame(4.5, AY, 4.5)); CHECK(chestFrame(40, AY, 40));   // longe: fecha
        chestClosed(); CHECK(!gChest && gChests.size() == 1);
        gPropOpenChest = false; CHECK(propUse(eye, d, 5.f) == 1 && chestSlotRef(5) && chestSlotRef(5)->id == STONE && chestSlotRef(5)->n == 7); chestClosed();
        std::filesystem::create_directories("saves/_tprops"); chestsSave("_tprops"); gChests.clear(); chestsLoad("_tprops"); std::filesystem::remove_all("saves/_tprops");
        gPropOpenChest = false; CHECK(propUse(eye, d, 5.f) == 1 && chestSlotRef(5) && chestSlotRef(5)->n == 7); chestClosed();   // sobreviveu ao save/load
        const size_t drops = DROPS.size(); CHECK(propHit(eye, d, 5.f, false) && gProps.empty() && gChests.empty() && DROPS.size() > drops); }
      gProps.clear(); gChests.clear(); }
    printf("test_propsys: %d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
