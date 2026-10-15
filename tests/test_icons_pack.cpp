// patchV.4: modelos/icones com o pack 1.12 (precisa do pack)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
int main() {
    if (!gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip")) { printf("pack nao abriu\n"); return 1; }
    applyMcPack(); registerPackTab(); xbInit();
    for (int dm = 0; dm < 3; dm++) { std::string o = "ANV" + std::to_string(dm) + ":"; for (const XQuad& q : gAnvQ[dm][0]) o += " " + std::to_string(q.tile); printf("%s\n", o.c_str()); }
    for (int k = 0; k < 4; k++) printf("gAnvQ0[%d]=%d\n", k, (int)gAnvQ[0][k].size());
    static const char* PL[][2] = {{"block/flower_rose", "3"}, {"block/flower_dandelion", "2"}, {"block/double_plant_sunflower_top", "19"}, {"block/sapling_oak", "-1"}};
    int bad = 0; for (auto& e : PL) { auto it = gPkByFile.find(e[0]); const int xb = it == gPkByFile.end() ? 0 : xbItemBlock(it->second); std::string o = std::string("PLANT ") + e[0] + " xb=" + std::to_string(xb); printf("%s\n", o.c_str()); if (!xb) bad++; }
    printf("ruins plantas: %d\n", bad);
    for (const char* f : {"block/pumpkin_face_off", "block/pumpkin_face_on", "block/furnace_top"}) { auto it = gPkByFile.find(f); if (it == gPkByFile.end()) { printf("FALHOU: sem %s\n", f); continue; } const PkItem& e = PK[it->second - PK0]; std::string o = std::string("FACE ") + f + " cube=" + std::to_string(e.cube) + " top=" + std::to_string(e.ft[0]) + " side=" + std::to_string(e.ft[1]) + " fr=" + std::to_string(e.fr) + " tile=" + std::to_string(e.tile); printf("%s\n", o.c_str()); if (!e.cube || e.fr == e.ft[0] || e.fr <= 0) printf("FALHOU: frente %s\n", f); }
    printf("XM anvil=%d\n", (int)gXM[XB_ANVIL - XB0].size()); return 0;
}
