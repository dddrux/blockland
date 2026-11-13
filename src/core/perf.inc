// ================= perf: profiler de lag (escreve em logs/latest.log) =================
// Mede quanto cada etapa custa por quadro. A cada 5 s escreve "[PERF]" (media, pior quadro, gargalo) e,
// quando um quadro passa de 45 ms, escreve "[LAG]" dizendo QUEM causou a travada.
#include <chrono>
// Custo do proprio profiler: ~2 leituras de relogio por etapa (desprezivel).
enum PerfSec { PS_STREAM, PS_GEN, PS_LIGHT, PS_MESH, PS_UPLOAD, PS_DRAW_SOLID, PS_DRAW_WATER, PS_SIM, PS_SWAP, PS_N };
static const char* const PS_NAME[PS_N] = {"stream(total)", "geracao", "luz", "malha", "upload GPU", "desenho chunks", "desenho agua", "fluidos/folhas", "espera da GPU (swap)"};
static double gR2[6]; static int gR2n = 0; static double gR2prev[6], gR2d[6];   // patchRpiOpt.4: [PERF2] ceu/entidades/hud/glFinish
static double gPf[PS_N];                 // ms no quadro atual
static double gPfSum[PS_N], gPfMax[PS_N];   // janela de 5 s
static int gPfGenN = 0, gPfMeshN = 0, gPfDrawn = 0, gPfCalls = 0; static long long gPfVerts = 0;   // contadores do quadro
static float gFarSkipD = 56.f;   // patchFps.4: distancia (blocos) alem da qual plantas/modelos nao sao desenhados; encolhe para 48 quando o ajuste automatico ja cortou >=2 chunks
static int gAdaptCut = 0, gAdaptUp = 0;   // celular: chunks a menos na distancia de render, ajustados sozinhos para manter ~60 fps
static BL_TLS double gBuildLight = 0; static double gPfGenWorst = 0, gPfFrameMs = 16.0, gPfOtherMs = 8.0, gStreamBudgetMs = 3.0, gStreamBudgetSum = 0;
static int gLastDrawn = 0, gLastCalls = 0, gGpuChunks = 0; static long long gLastVerts = 0; static double gGpuMB = 0; static bool gCullOn = false;   // gPfOtherMs = quadro SEM o stream (para o orcamento adaptativo)
static inline double perfNow() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
// patchSeam.3: sonda de checkpoints por quadro. Quadro > 40 ms: loga qual trecho (entre dois PROBE) demorou mais.
static bool gSoftPace = false; static int gSoftHz = 60;   // patchSeam.6: Wayland/PRIME: vsync do driver trava o swap 15-50 ms; usa swap 0 + limitador na taxa do monitor
static inline int gSoftHzNow(SDL_Window* w) {   // patchPerf.1: sem foco => 30 FPS (poupa a GPU das outras instancias)
#ifdef SDL_DROPBEGIN
    if (!(SDL_GetWindowFlags(w) & SDL_WINDOW_INPUT_FOCUS)) return std::min(gSoftHz, 30);
#endif
    return gSoftHz; }
static bool gSwapSkip = false; static Uint32 gSwapT = 0;   // patchPerf.2: pula SwapWindow quando o compositor estrangula a janela
static inline Uint32 gSwapGapMs(SDL_Window* w) {   // patchPerf.3: intervalo entre SwapWindow no modo gSwapSkip
#ifdef SDL_DROPBEGIN
    if (!(SDL_GetWindowFlags(w) & SDL_WINDOW_INPUT_FOCUS)) return 3000u;
#endif
    return 1000u; }
static bool gVsyncUser = false;   // patchPerf.1: /vsync manual desliga a deteccao automatica de swap estrangulado
static bool gPerfShDrop = false;   // patchPerf.7: janela de 5 s com FPS baixo e GPU no limite => loop/render.inc baixa a qualidade das sombras 1 nivel
static bool gVsyncOn = true;   // patchSeam.5: com vsync ligado a espera do swap e ritmo do monitor/compositor, nao carga da GPU
static const char* gPrN[16]; static double gPrT[16]; static int gProbeN = 0;
static const bool gProbeFin = getenv("BLOCKLAND_PROBE_GPU") != nullptr;   // patchPerf.5: BLOCKLAND_PROBE_GPU=1 => glFinish em cada sonda: o tempo por trecho passa a incluir a GPU (diagnostico; custa FPS)
#define PROBE(nm) do { if (gProbeN < 16) { if (gProbeFin) glFinish(); gPrN[gProbeN] = nm; gPrT[gProbeN++] = perfNow(); } } while (0)
static void probeReport() {
    if (gProbeN < 2) return; const double tot = gPrT[gProbeN - 1] - gPrT[0]; if (tot < 40.0) return;
    int b = 1; for (int i = 2; i < gProbeN; i++) if (gPrT[i] - gPrT[i - 1] > gPrT[b] - gPrT[b - 1]) b = i;
    logWarn("[SONDA2] quadro %.0f ms (dentro do loop) | trecho mais lento: %s -> %s = %.0f ms", tot, gPrN[b - 1], gPrN[b], gPrT[b] - gPrT[b - 1]);
}
struct PerfScope { int s; double t; PerfScope(int s_) : s(s_), t(perfNow()) {} ~PerfScope() { gPf[s] += perfNow() - t; } };
struct PerfDrawScope {   // desenho: desconta o upload feito dentro (ele ja conta em PS_UPLOAD)
    int s; double t, u0; PerfDrawScope(int s_) : s(s_), t(perfNow()), u0(gPf[PS_UPLOAD]) {}
    ~PerfDrawScope() { gPf[s] += (perfNow() - t) - (gPf[PS_UPLOAD] - u0); } };
static thread_local bool gPerfWorker = false; static void perfMarkWorker() { gPerfWorker = true; }   // patchChunks.3b: thread de malha nao mexe nos contadores (so main)
struct PerfBuildScope {   // malha = tempo do build() menos a luz (a luz tem linha propria)
    double t; PerfBuildScope() : t(perfNow()) { gBuildLight = 0; }
    ~PerfBuildScope() { if (gPerfWorker) return; gPf[PS_MESH] += (perfNow() - t) - gBuildLight; gPf[PS_LIGHT] += gBuildLight; gPfMeshN++; } };

static void perfFrameEnd(size_t loadedChunks) {
    logFlushIfDue();
    static double last = perfNow(), winT0 = last, lastLag = 0, avgPrev = 16.0; static int frames = 0, lagsInWin = 0; static double worst = 0, sumFrame = 0;
    const double now = perfNow(), fm = now - last; last = now;
    if (fm > 1000.0) {   // patchLog.1: antes ignorado em silencio (por isso as travadas de 1,5-2,6 s nao apareciam). Agora vai pro log com a causa conhecida
        static double lastBig = -1e9; if (now - lastBig > 3000.0) { lastBig = now; logWarn("[LAG] quadro de %.0f ms (>1 s) | etapas medidas somam so %.1f ms => o resto foi fora do desenho/stream (audio, I/O, salvar, driver) | causas registradas: %s", fm, gPf[PS_STREAM] + gPf[PS_SIM] + gPf[PS_DRAW_SOLID] + gPf[PS_DRAW_WATER] + gPf[PS_UPLOAD] + gPf[PS_SWAP], gPfNotes.empty() ? "(nenhuma)" : gPfNotes.c_str()); }
        gPfNotes.clear(); for (double& v : gPf) v = 0; gPfGenN = gPfMeshN = gPfDrawn = gPfCalls = 0; gPfVerts = 0; return; }   // pausa/menu/loading: ignora
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
    if (!gPfNotes.empty()) { if (fm > 45.0) logWarn("[LAG]   causas registradas no quadro: %s", gPfNotes.c_str()); gPfNotes.clear(); }
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
        if (true)   // patchFps.1: ajuste automatico da distancia em todas as plataformas (no PC, limite -4 chunks)
        {   // distancia de render dinamica: GPU/desenho no limite e FPS baixo => -1 chunk (min. 3 no jogo); FPS folgado por 3 janelas (15 s) => +1. Carga de chunks (CPU) nao conta: reduzir a distancia nao resolve isso.
            const double fps = n * 1000.0 / (now - winT0); const double gpuMs = ((gVsyncOn ? 0.0 : gPfSum[PS_SWAP]) + gPfSum[PS_DRAW_SOLID] + gPfSum[PS_DRAW_WATER] + gPfSum[PS_UPLOAD]) / n; const bool swThr = gPfSum[PS_SWAP] / n > 300.0; const bool gpuBound = !swThr && ((top == PS_SWAP && !gVsyncOn) || top == PS_DRAW_SOLID || top == PS_DRAW_WATER || top == PS_UPLOAD) && gpuMs >= 0.4 * (sumFrame / n);   // patchSeam.5
            const double cpuMs = (gPfSum[PS_GEN] + gPfSum[PS_LIGHT] + gPfSum[PS_MESH] + scan * n + gPfSum[PS_SIM]) / n; const bool stallGpu = !swThr && cpuMs < 0.25 * (sumFrame / n) && (sumFrame / n) > 22.0;   // patchPerf.8: o tempo do quadro some em esperas da GPU nao atribuidas (swap/driver; ~30 ms com 1,5 ms medidos): conta como GPU no limite: espera de swap = vsync/compositor, nao carga da GPU   // patchSeam.1: so reduz se GPU/desenho ocupa >=40% do quadro (travadas de driver/compositor nao contam)
            const double fLo = gPerf ? 25.0 : 
#ifdef __ANDROID__
48.0
#else
50.0
#endif
, fHi = gPerf ? 32.0 : 58.0; const int cutMax = gPerf ? 3 : 
#ifdef __ANDROID__
8
#else
4
#endif
;   // patchPerf.9 (revertido: -6 chunks nao subiu o FPS): PC ate -6 chunks (era -4); sobe de volta sozinho com FPS folgado. modo desempenho: alvo ~30 fps
            gPerfShDrop = !swThr && fps < (gPerf ? 25.0 : 45.0) && (top == PS_SWAP || top == PS_DRAW_SOLID || top == PS_DRAW_WATER);   // patchPerf.7: com vsync ligado o swap nao conta como GPU e o ajuste de distancia nunca disparava (34 FPS com sombras)
            if (fps < fLo && gpuBound && gAdaptCut < cutMax) { gAdaptCut++; gAdaptUp = 0; logInfo("[PERF] %.0f FPS: distancia de render -1 chunk (ajuste automatico, total -%d)", fps, gAdaptCut); }
            else if (fps >= fHi && gAdaptCut > 0) { if (++gAdaptUp >= 3) { gAdaptCut--; gAdaptUp = 0; logInfo("[PERF] %.0f FPS estavel: distancia de render +1 chunk (total -%d)", fps, gAdaptCut); } }
            else gAdaptUp = 0;
        }
        {   // patchPerf.11: teste unico de vsync: FPS preso em ~30 com a CPU ociosa (quadro some na espera do swap, mesmo com distancia minima) = vsync do driver/compositor cortando pela metade. Desliga o vsync por 1 janela; se o FPS nao subir >=25%, volta.
            static int vsTry = 0; static double vsPrev = 0; const double fps2 = n * 1000.0 / (now - winT0), fr = sumFrame / n;
            const double cpu2 = (gPfSum[PS_GEN] + gPfSum[PS_LIGHT] + gPfSum[PS_MESH] + gPfSum[PS_SIM]) / n;
            if (vsTry == 0 && gVsyncOn && !gPerf && fps2 >= 22.0 && fps2 <= 36.0 && cpu2 < 0.25 * fr && gPfSum[PS_SWAP] / n < 300.0) { vsPrev = fps2; vsTry = 1; SDL_GL_SetSwapInterval(0); gVsyncOn = false; logInfo("[PERF] FPS preso em %.0f com a CPU ociosa: testando sem vsync por 5 s", fps2); }
            else if (vsTry == 1) { vsTry = 2; if (fps2 < vsPrev * 1.25) { SDL_GL_SetSwapInterval(1); gVsyncOn = true; logInfo("[PERF] sem vsync nao ajudou (%.0f FPS): vsync de volta", fps2); } else logInfo("[PERF] sem vsync: %.0f -> %.0f FPS (mantido; /vsync 1 religa)", vsPrev, fps2); }
        }
        gFarSkipD = gAdaptCut >= 2 ? 40.f : 56.f;
        gStreamBudgetSum = 0; avgPrev = sumFrame / n;
        if (top == PS_SWAP || top == PS_DRAW_SOLID) { if (gPfSum[PS_SWAP] / n > 300.0) logInfo("[PERF] dica: swap preso ~1 s = janela estrangulada pelo compositor (2o plano/oculta), nao carga da GPU. Traga a janela para frente; /vsync 0 destrava."); else logInfo("[PERF] dica: o limite e a GPU/geometria. Reduza a distancia de renderizacao ou ative /vsync 1 para estabilizar."); }
        else if (top == PS_GEN || top == PS_LIGHT || top == PS_MESH || top == -1) logInfo("[PERF] dica: gargalo e carregar chunks novos (CPU). Ocorre ao andar/girar rapido; o orcamento adaptativo ja usa a folga do quadro.");
        for (int i = 0; i < PS_N; i++) { gPfSum[i] = 0; gPfMax[i] = 0; }
        frames = 0; worst = 0; sumFrame = 0; winT0 = now; lagsInWin = 0;
    }
}
// HZ-008: benchmark headless. --benchmark [--frames N] [--smoke-report arquivo.json]: mede N quadros (descarta o aquecimento), grava JSON e fecha o jogo.
struct BenchStats { std::vector<double> ms; double last = 0; int warm = 30; };
static bool gBenchOn = false; static int gBenchFrames = 600; static std::string gBenchOut = "benchmark.json"; static BenchStats gBench;
static double benchPct(std::vector<double> v, double q) {   // percentil por posicao (0..1) de v ordenado; v vazio = 0
    if (v.empty()) return 0; std::sort(v.begin(), v.end()); const size_t i = (size_t)std::min<double>(v.size() - 1, std::max(0.0, q) * (v.size() - 1) + .5); return v[i];
}
static std::string benchJson(const std::vector<double>& ms) {   // fps_p1 = 1% mais lento (quadros no percentil 99 do tempo); fps_p99 = 1% mais rapido
    double sum = 0; for (double m : ms) sum += m; const double avg = ms.empty() ? 0 : sum / ms.size();
    auto fps = [](double m) { return m > 0 ? 1000.0 / m : 0.0; };
    char b[512]; snprintf(b, sizeof b, "{\"frames\":%d,\"seconds\":%.3f,\"fps_avg\":%.2f,\"fps_p1\":%.2f,\"fps_p99\":%.2f,\"frame_ms_avg\":%.3f,\"frame_ms_p50\":%.3f,\"frame_ms_p99\":%.3f,\"frame_ms_max\":%.3f}\n",
        (int)ms.size(), sum / 1000.0, fps(avg), fps(benchPct(ms, .99)), fps(benchPct(ms, .01)), avg, benchPct(ms, .5), benchPct(ms, .99), benchPct(ms, 1.0));
    return b;
}
static bool benchTick(double nowMs) {   // 1x por quadro; true = terminou (o chamador fecha o jogo)
    if (!gBenchOn) return false;
    if (gBench.last > 0) { if (gBench.warm > 0) gBench.warm--; else gBench.ms.push_back(nowMs - gBench.last); }
    gBench.last = nowMs;
    if ((int)gBench.ms.size() < gBenchFrames) return false;
    FILE* f = fopen(gBenchOut.c_str(), "wb"); const std::string j = benchJson(gBench.ms); if (f) { fwrite(j.data(), 1, j.size(), f); fclose(f); }
    logInfo("[BENCH] %s", j.c_str()); gBenchOn = false; return true;
}
