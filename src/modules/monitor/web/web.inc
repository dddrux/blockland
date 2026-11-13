// ---------- PIE Web: navegador proprio do Monitor Touch (desktop: Windows / Linux / macOS) ----------
// Incluido por monitor.inc DENTRO de namespace mod_monitor. Substitui o Chrome headless: nada externo para instalar.
//   rede (HTTPS pelo sistema) -> HTML -> CSS -> layout -> rasterizador por software -> textura do monitor (+ JPEG para os outros jogadores).
// LIMITE HONESTO: nao executa JavaScript. Sites estaticos / renderizados no servidor (Wikipedia, DuckDuckGo html, noticias, blogs, docs) funcionam; apps (YouTube, Google, redes sociais) nao.
#ifdef _WIN32
typedef SOCKET mon_sock_t;
#define MON_SEND_FLAGS 0
#else
typedef int mon_sock_t;
#ifdef MSG_NOSIGNAL
#define MON_SEND_FLAGS MSG_NOSIGNAL
#else
#define MON_SEND_FLAGS 0
#endif
#endif
static void mlog(const char* fmt, ...) { char b[600]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof b, fmt, a); va_end(a); if (gLock) SDL_LockMutex(gLock); logInfo("[Monitor] %s", b); if (gLock) SDL_UnlockMutex(gLock); }
static std::string gWantUrl; static std::array<int, 3> gWantKey{0, 0, 0}; static int gSeq = 0, gSzSeq = 0, gWantW = 480, gWantH = 480; static bool gVis = false, gThread = false; static const char* gName = "PIE Web (motor proprio)";
struct DecJob { std::array<int, 3> k; std::string j; };
static std::deque<DecJob> gDecQ; static std::string gNavIn;   // gNavIn: URL atual da pagina (links clicados), aplicada pelo jogo em applyNav()
#include "jpeg.inc"
#include "http.inc"
#include "html.inc"
#include "css.inc"
#include "img.inc"
#include "layout.inc"
#include "layout2.inc"
#include "raster.inc"
namespace pw {
static WebInfo gInfo;
enum { CT_TOUCH, CT_MOVE, CT_WHEEL, CT_TEXT, CT_KEY, CT_BACK, CT_FWD, CT_RELOAD, CT_STOP, CT_ZOOM, CT_SCROLLTO };
struct Cmd { int t = 0, a = 0; float x = 0, y = 0; std::string s; };
static std::deque<Cmd> gCmds;
struct Job { int kind = 0, gen = 0, id = 0; HttpReq rq; };   // kind: 0 pagina, 1 css, 2 imagem
struct Res { Job j; HttpRes r; };
static std::deque<Job> gJobs[2]; static std::deque<Res> gRes; static bool gNetUp = false;
static std::string urlEnc(const std::string& s) { std::string o; for (unsigned char c : s) { if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o += (char)c; else if (c == ' ') o += '+'; else { char h[4]; snprintf(h, 4, "%%%02X", c); o += h; } } return o; }
static const char* HOME_HTML = "<html><head><title>Blockland Web</title><style>body{background:#1c2733;color:#e6edf3;margin:0;padding:24px}h1{font-size:40px;margin:16px 0 4px}p{color:#9fb0c0}.box{margin:18px 0}input{font-size:18px}a{color:#7cc4ff}li{margin:6px 0}</style></head><body><h1>Blockland Web</h1><p>Navegador proprio do jogo (sem JavaScript).</p><form class=box action=\"https://html.duckduckgo.com/html/\" method=get><input type=text name=q size=26 placeholder=\"Pesquisar com DuckDuckGo\"> <input type=submit value=Buscar></form><ul><li><a href=\"https://pt.wikipedia.org\">Wikipedia</a></li><li><a href=\"https://news.ycombinator.com\">Hacker News</a></li><li><a href=\"https://text.npr.org\">NPR (texto)</a></li><li><a href=\"https://example.com\">example.com</a></li></ul></body></html>";
static std::string errPage(const std::string& url, const std::string& err) { return "<html><head><title>Erro</title></head><body style=\"background:#fff;color:#222\"><h2>Nao foi possivel abrir a pagina</h2><p>" + url.substr(0, 120) + "</p><p style=\"color:#b00\">" + err + "</p><p><a href=\"about:home\">Pagina inicial</a></p></body></html>"; }
static int netMain(void* p) { const int lane = (int)(intptr_t)p; while (!gQuit) { Job j; bool has = false; SDL_LockMutex(gLock); if (!gJobs[lane].empty()) { j = std::move(gJobs[lane].front()); gJobs[lane].pop_front(); has = true; } SDL_UnlockMutex(gLock); if (!has) { SDL_Delay(10); continue; }
        Res r; r.j = j; if (j.rq.url.compare(0, 6, "about:") == 0) { r.r.status = 200; r.r.ctype = "text/html"; r.r.url = j.rq.url; r.r.body = HOME_HTML; } else httpFetch(j.rq, r.r); SDL_LockMutex(gLock); gRes.push_back(std::move(r)); SDL_UnlockMutex(gLock); } return 0; }
struct HistE { std::string url; float scroll = 0; };
struct SheetSrc { bool inl = false, done = false; std::string text, url; };
struct Br {
    Eng e; int gen = 0; std::string url, pendingFrag, hoverUrl; std::vector<HistE> hist; int hidx = -1; std::vector<SheetSrc> sheets; Uint32 pageT = 0, lastLayout = 0, lastShare = 0; bool laid = false, loading = false, needStyle = false, needLayout = false, dirty = true, built = false, needScroll = false; int cssPending = 0, imgPending = 0, imgTotal = 0;
    float scrollY = 0, wantScroll = 0, mx = 0, my = 0, downX = 0, downY = 0; bool down = false; int focus = -1; int zoomI = 2; int quality = 60; std::string title; float lastFrac = 0; bool pushOnLoad = true; int refreshes = 0; std::string refreshTo;
    static float zoomOf(int i) { static const float Z[] = {.6f, .8f, 1.f, 1.25f, 1.5f, 2.f}; return Z[std::max(0, std::min(5, i))]; }
    void post(int lane, Job j) { SDL_LockMutex(gLock); gJobs[lane].push_back(std::move(j)); SDL_UnlockMutex(gLock); }
    void go(const std::string& u0, const std::string& method = "GET", const std::string& body = "", bool push = true) {
        std::string u = u0; pendingFrag.clear(); const size_t h = u.find('#'); if (h != std::string::npos && u.compare(0, 5, "data:") != 0) { pendingFrag = u.substr(h + 1); u.resize(h); }
        if (u.empty()) { if (!pendingFrag.empty() && e.anchors.count(pendingFrag)) { wantScroll = e.anchors[pendingFrag]; needScroll = true; } return; }
        if (!hist.empty() && hidx >= 0 && hidx < (int)hist.size()) hist[hidx].scroll = scrollY; gen++; loading = true; cssPending = imgPending = imgTotal = 0; pageT = SDL_GetTicks(); pushOnLoad = push; built = false;
        Job j; j.kind = 0; j.gen = gen; j.rq.url = u; j.rq.method = method; j.rq.body = body; j.rq.referer = url.compare(0, 4, "http") == 0 ? url : ""; j.rq.lane = 0; post(0, j); setStatus("Carregando " + u.substr(0, 60) + "...");
    }
    void setStatus(const std::string& s) { SDL_LockMutex(gLock); gInfo.status = s; gStatus = s; SDL_UnlockMutex(gLock); }
    void onPage(Res& r) {
        std::string body = r.r.body, ct = lower(r.r.ctype); const std::string finalUrl = r.r.url.empty() ? r.j.rq.url : r.r.url; std::string html;
        if (r.r.status == 0 || (r.r.status >= 400 && body.empty())) html = errPage(r.j.rq.url, r.r.err.empty() ? "HTTP " + std::to_string(r.r.status) : r.r.err);
        else if (ct.find("image/") == 0) html = "<html><body style=\"background:#222;margin:0\"><img src=\"" + finalUrl + "\"></body></html>";
        else if (ct.find("text/plain") == 0) { html = "<html><body><pre>"; for (char c : body) { if (c == '<') html += "&lt;"; else if (c == '&') html += "&amp;"; else html += c; } html += "</pre></body></html>"; }
        else if (!ct.empty() && ct.find("html") == std::string::npos && ct.find("xml") == std::string::npos) html = errPage(finalUrl, "Tipo de arquivo nao suportado (" + r.r.ctype.substr(0, 40) + "). Use 'Abrir no sistema'.");
        else { if (ct.find("8859-1") != std::string::npos || ct.find("1252") != std::string::npos) body = latin1ToUtf8(body); html = body; }
        url = finalUrl; e.doc = Doc(); parseHtml(html, e.doc); e.base = e.doc.baseUrl.empty() ? finalUrl : Eng::resolveUrl(finalUrl, e.doc.baseUrl); e.imgs.clear(); e.imgMap.clear(); e.imgUrls.clear(); e.val.clear(); e.chk.clear(); e.selIdx.clear(); focus = -1; title = e.doc.title.empty() ? finalUrl.substr(0, 60) : e.doc.title;
        sheets.clear(); cssPending = 0; int ext = 0;
        for (size_t i = 0; i < e.doc.n.size(); i++) { const Node& nd = e.doc.n[i]; if (nd.isText) continue;
            if (nd.tag == "style") { SheetSrc s; s.inl = true; s.done = true; for (int k : nd.kids) s.text += e.doc.n[k].text; const std::string m = attrS(nd, "media"); if (m.empty() || mediaOk(m, e.vw)) sheets.push_back(s); }
            else if (nd.tag == "link" && lower(attrS(nd, "rel")).find("stylesheet") != std::string::npos && !attrS(nd, "href").empty() && ext < 8) { const std::string m = attrS(nd, "media"); if (!m.empty() && !mediaOk(m, e.vw)) continue; SheetSrc s; s.url = Eng::resolveUrl(e.base, attrS(nd, "href")); if (s.url.compare(0, 4, "http") != 0) continue; ext++; sheets.push_back(s); Job j; j.kind = 1; j.gen = gen; j.id = (int)sheets.size() - 1; j.rq.url = s.url; j.rq.referer = e.base; j.rq.maxBytes = 1u << 20; j.rq.timeoutS = 12; j.rq.lane = 0; post(0, j); cssPending++; } }
        loading = false; built = false; pageT = SDL_GetTicks(); needStyle = true; scrollY = 0; wantScroll = 0;
        for (const Node& nd : e.doc.n) if (nd.tag == "meta" && lower(attrS(nd, "http-equiv")) == "refresh" && refreshes < 3) { const std::string c = attrS(nd, "content"); const size_t u = lower(c).find("url="); if (u != std::string::npos && atof(c.c_str()) <= 5) { std::string t = trim(c.substr(u + 4)); if (!t.empty() && (t[0] == '\'' || t[0] == '"')) t = t.substr(1, t.find_last_of("'\"") - 1); refreshes++; refreshTo = Eng::resolveUrl(finalUrl, t); } } 
        if (pushOnLoad) { if (hidx >= 0 && hidx + 1 < (int)hist.size()) hist.resize(hidx + 1); if (hist.empty() || hist.back().url != finalUrl) { HistE h; h.url = finalUrl; hist.push_back(h); hidx = (int)hist.size() - 1; } else hidx = (int)hist.size() - 1; } else if (hidx >= 0 && hidx < (int)hist.size()) { hist[hidx].url = finalUrl; needScroll = true; wantScroll = hist[hidx].scroll; }
        SDL_LockMutex(gLock); gNavIn = finalUrl; SDL_UnlockMutex(gLock); mlog("pagina: %s (%d, %d bytes, %d CSS)", finalUrl.c_str(), r.r.status, (int)r.r.body.size(), cssPending); setStatus(cssPending ? "Carregando estilos..." : "");
    }
    void buildStyle() {
        e.au = Sheet(); for (const SheetSrc& s : sheets) if (!s.text.empty()) parseSheet(s.text, e.au, e.vw); e.au.index(); e.computeStyles(); built = true; needStyle = false; needLayout = true; laid = false;
    }
    void relayout() {
        e.layoutAll(); needLayout = false; laid = true; dirty = true; lastLayout = SDL_GetTicks(); const float maxS = std::max(0.f, e.docH - e.vh);
        if (!pendingFrag.empty() && e.anchors.count(pendingFrag)) { wantScroll = e.anchors[pendingFrag]; needScroll = true; pendingFrag.clear(); }
        if (needScroll) { scrollY = wantScroll; needScroll = false; } scrollY = std::max(0.f, std::min(scrollY, maxS));
        for (int id : e.imgWant) { if (id < (int)e.imgs.size() && e.imgs[id].state == 0 && imgTotal < 80) { e.imgs[id].state = 1; imgTotal++; imgPending++; Job j; j.kind = 2; j.gen = gen; j.id = id; j.rq.url = e.imgUrls[id]; j.rq.referer = e.base; j.rq.maxBytes = 3u << 20; j.rq.timeoutS = 15; j.rq.lane = 1; post(1, j); } else if (id < (int)e.imgs.size() && e.imgs[id].state == 0) e.imgs[id].state = 3; }
        e.imgWant.clear();
    }
    void onRes(Res& r) {
        if (r.j.gen != gen) return;
        if (r.j.kind == 0) onPage(r);
        else if (r.j.kind == 1) { if (r.j.id < (int)sheets.size()) { if (r.r.status == 200 || (r.r.status > 0 && r.r.status < 300)) sheets[r.j.id].text = r.r.body; sheets[r.j.id].done = true; } if (cssPending > 0) cssPending--; if (built) needStyle = true; }
        else if (r.j.kind == 2) { if (r.j.id < (int)e.imgs.size()) { Img& im = e.imgs[r.j.id]; if (!(r.r.status >= 200 && r.r.status < 300 && decodeImageBytes(r.r.body, im))) im.state = 3; } if (imgPending > 0) imgPending--; if (built) needLayout = true; }
    }
    float maxScroll() const { return std::max(0.f, e.docH - (float)e.vh); }
    void scrollBy(float d) { scrollY = std::max(0.f, std::min(maxScroll(), scrollY + d)); dirty = true; }
    int formOf(int n) { for (int p = e.doc.n[n].parent; p > 0; p = e.doc.n[p].parent) if (e.doc.n[p].tag == "form") return p; return -1; }
    void submit(int form, int submitter) {
        if (form < 0) return; std::string q; std::vector<int> st{form}; std::vector<int> all; while (!st.empty()) { const int x = st.back(); st.pop_back(); for (int i = (int)e.doc.n[x].kids.size() - 1; i >= 0; i--) { const int k = e.doc.n[x].kids[i]; if (!e.doc.n[k].isText) { all.push_back(k); st.push_back(k); } } }
        std::sort(all.begin(), all.end()); auto add = [&](const std::string& n, const std::string& v) { if (n.empty()) return; if (!q.empty()) q += '&'; q += urlEnc(n) + "=" + urlEnc(v); };
        for (int c : all) { const Node& nd = e.doc.n[c]; const std::string name = attrS(nd, "name"); if (attrOf(nd, "disabled")) continue;
            if (nd.tag == "input") { const std::string ty = e.inType(c); if (ty == "submit" || ty == "image" || ty == "button") { if (c == submitter) add(name, e.btnLabel(c)); } else if (ty == "reset" || ty == "file") {} else if (ty == "checkbox" || ty == "radio") { if (e.checked(c)) add(name, attrOf(nd, "value") ? attrS(nd, "value") : "on"); } else add(name, e.valueOf(c)); }
            else if (nd.tag == "textarea") add(name, e.valueOf(c)); else if (nd.tag == "button") { const std::string ty = lower(attrS(nd, "type")); if (c == submitter && ty != "button" && ty != "reset") add(name, attrS(nd, "value")); }
            else if (nd.tag == "select") { std::vector<int> o = e.optionsOf(c); if (!o.empty()) { const int i = std::min((int)o.size() - 1, e.selIndex(c)); add(name, attrOf(e.doc.n[o[i]], "value") ? attrS(e.doc.n[o[i]], "value") : e.optText(o[i])); } } }
        std::string act = trim(attrS(e.doc.n[form], "action")); std::string target = act.empty() ? url : Eng::resolveUrl(e.base, act); if (target.compare(0, 4, "http") != 0) return; const std::string method = lower(attrS(e.doc.n[form], "method"));
        if (method == "post") { go(target, "POST", q, true); } else { const size_t h = target.find('#'); if (h != std::string::npos) target.resize(h); const size_t qm = target.find('?'); if (qm != std::string::npos) target.resize(qm); go(target + "?" + q, "GET", "", true); }
    }
    int hitAt(float px, float py, bool& isLink) {
        const float dy = py + scrollY; for (int i = (int)e.hits.size() - 1; i >= 0; i--) { const HitR& h = e.hits[i]; if (px >= h.x && px < h.x + h.w && dy >= h.y && dy < h.y + h.h) { isLink = h.node < 0; return isLink ? h.link : h.node; } } return -1;
    }
    void click(float px, float py) {
        bool isLink = false; const int id = hitAt(px, py, isLink); if (id < 0) { if (focus >= 0) { focus = -1; dirty = true; } return; }
        if (isLink) { std::string u = e.links[id]; focus = -1; if (u == "#") { scrollY = 0; dirty = true; return; } if (u.compare(0, 11, "javascript:") == 0) return; if (u.compare(0, 4, "http") == 0 || u.compare(0, 6, "about:") == 0 || u.compare(0, 5, "data:") == 0) go(u); else if (u.compare(0, 7, "mailto:") == 0 || u.compare(0, 4, "tel:") == 0) SDL_OpenURL(u.c_str()); return; }
        const Node& nd = e.doc.n[id]; const std::string ty = nd.tag == "input" ? e.inType(id) : nd.tag;
        if (ty == "checkbox") { e.chk[id] = e.checked(id) ? 0 : 1; needLayout = true; focus = -1; }
        else if (ty == "radio") { const std::string nm = attrS(nd, "name"); const int f = formOf(id); for (size_t i = 0; i < e.doc.n.size(); i++) if (e.doc.n[i].tag == "input" && e.inType((int)i) == "radio" && attrS(e.doc.n[i], "name") == nm && formOf((int)i) == f) e.chk[(int)i] = 0; e.chk[id] = 1; needLayout = true; focus = -1; }
        else if (ty == "submit" || ty == "image" || (nd.tag == "button" && lower(attrS(nd, "type")) != "button" && lower(attrS(nd, "type")) != "reset")) { focus = -1; submit(formOf(id), id); }
        else if (ty == "reset") { for (size_t i = 0; i < e.doc.n.size(); i++) if (formOf((int)i) == formOf(id)) { e.val.erase((int)i); e.chk.erase((int)i); } needLayout = true; }
        else if (ty == "select") { const int n = (int)e.optionsOf(id).size(); if (n) { e.selIdx[id] = (e.selIndex(id) + 1) % n; needLayout = true; } focus = -1; }
        else if (ty == "textarea" || ty == "text" || ty == "search" || ty == "email" || ty == "url" || ty == "password" || ty == "number" || ty == "tel") { focus = id; if (!e.val.count(id)) e.val[id] = e.valueOf(id); dirty = true; SDL_StartTextInput(); }
        else if (ty == "button" || ty == "file") {} else { focus = id; if (!e.val.count(id)) e.val[id] = e.valueOf(id); dirty = true; }
    }
    void key(int k) {
        const bool inp = focus >= 0; std::string* v = inp ? &e.val[focus] : nullptr; const bool ta = inp && e.doc.n[focus].tag == "textarea";
        if (inp) { if (k == 8) { if (!v->empty()) { while (!v->empty() && ((v->back() & 0xC0) == 0x80)) v->pop_back(); if (!v->empty()) v->pop_back(); needLayout = true; } return; }
            if (k == 13 || k == 0x40000058) { if (ta) { v->push_back('\n'); needLayout = true; } else submit(formOf(focus), -1); return; } if (k == 9) { focus = -1; dirty = true; return; } if (k == 32) { v->push_back(' '); needLayout = true; return; } }
        const float pg = std::max(40.f, e.vh * .85f); switch (k) { case 32: scrollBy(pg); break; case 0x40000052: scrollBy(-60 * gZoom); break; case 0x40000051: scrollBy(60 * gZoom); break; case 0x4000004B: scrollBy(-pg); break; case 0x4000004E: scrollBy(pg); break; case 0x4000004A: scrollBy(-1e9f); break; case 0x4000004D: scrollBy(1e9f); break; default: break; }
    }
    void text(const std::string& t) { if (focus < 0) return; std::string& v = e.val[focus]; std::string s; for (char c : t) { if (c == '\r') continue; if (c == '\n' && e.doc.n[focus].tag != "textarea") continue; s += c; } if (v.size() + s.size() < 2000) { v += sanitize(s); needLayout = true; } }
    void zoom(int d) { const int ni = d == 0 ? 2 : std::max(0, std::min(5, zoomI + d)); if (ni == zoomI) return; zoomI = ni; gZoom = zoomOf(zoomI); needStyle = true; dirty = true; }
    void navHist(int d) { if (!hist.empty() && hidx >= 0) hist[hidx].scroll = scrollY; const int ni = hidx + d; if (ni < 0 || ni >= (int)hist.size()) return; hidx = ni; go(hist[ni].url, "GET", "", false); }
    void command(const Cmd& c) {
        switch (c.t) {
        case CT_TOUCH: mx = c.x; my = c.y; if (c.a == 0) { down = true; downX = mx; downY = my; } else if (c.a == 2) { if (down && fabsf(mx - downX) < 8 && fabsf(my - downY) < 8) click(mx, my); down = false; } break;
        case CT_MOVE: { mx = c.x; my = c.y; bool il = false; const int id = hitAt(mx, my, il); std::string h = il && id >= 0 ? e.links[id] : ""; if (h != hoverUrl) { hoverUrl = h; dirty = true; } break; }
        case CT_WHEEL: scrollBy(c.y); break; case CT_TEXT: text(c.s); break; case CT_KEY: key(c.a); break;
        case CT_BACK: navHist(-1); break; case CT_FWD: navHist(1); break; case CT_RELOAD: if (!url.empty()) go(url, "GET", "", false); break; case CT_STOP: gen++; loading = false; setStatus(""); break; case CT_ZOOM: zoom(c.a); break; default: break; }
    }
};
static int worker(void*) {
    Br b; int seenSeq = -1, seenSz = -1, W = 480, H = 480; Uint32 lastRender = 0, lastBlink = 0; std::array<int, 3> key{0, 0, 0}; Surf surf; bool caretOn = true; std::string lastShareStr; Uint32 lastLoadMsg = 0;
    while (!gQuit) {
        for (;;) { DecJob j; SDL_LockMutex(gLock); const bool has = !gDecQ.empty(); if (has) { j = std::move(gDecQ.front()); gDecQ.pop_front(); } SDL_UnlockMutex(gLock); if (!has) break; std::vector<uint32_t> px; int w = 0, h = 0; if (jpegDecode((const uint8_t*)j.j.data(), j.j.size(), px, w, h)) setPending(j.k, px.data(), w, h); }
        SDL_LockMutex(gLock); const std::string wu = gWantUrl; key = gWantKey; const bool vis = gVis; const int seq = gSeq, szs = gSzSeq; W = gWantW; H = gWantH; std::deque<Cmd> cmds; cmds.swap(gCmds); std::deque<Res> rs; rs.swap(gRes); SDL_UnlockMutex(gLock);
        if (wu.empty() || !vis) { SDL_Delay(30); if (!rs.empty()) for (Res& r : rs) b.onRes(r); continue; }
        if (szs != seenSz) { seenSz = szs; gVW = b.e.vw = std::max(160, W); gVH = b.e.vh = std::max(160, H); if (b.built) b.needStyle = true; b.dirty = true; surf.w = W; surf.h = H; }
        if (seq != seenSeq) { seenSeq = seq; b.refreshes = 0; if (wu != b.url || b.hist.empty()) b.go(wu, "GET", "", true); else b.go(wu, "GET", "", false); }
        for (Res& r : rs) b.onRes(r); for (const Cmd& c : cmds) b.command(c);
        if (!b.refreshTo.empty()) { const std::string u = b.refreshTo; b.refreshTo.clear(); b.pushOnLoad = false; b.go(u, "GET", "", false); }
        const Uint32 now = SDL_GetTicks();
        if (!b.loading && b.cssPending == 0 && b.needStyle) b.buildStyle(); else if (!b.loading && b.needStyle && now - b.pageT > 2500) b.buildStyle();
        if (b.needLayout && (now - b.lastLayout > 40 || !b.laid)) b.relayout();
        if (b.focus >= 0 && now - lastBlink > 500) { lastBlink = now; caretOn = !caretOn; b.dirty = true; }
        const bool busy = b.loading || b.cssPending > 0 || b.imgPending > 0;
        if (busy && now - lastLoadMsg > 250) { lastLoadMsg = now; b.dirty = true; }
        if (b.dirty && now - lastRender >= 16 && surf.w > 0 && b.built && b.laid) {
            lastRender = now; b.dirty = false; if ((int)surf.px.size() != surf.w * surf.h) surf.px.assign((size_t)surf.w * surf.h, 0); renderPage(b.e, b.scrollY, surf);
            if (b.focus >= 0 && caretOn) { auto it = b.e.inputs.find(b.focus); if (it != b.e.inputs.end()) { const InInfo& f = it->second; const std::string v = b.e.val[b.focus]; std::string tx = v; const bool ta = b.e.doc.n[b.focus].tag == "textarea"; if (!ta) while (tx.size() > 1 && Eng::tw(tx, f.scale) > f.w - 10) tx.erase(0, 1); const float cx = f.x + 5 + (ta ? 0 : Eng::tw(tx, f.scale)); fillRect(surf, cx, f.y - b.scrollY + 4, cx + std::max(1, f.scale), f.y - b.scrollY + f.h - 4, 0xFF000000u); fillRect(surf, f.x - 1, f.y - b.scrollY - 1, f.x + f.w + 1, f.y - b.scrollY, 0xFF3070D0); fillRect(surf, f.x - 1, f.y - b.scrollY + f.h, f.x + f.w + 1, f.y - b.scrollY + f.h + 1, 0xFF3070D0); } }
            setPending(key, surf.px.data(), surf.w, surf.h);
            if (netOn && now - b.lastShare >= 100) { b.lastShare = now; std::string jpg; if (jpegEncode(surf.px.data(), surf.w, surf.h, b.quality, jpg)) { if (jpg.size() <= 46000) queueJpeg(key, (const uint8_t*)jpg.data(), (int)jpg.size()); else b.quality = std::max(15, b.quality - 10); } }
        }
        { SDL_LockMutex(gLock); gInfo.title = b.title; gInfo.hover = b.hoverUrl; gInfo.canBack = b.hidx > 0; gInfo.canFwd = b.hidx >= 0 && b.hidx + 1 < (int)b.hist.size(); gInfo.loading = busy; gInfo.docH = b.e.docH; gInfo.scrollY = b.scrollY; gInfo.viewH = (float)b.e.vh; gInfo.zoomPct = (int)(Br::zoomOf(b.zoomI) * 100 + .5f);
          gInfo.progress = b.loading ? 15 : (b.cssPending > 0 ? 50 : (b.imgTotal > 0 ? 60 + 40 * (b.imgTotal - b.imgPending) / b.imgTotal : 100)); if (!b.loading && b.cssPending == 0 && !busy && gInfo.status.compare(0, 9, "Carregand") == 0) { gInfo.status.clear(); gStatus.clear(); } SDL_UnlockMutex(gLock); }
        SDL_Delay(busy || b.dirty ? 4 : 10);
    }
    return 0;
}
static void ensureNet() { if (gNetUp) return; gNetUp = true; sysWarm(); for (intptr_t l = 0; l < 2; l++) SDL_DetachThread(SDL_CreateThread(netMain, l ? "monnet1" : "monnet0", (void*)l)); }
}   // namespace pw
// ---- API de backend usada por monitor.inc ----
static void cmdPush(int t, int a, float x, float y, const std::string& str = "") { if (!gLock) return; pw::Cmd c; c.t = t; c.a = a; c.x = x; c.y = y; c.s = str; SDL_LockMutex(gLock); if (t == pw::CT_MOVE && !pw::gCmds.empty() && pw::gCmds.back().t == pw::CT_MOVE) pw::gCmds.back() = c; else if (pw::gCmds.size() < 300) pw::gCmds.push_back(c); SDL_UnlockMutex(gLock); }
static void ensureWorker() { if (!gThread) { gThread = true; pw::ensureNet(); SDL_DetachThread(SDL_CreateThread(pw::worker, "monweb", nullptr)); } }
static void queueDecode(const std::array<int, 3>& k, const std::string& j) { if (!gLock) return; ensureWorker(); SDL_LockMutex(gLock); gDecQ.push_back({k, j}); while (gDecQ.size() > 2) gDecQ.pop_front(); SDL_UnlockMutex(gLock); }
static void setSize(int w, int h) { if (!gLock) return; SDL_LockMutex(gLock); if (w != gWantW || h != gWantH) { gWantW = w; gWantH = h; gSzSeq++; } SDL_UnlockMutex(gLock); }
static void killBrowser() {}
// coordenadas normalizadas (0..1) da imagem -> pixels da pagina
static float pxX(float n) { return std::max(0.f, std::min(1.f, n)) * (float)(gWantW - 1); } static float pxY(float n) { return std::max(0.f, std::min(1.f, n)) * (float)(gWantH - 1); }
static void beTouch(int act, float nx, float ny) { cmdPush(pw::CT_TOUCH, act, pxX(nx), pxY(ny)); }
static void beHover(float nx, float ny) { cmdPush(pw::CT_MOVE, 0, pxX(nx), pxY(ny)); }
static void beWheel(float, float, int dy) { cmdPush(pw::CT_WHEEL, 0, 0, (float)dy * 0.5f); }   // 1 notch (120) = 60 px
static void beText(const char* t) { if (t && *t) cmdPush(pw::CT_TEXT, 0, 0, 0, t); }
static void beKey(int k) { cmdPush(pw::CT_KEY, k, 0, 0); }
static void beScroll(int dy) { cmdPush(pw::CT_WHEEL, 0, 0, (float)dy); }
static void beBack() { cmdPush(pw::CT_BACK, 0, 0, 0); } static void beForward() { cmdPush(pw::CT_FWD, 0, 0, 0); } static void beStop() { cmdPush(pw::CT_STOP, 0, 0, 0); } static void beZoom(int d) { cmdPush(pw::CT_ZOOM, d, 0, 0); }
static pw::WebInfo webInfo() { pw::WebInfo w; if (!gLock) return w; SDL_LockMutex(gLock); w = pw::gInfo; SDL_UnlockMutex(gLock); return w; }
static std::string getStatus() { if (!gLock) return ""; SDL_LockMutex(gLock); std::string s = gStatus; SDL_UnlockMutex(gLock); return s; }
