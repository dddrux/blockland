// PIE Web: robustez (HTML bagunçado, aninhamento extremo, tabelas enormes, bytes aleatorios) + tempo de layout.
#include <unistd.h>
#include <chrono>
#define main game_main
#include "../src/main.cpp"
#undef main
void* SDL_GL_GetProcAddress(const char*) { return nullptr; }
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("  FALHOU linha %d: %s\n", __LINE__, #c); } } while (0)
static double run(const std::string& html, int W, float* docH = nullptr) {
    using namespace mod_monitor::pw; auto t0 = std::chrono::steady_clock::now(); gZoom = 1; gVW = W; gVH = 600;
    Eng e; parseHtml(html, e.doc); e.base = "http://x.com/"; e.vw = W; e.vh = 600; e.computeStyles(); e.layoutAll(); Surf s; s.w = W; s.h = 600; renderPage(e, 0, s); if (docH) *docH = e.docH;
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}
int main() {
    srand(7); std::string h; double t; float dh;
    printf("[F1 aninhamento 3000 divs]\n"); h = "<body>"; for (int i = 0; i < 3000; i++) h += "<div style='padding:1px'>x"; t = run(h, 480); printf("  %.2fs\n", t); CHECK(t < 5);
    printf("[F2 20000 spans]\n"); h = "<p>"; for (int i = 0; i < 20000; i++) h += "<span>palavra" + std::to_string(i) + " </span>"; t = run(h, 480, &dh); printf("  %.2fs docH=%.0f\n", t, dh); CHECK(t < 5 && dh > 1000);
    printf("[F3 tabela 150x20]\n"); h = "<table>"; for (int r = 0; r < 150; r++) { h += "<tr>"; for (int c = 0; c < 20; c++) h += "<td>c" + std::to_string(r * c) + "</td>"; h += "</tr>"; } h += "</table>"; t = run(h, 960); printf("  %.2fs\n", t); CHECK(t < 5);
    printf("[F4 tabelas aninhadas x12]\n"); h = ""; for (int i = 0; i < 12; i++) h += "<table><tr><td>a</td><td>"; h += "fim"; for (int i = 0; i < 12; i++) h += "</td></tr></table>"; t = run(h, 960); printf("  %.2fs\n", t); CHECK(t < 5);
    printf("[F5 flex aninhado x40 + palavra gigante]\n"); h = "<body>"; for (int i = 0; i < 40; i++) h += "<div style='display:flex'><div style='flex:1'>"; h += std::string(5000, 'W'); for (int i = 0; i < 40; i++) h += "</div></div>"; t = run(h, 480); printf("  %.2fs\n", t); CHECK(t < 5);
    printf("[F6 100 documentos aleatorios]\n"); const char* tags[] = {"div", "span", "p", "a", "table", "tr", "td", "ul", "li", "b", "img", "input", "select", "option", "form", "h1", "br", "pre", "button", "textarea", "style", "svg"};
    double worst = 0; for (int d = 0; d < 100; d++) { h.clear(); for (int i = 0; i < 400; i++) { const int k = rand() % 6; const char* tg = tags[rand() % 22]; if (k == 0) h += std::string("</") + tg + ">"; else if (k == 1) h += "texto " + std::to_string(rand()) + " "; else if (k == 2) h += std::string("<") + tg + " style='" + (rand() % 2 ? "display:flex;width:" + std::to_string(rand() % 900) + "px" : "float:left;margin:" + std::to_string(rand() % 40) + "px auto") + "' class='a b'>"; else if (k == 3) h += std::string("<") + tg + " href=/x src=/y.png width=" + std::to_string(rand() % 3000) + ">"; else if (k == 4) { h += "<"; for (int q = 0; q < rand() % 20; q++) h += (char)(rand() % 256); } else h += std::string("<") + tg + ">"; } t = run(h, 160 + rand() % 1500); worst = std::max(worst, t); } printf("  pior %.2fs\n", worst); CHECK(worst < 3);
    printf("[F7 bytes aleatorios]\n"); for (int d = 0; d < 50; d++) { h.clear(); for (int i = 0; i < 5000; i++) h += (char)(rand() % 256); run(h, 480); } CHECK(true);
    printf("[F8 CSS hostil]\n"); h = "<style>"; for (int i = 0; i < 3000; i++) h += ".c" + std::to_string(i) + " div > p + span:not(.x):nth-child(2n+1){color:red;margin:calc(10px - 1em) var(--q,3px)}\n"; h += "@media (max-width:" + std::to_string(99999) + "px){a{b:c}}} }}{{ a{ </style><div class=c5><p>x</p></div>"; t = run(h, 480); printf("  %.2fs\n", t); CHECK(t < 5);
    printf("%d verificacoes, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
