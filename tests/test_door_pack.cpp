// patchV.3: portas com o pack 1.12 (precisa do pack)
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
int main() {
    if (!gPack.open("assets/resourcepacks/Default-Java-1.12.2.zip")) { printf("SKIP: pack Default-Java-1.12.2.zip ausente (assets/resourcepacks)\n"); return 0; }
    applyMcPack(); registerPackTab(); xbInit();
    int bad = 0;
    for (int ty = 0; ty <= 6; ty++) for (int up = 0; up < 2; up++) for (int op = 0; op < 2; op++) for (int hi = 0; hi < 2; hi++) {
        const int m = (ty << 5) | (hi << 4) | (up << 3) | (op << 2) | 1;
        const std::vector<XQuad>& q = rsQuads(XB_DOOR, m);
        int n = (int)q.size(); int t = n ? q[0].tile : -9;
        printf("ty%d up%d op%d hi%d quads=%d tile=%d\n", ty, up, op, hi, n, t); if (!n || t < 0) bad++;
    }
    for (int op = 0; op < 2; op++) for (int hi = 0; hi < 2; hi++) for (int f = 0; f < 4; f++) {
        const std::vector<XQuad>& q = rsQuads(XB_DOOR, (1 << 5) | (hi << 4) | (op << 2) | f); float mn[3] = {9, 9, 9}, mx[3] = {-9, -9, -9};
        for (const XQuad& a : q) for (int v = 0; v < 4; v++) for (int i = 0; i < 3; i++) { mn[i] = std::min(mn[i], a.p[v][i]); mx[i] = std::max(mx[i], a.p[v][i]); }
        printf("BB op%d hi%d f%d x%.2f-%.2f z%.2f-%.2f\n", op, hi, f, mn[0], mx[0], mn[2], mx[2]);
        if (mx[1] - mn[1] < .95f || (mx[0] - mn[0] > .2f && mx[2] - mn[2] > .2f)) bad++;
    }
    printf("ruins: %d\n", bad); return bad ? 2 : 0;
}
