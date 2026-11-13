// patchSkin3D.1: camadas externas da skin em voxels 3D (voxBox/voxSet): contagem de faces, orientacao, UV no centro do pixel, cache por textura.
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { return 1000; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    static uint8_t px[64 * 64 * 4];
    printf("[skin transparente = sem voxels]\n");
    { std::vector<V> o; memset(px, 0, sizeof px); voxBox(o, px, -4,4, 0,8, -4,4, 32,0, 8,8,8, .85f); CHECK(o.empty()); }
    printf("[1 pixel = 1 face externa + 4 paredes]\n");
    { std::vector<V> o; memset(px, 0, sizeof px);
      px[((8 + 3) * 64 + 8 + 3) * 4 + 3] = 255;   // frente do chapeu seria (40,8); aqui na face base da cabeca? usa o layout u=0,v=0
      voxBox(o, px, -4,4, 0,8, -4,4, 0,0, 8,8,8, 1.f);   // u=0: face frontal em x 8..16, y 8..16 -> pixel (11,11)
      CHECK(o.size() == 5 * 4);
      for (const V& v : o) CHECK(fabsf(v.u * 64.f - 11.5f) < 1e-4f && fabsf(v.v * 64.f - 11.5f) < 1e-4f);
      // face externa (primeiros 4 vertices): z = -4 - 1 = -5 (frente = -Z); e1 x e2 aponta para dentro (+Z), convencao de boxM
      for (int k = 0; k < 4; k++) CHECK(fabsf(o[k].z + 5.f) < 1e-4f);
      float e1[3] = {o[1].x-o[0].x, o[1].y-o[0].y, o[1].z-o[0].z}, e2[3] = {o[3].x-o[0].x, o[3].y-o[0].y, o[3].z-o[0].z};
      CHECK(e1[0]*e2[1] - e1[1]*e2[0] > 0.f);   // componente z do produto vetorial > 0 (para dentro)
    }
    printf("[hexaedro cheio 8x8x8: 6*(64+32) quads]\n");
    { std::vector<V> o; for (int j = 0; j < 16; j++) for (int i = 0; i < 64; i++) px[(j * 64 + i) * 4 + 3] = 255; voxBox(o, px, -4,4, 0,8, -4,4, 0,0, 8,8,8, .85f); CHECK(o.size() == (size_t)6 * 96 * 4);
      float mx = 0; for (const V& v : o) { mx = std::max(mx, fabsf(v.x)); CHECK(v.x > -5.f && v.x < 5.f && v.y > -1.f && v.y < 9.f && v.z > -5.f && v.z < 5.f); } CHECK(mx > 4.8f); }
    printf("[alfa <= .4 nao vira voxel (mesmo corte do alpha test)]\n");
    { std::vector<V> o; memset(px, 0, sizeof px); px[((11) * 64 + 11) * 4 + 3] = 100; voxBox(o, px, -4,4, 0,8, -4,4, 0,0, 8,8,8, 1.f); CHECK(o.empty()); }
    printf("[voxSet: cache por textura + invalidacao no upload]\n");
    { memset(skin, 0, sizeof skin); skinTex = 7; gTexPx[7].assign(skin, skin + sizeof skin); gVox.clear();
      gTexPx[7][((8 + 3) * 64 + 40 + 3) * 4 + 3] = 255;   // chapeu: frente em (40,8)
      gSkin3D = true; slim = false;
      const VoxSet& a = voxSet(); CHECK(a.m[0].size() == 20 && a.m[1].empty() && a.m[2].empty());
      CHECK(&voxSet() == &a);   // mesmo objeto (cache)
      CHECK(&ovHat() == &a.m[0]);
      slim = true; const VoxSet& b = voxSet(); CHECK(&b != &a);   // chave inclui bracos finos
      skinTex = 8; gTexPx[8].assign(64 * 64 * 4, 0); CHECK(voxSet().m[0].empty());   // outra textura, outra malha
      gSkin3D = false; CHECK(&ovHat() == &pHat);
    }
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
