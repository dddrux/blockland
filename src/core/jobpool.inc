// patchChunks.1: pool de threads para trabalho de chunks (malha/luz/geracao) fora do laco principal. So CPU: GL (upload/desenho) fica sempre na thread principal.
// Uso: jobPool().submit([=]{...}); jobPool().pending() (em fila + rodando); jobPool().waitAll(). Resultados voltam por jobDone* (fila protegida) e a thread principal consome.
// BL_TLS: estado de trabalho do mesher/luz (buffers rBlk/rSky/..., gMeshLod...) e POR THREAD; 1 thread so (main) continua identico ao antes.
#pragma once
#include <cstdlib>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <functional>
#include <atomic>
#include <vector>
#include <algorithm>
#define BL_TLS thread_local
struct JobPool {
    std::mutex mu; std::condition_variable cv, cvIdle; std::deque<std::function<void()>> q; std::vector<std::thread> th;
    int running = 0; bool stopping = false; std::atomic<int> nth{0};
    static int defaultThreads() {
        const unsigned hw = std::thread::hardware_concurrency();
#if defined(BL_RPI) || defined(BL_XP)
        const unsigned cap = 2;
#elif defined(__ANDROID__)
        const unsigned cap = 3;
#else
        const unsigned cap = 4;
#endif
        return (int)std::max(1u, std::min(cap, hw > 1 ? hw - 1 : 1u));   // deixa 1 nucleo para a thread principal/GL
    }
    void start(int n) {   // idempotente: so cria threads se ainda nao ha (n = 0 -> padrao)
        std::lock_guard<std::mutex> lk(mu); if (!th.empty()) return; if (n <= 0) { const char* e = getenv("BL_THREADS"); const int v = e ? atoi(e) : 0; n = (v >= 1 && v <= 8) ? v : defaultThreads(); } stopping = false;   // patchChunks.8: BL_THREADS=1..8 forca o numero de threads
        for (int i = 0; i < n; i++) th.emplace_back([this] { loop(); }); nth = n;
    }
    void loop() {
        for (;;) {
            std::function<void()> f;
            { std::unique_lock<std::mutex> lk(mu); cv.wait(lk, [this] { return stopping || !q.empty(); }); if (stopping && q.empty()) return; f = std::move(q.front()); q.pop_front(); running++; }
            f();
            { std::lock_guard<std::mutex> lk(mu); running--; if (q.empty() && running == 0) cvIdle.notify_all(); }
        }
    }
    bool ready() const { return nth.load() > 0; }
    void submit(std::function<void()> f) { { std::lock_guard<std::mutex> lk(mu); q.push_back(std::move(f)); } cv.notify_one(); }
    int pending() { std::lock_guard<std::mutex> lk(mu); return (int)q.size() + running; }
    void waitAll() { std::unique_lock<std::mutex> lk(mu); cvIdle.wait(lk, [this] { return q.empty() && running == 0; }); }
    void stop() { { std::lock_guard<std::mutex> lk(mu); stopping = true; } cv.notify_all(); for (auto& t : th) if (t.joinable()) t.join(); th.clear(); nth = 0; }
};
static JobPool& jobPool() { static JobPool* p = new JobPool(); return *p; }   // nunca destruido (threads podem ainda rodar no fim do processo)
