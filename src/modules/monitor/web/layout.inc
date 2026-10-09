// ---------- PIE Web: estilos, layout (bloco, linha, tabela, flex/grid simples) e lista de desenho ----------
// Incluido por web/web.inc DENTRO de namespace mod_monitor. O layout gera "ops" em coordenadas do documento; o rasterizador (raster.inc) pinta.
namespace pw {
struct DOp { uint8_t t = 0; float x = 0, y = 0, w = 0, h = 0; uint32_t col = 0; std::string s; int scale = 1; bool bold = false, italic = false, ul = false, strike = false; int img = -1; };   // t: 0 retangulo, 1 texto, 2 imagem
struct HitR { float x, y, w, h; int link = -1, node = -1; };
struct InInfo { float x, y, w, h; int scale; };
struct Item { int k = 0; std::string s; float w = 0, h = 0, asc = 0; int sty = -1, link = -1, node = -1; size_t o0 = 0, o1 = 0, h0 = 0, h1 = 0; bool nobreak = false; int ctl = -1; };   // k: 0 palavra, 1 espaco, 2 atomico, 3 quebra, 4 espaco de moldura
static const char* UA_CSS =
    "html,body,div,section,article,aside,nav,header,footer,main,address,blockquote,center,dd,dl,dt,fieldset,figure,figcaption,form,h1,h2,h3,h4,h5,h6,hr,menu,ol,p,pre,ul,details,summary,legend,hgroup,dir,noscript,optgroup,search{display:block}"
    "head,script,style,title,meta,link,base,template,area,datalist,param,source,track,noembed,rp,dialog,svg,canvas,audio,video,iframe,object,embed,map,option,[hidden],input[type=hidden],picture source{display:none}"
    "li{display:list-item}table{display:table;border-spacing:2px}tr{display:table-row}td,th{display:table-cell;padding:1px}thead,tbody,tfoot{display:table-row-group}caption{display:table-caption;text-align:center}"
    "img,input,button,select,textarea,br{display:inline-block}img{max-width:100%}body{margin:8px}h1{font-size:2em;margin:.67em 0}h2{font-size:1.5em;margin:.83em 0}h3{font-size:1.17em;margin:1em 0}h4{margin:1.33em 0}h5{font-size:.83em;margin:1.67em 0}h6{font-size:.67em;margin:2.33em 0}"
    "p,dl,ul,ol,pre,fieldset,figure{margin:1em 0}ul,ol,dir,menu{padding-left:40px}ul ul,ol ol,ul ol,ol ul{margin:0}blockquote{margin:1em 40px}dd{margin-left:40px}pre{white-space:pre}hr{margin:8px 0;border:1px solid #9a9a9a}"
    "b,strong,th,h1,h2,h3,h4,h5,h6,dt,legend{font-weight:bold}i,em,cite,var,dfn,address{font-style:italic}u,ins{text-decoration:underline}s,strike,del{text-decoration:line-through}small{font-size:.83em}big{font-size:1.17em}sub,sup{font-size:.75em}"
    "a:link{color:#1a0dab;text-decoration:underline}mark{background:#ff0}center,th,caption{text-align:center}th{text-align:center}ul{list-style-type:disc}ol{list-style-type:decimal}ul ul{list-style-type:circle}"
    "button{padding:2px 8px;border:1px solid #767676;background:#efefef;text-align:center}fieldset{border:1px solid #c0c0c0;padding:4px 8px}summary{display:block}";
static bool isInlineTag(const std::string& t) { return t == "img" || t == "input" || t == "button" || t == "select" || t == "textarea"; }
struct Eng {
    Doc doc; std::vector<Style> sty; Sheet ua, au; std::string base; std::vector<Img> imgs; std::unordered_map<std::string, int> imgMap; std::vector<int> imgWant;
    std::map<int, std::string> val; std::map<int, int> chk, selIdx;
    std::vector<DOp> ops; std::vector<HitR> hits; std::vector<std::string> links; std::unordered_map<std::string, float> anchors; std::map<int, InInfo> inputs;
    float lastOuterW = 0; float docH = 0; int vw = 480, vh = 480; uint32_t canvas = 0xFFFFFFFF; std::vector<float> cMax, cMin; std::vector<int8_t> bDesc; int curLink = -1, depth = 0; bool uaReady = false;

    // ---------- URLs ----------
    static std::string resolveUrl(const std::string& baseU, const std::string& ref0) {
        std::string ref = trim(ref0); if (ref.empty()) return baseU; if (ref.compare(0, 5, "data:") == 0) return ref;
        const size_t cs = ref.find(':'); if (cs != std::string::npos && cs > 0 && cs < 12 && ref.find_first_of("/?#") > cs) return ref;   // esquema proprio
        UrlParts b; if (!urlSplit(baseU, b)) return ref; const std::string org = b.scheme + "://" + b.host + ((b.port != 80 && b.port != 443) ? ":" + std::to_string(b.port) : "");
        if (ref.compare(0, 2, "//") == 0) return b.scheme + ":" + ref; if (ref[0] == '#') { std::string u = baseU; const size_t h = u.find('#'); if (h != std::string::npos) u.resize(h); return u + ref; }
        std::string path; if (ref[0] == '/') path = ref; else if (ref[0] == '?') { path = b.path; const size_t q = path.find('?'); if (q != std::string::npos) path.resize(q); path += ref; } else { std::string dir = b.path; const size_t q = dir.find('?'); if (q != std::string::npos) dir.resize(q); dir.resize(dir.rfind('/') + 1); path = dir + ref; }
        std::string q, frag; size_t h = path.find('#'); if (h != std::string::npos) { frag = path.substr(h); path.resize(h); } const size_t qq = path.find('?'); if (qq != std::string::npos) { q = path.substr(qq); path.resize(qq); }
        std::vector<std::string> seg; size_t i = 0; while (i <= path.size()) { size_t e = path.find('/', i); if (e == std::string::npos) e = path.size(); const std::string s = path.substr(i, e - i); if (s == "..") { if (!seg.empty()) seg.pop_back(); } else if (s != ".") seg.push_back(s); i = e + 1; }
        std::string out; for (size_t k = 0; k < seg.size(); k++) { if (k == 0 && seg[k].empty()) continue; out += "/" + seg[k]; } if (out.empty()) out = "/"; if (!path.empty() && path.back() == '/' && out.back() != '/') out += "/"; return org + out + q + frag;
    }
    // ---------- estilos ----------
    void presentational(const Node& nd, Style& st) {
        const std::string& t = nd.tag; uint32_t c; std::string v;
        if (!(v = attrS(nd, "bgcolor")).empty() && parseColor(v[0] == '#' || isalpha((unsigned char)v[0]) ? v : "#" + v, c)) st.bg = c;
        if (t == "body") { if (!(v = attrS(nd, "text")).empty() && parseColor(v[0] == '#' || isalpha((unsigned char)v[0]) ? v : "#" + v, c)) st.color = c; }
        if (t == "font") { if (!(v = attrS(nd, "color")).empty() && parseColor(v[0] == '#' || isalpha((unsigned char)v[0]) ? v : "#" + v, c)) st.color = c; if (!(v = attrS(nd, "size")).empty()) { static const float F[] = {10, 13, 16, 18, 24, 32, 48}; int k = atoi(v.c_str()); if (v[0] == '+' || v[0] == '-') k = 3 + k; st.fontPx = F[std::max(1, std::min(7, k)) - 1] * gZoom; } }
        auto dim = [&](const char* a, Len& L) { v = attrS(nd, a); if (v.empty()) return; Len x; if (v.back() == '%') { x.u = '%'; x.v = (float)atof(v.c_str()); L = x; } else if (isdigit((unsigned char)v[0])) { x.u = 'p'; x.v = (float)atof(v.c_str()) * gZoom; L = x; } };
        if (t == "table" || t == "td" || t == "th" || t == "img" || t == "hr" || t == "col") { if (st.width.u == 'a') dim("width", st.width); if (st.height.u == 'a') dim("height", st.height); }
        if (!(v = attrS(nd, "align")).empty() && (t == "div" || t == "p" || t == "td" || t == "th" || t == "tr" || t == "h1" || t == "h2" || t == "h3" || t == "h4" || t == "table" || t == "caption")) { v = lower(v); if (t == "table" && (v == "center")) { st.margin[1].u = 'a'; st.margin[3].u = 'a'; st.margin[1].v = st.margin[3].v = 0; st.margin[1].u = st.margin[3].u = 'a'; st.align = 1; } else st.align = v == "center" ? 1 : v == "right" ? 2 : 0; }
        if (t == "img" && !(v = lower(attrS(nd, "align"))).empty()) { if (v == "left") st.floatv = 1; else if (v == "right") st.floatv = 2; }
        if ((t == "td" || t == "th") && lower(attrS(nd, "valign")) == "middle") st.valign = 1; if ((t == "td" || t == "th") && lower(attrS(nd, "valign")) == "bottom") st.valign = 2;
        if (t == "table") { v = attrS(nd, "border"); if (!v.empty() && atoi(v.c_str()) > 0) { for (float& b : st.border) b = 1; for (uint32_t& k : st.bcol) k = 0xFF808080; } v = attrS(nd, "cellspacing"); if (!v.empty()) {} }
        if (t == "td" || t == "th") { /* cellpadding e borda da tabela sao aplicados em computeStyles (precisa do pai) */ }
        if (attrOf(nd, "nowrap") && (t == "td" || t == "th")) st.white = 2;
        if (t == "center") st.align = 1;
    }
    void computeStyles() {
        if (!uaReady) { ua = Sheet(); parseSheet(UA_CSS, ua, 480); ua.index(); uaReady = true; }
        sty.assign(doc.n.size(), Style()); Style rootSt; rootSt.fontPx = 16 * gZoom; rootSt.vars = std::make_shared<std::map<std::string, std::string>>(); const float rootPx = 16;
        std::vector<int> st{0}; sty[0] = rootSt; sty[0].display = 1; std::vector<std::pair<int, int>> stack;   // (no, pai) iterativo
        for (int k = (int)doc.n[0].kids.size() - 1; k >= 0; k--) stack.push_back({doc.n[0].kids[k], 0});
        std::vector<const Rule*> cand; std::vector<std::tuple<int, int, const Rule*>> m;
        while (!stack.empty()) {
            const int n = stack.back().first, par = stack.back().second; stack.pop_back(); const Node& nd = doc.n[n]; const Style& ps = sty[par];
            if (nd.isText) { sty[n] = ps; continue; }
            Style s; s.fontPx = ps.fontPx; s.color = ps.color; s.weight = ps.weight; s.italic = ps.italic; s.ul = false; s.align = ps.align; s.lineH = ps.lineH; s.white = ps.white; s.hidden = ps.hidden; s.transform = ps.transform; s.list = ps.list; s.vars = ps.vars; s.ul = ps.ul && false; s.strike = false; s.opacity = ps.opacity;
            if (par > 0 || true) { s.ul = ps.ul; s.strike = ps.strike; }   // decoracao propaga visualmente
            s.display = 2; presentational(nd, s);
            if (nd.tag == "td" || nd.tag == "th") { int t = nd.parent; while (t > 0 && doc.n[t].tag != "table") t = doc.n[t].parent; if (t > 0) { const std::string cp = attrS(doc.n[t], "cellpadding"); if (!cp.empty() && isdigit((unsigned char)cp[0])) for (Len& L : s.padding) { L.u = 'p'; L.v = (float)atof(cp.c_str()) * gZoom; } const std::string bd = attrS(doc.n[t], "border"); if (!bd.empty() && atoi(bd.c_str()) > 0) { for (float& b : s.border) b = 1; for (uint32_t& c2 : s.bcol) c2 = 0xFF808080; } } }
            m.clear(); auto gather = [&](const Sheet& sh, int origin) { auto addList = [&](const std::vector<int>& v) { for (int ri : v) { const Rule& r = sh.rules[ri]; if (matchSel(doc, n, r.sel, (int)r.sel.c.size() - 1)) m.push_back({origin * 100000000 + r.sel.spec * 10000 + (r.order > 9999 ? 9999 : r.order), 0, &r}); } };
                const std::string id = attrS(nd, "id"); if (!id.empty()) { auto it = sh.byId.find(id); if (it != sh.byId.end()) addList(it->second); } const std::string* cl = attrOf(nd, "class"); if (cl) { size_t i = 0; while (i < cl->size()) { while (i < cl->size() && isspace((unsigned char)(*cl)[i])) i++; size_t j = i; while (j < cl->size() && !isspace((unsigned char)(*cl)[j])) j++; if (j > i) { auto it = sh.byCls.find(cl->substr(i, j - i)); if (it != sh.byCls.end()) addList(it->second); } i = j; } }
                { auto it = sh.byTag.find(nd.tag); if (it != sh.byTag.end()) addList(it->second); } addList(sh.any); };
            gather(ua, 0); gather(au, 1); std::sort(m.begin(), m.end(), [](const std::tuple<int, int, const Rule*>& a, const std::tuple<int, int, const Rule*>& b) { return std::get<0>(a) < std::get<0>(b); });
            std::vector<Decl> inl; { const std::string* sa = attrOf(nd, "style"); if (sa) parseDecls(*sa, inl); }
            std::vector<const Decl*> ord; for (auto& x : m) for (const Decl& d : std::get<2>(x)->d) if (!d.imp) ord.push_back(&d); for (const Decl& d : inl) if (!d.imp) ord.push_back(&d); for (auto& x : m) for (const Decl& d : std::get<2>(x)->d) if (d.imp) ord.push_back(&d); for (const Decl& d : inl) if (d.imp) ord.push_back(&d);
            bool hasVar = false; for (const Decl* d : ord) if (d->p.compare(0, 2, "--") == 0) { if (!hasVar) { s.vars = std::make_shared<std::map<std::string, std::string>>(*ps.vars); hasVar = true; } (*s.vars)[d->p] = d->v; }
            for (const Decl* d : ord) { if (d->p.compare(0, 2, "--") == 0) continue; applyDecl(s, ps, d->p, d->v.find("var(") != std::string::npos ? subVars(d->v, s) : d->v, rootPx); }
            if (nd.tag == "html" || nd.tag == "body") { if (s.display == 2) s.display = 1; }
            if (nd.tag != "summary" && nd.parent > 0 && doc.n[nd.parent].tag == "details" && !attrOf(doc.n[nd.parent], "open")) s.display = 0;
            if (s.fontPx < 1) s.display = 0; if (s.opacity <= 0.01f) s.display = 0;
            if ((s.pos == 1 || s.pos == 2) && s.left.u == 'p' && s.left.v < -800) s.display = 0; if ((s.pos == 1 || s.pos == 2) && s.top.u == 'p' && s.top.v < -800) s.display = 0;
            if (s.ovHidden && s.width.u == 'p' && s.height.u == 'p' && s.width.v <= 2 && s.height.v <= 2) s.display = 0; if (s.ovHidden && s.height.u == 'p' && s.height.v <= 0 && s.display != 0 && nd.tag != "td") s.display = 0;
            if (s.pos == 2 && s.display == 1 && (s.height.u == 'p' || s.width.u == '%') && s.bg != 0 && (nd.tag == "div" || nd.tag == "section" || nd.tag == "aside") && s.top.u != 'a' && s.top.v == 0 && false) s.display = 0;
            if (s.display == 3 && (nd.tag == "img")) s.display = 3; sty[n] = s;
            for (int k = (int)nd.kids.size() - 1; k >= 0; k--) stack.push_back({nd.kids[k], n});
        }
        // fundo da pagina: html, senao body
        canvas = 0xFFFFFFFF; for (int k : doc.n[0].kids) if (doc.n[k].tag == "html") { if ((sty[k].bg >> 24) > 0) canvas = sty[k].bg | 0xFF000000u; else for (int b : doc.n[k].kids) if (doc.n[b].tag == "body" && (sty[b].bg >> 24) > 0) canvas = sty[b].bg | 0xFF000000u; }
        if (doc.n[0].kids.size() && doc.n[doc.n[0].kids[0]].tag != "html") for (int k : doc.n[0].kids) if (doc.n[k].tag == "body" && (sty[k].bg >> 24) > 0) canvas = sty[k].bg | 0xFF000000u;
    }
    // ---------- medidas ----------
    const Style& S(int n) const { return sty[n]; }
    static int sc(const Style& s) { return fontScale(s.fontPx); }
    static float tw(const std::string& t, int scl) { float w = 0; for (const char* p = t.c_str(); *p;) { int f, a; glyphMetrics(u8next(p), f, a); w += (float)a; } return w * (float)scl; }
    static float lhOf(const Style& s) { const int c = fontScale(s.fontPx); if (s.lineH > 0) return std::max((float)(8 * c), s.lineH * (8.f * c / std::max(1.f, s.fontPx))); return 10.f * c; }
    float px(const Len& l, float base, float d = 0) const { return lenPx(l, base, d); }
    bool blockLevel(int n) { const Node& nd = doc.n[n]; if (nd.isText) return false; const int d = S(n).display; if (d == 0) return false; if (d == 1 || d == 4 || d == 5 || d == 7 || d == 8 || d == 9 || d == 10 || d == 11) return true; if (d == 2 && hasBlockDesc(n)) return true; return false; }
    bool hasBlockDesc(int n) { if (bDesc.size() != doc.n.size()) bDesc.assign(doc.n.size(), -1); if (bDesc[n] >= 0) return bDesc[n]; bool r = false; for (int k : doc.n[n].kids) { const Node& c = doc.n[k]; if (c.isText) continue; const int d = S(k).display; if (d == 0) continue; if (d == 1 || d == 4 || d == 5 || d == 7 || d == 11 || (d == 2 && hasBlockDesc(k))) { r = true; break; } } bDesc[n] = r; return r; }
    float extraH(int n, float base) { const Style& s = S(n); return px(s.padding[1], base) + px(s.padding[3], base) + s.border[1] + s.border[3] + (s.margin[1].u == 'p' ? s.margin[1].v : 0) + (s.margin[3].u == 'p' ? s.margin[3].v : 0); }
    std::string xform(const std::string& t, const Style& s) { if (!s.transform) return t; std::string o = t; bool st = true; for (char& c : o) { if (s.transform == 1) c = (char)toupper((unsigned char)c); else if (s.transform == 2) c = (char)tolower((unsigned char)c); else { c = st ? (char)toupper((unsigned char)c) : c; st = isspace((unsigned char)c); } } return o; }
    // largura de conteudo natural (max) e minima (min), em px, incluindo moldura do proprio elemento
    float contentMax(int n, int dep = 0); float contentMin(int n, int dep = 0);
    float runWidth(const std::vector<int>& run, bool mn, int dep) {   // largura de uma sequencia de itens em linha
        float cur = 0, best = 0;
        for (int c : run) { const Node& nd = doc.n[c]; if (nd.isText) { const Style& s = S(c); const int k = sc(s); std::string t = xform(nd.text, s); if (s.white == 1 || s.white == 3) { size_t i = 0; while (i <= t.size()) { size_t e = t.find('\n', i); if (e == std::string::npos) e = t.size(); const float w = tw(t.substr(i, e - i), k); cur += w; best = std::max(best, cur); if (e < t.size()) cur = 0; i = e + 1; } continue; }
                size_t i = 0; bool lastSp = false; while (i < t.size()) { if (isspace((unsigned char)t[i])) { if (mn && s.white != 2) { best = std::max(best, cur); cur = 0; } else if (!lastSp) cur += tw(" ", k); lastSp = true; i++; continue; } size_t j = i; while (j < t.size() && !isspace((unsigned char)t[j])) j++; cur += tw(t.substr(i, j - i), k); lastSp = false; i = j; } if (!mn) best = std::max(best, cur); else best = std::max(best, cur); continue; }
            const Style& s = S(c); if (s.display == 0) continue; if (nd.tag == "br") { best = std::max(best, cur); cur = 0; continue; }
            if (s.display == 2 && !isInlineTag(nd.tag)) { std::vector<int> sub; for (int k : nd.kids) sub.push_back(k); const float w = runWidth(sub, mn, dep + 1) + extraH(c, 0); if (mn) best = std::max(best, w); else { cur += w; best = std::max(best, cur); } continue; }
            const float w = mn ? contentMin(c, dep + 1) : contentMax(c, dep + 1); if (mn) best = std::max(best, w); else { cur += w; best = std::max(best, cur); } }
        return std::max(best, cur);
    }
    float controlSize(int n, float& w, float& h);
    // ---------- imagens ----------
    std::vector<std::string> imgUrls;   // id -> url
    int imgOf(const std::string& url) { auto it = imgMap.find(url); if (it != imgMap.end()) return it->second; imgs.push_back(Img()); imgUrls.push_back(url); const int id = (int)imgs.size() - 1; imgMap[url] = id; if (url.compare(0, 5, "data:") == 0) decodeDataImg(url, imgs[id]); else imgWant.push_back(id); return id; }
        // ---------- desenho ----------
    void rect(float x, float y, float w, float h, uint32_t c) { if (w <= 0 || h <= 0 || (c >> 24) == 0) return; DOp o; o.t = 0; o.x = x; o.y = y; o.w = w; o.h = h; o.col = c; ops.push_back(o); }
    void textOp(float x, float y, const std::string& s, int scl, uint32_t c, bool bold, bool it, bool ul = false, bool st = false) { DOp o; o.t = 1; o.x = x; o.y = y; o.s = s; o.scale = scl; o.col = c; o.bold = bold; o.italic = it; o.ul = ul; o.strike = st; ops.push_back(o); }
    void shift(size_t o0, size_t o1, size_t h0, size_t h1, float dx, float dy) { for (size_t i = o0; i < o1 && i < ops.size(); i++) { ops[i].x += dx; ops[i].y += dy; } for (size_t i = h0; i < h1 && i < hits.size(); i++) { hits[i].x += dx; hits[i].y += dy; } }
    void shiftInputs(float dx, float dy, std::vector<int>& ids) { for (int id : ids) { auto it = inputs.find(id); if (it != inputs.end()) { it->second.x += dx; it->second.y += dy; } } }
    int linkIdx(int n) { const std::string h = trim(attrS(doc.n[n], "href")); if (h.empty()) return -1; links.push_back(h[0] == '#' && h.size() == 1 ? std::string("#") : resolveUrl(base, h)); return (int)links.size() - 1; }
    void addHit(float x, float y, float w, float h, int link, int node) { if (link < 0 && node < 0) return; if (link >= 0 && node < 0 && !hits.empty()) { HitR& l = hits.back(); if (l.link == link && l.node < 0 && fabsf(l.y - y) < .5f && fabsf(l.x + l.w - x) < 1.5f && fabsf(l.h - h) < .5f) { l.w = x + w - l.x; return; } } HitR r; r.x = x; r.y = y; r.w = w; r.h = h; r.link = link; r.node = node; hits.push_back(r); }
    // ---------- controles de formulario ----------
    std::string valueOf(int n) { auto it = val.find(n); if (it != val.end()) return it->second; const Node& nd = doc.n[n]; if (nd.tag == "textarea") { std::string t; for (int k : nd.kids) if (doc.n[k].isText) t += doc.n[k].text; if (!t.empty() && t[0] == '\n') t.erase(0, 1); return t; } return attrS(nd, "value"); }
    std::string inType(int n) { std::string t = lower(attrS(doc.n[n], "type")); return t.empty() ? "text" : t; }
    bool checked(int n) { auto it = chk.find(n); if (it != chk.end()) return it->second > 0; return attrOf(doc.n[n], "checked") != nullptr; }
    std::vector<int> optionsOf(int sel) { std::vector<int> o; std::vector<int> st{sel}; while (!st.empty()) { const int x = st.back(); st.pop_back(); for (int i = (int)doc.n[x].kids.size() - 1; i >= 0; i--) { const int k = doc.n[x].kids[i]; if (doc.n[k].tag == "option") o.insert(o.begin(), k); else if (doc.n[k].tag == "optgroup") st.push_back(k); } } std::vector<int> r; for (int k : doc.n[sel].kids) { if (doc.n[k].tag == "option") r.push_back(k); else if (doc.n[k].tag == "optgroup") for (int j : doc.n[k].kids) if (doc.n[j].tag == "option") r.push_back(j); } return r; }
    std::string optText(int o) { std::string t; for (int k : doc.n[o].kids) if (doc.n[k].isText) t += doc.n[k].text; std::string r; bool sp = false; for (char c : t) { if (isspace((unsigned char)c)) { sp = true; continue; } if (sp && !r.empty()) r += ' '; sp = false; r += c; } return r; }
    int selIndex(int sel) { auto it = selIdx.find(sel); if (it != selIdx.end()) return it->second; std::vector<int> o = optionsOf(sel); for (size_t i = 0; i < o.size(); i++) if (attrOf(doc.n[o[i]], "selected")) return (int)i; return 0; }
    std::string textOf(int n) { std::string t; std::vector<int> st{n}; while (!st.empty()) { const int x = st.back(); st.pop_back(); for (int i = (int)doc.n[x].kids.size() - 1; i >= 0; i--) { const int k = doc.n[x].kids[i]; if (doc.n[k].isText) { if (doc.n[x].tag != "script" && doc.n[x].tag != "style") t = t; } else st.push_back(k); } } std::string o; std::vector<int> ord; std::function<void(int)> rec = [&](int x) { for (int k : doc.n[x].kids) { if (doc.n[k].isText) o += doc.n[k].text; else if (doc.n[k].tag != "script" && doc.n[k].tag != "style") rec(k); } }; rec(n); std::string r; bool sp = false; for (char c : o) { if (isspace((unsigned char)c)) { sp = true; continue; } if (sp && !r.empty()) r += ' '; sp = false; r += c; } return r; }
    std::string btnLabel(int n) { const Node& nd = doc.n[n]; if (nd.tag == "input") { std::string v = valueOf(n); if (v.empty()) { const std::string t = inType(n); v = t == "reset" ? "Limpar" : t == "file" ? "Escolher arquivo" : t == "image" ? attrS(nd, "alt") : "Enviar"; } return v; } return textOf(n); }
    float flowLen(const Len& l, float base) { return l.u == 'a' ? -1 : px(l, base); }
    // implementados abaixo
    float block(int n, float x, float y, float availW, float* mbOut, float forceBW = -1, int* slotOut = nullptr);
    float flow(int n, float cx, float cy, float cw, float* mbOut, bool skipFirstGap);
    float inlineRun(std::vector<Item>& it, float x, float y, float w, const Style& cs);
    void collect(const std::vector<int>& run, std::vector<Item>& out, float availW, int dep);
    void atomic(int n, Item& it, float availW, int dep);
    float table(int n, float cx, float cy, float cw, float tableW);
    float flex(int n, float cx, float cy, float cw);
    float effMT(int n, float base);
    void patchBox(int slot, float newH);
    void layoutAll(); 
};
}   // namespace pw
