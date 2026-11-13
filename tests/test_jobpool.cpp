// test_jobpool: o que este teste cobre (1 linha). Rodar: tools/t.sh jobpool
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return 0; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
#define SECTION(n) printf("[%s]\n", n)
int main() {
    SECTION("pool");
    JobPool& P = jobPool(); P.start(3); CHECK(P.ready());
    std::atomic<int> n{0}; std::mutex mu; std::vector<std::thread::id> ids;
    for (int i = 0; i < 200; i++) P.submit([&] { n++; std::lock_guard<std::mutex> lk(mu); ids.push_back(std::this_thread::get_id()); });
    P.waitAll(); CHECK(n == 200); CHECK(P.pending() == 0);
    for (auto& id : ids) CHECK(id != std::this_thread::get_id());
    SECTION("tls");   // estado do mesher e por thread: a thread do pool nao ve/altera o da principal
    rTop = 77; gMeshLod = 2; int seen = -1, lod = -1;
    P.submit([&] { seen = rTop; lod = gMeshLod; rTop = 5; gMeshLod = 9; }); P.waitAll();
    CHECK(seen == CH - 1); CHECK(lod == 0); CHECK(rTop == 77); CHECK(gMeshLod == 2);
    rTop = CH - 1; gMeshLod = 0;
    printf("%d checks, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
