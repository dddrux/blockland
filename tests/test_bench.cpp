// HZ-008: estatisticas do --benchmark (percentis, JSON, aquecimento, arquivo). Roda o main.cpp REAL.
#include <unistd.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    std::vector<double> v; for (int i = 1; i <= 100; i++) v.push_back(i);   // 1..100 ms
    CHECK(benchPct({}, .5) == 0 && benchPct(v, 0) == 1 && benchPct(v, 1) == 100 && fabs(benchPct(v, .5) - 51) <= 1);
    const std::string j = benchJson(v);
    CHECK(j.find("\"frames\":100") != std::string::npos && j.find("\"fps_avg\":") != std::string::npos && j.back() == '\n');
    CHECK(benchJson({}).find("\"frames\":0") != std::string::npos);   // sem quadros: nao divide por zero
    gBenchOn = true; gBenchFrames = 5; gBenchOut = "/tmp/bench_t.json"; gBench = BenchStats(); unlink("/tmp/bench_t.json");
    bool done = false; int n = 0; double t = 1000;
    while (!done && n < 100) { t += 16.0; done = benchTick(t); n++; }
    CHECK(done && n == 1 + 30 + 5);   // 1o quadro so marca o tempo, 30 de aquecimento, 5 medidos
    FILE* f = fopen("/tmp/bench_t.json", "rb"); CHECK(f != nullptr); char b[600] = {0}; if (f) { fread(b, 1, sizeof b - 1, f); fclose(f); }
    CHECK(strstr(b, "\"frames\":5") && strstr(b, "\"fps_avg\":62.50"));
    CHECK(!gBenchOn && !benchTick(t + 16));   // terminado: nao dispara de novo
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
