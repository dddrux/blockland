// Regressao do flickering do mato/flores: o modelo block/cross nao pode gerar quads coplanares duplicados (z-fighting).
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static const char* CROSS = R"({"textures":{"cross":"block/stone"},"elements":[
{"from":[0.8,0,8],"to":[15.2,16,8],"rotation":{"origin":[8,8,8],"axis":"y","angle":45,"rescale":true},"shade":false,"faces":{"north":{"uv":[0,0,16,16],"texture":"#cross"},"south":{"uv":[0,0,16,16],"texture":"#cross"}}},
{"from":[8,0,0.8],"to":[8,16,15.2],"rotation":{"origin":[8,8,8],"axis":"y","angle":45,"rescale":true},"shade":false,"faces":{"west":{"uv":[0,0,16,16],"texture":"#cross"},"east":{"uv":[0,0,16,16],"texture":"#cross"}}}]})";
int main() {
    printf("[cross sem quads coplanares duplicados]\n");
    gXbTex["block/stone"] = 5; XModel m; xbParse(CROSS, m, 0);
    std::vector<XQuad> q; xbBake(m, 0, 0, false, q);
    printf("  quads=%d\n",(int)q.size()); CHECK(q.size() == 2);
    for (size_t i = 0; i < q.size(); i++) for (size_t j = i + 1; j < q.size(); j++) {
        int shared = 0; for (int a = 0; a < 4; a++) for (int b = 0; b < 4; b++) if (fabsf(q[i].p[a][0]-q[j].p[b][0])<1e-4f && fabsf(q[i].p[a][1]-q[j].p[b][1])<1e-4f && fabsf(q[i].p[a][2]-q[j].p[b][2])<1e-4f) shared++;
        CHECK(shared < 4);
    }
    printf("\n%d verificacoes, %d falhas\n", checks, fails); return fails != 0;
}
