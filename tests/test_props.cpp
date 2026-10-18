// patchProp.1: leitor props.bin (conversor/modconv). Usa o mod REAL convertido em mods/decocraft; sem ele => SKIP.
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    PropPack pk;
    if (!std::filesystem::exists("mods/decocraft/props.bin")) { printf("SKIP (sem mods/decocraft)\n"); return 0; }
    CHECK(propsLoad("mods/decocraft/props.bin", pk));
    CHECK(pk.models.size() == 1076); CHECK(pk.items.size() == 3119); CHECK(pk.pages.size() >= 1);
    size_t nb = 0; bool uvok = true, geo = true, texok = true;
    for (auto& m : pk.models) { nb += m.boxes.size(); for (auto& b : m.boxes) {
        for (int f = 0; f < 6; f++) if (b.f[f].slot >= 0) { const PropFace& pf = b.f[f]; if (pf.u0 < -1e-4f || pf.v0 < -1e-4f || pf.u1 > 1 + 1e-4f || pf.v1 > 1 + 1e-4f) uvok = false; }
        for (int j = 0; j < 10; j++) if (!std::isfinite(b.g[j])) geo = false;
        const float qn = b.g[6] * b.g[6] + b.g[7] * b.g[7] + b.g[8] * b.g[8] + b.g[9] * b.g[9]; if (fabsf(qn - 1) > 1e-2f) geo = false; } }
    CHECK(nb == 39018); CHECK(uvok); CHECK(geo);
    for (auto& t : pk.tex) if (!t.ref && (t.w <= 0 || t.h <= 0 || t.x + t.w > pk.pageW[t.page] || t.y + t.h > pk.pageH[t.page])) texok = false;
    CHECK(texok);
    // itens conhecidos (Decocraft 2.6.3.7): id 0 = plush do Creeper, 1 = garrafa de vinho (pt-BR com acento)
    CHECK(pk.items[0].id == "deco_0"); CHECK(pk.items[0].name == "Creeper de Pelúcia"); CHECK(pk.items[0].tab == "Brinquedos");
    CHECK(pk.items[1].id == "deco_1" || pk.items[2].id == "deco_1");
    bool lamp = false, chair = false, light = false; for (auto& it : pk.items) { if (it.beh == "Chair") chair = true; if (it.light == 15) light = true; if (it.beh == "SwitchableLamp") lamp = true; }
    CHECK(chair); CHECK(lamp); CHECK(light);
    // bbox do plush ~0.47 x 0.6 x 0.47 blocos (1 px = 1/16) e quad da face frontal (+Z) fica no plano z = cz + hz
    const PropModel& pm = pk.models[pk.items[0].model]; CHECK(pm.bb[4] > 0.5f && pm.bb[4] < 0.7f); CHECK(pm.bb[3] - pm.bb[0] < 0.6f);
    PropBox bx; for (int j = 0; j < 10; j++) bx.g[j] = 0; bx.g[3] = bx.g[4] = bx.g[5] = .5f; bx.g[6] = 1; bx.f[5].slot = 0; bx.f[5].u1 = bx.f[5].v1 = 1;
    float q[4][5]; propsFaceQuad(bx, 5, q);
    for (int k = 0; k < 4; k++) CHECK(fabsf(q[k][2] - .5f) < 1e-5f);
    CHECK(q[0][0] < 0 && q[0][1] > 0 && q[1][0] > 0 && q[2][1] < 0 && q[0][3] == 0 && q[1][3] == 1 && q[2][4] == 1);   // TL, TR, BR: textura em pe, sem espelho
    // arquivo truncado/corrompido e rejeitado sem travar
    std::vector<unsigned char> b; { FILE* f = fopen("mods/decocraft/props.bin", "rb"); unsigned char t[4096]; size_t n; while ((n = fread(t, 1, sizeof t, f)) > 0) b.insert(b.end(), t, t + n); fclose(f); }
    { PropPack x; std::vector<unsigned char> c(b.begin(), b.begin() + b.size() / 2); CHECK(!propsParse(c, x)); }
    { PropPack x; std::vector<unsigned char> c = b; c[0] = 'X'; CHECK(!propsParse(c, x)); }
    { PropPack x; std::vector<unsigned char> c = b; c.push_back(0); CHECK(!propsParse(c, x)); }
    printf("test_props: %d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
