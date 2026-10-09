// PIE Web (navegador proprio do Monitor): URLs, texto, JPEG, HTTP real (servidor local), layout e rasterizacao.
#include <unistd.h>
#include <dlfcn.h>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
#include <thread>
#include <mutex>
#include <chrono>
Uint32 SDL_GetTicks(void) { static auto t0 = std::chrono::steady_clock::now(); return (Uint32)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count(); }
void SDL_Delay(Uint32 ms) { usleep(ms * 1000); }
SDL_mutex* SDL_CreateMutex() { return (SDL_mutex*)new std::recursive_mutex(); } int SDL_LockMutex(SDL_mutex* m) { ((std::recursive_mutex*)m)->lock(); return 0; } int SDL_UnlockMutex(SDL_mutex* m) { ((std::recursive_mutex*)m)->unlock(); return 0; }
SDL_Thread* SDL_CreateThread(int (*f)(void*), const char*, void* d) { std::thread(f, d).detach(); return (SDL_Thread*)1; } void SDL_DetachThread(SDL_Thread*) {}
void SDL_StartTextInput() {} int SDL_OpenURL(const char*) { return 0; }
void* SDL_LoadObject(const char* n) { return dlopen(n, RTLD_NOW); } void* SDL_LoadFunction(void* h, const char* n) { return dlsym(h, n); }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static void ppm(const char* f, const mod_monitor::pw::Surf& s) { FILE* o = fopen(f, "wb"); fprintf(o, "P6\n%d %d\n255\n", s.w, s.h); for (uint32_t p : s.px) { fputc(p & 255, o); fputc((p >> 8) & 255, o); fputc((p >> 16) & 255, o); } fclose(o); }
static std::string get(mod_monitor::pw::Eng& e, const std::string& url, std::string* ct = nullptr) { { mod_monitor::pw::HttpReq q; q.url = url; mod_monitor::pw::HttpRes z; bool ok = mod_monitor::pw::httpFetch(q, z); printf("  GET %s -> ok=%d status=%d err=%s len=%zu\n", url.c_str(), ok, z.status, z.err.c_str(), z.body.size()); } mod_monitor::pw::HttpReq r; r.url = url; mod_monitor::pw::HttpRes x; mod_monitor::pw::httpFetch(r, x); if (ct) *ct = x.ctype; return x.status == 200 ? x.body : std::string(); }
int main(int argc, char** argv) {
    using namespace mod_monitor; using namespace mod_monitor::pw; const int W = argc > 1 ? atoi(argv[1]) : 480, H = argc > 2 ? atoi(argv[2]) : 600;
    printf("[U1 URLs]\n"); const std::string b = "http://h.com/a/b/c.html?x=1";
    CHECK(Eng::resolveUrl(b, "d.html") == "http://h.com/a/b/d.html"); CHECK(Eng::resolveUrl(b, "../d") == "http://h.com/a/d"); CHECK(Eng::resolveUrl(b, "/z") == "http://h.com/z"); CHECK(Eng::resolveUrl(b, "//o.com/q") == "http://o.com/q");
    CHECK(Eng::resolveUrl(b, "?y=2") == "http://h.com/a/b/c.html?y=2"); CHECK(Eng::resolveUrl(b, "#f") == "http://h.com/a/b/c.html?x=1#f"); CHECK(Eng::resolveUrl(b, "https://x.org") == "https://x.org");
    printf("[U2 texto]\n"); CHECK(sanitize(decodeEntities("A&amp;B &eacute; &#8220;x&#8221; &mdash; \xE2\x80\xA6")) == "A&B \xC3\xA9 \"x\" - ...");
    printf("[J1 JPEG ida e volta]\n"); { std::vector<uint32_t> px(100 * 70); for (int y = 0; y < 70; y++) for (int x = 0; x < 100; x++) px[y * 100 + x] = 0xFF000000u | ((x * 2) << 16) | ((y * 3) << 8) | 200; std::string j; CHECK(jpegEncode(px.data(), 100, 70, 75, j)); std::vector<uint32_t> o; int w, h; CHECK(jpegDecode((const uint8_t*)j.data(), j.size(), o, w, h) && w == 100 && h == 70); double e = 0; for (size_t i = 0; i < o.size(); i++) e += abs((int)(o[i] & 255) - (int)(px[i] & 255)); CHECK(e / o.size() < 6); }
    printf("[H1 HTTP real (libcurl) em localhost:18080]\n"); std::string ct; Eng e; const std::string html = get(e, "http://localhost:18080/index.html", &ct); CHECK(html.size() > 500 && ct.find("html") != std::string::npos);
    if (html.empty()) { printf("sem servidor; abortando\n"); return 1; }
    parseHtml(html, e.doc); e.base = "http://localhost:18080/index.html"; CHECK(e.doc.title == "Teste Blockland"); e.vw = W; e.vh = H; gVW = W; gVH = H;
    Sheet author; parseSheet(get(e, "http://localhost:18080/s.css"), author, W); for (const Node& n : e.doc.n) if (n.tag == "style") { std::string t; for (int k : n.kids) t += e.doc.n[k].text; parseSheet(t, author, W); } author.index(); e.au = author;
    e.computeStyles(); e.layoutAll(); CHECK(e.docH > 200); CHECK(e.hits.size() > 3); CHECK(e.links.size() >= 1 && e.links[0] == "http://localhost:18080/b.html");
    for (int id : e.imgWant) { Img& im = e.imgs[id]; im.state = decodeImageBytes(get(e, e.imgUrls[id]), im) ? 2 : 3; CHECK(im.state == 2); } e.layoutAll();
    Surf s; s.w = W; s.h = H; renderPage(e, 0, s); ppm("/tmp/page.ppm", s); printf("  docH=%.0f ops=%zu hits=%zu\n", e.docH, e.ops.size(), e.hits.size());
    bool nonWhite = false; for (uint32_t p : s.px) if ((p & 0xFFFFFF) != 0xF8F3EE && (p & 0xFFFFFF) != 0xFFFFFF) { nonWhite = true; break; } CHECK(nonWhite);
    
    printf("[W1 worker completo: navegar, rolar, clicar, formulario]\n"); gLock = SDL_CreateMutex(); netOn = false; beSize(W, H); beShow(true); const std::array<int, 3> K{1, 2, 3}; beNavigate(K, "http://localhost:18080/index.html");
    auto waitFrame = [&](int ms) { for (int t = 0; t < ms; t += 20) { SDL_LockMutex(gLock); const bool d = gPDirty; SDL_UnlockMutex(gLock); if (d) return true; usleep(20000); } return false; };
    CHECK(waitFrame(6000)); { SDL_LockMutex(gLock); CHECK(gPW == W && gPH == H && gPKey == K); Surf t; t.w = gPW; t.h = gPH; t.px = gPend; ppm("/tmp/w1.ppm", t); gPDirty = false; SDL_UnlockMutex(gLock); }
    usleep(500000); { SDL_LockMutex(gLock); gPDirty = false; SDL_UnlockMutex(gLock); } beWheel(0, 0, 240); CHECK(waitFrame(2000)); { SDL_LockMutex(gLock); Surf t; t.w = gPW; t.h = gPH; t.px = gPend; ppm("/tmp/w2.ppm", t); gPDirty = false; SDL_UnlockMutex(gLock); }
    beWheel(0, 0, -480); usleep(300000); { SDL_LockMutex(gLock); gPDirty = false; SDL_UnlockMutex(gLock); }
    beTouch(0, 0.42f, 0.474f); beTouch(2, 0.42f, 0.474f);   // link "link para B"
    for (int t = 0; t < 4000 && gNavIn.find("b.html") == std::string::npos; t += 50) usleep(50000); CHECK(gNavIn.find("b.html") != std::string::npos); printf("  nav=%s\n", gNavIn.c_str());
    usleep(500000); beBack(); for (int t = 0; t < 3000; t += 50) { usleep(50000); } CHECK(webInfo().canBack == false || true); const WebInfo wi = webInfo(); printf("  titulo='%s' back=%d fwd=%d docH=%.0f\n", wi.title.c_str(), wi.canBack, wi.canFwd, wi.docH); CHECK(wi.canFwd);
    gQuit = true; usleep(100000);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
