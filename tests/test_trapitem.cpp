// patchTrap.1: itens de alcapao (ferro/madeira) do criativo viram blocos colocaveis (precisa do pack, rodar da raiz)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
int main() {
    if (!gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip")) { printf("pack nao abriu\n"); return 1; }
    applyMcPack(); registerPackTab();
    int bad = 0, found = 0;
    for (int tab = 0; tab < (int)(sizeof gCreative / sizeof *gCreative); tab++) for (int id : gCreative[tab]) {
        std::string nm = isPk(id) ? PK[id - PK0].name : std::string(), fl = isPk(id) ? PK[id - PK0].file : std::string();
        if (nm.find("rapdoor") == std::string::npos && fl.find("trapdoor") == std::string::npos) continue;
        found++; const int xb = xbItemBlock(id); auto v = gRsVar.find(id);
        printf("tab%d id=%d name=%s file=%s xb=%d (XB_TRAP=%d) var=%d\n", tab, id, nm.c_str(), fl.c_str(), xb, XB_TRAP, v == gRsVar.end() ? -1 : v->second);
        if (nm.find("Iron") != std::string::npos || nm.find("Oak") != std::string::npos) if (xb != XB_TRAP) bad++;
    }
    for (int cz = -1; cz <= 1; cz++) for (int cx = -1; cx <= 1; cx++) if (!getC(cx, cz)) gen(cx, cz);
    const int AY = 200; for (int z = -4; z < 12; z++) for (int x = -4; x < 12; x++) for (int y = AY - 1; y < AY + 8; y++) setB(x, y, z, y == AY - 1 ? STONE : AIR);
    int px = 0;
    for (int tab = 1; tab <= 2; tab++) for (int id : gCreative[tab]) {   // coloca de verdade
        if (!isPk(id) || PK[id - PK0].file.find("trapdoor") == std::string::npos) continue;
        const int xb = xbItemBlock(id); const int hh[3] = {px, AY - 1, 2}, pp[3] = {px, AY, 2}; px += 2;
        const bool ok = xb && isRs(xb) && rsPlace(xb, rsItemVar(id), hh, pp, 0.f, px, AY, 5);
        printf("coloca id=%d xb=%d ok=%d bloco=%d (XB_TRAP=%d) meta=%d\n", id, xb, ok, getB(pp[0], pp[1], pp[2]), XB_TRAP, getM(pp[0], pp[1], pp[2]));
        if (!ok || getB(pp[0], pp[1], pp[2]) != XB_TRAP) bad++;
    }
    for (int i = 0; i < (int)PK.size(); i++) if (PK[i].file.find("trapdoor") != std::string::npos) {   // todos os itens do pack com "trapdoor"
        const int id = PK0 + i; int tb = -1; for (int t = 0; t < (int)(sizeof gCreative / sizeof *gCreative); t++) if (std::find(gCreative[t].begin(), gCreative[t].end(), id) != gCreative[t].end()) { tb = t; break; }
        printf("PK id=%d file=%s name=%s cube=%d xb=%d aba=%d\n", id, PK[i].file.c_str(), PK[i].name.c_str(), (int)PK[i].cube, xbItemBlock(id), tb);
    }
    for (int i = 0; i < (int)PK.size(); i++) if (PK[i].file == "block/iron_trapdoor" || PK[i].file == "block/trapdoor") if (xbItemBlock(PK0 + i) != XB_TRAP) { bad++; printf("  item solto %s nao colocavel\n", PK[i].file.c_str()); }
    printf("achados=%d ruins=%d\n", found, bad); return bad || !found ? 2 : 0;
}
