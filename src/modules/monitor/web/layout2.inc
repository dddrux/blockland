// ---------- PIE Web: implementacao do layout ----------
namespace pw {
struct TInfo { std::vector<std::vector<int>> rows; std::vector<int> rowNode; std::vector<std::vector<int>> col; int ncols = 0; std::vector<float> cmin, cmax; int caption = -1; };
static int cellSpan(const Doc& d, int c) { const int v = atoi(attrS(d.n[c], "colspan").c_str()); return std::max(1, std::min(30, v)); }
float Eng::controlSize(int n, float& w, float& h) {
    const Node& nd = doc.n[n]; const Style& s = S(n); const int k = sc(s); const float lh = lhOf(s); w = h = 0; const std::string& t = nd.tag;
    if (t == "input") { const std::string ty = inType(n); if (ty == "hidden") return 0; if (ty == "checkbox" || ty == "radio") { w = h = std::max(11.f, 6.5f * k); return 1; }
        if (ty == "submit" || ty == "button" || ty == "reset" || ty == "file" || ty == "image") { w = tw(btnLabel(n), k) + 16 * gZoom; h = lh + 8 * gZoom; return 1; }
        const int size = std::max(4, std::min(80, atoi(attrS(nd, "size").c_str()) ? atoi(attrS(nd, "size").c_str()) : 20)); w = size * 6.f * k + 10 * gZoom; h = lh + 8 * gZoom; if (s.width.u == 'p') w = s.width.v; return 1; }
    if (t == "select") { float mx = 0; for (int o : optionsOf(n)) mx = std::max(mx, tw(optText(o), k)); w = mx + 26 * gZoom + 8; h = lh + 8 * gZoom; return 1; }
    if (t == "textarea") { const int cols = std::max(4, std::min(120, atoi(attrS(nd, "cols").c_str()) ? atoi(attrS(nd, "cols").c_str()) : 20)), rows = std::max(2, std::min(30, atoi(attrS(nd, "rows").c_str()) ? atoi(attrS(nd, "rows").c_str()) : 3)); w = cols * 6.f * k + 10 * gZoom; h = rows * lh + 8 * gZoom; if (s.width.u == 'p') w = s.width.v; return 1; }
    return 0;
}
static std::vector<int> elemKids(const Doc& d, int n) { std::vector<int> o; for (int k : d.n[n].kids) if (!d.n[k].isText) o.push_back(k); return o; }
static TInfo tinfo(const Eng& e, int n) {
    TInfo T; std::vector<int> st{n}; std::function<void(int)> rec = [&](int x) { for (int k : e.doc.n[x].kids) { if (e.doc.n[k].isText) continue; const int dsp = e.S(k).display; if (dsp == 0) continue; if (dsp == 8) { std::vector<int> cells; for (int c : e.doc.n[k].kids) if (!e.doc.n[c].isText && e.S(c).display == 9) cells.push_back(c); T.rows.push_back(cells); T.rowNode.push_back(k); } else if (dsp == 10) rec(k); else if (dsp == 11 && T.caption < 0) T.caption = k; else if (dsp == 9) { if (T.rows.empty() || T.rowNode.back() != -2) { T.rows.push_back({}); T.rowNode.push_back(-2); } T.rows.back().push_back(k); } } };
    rec(n); for (auto& r : T.rows) { int c = 0; std::vector<int> cs; for (int cell : r) { cs.push_back(c); c += cellSpan(e.doc, cell); } T.col.push_back(cs); T.ncols = std::max(T.ncols, c); } return T;
}
float Eng::contentMax(int n, int dep) {
    if (dep > 60) return 0; if (cMax.size() != doc.n.size()) { cMax.assign(doc.n.size(), -1); cMin.assign(doc.n.size(), -1); } if (cMax[n] >= 0) return cMax[n];
    const Node& nd = doc.n[n]; const Style& s = S(n); float r = 0;
    if (s.display == 0) r = 0; else if (nd.tag == "img") { r = 0; Len w = s.width; float iw = 0; if (w.u == 'p') iw = w.v; else { iw = (float)atoi(attrS(nd, "width").c_str()) * gZoom; if (iw <= 0) { const std::string src = attrS(nd, "src"); if (!src.empty()) { const int id = imgOf(resolveUrl(base, src)); if (imgs[id].state == 2) iw = (float)imgs[id].w * gZoom; else iw = 16; } } } r = iw + extraH(n, 0); }
    else { float cw, ch; if (controlSize(n, cw, ch)) r = cw + extraH(n, 0); else {
        float inner = 0;
        if (s.display == 7) { TInfo T = tinfo(*this, n); std::vector<float> cm(T.ncols + 1, 0); for (size_t i = 0; i < T.rows.size(); i++) for (size_t j = 0; j < T.rows[i].size(); j++) { const int c = T.rows[i][j]; const int sp = cellSpan(doc, c); const float w = contentMax(c, dep + 1); const float per = w / sp; for (int q = 0; q < sp; q++) cm[T.col[i][j] + q] = std::max(cm[T.col[i][j] + q], per); } const float spc = s.spacing >= 0 ? s.spacing : 2 * gZoom; for (int q = 0; q < T.ncols; q++) inner += cm[q]; inner += spc * (T.ncols + 1); }
        else if (s.display == 5 || s.display == 6) { const bool row = s.dir != 1; for (int c : elemKids(doc, n)) { if (S(c).display == 0) continue; const float w = contentMax(c, dep + 1); if (row) inner += w + s.gap; else inner = std::max(inner, w); } if (row && inner > 0) inner -= s.gap; }
        else { std::vector<int> run; for (int c : doc.n[n].kids) { if (!doc.n[c].isText && blockLevel(c)) { if (!run.empty()) { inner = std::max(inner, runWidth(run, false, dep)); run.clear(); } inner = std::max(inner, contentMax(c, dep + 1)); } else run.push_back(c); } if (!run.empty()) inner = std::max(inner, runWidth(run, false, dep)); }
        if (s.width.u == 'p') { inner = s.width.v - (s.bsBorder ? extraH(n, 0) - (s.margin[1].u == 'p' ? s.margin[1].v : 0) - (s.margin[3].u == 'p' ? s.margin[3].v : 0) : 0); if (inner < 0) inner = 0; }
        r = inner + extraH(n, 0); } }
    return cMax[n] = r;
}
float Eng::contentMin(int n, int dep) {
    if (dep > 60) return 0; if (cMin.size() != doc.n.size()) { cMax.assign(doc.n.size(), -1); cMin.assign(doc.n.size(), -1); } if (cMin[n] >= 0) return cMin[n];
    const Node& nd = doc.n[n]; const Style& s = S(n); float r = 0;
    if (s.display == 0) r = 0; else if (nd.tag == "img") { r = std::min(contentMax(n, dep + 1), 24.f * gZoom); if (s.width.u == 'p') r = s.width.v + extraH(n, 0); }
    else { float cw, ch; if (controlSize(n, cw, ch)) r = cw + extraH(n, 0); else {
        float inner = 0;
        if (s.display == 7) { TInfo T = tinfo(*this, n); std::vector<float> cm(T.ncols + 1, 0); for (size_t i = 0; i < T.rows.size(); i++) for (size_t j = 0; j < T.rows[i].size(); j++) { const int c = T.rows[i][j]; const int sp = cellSpan(doc, c); const float w = contentMin(c, dep + 1) / sp; for (int q = 0; q < sp; q++) cm[T.col[i][j] + q] = std::max(cm[T.col[i][j] + q], w); } for (int q = 0; q < T.ncols; q++) inner += cm[q]; inner += (s.spacing >= 0 ? s.spacing : 2 * gZoom) * (T.ncols + 1); }
        else if (s.display == 5 || s.display == 6) { const bool row = s.dir != 1 && !s.wrap && s.gridCols == 0 && s.gridMin == 0; for (int c : elemKids(doc, n)) { if (S(c).display == 0) continue; const float w = contentMin(c, dep + 1); if (row) inner += w + s.gap; else inner = std::max(inner, w); } if (row && inner > 0) inner -= s.gap; }
        else { std::vector<int> run; for (int c : doc.n[n].kids) { if (!doc.n[c].isText && blockLevel(c)) { if (!run.empty()) { inner = std::max(inner, runWidth(run, true, dep)); run.clear(); } inner = std::max(inner, contentMin(c, dep + 1)); } else run.push_back(c); } if (!run.empty()) inner = std::max(inner, runWidth(run, true, dep)); }
        if (s.width.u == 'p') inner = std::max(0.f, s.width.v - (s.bsBorder ? extraH(n, 0) : 0)); r = inner + extraH(n, 0); } }
    return cMin[n] = r;
}
static bool collapsibleBox(const Eng& e, int n, float cw) { const Style& s = e.S(n); return (s.display == 1 || s.display == 4) && !s.ovHidden && s.floatv == 0 && s.border[0] == 0 && lenPx(s.padding[0], cw) == 0 && s.height.u == 'a' && s.pos != 1 && s.pos != 2; }
float Eng::effMT(int n, float base) {
    const Style& s = S(n); float mt = s.margin[0].u == 'p' ? s.margin[0].v : s.margin[0].u == '%' ? base * s.margin[0].v / 100.f : 0;
    if (collapsibleBox(*this, n, base)) for (int c : doc.n[n].kids) { const Node& cn = doc.n[c]; if (cn.isText) { bool ws = true; for (char ch : cn.text) if (!isspace((unsigned char)ch)) ws = false; if (ws) continue; break; } if (S(c).display == 0) continue; if (blockLevel(c) && (S(c).display == 1 || S(c).display == 4)) { const float cm = effMT(c, base); mt = (mt < 0 || cm < 0) ? mt + cm : std::max(mt, cm); } break; }
    return mt;
}
void Eng::patchBox(int slot, float h) { if (slot < 0 || slot + 4 >= (int)ops.size()) return; DOp& bg = ops[slot]; bg.h = h; ops[slot + 2].h = h; ops[slot + 4].h = h; DOp& bt = ops[slot + 3]; bt.y = bg.y + h - bt.h; }
void Eng::collect(const std::vector<int>& run, std::vector<Item>& out, float availW, int dep) {
    for (int c : run) {
        const Node& nd = doc.n[c]; const Style& s = S(c);
        if (nd.isText) {
            const int k = sc(s); const float lh = lhOf(s); const float asc = (lh - 8 * k) / 2 + 7 * k; std::string t = xform(nd.text, s); const float spw = tw(" ", k);
            auto mk = [&](int kind, const std::string& str, float w) { Item it; it.k = kind; it.s = str; it.w = w; it.h = lh; it.asc = asc; it.sty = c; it.link = curLink; out.push_back(it); };
            if (s.white == 1 || s.white == 3) { std::string w; for (char ch : t) { if (ch == '\n') { if (!w.empty()) { mk(0, w, tw(w, k)); w.clear(); } mk(3, "", 0); } else if (ch == ' ' || ch == '\t' || ch == '\r') { if (!w.empty()) { mk(0, w, tw(w, k)); w.clear(); } const int n2 = ch == '\t' ? 4 : ch == '\r' ? 0 : 1; for (int q = 0; q < n2; q++) mk(1, " ", spw); } else w += ch; } if (!w.empty()) mk(0, w, tw(w, k)); continue; }
            if (s.white == 2) { std::string o; bool sp = false; for (char ch : t) { if (isspace((unsigned char)ch)) { sp = true; continue; } if (sp && !o.empty()) o += ' '; sp = false; o += ch; } const bool lead = !t.empty() && isspace((unsigned char)t[0]), trail = !t.empty() && isspace((unsigned char)t.back()); if (lead) mk(1, " ", spw); if (!o.empty()) { mk(0, o, tw(o, k)); out.back().nobreak = true; } if (trail && !o.empty()) mk(1, " ", spw); continue; }
            size_t i = 0; while (i < t.size()) { if (isspace((unsigned char)t[i])) { while (i < t.size() && isspace((unsigned char)t[i])) i++; if (!out.empty() && out.back().k == 1 && out.back().sty == c) continue; mk(1, " ", spw); continue; } size_t j = i; while (j < t.size() && !isspace((unsigned char)t[j])) j++; const std::string w = t.substr(i, j - i); mk(0, w, tw(w, k)); i = j; }
            continue; }
        if (s.display == 0) continue;
        if (nd.tag == "br") { Item it; it.k = 3; it.h = lhOf(s); it.asc = (it.h - 8 * sc(s)) / 2 + 7 * sc(s); it.sty = c; out.push_back(it); continue; }
        if (s.display == 2 && !isInlineTag(nd.tag)) {
            const int saved = curLink; if (nd.tag == "a" && attrOf(nd, "href")) curLink = linkIdx(c); if (!attrS(nd, "id").empty()) anchors[attrS(nd, "id")] = -1;   // posicao so e conhecida ao posicionar
            const float ml = s.margin[3].u == 'p' ? s.margin[3].v : 0, mr = s.margin[1].u == 'p' ? s.margin[1].v : 0; const float lw = ml + px(s.padding[3], availW) + s.border[3], rw = mr + px(s.padding[1], availW) + s.border[1];
            if (lw > 0) { Item it; it.k = 4; it.w = lw; it.sty = c; it.link = curLink; it.h = lhOf(s); it.asc = (it.h - 8 * sc(s)) / 2 + 7 * sc(s); out.push_back(it); }
            if (!attrS(nd, "id").empty()) { Item it; it.k = 4; it.w = 0; it.sty = c; it.node = c; it.h = 0; it.asc = 0; out.push_back(it); }
            collect(nd.kids, out, availW, dep + 1);
            if (rw > 0) { Item it; it.k = 4; it.w = rw; it.sty = c; it.link = curLink; it.h = lhOf(s); it.asc = (it.h - 8 * sc(s)) / 2 + 7 * sc(s); out.push_back(it); } curLink = saved; continue; }
        Item it; it.k = 2; it.link = curLink; it.sty = c; it.node = c; atomic(c, it, availW, dep + 1); if (it.w > 0 || it.h > 0) out.push_back(it);
    }
}
void Eng::atomic(int n, Item& it, float availW, int dep) {
    const Node& nd = doc.n[n]; const Style& s = S(n); it.o0 = ops.size(); it.h0 = hits.size(); float w = 0, h = 0; const int k = sc(s);
    if (nd.tag == "img") {
        const std::string src = attrS(nd, "src").empty() ? attrS(nd, "data-src") : attrS(nd, "src"); int id = src.empty() ? -1 : imgOf(resolveUrl(base, src)); const Img* im = id >= 0 ? &imgs[id] : nullptr; const bool ok = im && im->state == 2 && im->w > 0;
        float aw = s.width.u == 'p' ? s.width.v : s.width.u == '%' ? availW * s.width.v / 100.f : (float)atoi(attrS(nd, "width").c_str()) * gZoom; float ah = s.height.u == 'p' ? s.height.v : (float)atoi(attrS(nd, "height").c_str()) * gZoom;
        const float nw = ok ? (float)im->w * gZoom : 0, nh = ok ? (float)im->h * gZoom : 0;
        if (aw > 0 && ah <= 0) ah = ok ? aw * nh / nw : aw * .75f; else if (ah > 0 && aw <= 0) aw = ok ? ah * nw / nh : ah; else if (aw <= 0 && ah <= 0) { if (ok) { aw = nw; ah = nh; } else { aw = ah = 16 * gZoom; } }
        const float maxw = std::max(8.f, availW - extraH(n, availW)); if (aw > maxw) { ah *= maxw / aw; aw = maxw; } if (s.maxW.u == 'p' && aw > s.maxW.v) { ah *= s.maxW.v / aw; aw = s.maxW.v; } w = aw; h = std::max(1.f, ah);
        if (ok) { DOp o; o.t = 2; o.x = 0; o.y = 0; o.w = w; o.h = h; o.img = id; ops.push_back(o); }
        else { const bool dead = im && im->state == 3; if (dead || attrOf(nd, "alt")) { rect(0, 0, w, h, dead ? 0xFFF0F0F0 : 0x00000000); const float b = 1; rect(0, 0, w, b, 0xFFC0C0C0); rect(0, h - b, w, b, 0xFFC0C0C0); rect(0, 0, b, h, 0xFFC0C0C0); rect(w - b, 0, b, h, 0xFFC0C0C0); const std::string alt = attrS(nd, "alt"); if (!alt.empty() && w > 30 && h >= 10) { const int a = fontScale(11 * gZoom); std::string tx = alt; while (tx.size() > 1 && tw(tx, a) > w - 6) tx.pop_back(); textOp(3, std::max(1.f, (h - 8 * a) / 2), tx, a, 0xFF707070, false, false); } } else rect(0, 0, w, h, 0xFFF4F4F4); }
        it.w = w + (s.margin[3].u == 'p' ? s.margin[3].v : 0) + (s.margin[1].u == 'p' ? s.margin[1].v : 0); it.h = h; it.asc = h; if (s.margin[3].u == 'p' && s.margin[3].v != 0) shift(it.o0, ops.size(), it.h0, hits.size(), s.margin[3].v, 0);
        if (it.link >= 0) addHit(0, 0, w, h, it.link, -1); it.o1 = ops.size(); it.h1 = hits.size(); return; }
    float cw, ch; if (controlSize(n, cw, ch)) {
        if (cw <= 0) { it.w = it.h = 0; it.o1 = ops.size(); it.h1 = hits.size(); return; } w = cw; h = ch; const std::string ty = nd.tag == "input" ? inType(n) : nd.tag; const float lh = lhOf(s);
        const bool focus = false; (void)focus; uint32_t bd = 0xFF767676; const uint32_t fg = s.color | 0xFF000000u;
        if (ty == "checkbox" || ty == "radio") { rect(0, 0, w, h, 0xFFFFFFFF); rect(0, 0, w, 1.5f, bd); rect(0, h - 1.5f, w, 1.5f, bd); rect(0, 0, 1.5f, h, bd); rect(w - 1.5f, 0, 1.5f, h, bd); if (checked(n)) rect(3, 3, w - 6, h - 6, 0xFF2060C0); }
        else if (ty == "submit" || ty == "button" || ty == "reset" || ty == "file" || ty == "image") { const uint32_t bgc = (s.bg >> 24) ? s.bg : 0xFFEFEFEF; rect(0, 0, w, h, bgc); rect(0, 0, w, 1.5f, bd); rect(0, h - 1.5f, w, 1.5f, bd); rect(0, 0, 1.5f, h, bd); rect(w - 1.5f, 0, 1.5f, h, bd); const std::string lb = btnLabel(n); textOp(std::max(2.f, (w - tw(lb, k)) / 2), (h - 8 * k) / 2, lb, k, fg, s.weight >= 600, false); }
        else { rect(0, 0, w, h, 0xFFFFFFFF); rect(0, 0, w, 1.5f, bd); rect(0, h - 1.5f, w, 1.5f, bd); rect(0, 0, 1.5f, h, bd); rect(w - 1.5f, 0, 1.5f, h, bd); inputs[n] = {0, 0, w, h, k};
            if (ty == "select") { std::vector<int> o = optionsOf(n); std::string tx = o.empty() ? "" : optText(o[std::min((int)o.size() - 1, selIndex(n))]); while (tx.size() > 1 && tw(tx, k) > w - 28) tx.pop_back(); textOp(5, (h - 8 * k) / 2, tx, k, fg, false, false); rect(w - 20 * gZoom, 1.5f, 1.5f, h - 3, 0xFFC0C0C0); textOp(w - 15 * gZoom, (h - 8 * k) / 2, "v", k, 0xFF505050, false, false); }
            else { std::string v = valueOf(n); uint32_t col = fg; bool ph = false; if (v.empty()) { v = attrS(nd, "placeholder"); col = 0xFF8A8A8A; ph = true; } if (ty == "password" && !ph) v = std::string(v.size(), '*');
                if (ty == "textarea") { std::string line; int ln = 0; const float lh2 = lh; for (size_t i = 0; i <= v.size() && ln < 40; i++) { if (i == v.size() || v[i] == '\n' || tw(line + v[i], k) > w - 10) { textOp(5, 4 + ln * lh2, line, k, col, false, false); ln++; line.clear(); if (i < v.size() && v[i] != '\n') line += v[i]; } else line += v[i]; } }
                else { std::string tx = v; while (tx.size() > 1 && tw(tx, k) > w - 10) tx.erase(0, 1); textOp(5, (h - 8 * k) / 2, tx, k, col, false, false); } } }
        addHit(0, 0, w, h, -1, n); it.ctl = inputs.count(n) ? n : -1; it.w = w + (s.margin[3].u == 'p' ? s.margin[3].v : 0) + (s.margin[1].u == 'p' ? s.margin[1].v : 0); it.h = h; it.asc = h; if (s.margin[3].u == 'p' && s.margin[3].v != 0) shift(it.o0, ops.size(), it.h0, hits.size(), s.margin[3].v, 0); it.o1 = ops.size(); it.h1 = hits.size(); return; }
    // inline-block / float / flex em linha: caixa com largura "encolhida"
    const float outerMax = contentMax(n, dep); float avail = std::max(8.f, availW); float bw = std::min(outerMax, avail); if (s.width.u == '%') bw = avail; if (bw < 1) bw = std::min(avail, std::max(contentMin(n, dep), 1.f));
    float mb = 0; const float mt = s.margin[0].u == 'p' ? s.margin[0].v : 0, mbt = s.margin[2].u == 'p' ? s.margin[2].v : 0; const int saved = curLink; if (nd.tag == "a" && attrOf(nd, "href")) curLink = linkIdx(n);
    const size_t o0 = ops.size(); const float bh = block(n, 0, 0, bw, &mb); curLink = saved; if (mt != 0) shift(o0, ops.size(), it.h0, hits.size(), 0, mt);
    it.w = lastOuterW; it.h = bh + mt + mbt; it.asc = it.h; it.o1 = ops.size(); it.h1 = hits.size();
}
float Eng::inlineRun(std::vector<Item>& it, float x, float y, float W, const Style& cs) {
    bool any = false; for (const Item& m : it) if (m.k != 1) any = true; if (!any) return 0; const bool pre = cs.white == 1 || cs.white == 3; W = std::max(W, 1.f);
    const int cscl = sc(cs); const float clh = lhOf(cs); const float casc = (clh - 8 * cscl) / 2 + 7 * cscl; float cy = y; size_t i = 0;
    while (i < it.size()) {
        std::vector<Item> line; float lw = 0; bool content = false;
        while (i < it.size()) {
            Item m = it[i];
            if (m.k == 3) { line.push_back(m); i++; break; }
            if (m.k == 1 && !pre) { if (line.empty() || (line.back().k == 1)) { i++; continue; } line.push_back(m); lw += m.w; i++; continue; }
            if (m.k != 1 && lw + m.w > W + 0.01f && content) { break; }
            if (m.k == 0 && m.w > W && !content && !m.nobreak) { const int k = sc(S(m.sty)); size_t cnt = 0; float acc = 0; while (cnt < m.s.size()) { size_t nx = cnt + 1; while (nx < m.s.size() && (m.s[nx] & 0xC0) == 0x80) nx++; const float cw = tw(m.s.substr(cnt, nx - cnt), k); if (acc + cw > W && cnt > 0) break; acc += cw; cnt = nx; } Item piece = m; piece.s = m.s.substr(0, cnt); piece.w = tw(piece.s, k); line.push_back(piece); lw += piece.w; it[i].s = m.s.substr(cnt); it[i].w = tw(it[i].s, k); if (it[i].s.empty()) i++; break; }
            line.push_back(m); lw += m.w; if (m.k == 0 || m.k == 2) content = true; i++;
        }
        while (!pre && !line.empty() && line.back().k == 1) { lw -= line.back().w; line.pop_back(); } if (line.empty() && i >= it.size()) break; if (line.empty()) continue;
        float asc = casc, desc = clh - casc; for (const Item& m : line) { asc = std::max(asc, m.asc); desc = std::max(desc, m.h - m.asc); } const float lineH = asc + desc;
        float ox = 0; const float free = W - lw; if (cs.align == 1) ox = std::max(0.f, free / 2); else if (cs.align == 2) ox = std::max(0.f, free); float cx = x + ox;
        for (Item& m : line) {
            const Style& s = S(m.sty); const float top = cy + (asc - m.asc); const int k = sc(s);
            if (m.k == 0 || m.k == 1 || m.k == 4) {
                if ((s.bg >> 24) && s.display == 2 && m.w > 0) rect(cx, top, m.w, std::max(m.h, 1.f), s.bg);
                if (m.k == 4 && m.node >= 0) { const std::string id = attrS(doc.n[m.node], "id"); if (!id.empty()) anchors[id] = cy; }
                if (m.k == 0) { const float ty = top + (m.h - 8 * k) / 2; textOp(cx, ty, m.s, k, s.color | (s.color >> 24 ? 0 : 0xFF000000u), s.weight >= 600, s.italic, s.ul, s.strike); }
                else if (m.k == 1 && (s.ul || s.strike)) { const float ty = top + (m.h - 8 * k) / 2; if (s.ul) rect(cx, ty + 8 * k, m.w, std::max(1.f, (float)(k / 2)), s.color | 0xFF000000u); }
                if (m.link >= 0 && m.w > 0) addHit(cx, cy, m.w, lineH, m.link, -1);
            } else if (m.k == 2) { shift(m.o0, m.o1, m.h0, m.h1, cx, top); if (m.ctl >= 0) { auto f = inputs.find(m.ctl); if (f != inputs.end()) { f->second.x += cx; f->second.y += top; } } if (!attrS(doc.n[m.node], "id").empty()) anchors[attrS(doc.n[m.node], "id")] = top; }
            cx += m.w;
        }
        cy += lineH;
    }
    return cy - y;
}
float Eng::flow(int n, float cx, float cy, float cw, float* mbOut, bool skipFirstGap) {
    const Node& nd = doc.n[n]; float y = cy, pend = 0; bool first = true; std::vector<int> run;
    auto flush = [&]() { if (run.empty()) return; std::vector<Item> items; collect(run, items, cw, depth); run.clear(); bool anyc = false; for (const Item& m : items) if (m.k != 1) anyc = true; if (!anyc) return; y += pend; pend = 0; y += inlineRun(items, cx, y, cw, S(n)); first = false; };
    if (depth > 80) { *mbOut = 0; return 0; } depth++;
    for (int c : nd.kids) {
        const Node& cn = doc.n[c]; if (cn.isText) { run.push_back(c); continue; } if (S(c).display == 0) continue;
        if (blockLevel(c)) { flush(); const float mt = effMT(c, cw); const float gap = (first && skipFirstGap) ? 0 : ((pend < 0 || mt < 0) ? pend + mt : std::max(pend, mt)); y += gap; float mb = 0; const float h = block(c, cx, y, cw, &mb); y += h; pend = mb; first = false; }
        else run.push_back(c);
    }
    flush(); depth--; *mbOut = pend; return y - cy;
}
float Eng::block(int n, float x, float y, float availW, float* mbOut, float forceBW, int* slotOut) {
    const Node& nd = doc.n[n]; const Style& s = S(n); *mbOut = 0;
    auto L = [&](const Len& l) { return l.u == 'p' ? l.v : l.u == '%' ? availW * l.v / 100.f : 0.f; };
    float ml = L(s.margin[3]), mr = L(s.margin[1]); const float mbo = L(s.margin[2]); const float pl = L(s.padding[3]), pr = L(s.padding[1]), pt = L(s.padding[0]), pb = L(s.padding[2]); const float bl = s.border[3], br = s.border[1], bt = s.border[0], bb = s.border[2];
    const float extra = pl + pr + bl + br; float bw;
    if (forceBW >= 0) bw = forceBW;
    else {
        float cwid; if (s.width.u != 'a') { cwid = L(s.width); if (!s.bsBorder) cwid += extra; } else { cwid = availW - ml - mr; if (s.display == 7) { const float mx = contentMax(n, 0) - (s.margin[1].u == 'p' ? s.margin[1].v : 0) - (s.margin[3].u == 'p' ? s.margin[3].v : 0); cwid = std::min(cwid, mx); } }
        if (s.maxW.u != 'a') { float m = L(s.maxW); if (!s.bsBorder) m += extra; if (cwid > m) cwid = m; } if (s.minW.u != 'a') { float m = L(s.minW); if (!s.bsBorder) m += extra; if (cwid < m) cwid = m; }
        bw = std::max(extra, cwid);
        if (s.width.u != 'a' || s.maxW.u != 'a' || s.display == 7) { const float freeW = availW - bw - ml - mr; if (freeW > 0) { if (s.margin[3].u == 'a' && s.margin[1].u == 'a') { ml += freeW / 2; mr += freeW / 2; } else if (s.margin[3].u == 'a') ml += freeW; } }
    }
    lastOuterW = ml + bw + mr; const float bx = x + ml; const float cx = bx + bl + pl, cw = std::max(1.f, bw - extra);
    const int slot = (int)ops.size(); for (int i = 0; i < 5; i++) ops.push_back(DOp()); if (slotOut) *slotOut = slot;
    const std::string id = attrS(nd, "id"); if (!id.empty()) anchors[id] = y; int hitSlot = -1; const int savedLink = curLink; if (nd.tag == "a" && attrOf(nd, "href")) { curLink = linkIdx(n); hitSlot = (int)hits.size(); HitR hr; hr.x = bx; hr.y = y; hr.w = bw; hr.h = 0; hr.link = curLink; hits.push_back(hr); }
    float contentH = 0, childMB = 0; const bool colTop = forceBW < 0 && collapsibleBox(*this, n, availW), colBot = forceBW < 0 && collapsibleBox(*this, n, availW) && pb == 0 && bb == 0;
    const float cy = y + bt + pt;
    if (s.display == 7) contentH = table(n, cx, cy, cw, cw);
    else if (s.display == 5 || s.display == 6) contentH = flex(n, cx, cy, cw);
    else { contentH = flow(n, cx, cy, cw, &childMB, colTop); if (!colBot) { contentH += childMB; childMB = 0; } }
    if (s.display == 4) {   // marcador da lista
        const int k = sc(s); const float lh = lhOf(s); const float ty = cy + (lh - 8 * k) / 2; const uint32_t col = s.color | 0xFF000000u;
        if (s.list != 2) { if (s.list == 1) { int num = 1; const Node& par = doc.n[nd.parent]; const int st0 = atoi(attrS(par, "start").c_str()); if (st0) num = st0; for (int sib : par.kids) { if (sib == n) break; if (!doc.n[sib].isText && S(sib).display == 4) num++; } const std::string lab = std::to_string(num) + "."; textOp(cx - 4 * k - tw(lab, k), ty, lab, k, col, false, false); }
            else { const float d = std::max(3.f, 2.f * k); const float mx = cx - 4 * k - d, my = cy + lh / 2 - d / 2; if (s.list == 3) { rect(mx, my, d, 1, col); rect(mx, my + d - 1, d, 1, col); rect(mx, my, 1, d, col); rect(mx + d - 1, my, 1, d, col); } else rect(mx, my, d, d, col); } } }
    float h = contentH; if (s.height.u == 'p') { float hh = s.height.v; if (!s.bsBorder) hh += pt + pb + bt + bb; h = std::max(0.f, hh - pt - pb - bt - bb); }
    if (s.minH.u == 'p') h = std::max(h, s.minH.v - (s.bsBorder ? pt + pb + bt + bb : 0)); if (s.maxH.u == 'p' && s.ovHidden) h = std::min(h, s.maxH.v); const float bh = h + pt + pb + bt + bb;
    DOp& bg = ops[slot]; bg.t = 0; bg.x = bx; bg.y = y; bg.w = bw; bg.h = bh; bg.col = (s.bg >> 24) ? s.bg : 0;
    auto bord = [&](int i, float rx, float ry, float rw, float rh, int side) { DOp& o = ops[slot + 1 + i]; o.t = 0; o.x = rx; o.y = ry; o.w = rw; o.h = rh; o.col = s.border[side] > 0 ? s.bcol[side] : 0; };
    bord(0, bx, y, bw, bt, 0); bord(1, bx + bw - br, y, br, bh, 1); bord(2, bx, y + bh - bb, bw, bb, 2); bord(3, bx, y, bl, bh, 3);
    if (hitSlot >= 0) hits[hitSlot].h = bh; curLink = savedLink;
    *mbOut = colBot ? ((mbo < 0 || childMB < 0) ? mbo + childMB : std::max(mbo, childMB)) : mbo; return bh;
}
float Eng::table(int n, float cx, float cy, float cw, float) {
    const Style& s = S(n); TInfo T = tinfo(*this, n); const float spc = s.spacing >= 0 ? s.spacing : 2 * gZoom; float y = cy;
    if (T.caption >= 0) { float mb = 0; y += block(T.caption, cx, y, cw, &mb) + 4; }
    if (T.ncols == 0) return y - cy; const int NC = T.ncols; std::vector<float> cmin(NC, 0), cmax(NC, 0), pref(NC, -1);
    for (size_t i = 0; i < T.rows.size(); i++) for (size_t j = 0; j < T.rows[i].size(); j++) { const int c = T.rows[i][j], sp = cellSpan(doc, c), c0 = T.col[i][j]; const float mn = contentMin(c, 0), mx = contentMax(c, 0); const Style& cs = S(c);
        for (int q = 0; q < sp; q++) { cmin[c0 + q] = std::max(cmin[c0 + q], mn / sp); cmax[c0 + q] = std::max(cmax[c0 + q], mx / sp); }
        if (sp == 1) { if (cs.width.u == 'p') pref[c0] = std::max(pref[c0], cs.width.v + (cs.bsBorder ? 0 : extraH(c, 0))); else if (cs.width.u == '%') pref[c0] = std::max(pref[c0], cw * cs.width.v / 100.f); } }
    for (int q = 0; q < NC; q++) { if (pref[q] >= 0) { cmax[q] = std::max(cmin[q], pref[q]); if (s.width.u == 'a') cmin[q] = std::max(cmin[q], std::min(pref[q], cmax[q])); } cmax[q] = std::max(cmax[q], cmin[q]); }
    const float avail = cw - spc * (NC + 1); float sumMin = 0, sumMax = 0; for (int q = 0; q < NC; q++) { sumMin += cmin[q]; sumMax += cmax[q]; } std::vector<float> cwid(NC);
    const bool fixedW = s.width.u != 'a' || true;   // a caixa do bloco ja encolheu a largura quando era "auto"
    if (sumMax <= avail) { for (int q = 0; q < NC; q++) cwid[q] = cmax[q]; const float extraW = avail - sumMax; if (extraW > 0 && fixedW && s.width.u != 'a' && sumMax > 0) for (int q = 0; q < NC; q++) cwid[q] += extraW * cmax[q] / sumMax; else if (extraW > 0 && sumMax <= 0) for (int q = 0; q < NC; q++) cwid[q] = avail / NC; }
    else if (sumMin >= avail) for (int q = 0; q < NC; q++) cwid[q] = cmin[q];
    else { const float t = (avail - sumMin) / std::max(1.f, sumMax - sumMin); for (int q = 0; q < NC; q++) cwid[q] = cmin[q] + (cmax[q] - cmin[q]) * t; }
    std::vector<float> colX(NC + 1); { float x = cx + spc; for (int q = 0; q < NC; q++) { colX[q] = x; x += cwid[q] + spc; } colX[NC] = x; }
    y += spc; const float tblW = colX[NC] - cx;
    for (size_t i = 0; i < T.rows.size(); i++) {
        const int rn = T.rowNode[i]; const float rowY = y; int rslot = (int)ops.size(); ops.push_back(DOp()); float rowH = 0; struct CI { int slot; float h; int c; size_t o0, o1, h0, h1; }; std::vector<CI> cells;
        for (size_t j = 0; j < T.rows[i].size(); j++) { const int c = T.rows[i][j]; const int sp = cellSpan(doc, c), c0 = T.col[i][j]; float w = 0; for (int q = 0; q < sp && c0 + q < NC; q++) w += cwid[c0 + q]; w += spc * (sp - 1); int slot = -1; float mb = 0; CI ci; ci.c = c; ci.o0 = ops.size(); ci.h0 = hits.size(); const float h = block(c, colX[c0], y, w + 0.01f, &mb, w, &slot); ci.o1 = ops.size(); ci.h1 = hits.size(); ci.slot = slot; ci.h = h; rowH = std::max(rowH, h); cells.push_back(ci); }
        if (rn >= 0 && (S(rn).bg >> 24)) { DOp& r = ops[rslot]; r.t = 0; r.x = cx + spc * 0.5f; r.y = rowY; r.w = tblW - spc; r.h = rowH; r.col = S(rn).bg; }
        for (const CI& ci : cells) { patchBox(ci.slot, rowH); const Style& cs = S(ci.c); if (cs.valign && ci.h < rowH) { const float dy = (rowH - ci.h) * (cs.valign == 1 ? .5f : 1.f); shift(ci.o0 + 5, ci.o1, ci.h0, ci.h1, 0, dy); } }
        y += rowH + spc;
    }
    return y - cy;
}
float Eng::flex(int n, float cx, float cy, float cw) {
    const Style& s = S(n); struct FI { int n; float ob, mn, grow, shrink, w, h; float ml, mr, mt, mb; }; std::vector<FI> it;
    for (int c : doc.n[n].kids) { if (doc.n[c].isText) continue; const Style& cs = S(c); if (cs.display == 0) continue; FI f; f.n = c; auto M = [&](const Len& l) { return l.u == 'p' ? l.v : l.u == '%' ? cw * l.v / 100.f : 0.f; }; f.ml = M(cs.margin[3]); f.mr = M(cs.margin[1]); f.mt = M(cs.margin[0]); f.mb = M(cs.margin[2]);
        const float ext = (cs.bsBorder ? 0 : M(cs.padding[1]) + M(cs.padding[3]) + cs.border[1] + cs.border[3]);
        if (cs.flexBasis.u != 'a') f.ob = M(cs.flexBasis) + ext + f.ml + f.mr; else if (cs.width.u != 'a') f.ob = M(cs.width) + ext + f.ml + f.mr; else f.ob = contentMax(c, 0);
        f.mn = contentMin(c, 0); f.grow = cs.grow; f.shrink = cs.shrink; f.w = f.ob; f.h = 0; it.push_back(f); }
    if (it.empty()) return 0; float y = cy; const float gap = s.gap, rgap = s.rowGap;
    const bool grid = s.dir == 3; int gcols = 0; if (grid) { if (s.gridCols > 0) gcols = s.gridCols; else if (s.gridMin > 0) gcols = std::max(1, (int)((cw + gap) / (s.gridMin + gap))); else gcols = 1; }
    if (grid) { const float colw = (cw - gap * (gcols - 1)) / gcols; for (FI& f : it) { f.ob = colw; f.grow = 0; f.shrink = 0; } }
    if (s.dir == 1) {   // coluna
        for (size_t i = 0; i < it.size(); i++) { FI& f = it[i]; const Style& cs = S(f.n); float avail = cw - f.ml - f.mr; float mb = 0; float bw = -1; if (s.alignI == 1 || s.alignI == 2) { bw = std::min(std::max(0.f, f.ob - f.ml - f.mr), avail); if (cs.width.u == '%') bw = avail * cs.width.v / 100; }
            float off = 0; if (bw >= 0) off = s.alignI == 1 ? (avail - bw) / 2 : avail - bw; y += f.mt; const float h = block(f.n, cx + off, y, cw, &mb, bw); y += h + f.mb; if (i + 1 < it.size()) y += rgap; }
        return y - cy; }
    size_t i = 0; const bool wrap = s.wrap || grid;
    while (i < it.size()) {
        size_t j = i; float sum = 0; while (j < it.size()) { const float add = it[j].ob + (j > i ? gap : 0); if (wrap && j > i && sum + add > cw + 0.5f) break; sum += add; j++; }
        const int cnt = (int)(j - i); float free = cw - sum; float gt = 0, st = 0; for (size_t q = i; q < j; q++) { gt += it[q].grow; st += it[q].shrink * it[q].ob; }
        if (free > 0 && gt > 0) { for (size_t q = i; q < j; q++) it[q].w = it[q].ob + free * it[q].grow / gt; free = 0; }
        else if (free < 0 && st > 0 && !wrap) { for (size_t q = i; q < j; q++) it[q].w = std::max(std::min(it[q].mn, it[q].ob), it[q].ob + free * it[q].shrink * it[q].ob / st); free = 0; }
        float x = cx, between = gap; if (free > 0) { if (s.justify == 1) x += free / 2; else if (s.justify == 2) x += free; else if (s.justify == 3 && cnt > 1) between += free / (cnt - 1); else if (s.justify == 4) { between += free / cnt; x += free / cnt / 2; } }
        struct P { int slot; size_t o0, o1, h0, h1; float h; }; std::vector<P> ps; float lineH = 0;
        for (size_t q = i; q < j; q++) { FI& f = it[q]; float mb = 0; int slot = -1; P p; p.o0 = ops.size(); p.h0 = hits.size(); const float bw = std::max(1.f, f.w - f.ml - f.mr); const float h = block(f.n, x, y + f.mt, f.w, &mb, bw, &slot); p.o1 = ops.size(); p.h1 = hits.size(); p.slot = slot; p.h = h; ps.push_back(p); f.h = h + f.mt + f.mb; lineH = std::max(lineH, f.h); x += f.w + between; }
        for (size_t q = i; q < j; q++) { FI& f = it[q]; P& p = ps[q - i]; const Style& cs = S(f.n); if ((s.alignI == 0 || s.alignI == 3) && cs.height.u == 'a') patchBox(p.slot, lineH - f.mt - f.mb); else if (s.alignI == 1) shift(p.o0, p.o1, p.h0, p.h1, 0, (lineH - f.h) / 2); else if (s.alignI == 2) shift(p.o0, p.o1, p.h0, p.h1, 0, lineH - f.h); }
        y += lineH; if (j < it.size()) y += rgap; i = j;
    }
    return y - cy;
}
void Eng::layoutAll() {
    ops.clear(); hits.clear(); links.clear(); anchors.clear(); inputs.clear(); cMax.assign(doc.n.size(), -1); cMin.assign(doc.n.size(), -1); bDesc.assign(doc.n.size(), -1); curLink = -1; depth = 0; imgWant.clear();
    float mb = 0; const float h = flow(0, 0, 0, (float)vw, &mb, false); docH = h + mb;
}
}   // namespace pw
