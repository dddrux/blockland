// ================= perf: profiler de lag (escreve em logs/latest.log) =================
// Mede quanto cada etapa custa por quadro. A cada 5 s escreve "[PERF]" (media, pior quadro, gargalo) e,
// quando um quadro passa de 45 ms, escreve "[LAG]" dizendo QUEM causou a travada.
#include <chrono>
// Custo do proprio profiler: ~2 leituras de relogio por etapa (desprezivel).
enum PerfSec { PS_STREAM, PS_GEN, PS_LIGHT, PS_MESH, PS_UPLOAD, PS_DRAW_SOLID, PS_DRAW_WATER, PS_SIM, PS_SWAP, PS_N };
static const char* const PS_NAME[PS_N] = {"stream(total)", "geracao", "luz", "malha", "upload GPU", "desenho chunks", "desenho agua", "fluidos/folhas", "espera da GPU (swap)"};
static double gPf[PS_N];                 // ms no quadro atual
static double gPfSum[PS_N], gPfMax[PS_N];   // janela de 5 s
static int gPfGenN = 0, gPfMeshN = 0, gPfDrawn = 0, gPfCalls = 0; static long long gPfVerts = 0;   // contadores do quadro
static int gAdaptCut = 0, gAdaptUp = 0;   // celular: chunks a menos na distancia de render, ajustados sozinhos para manter ~60 fps
static double gPfGenWorst = 0, gBuildLight = 0, gPfFrameMs = 16.0, gPfOtherMs = 8.0, gStreamBudgetMs = 3.0, gStreamBudgetSum = 0;
static int gLastDrawn = 0, gLastCalls = 0, gGpuChunks = 0; static long long gLastVerts = 0; static double gGpuMB = 0; static bool gCullOn = false;   // gPfOtherMs = quadro SEM o stream (para o orcamento adaptativo)
static inline double perfNow() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct PerfScope { int s; double t; PerfScope(int s_) : s(s_), t(perfNow()) {} ~PerfScope() { gPf[s] += perfNow() - t; } };
struct PerfDrawScope {   // desenho: desconta o upload feito dentro (ele ja conta em PS_UPLOAD)
    int s; double t, u0; PerfDrawScope(int s_) : s(s_), t(perfNow()), u0(gPf[PS_UPLOAD]) {}
    ~PerfDrawScope() { gPf[s] += (perfNow() - t) - (gPf[PS_UPLOAD] - u0); } };
struct PerfBuildScope {   // malha = tempo do build() menos a luz (a luz tem linha propria)
    double t; PerfBuildScope() : t(perfNow()) { gBuildLight = 0; }
    ~PerfBuildScope() { gPf[PS_MESH] += (perfNow() - t) - gBuildLight; gPf[PS_LIGHT] += gBuildLight; gPfMeshN++; } };

static void perfFrameEnd(size_t loadedChunks) {
    static double last = perfNow(), winT0 = last, lastLag = 0, avgPrev = 16.0; static int frames = 0, lagsInWin = 0; static double worst = 0, sumFrame = 0;
    const double now = perfNow(), fm = now - last; last = now;
    if (fm > 1000.0) { for (double& v : gPf) v = 0; gPfGenN = gPfMeshN = gPfDrawn = gPfCalls = 0; gPfVerts = 0; return; }   // pausa/menu/loading: ignora
    gPfFrameMs = fm; gPfOtherMs = std::max(1.0, fm - gPf[PS_STREAM] - gPf[PS_SWAP]);   // espera da GPU (swap) NAO e trabalho da CPU: e tempo livre para carregar chunks
    gStreamBudgetSum += gStreamBudgetMs;
    gLastDrawn = gPfDrawn; gLastCalls = gPfCalls; gLastVerts = gPfVerts;
    frames++; sumFrame += fm; worst = std::max(worst, fm);
    for (int i = 0; i < PS_N; i++) { gPfSum[i] += gPf[i]; gPfMax[i] = std::max(gPfMax[i], gPf[i]); }
    if (fm > std::max(45.0, avgPrev * 2.2) && now - lastLag > 250.0 && lagsInWin < 4) {   // travada: diz o culpado
        lastLag = now; lagsInWin++;
        int top = PS_GEN; double acc = 0; for (int i = 0; i < PS_N; i++) { if (i == PS_STREAM) continue; if (gPf[i] > gPf[top]) top = i; acc += gPf[i]; }
        const double scan = std::max(0.0, gPf[PS_STREAM] - gPf[PS_GEN] - gPf[PS_LIGHT] - gPf[PS_MESH]), other = std::max(0.0, fm - acc - scan);
        logWarn("[LAG] quadro de %.0f ms | maior: %s (%.0f ms) | geracao %.1f ms (%d chunks, pior %.1f) | luz %.1f | malha %.1f (%d chunks) | varredura %.1f | upload %.1f | desenho %.1f+%.1f agua | fluidos %.1f | swap/GPU %.1f | resto (fisica, mobs, UI) %.1f",
                fm, PS_NAME[top], gPf[top], gPf[PS_GEN], gPfGenN, gPfGenWorst, gPf[PS_LIGHT], gPf[PS_MESH], gPfMeshN, scan, gPf[PS_UPLOAD], gPf[PS_DRAW_SOLID], gPf[PS_DRAW_WATER], gPf[PS_SIM], gPf[PS_SWAP], other);
    }
    for (double& v : gPf) v = 0; gPfGenN = gPfMeshN = 0; gPfGenWorst = 0; gPfDrawn = gPfCalls = 0; gPfVerts = 0;
    if (now - winT0 >= 5000.0 && frames > 0) {
        const double n = frames, scan = std::max(0.0, (gPfSum[PS_STREAM] - gPfSum[PS_GEN] - gPfSum[PS_LIGHT] - gPfSum[PS_MESH]) / n);
        int top = 0; double tv = -1; for (int i = 0; i < PS_N; i++) { if (i == PS_STREAM) continue; const double v = gPfSum[i] / n; if (v > tv) { tv = v; top = i; } }
        if (scan > tv) { tv = scan; top = -1; }
        logInfo("[PERF] %.0f FPS | quadro medio %.1f ms, pior %.0f ms | por quadro: geracao %.2f luz %.2f malha %.2f varredura %.2f upload %.2f desenho %.2f agua %.2f fluidos %.2f swap %.2f (ms)",
                n * 1000.0 / (now - winT0), sumFrame / n, worst, gPfSum[PS_GEN] / n, gPfSum[PS_LIGHT] / n, gPfSum[PS_MESH] / n, scan, gPfSum[PS_UPLOAD] / n, gPfSum[PS_DRAW_SOLID] / n, gPfSum[PS_DRAW_WATER] / n, gPfSum[PS_SIM] / n, gPfSum[PS_SWAP] / n);
        logInfo("[PERF] chunks carregados %zu (~%.0f MB de blocos) | ultimo quadro: %d chunks desenhados, %lld mil vertices, %d draw calls | gargalo medio: %s (%.2f ms/quadro)",
                loadedChunks, loadedChunks * 0.2, gLastDrawn, gLastVerts / 1000, gLastCalls, top < 0 ? "varredura de chunks" : PS_NAME[top], tv);
        logInfo("[PERF] malhas na GPU: %d chunks (~%.0f MB de VRAM) | culling de visao: %s | orcamento de carga medio %.1f ms/quadro", gGpuChunks, gGpuMB, gCullOn ? "LIGADO" : "desligado", gStreamBudgetSum / n);
#ifdef __ANDROID__
        {   // distancia de render dinamica: GPU/desenho no limite e FPS baixo => -1 chunk (min. 3 no jogo); FPS folgado por 3 janelas (15 s) => +1. Carga de chunks (CPU) nao conta: reduzir a distancia nao resolve isso.
            const double fps = n * 1000.0 / (now - winT0); const bool gpuBound = top == PS_SWAP || top == PS_DRAW_SOLID || top == PS_DRAW_WATER || top == PS_UPLOAD;
            if (fps < 48.0 && gpuBound && gAdaptCut < 8) { gAdaptCut++; gAdaptUp = 0; logInfo("[PERF] %.0f FPS: distancia de render -1 chunk (ajuste automatico, total -%d)", fps, gAdaptCut); }
            else if (fps >= 58.0 && gAdaptCut > 0) { if (++gAdaptUp >= 3) { gAdaptCut--; gAdaptUp = 0; logInfo("[PERF] %.0f FPS estavel: distancia de render +1 chunk (total -%d)", fps, gAdaptCut); } }
            else gAdaptUp = 0;
        }
#endif
        gStreamBudgetSum = 0; avgPrev = sumFrame / n;
        if (top == PS_SWAP || top == PS_DRAW_SOLID) logInfo("[PERF] dica: o limite e a GPU/geometria. Reduza a distancia de renderizacao ou ative /vsync 1 para estabilizar.");
        else if (top == PS_GEN || top == PS_LIGHT || top == PS_MESH || top == -1) logInfo("[PERF] dica: gargalo e carregar chunks novos (CPU). Ocorre ao andar/girar rapido; o orcamento adaptativo ja usa a folga do quadro.");
        for (int i = 0; i < PS_N; i++) { gPfSum[i] = 0; gPfMax[i] = 0; }
        frames = 0; worst = 0; sumFrame = 0; winT0 = now; lagsInWin = 0;
    }
}
