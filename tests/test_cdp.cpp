// Monitor Touch (desktop/CDP): decodificador JPEG vs PIL, e ciclo completo com um "Chrome" falso (tests/fakecdp.py): lancar, conectar, navegar, screencast, ack, decodificar, enviar aos outros jogadores.
#include <unistd.h>
#include <thread>
#include <mutex>
#include <chrono>
#define main game_main
#include "../src/main.cpp"
#undef main
static std::chrono::steady_clock::time_point T0 = std::chrono::steady_clock::now();
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
Uint32 SDL_GetTicks(void) { return (Uint32)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - T0).count(); }
void SDL_Delay(Uint32 ms) { usleep(ms * 1000); }
SDL_mutex* SDL_CreateMutex() { return (SDL_mutex*)new std::recursive_mutex; } int SDL_LockMutex(SDL_mutex* m) { ((std::recursive_mutex*)m)->lock(); return 0; } int SDL_UnlockMutex(SDL_mutex* m) { ((std::recursive_mutex*)m)->unlock(); return 0; }
SDL_Thread* SDL_CreateThread(int (*f)(void*), const char*, void* a) { std::thread(f, a).detach(); return (SDL_Thread*)1; } void SDL_DetachThread(SDL_Thread*) {}
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static std::string rdfile(const std::string& p) { FILE* f = fopen(p.c_str(), "rb"); std::string s; if (!f) return s; char b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) s.append(b, n); fclose(f); return s; }
int main() {
    using namespace mod_monitor;
    if (system("python3 ../tests/cdp_fixtures.py 2>/dev/null || python3 cdp_fixtures.py") != 0) { printf("sem PIL: teste ignorado\n"); return 0; }
    printf("[J1 jpegDecode vs PIL]\n");
    const struct { const char* n; double tol; } F[] = {{"a420", 6}, {"a444", 2.5}, {"a422", 6}, {"gray", 2.5}, {"big", 2.5}, {"rst", 6}};
    for (auto& f : F) {
        const std::string j = rdfile(std::string("/tmp/jt/") + f.n + ".jpg"), ref = rdfile(std::string("/tmp/jt/") + f.n + ".rgb"); int rw = 0, rh = 0; sscanf(rdfile(std::string("/tmp/jt/") + f.n + ".txt").c_str(), "%d %d", &rw, &rh);
        std::vector<uint32_t> px; int w = 0, h = 0; const bool ok = jpegDecode((const uint8_t*)j.data(), j.size(), px, w, h); CHECK(ok && w == rw && h == rh && (int)ref.size() == rw * rh * 3);
        if (!ok || w != rw || h != rh) continue; double e = 0; for (int i = 0; i < w * h; i++) { const uint32_t u = px[i]; e += abs((int)(u & 255) - (uint8_t)ref[i * 3]) + abs((int)(u >> 8 & 255) - (uint8_t)ref[i * 3 + 1]) + abs((int)(u >> 16 & 255) - (uint8_t)ref[i * 3 + 2]); }
        e /= w * h * 3.0; printf("  %s %dx%d erro medio %.2f\n", f.n, w, h, e); CHECK(e <= f.tol && (px[0] >> 24) == 0xFF);
    }
    { std::string j = rdfile("/tmp/jt/a420.jpg"); std::vector<uint32_t> px; int w, h; CHECK(!jpegDecode((const uint8_t*)j.data(), 50, px, w, h)); j[100] ^= 0x55; jpegDecode((const uint8_t*)j.data(), j.size(), px, w, h); j.resize(j.size() / 2); jpegDecode((const uint8_t*)j.data(), j.size(), px, w, h); CHECK(true); }   // lixo/truncado nao trava nem estoura
    printf("[B1 base64/json]\n"); CHECK(b64dec("SGVsbG8gbXVuZG8=") == "Hello mundo"); std::string v; CHECK(jsonStr("{\"a\": 1, \"data\" : \"xyz\"}", "data", v) && v == "xyz"); CHECK(jsonInt("{\"params\":{\"sessionId\":42}}", "sessionId") == 42);
    printf("[W1 ciclo completo com Chrome falso]\n");
    system("rm -rf /tmp/fakebin /tmp/fakecdp.log && mkdir -p /tmp/fakebin"); { FILE* f = fopen("/tmp/fakebin/chromium", "w"); fputs("#!/bin/sh\nexec python3 \"$FAKECDP\" \"$@\"\n", f); fclose(f); }
    chmod("/tmp/fakebin/chromium", 0755); setenv("PATH", "/tmp/fakebin:/usr/bin:/bin", 1); char cwd[512]; getcwd(cwd, sizeof cwd); setenv("FAKECDP", (std::string(cwd) + (access("fakecdp.py", R_OK) == 0 ? "/fakecdp.py" : "/../tests/fakecdp.py")).c_str(), 1);
    gLock = SDL_CreateMutex(); netOn = true; const std::array<int, 3> K{3, 4, 5}; gMon[K]; beSize(480, 480); beShow(true); beNavigate(K, "https://example.com/a?b=1&c=\"x");
    bool gotFrame = false; for (int i = 0; i < 400 && !gotFrame; i++) { SDL_Delay(50); SDL_LockMutex(gLock); gotFrame = gPDirty && gPKey == K; SDL_UnlockMutex(gLock); }
    CHECK(gotFrame); CHECK(gPW > 100 && gPH > 100); bool net = false; for (int i = 0; i < 60 && !net; i++) { SDL_Delay(50); SDL_LockMutex(gLock); net = !gFq.empty(); SDL_UnlockMutex(gLock); } CHECK(net);   // quadro enfileirado p/ os outros jogadores
    SDL_Delay(1200); const std::string lg = rdfile("/tmp/fakecdp.log"); printf("  log do navegador falso:\n%s", lg.c_str());
    CHECK(lg.find("Page.navigate {\"url\": \"https://example.com/a?b=1&c=%22x\"}") != std::string::npos);   // aspas viram %22 (sem injecao no JSON do CDP)
    CHECK(lg.find("Emulation.setDeviceMetricsOverride") != std::string::npos && lg.find("Page.startScreencast") != std::string::npos);
    int acks = 0; for (size_t p = 0; (p = lg.find("Page.screencastFrameAck", p)) != std::string::npos; p++) acks++; CHECK(acks >= 3 || getenv("FAKE_NOSCREENCAST"));   // sem screencast: o jogo cai para Page.captureScreenshot
    if (getenv("FAKE_NOSCREENCAST")) CHECK(lg.find("Page.captureScreenshot") != std::string::npos);
    printf("[I1 entrada: mouse, roda, teclado, voltar, URL da pagina]\n");
    { std::string nv; for (int i = 0; i < 40 && nv.empty(); i++) { SDL_Delay(50); SDL_LockMutex(gLock); nv = gNavIn; SDL_UnlockMutex(gLock); } CHECK(nv == "https://example.com/redir?x=1&y=2"); }   // link/redirect: URL acompanha a pagina
    beTouch(0, .5f, .25f); beTouch(1, .6f, .25f); beTouch(2, .6f, .25f); beWheel(.5f, .5f, 120); beText("a"); beText("\xC3\xA9"); beText(" "); beKey(32); beKey(13); beKey(0x40000050); beBack();
    SDL_Delay(800); const std::string l2 = rdfile("/tmp/fakecdp.log");
    CHECK(l2.find("\"type\": \"mousePressed\", \"x\": 239, \"y\": 119") != std::string::npos && l2.find("mouseReleased") != std::string::npos);
    CHECK(l2.find("mouseWheel") != std::string::npos && l2.find("\"deltaY\": 120") != std::string::npos);
    CHECK(l2.find("\"type\": \"keyDown\", \"key\": \"a\", \"code\": \"KeyA\", \"windowsVirtualKeyCode\": 65") != std::string::npos);
    CHECK(l2.find("Input.insertText {\"text\": \"\\u00e9\"}") != std::string::npos);   // acento: texto
    CHECK(l2.find("\"code\": \"Space\"") != std::string::npos && l2.find("\"code\": \"Enter\"") != std::string::npos && l2.find("ArrowLeft") != std::string::npos);
    CHECK(l2.find("history.back()") != std::string::npos);
    { int sp = 0; for (size_t p = 0; (p = l2.find("\"code\": \"Space\"", p)) != std::string::npos; p++) sp++; CHECK(sp == 2); }   // so beKey(32) (keyDown+keyUp); o espaco do TEXTINPUT nao duplica
    printf("[R1 quadro de outro jogador]\n"); if (!getenv("FAKE_NOSCREENCAST")) { const std::array<int, 3> K2{9, 9, 9}; gMon[K2]; SDL_LockMutex(gLock); gPDirty = false; SDL_UnlockMutex(gLock); queueDecode(K2, rdfile("/tmp/jt/a444.jpg")); bool g2 = false; for (int i = 0; i < 100 && !g2; i++) { SDL_Delay(50); SDL_LockMutex(gLock); g2 = gPDirty && gPKey == K2; SDL_UnlockMutex(gLock); } CHECK(g2 && gPW == 203 && gPH == 117); }
    onShutdown(); bool gone = false; for (int i = 0; i < 100 && !gone; i++) { SDL_Delay(50); gone = system("kill -0 $(cat /tmp/fakecdp.pid) 2>/dev/null") != 0; } CHECK(gone);   // navegador encerrado junto com o jogo
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
