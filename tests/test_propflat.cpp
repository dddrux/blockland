// patchProp.7: itens planos (Techguns/cgm) = sprite 2D no slot + pose de item; props 3D = pose de bloco; luz do dia na mao
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks() { static Uint32 t = 0; return t += 7; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    if (chdir("..") != 0) return 99;
    if (!std::filesystem::exists("mods/techguns/props.bin")) { printf("SKIP\n"); return 0; }
    propsRegisterTab();
    int flat = 0, solid = 0, flatIconOk = 0, flatPose = 0, solidPose = 0;
    for (int i = 0; i < gPropTotal; i++) { const int id = PROP0 + i; PropMesh* m = propMesh(id); if (!m || m->q.empty()) continue;
        propBuildIcon(*m);
        if (m->flat) { flat++; bool ok = !m->icon.empty() && m->icon.size() <= 2; for (auto& q : m->icon) for (int k = 0; k < 4; k++) if (q.sx[k] < -.01f || q.sx[k] > 16.01f || q.sy[k] < -.01f || q.sy[k] > 16.01f) ok = false;
            if (ok) flatIconOk++; if (!isBlockLike(id)) flatPose++; }
        else { solid++; if (isBlockLike(id)) solidPose++; } }
    printf("planos %d (icone ok %d, pose item %d), 3D %d (pose bloco %d)\n", flat, flatIconOk, flatPose, solid, solidPose);
    CHECK(flat >= 250); CHECK(flatIconOk == flat); CHECK(flatPose == flat); CHECK(solidPose == solid);
    // icone plano: sprite de frente (todos os y distintos entre topo e base, sem a inclinacao isometrica)
    for (int i = 0; i < gPropTotal; i++) { PropMesh* m = propMesh(PROP0 + i); if (m && m->flat) { const auto& q = m->icon[0]; CHECK(std::fabs(q.sx[0] - q.sx[3]) < .01f || std::fabs(q.sx[0] - q.sx[1]) < .01f); break; } }
    printf("%d checagens, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
