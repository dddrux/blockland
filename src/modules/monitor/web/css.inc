// ---------- PIE Web: CSS (analise, seletores, cascata, herdanca) ----------
// Incluido por web/web.inc DENTRO de namespace mod_monitor. Subconjunto pratico de CSS 2.1 + flex + variaveis (var(--x)) + @media de largura.
namespace pw {
static float gZoom = 1.f; static int gVW = 480, gVH = 480;   // zoom e viewport (px da textura) usados ao resolver unidades
struct Len { float v = 0; char u = 'a'; };   // u: 'a' auto/nao definido, 'p' px, '%' porcento
struct Style {
    int display = 1; // 0 none,1 block,2 inline,3 inline-block,4 list-item,5 flex,6 inline-flex,7 table,8 table-row,9 table-cell,10 table-row-group,11 table-caption
    uint32_t color = 0xFF000000, bg = 0; uint32_t bcol[4] = {0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000};   // 0xAARRGGBB
    float fontPx = 16; float lineH = 0; /* 0 = normal */ int weight = 400; bool italic = false, ul = false, strike = false, hidden = false; int align = 0, white = 0, transform = 0, list = 0, valign = 0;
    Len margin[4], padding[4]; float border[4] = {0, 0, 0, 0}; Len width, height, minW, maxW, minH, maxH; Len flexBasis; float grow = 0, shrink = 1; int dir = 0, wrap = 0, justify = 0, alignI = 0; float gap = 0, rowGap = 0;
    int floatv = 0, pos = 0; bool ovHidden = false; float opacity = 1; Len top, left; int gridCols = 0; float gridMin = 0; bool bsBorder = false; float spacing = -1; std::shared_ptr<std::map<std::string, std::string>> vars;
    Style() { for (Len& l : margin) l.u = 'p'; for (Len& l : padding) l.u = 'p'; }
};
static int fontScale(float px) { return std::max(1, std::min(8, (int)floorf(px / 8.f + .5f))); }
// ---- cores ----
static bool parseColor(std::string v, uint32_t& out) {
    v = lower(v); while (!v.empty() && isspace((unsigned char)v.back())) v.pop_back(); size_t a = v.find_first_not_of(" \t"); if (a == std::string::npos) return false; v = v.substr(a);
    static const struct { const char* n; uint32_t c; } N[] = {{"black", 0x000000}, {"white", 0xFFFFFF}, {"red", 0xFF0000}, {"green", 0x008000}, {"blue", 0x0000FF}, {"yellow", 0xFFFF00}, {"gray", 0x808080}, {"grey", 0x808080}, {"silver", 0xC0C0C0}, {"orange", 0xFFA500}, {"purple", 0x800080}, {"maroon", 0x800000}, {"navy", 0x000080},
        {"teal", 0x008080}, {"lime", 0x00FF00}, {"aqua", 0x00FFFF}, {"cyan", 0x00FFFF}, {"fuchsia", 0xFF00FF}, {"magenta", 0xFF00FF}, {"olive", 0x808000}, {"brown", 0xA52A2A}, {"pink", 0xFFC0CB}, {"gold", 0xFFD700}, {"darkgray", 0xA9A9A9}, {"darkgrey", 0xA9A9A9}, {"lightgray", 0xD3D3D3}, {"lightgrey", 0xD3D3D3},
        {"dimgray", 0x696969}, {"whitesmoke", 0xF5F5F5}, {"gainsboro", 0xDCDCDC}, {"darkblue", 0x00008B}, {"darkred", 0x8B0000}, {"darkgreen", 0x006400}, {"lightblue", 0xADD8E6}, {"skyblue", 0x87CEEB}, {"royalblue", 0x4169E1}, {"steelblue", 0x4682B4}, {"crimson", 0xDC143C}, {"tomato", 0xFF6347},
        {"coral", 0xFF7F50}, {"indigo", 0x4B0082}, {"violet", 0xEE82EE}, {"beige", 0xF5F5DC}, {"ivory", 0xFFFFF0}, {"khaki", 0xF0E68C}, {"salmon", 0xFA8072}, {"tan", 0xD2B48C}, {"turquoise", 0x40E0D0}, {"lightgreen", 0x90EE90}, {"darkorange", 0xFF8C00}, {"firebrick", 0xB22222}, {"slategray", 0x708090}, {"slategrey", 0x708090},
        {"lightyellow", 0xFFFFE0}, {"aliceblue", 0xF0F8FF}, {"azure", 0xF0FFFF}, {"honeydew", 0xF0FFF0}, {"lavender", 0xE6E6FA}, {"linen", 0xFAF0E6}, {"snow", 0xFFFAFA}, {"mintcream", 0xF5FFFA}, {"seashell", 0xFFF5EE}, {"cornflowerblue", 0x6495ED}, {"dodgerblue", 0x1E90FF}, {"forestgreen", 0x228B22}, {"seagreen", 0x2E8B57}};
    if (v == "transparent") { out = 0; return true; } if (v == "currentcolor") return false;
    for (const auto& x : N) if (v == x.n) { out = 0xFF000000u | x.c; return true; }
    if (v[0] == '#') { std::string h = v.substr(1); for (char c : h) if (!isxdigit((unsigned char)c)) return false; unsigned r, g, b, al = 255;
        if (h.size() == 3 || h.size() == 4) { r = (unsigned)strtol(std::string(2, h[0]).c_str(), nullptr, 16); g = (unsigned)strtol(std::string(2, h[1]).c_str(), nullptr, 16); b = (unsigned)strtol(std::string(2, h[2]).c_str(), nullptr, 16); if (h.size() == 4) al = (unsigned)strtol(std::string(2, h[3]).c_str(), nullptr, 16); }
        else if (h.size() == 6 || h.size() == 8) { r = (unsigned)strtol(h.substr(0, 2).c_str(), nullptr, 16); g = (unsigned)strtol(h.substr(2, 2).c_str(), nullptr, 16); b = (unsigned)strtol(h.substr(4, 2).c_str(), nullptr, 16); if (h.size() == 8) al = (unsigned)strtol(h.substr(6, 2).c_str(), nullptr, 16); } else return false;
        out = (al << 24) | (r << 16) | (g << 8) | b; return true; }
    const bool rgb = v.compare(0, 3, "rgb") == 0, hsl = v.compare(0, 3, "hsl") == 0;
    if (rgb || hsl) { const size_t p = v.find('('), e = v.rfind(')'); if (p == std::string::npos || e == std::string::npos) return false; std::string in = v.substr(p + 1, e - p - 1); for (char& c : in) if (c == ',' || c == '/') c = ' '; float f[4] = {0, 0, 0, 1}; int n = 0; size_t i = 0;
        while (i < in.size() && n < 4) { while (i < in.size() && isspace((unsigned char)in[i])) i++; if (i >= in.size()) break; char* end; const float x = strtof(in.c_str() + i, &end); if (end == in.c_str() + i) { while (i < in.size() && !isspace((unsigned char)in[i])) i++; continue; } i = (size_t)(end - in.c_str()); const bool pct = i < in.size() && in[i] == '%'; if (pct) { i++; f[n] = (rgb && n < 3) ? x * 2.55f : (n == 3 ? x / 100.f : x); } else f[n] = x; while (i < in.size() && isalpha((unsigned char)in[i])) i++; n++; }
        if (n < 3) return false; float r = f[0], g = f[1], b = f[2];
        if (hsl) { const float h = fmodf(fmodf(f[0], 360.f) + 360.f, 360.f) / 360.f, s = std::max(0.f, std::min(1.f, f[1] / 100.f)), l = std::max(0.f, std::min(1.f, f[2] / 100.f)); const float q = l < .5f ? l * (1 + s) : l + s - l * s, pp = 2 * l - q;
            auto hue = [&](float t) { if (t < 0) t += 1; if (t > 1) t -= 1; return t < 1.f / 6 ? pp + (q - pp) * 6 * t : t < .5f ? q : t < 2.f / 3 ? pp + (q - pp) * (2.f / 3 - t) * 6 : pp; }; r = hue(h + 1.f / 3) * 255; g = hue(h) * 255; b = hue(h - 1.f / 3) * 255; }
        auto cl = [](float x) { return (unsigned)std::max(0.f, std::min(255.f, x + .5f)); }; out = (cl(f[3] * 255) << 24) | (cl(r) << 16) | (cl(g) << 8) | cl(b); return true; }
    return false;
}
// ---- comprimentos ----
static bool parseLen(const std::string& vIn, const Style& st, Len& out, float rootPx = 16) {
    std::string v = lower(vIn); while (!v.empty() && isspace((unsigned char)v.back())) v.pop_back(); size_t a = v.find_first_not_of(" \t"); if (a == std::string::npos) return false; v = v.substr(a);
    if (v == "auto" || v == "none" || v == "normal" || v == "initial" || v == "inherit" || v == "unset" || v == "fit-content" || v == "max-content" || v == "min-content") { out = Len(); return true; }
    if (v.compare(0, 5, "calc(") == 0 || v.compare(0, 4, "min(") == 0 || v.compare(0, 4, "max(") == 0 || v.compare(0, 6, "clamp(") == 0) {   // calc simples "a% - bpx": usa o primeiro termo
        size_t i = v.find('('); std::string in = v.substr(i + 1); if (v[0] == 'c' && v[1] == 'l') { const size_t c = in.find(','); if (c != std::string::npos) in = in.substr(c + 1); } return parseLen(in.substr(0, in.find_first_of(" ,)")), st, out, rootPx); }
    char* end; const float x = strtof(v.c_str(), &end); if (end == v.c_str()) return false; const std::string u = v.substr((size_t)(end - v.c_str()));
    out.u = 'p'; if (u.empty() || u == "px") { out.v = x == 0 ? 0 : x * gZoom; if (u.empty() && x != 0) return false; } else if (u == "%") { out.u = '%'; out.v = x; }
    else if (u == "em") out.v = x * st.fontPx; else if (u == "rem") out.v = x * rootPx * gZoom; else if (u == "pt") out.v = x * 4.f / 3 * gZoom; else if (u == "vw") out.v = x * gVW / 100.f; else if (u == "vh" || u == "vmin") out.v = x * gVH / 100.f; else if (u == "vmax") out.v = x * std::max(gVW, gVH) / 100.f;
    else if (u == "ex") out.v = x * st.fontPx * .5f; else if (u == "ch") out.v = x * st.fontPx * .5f; else if (u == "cm") out.v = x * 37.8f * gZoom; else if (u == "mm") out.v = x * 3.78f * gZoom; else if (u == "in") out.v = x * 96 * gZoom; else if (u == "pc") out.v = x * 16 * gZoom; else return false; return true;
}
static float lenPx(const Len& l, float base, float dflt = 0) { return l.u == 'p' ? l.v : l.u == '%' ? base * l.v / 100.f : dflt; }
static std::vector<std::string> splitWs(const std::string& s) {   // separa por espacos fora de parenteses
    std::vector<std::string> o; std::string c; int dp = 0; for (char ch : s) { if (ch == '(') dp++; if (ch == ')') dp--; if (isspace((unsigned char)ch) && dp <= 0) { if (!c.empty()) { o.push_back(c); c.clear(); } } else c += ch; } if (!c.empty()) o.push_back(c); return o; }
// ---- regras ----
struct Decl { std::string p, v; bool imp = false; };
struct Comp { std::string tag, id; std::vector<std::string> cls; struct At { std::string n, v; char op = 0; }; std::vector<At> at; int pseudo = 0; int nA = 0, nB = 0; bool hasNth = false, nthLast = false; std::vector<Comp> nots; char comb = ' '; };
enum { PS_FIRST = 1, PS_LAST = 2, PS_ONLY = 4, PS_ROOT = 8, PS_EMPTY = 16, PS_LINK = 32, PS_CHECKED = 64 };
struct Sel { std::vector<Comp> c; int spec = 0; };
struct Rule { Sel sel; std::vector<Decl> d; int order = 0; };
struct Sheet { std::vector<Rule> rules; int order = 0; std::unordered_map<std::string, std::vector<int>> byTag, byCls, byId; std::vector<int> any; void index() { byTag.clear(); byCls.clear(); byId.clear(); any.clear(); for (int i = 0; i < (int)rules.size(); i++) { const Comp& k = rules[i].sel.c.back(); if (!k.id.empty()) byId[k.id].push_back(i); else if (!k.cls.empty()) byCls[k.cls[0]].push_back(i); else if (!k.tag.empty() && k.tag != "*") byTag[k.tag].push_back(i); else any.push_back(i); } } };
static std::string stripComments(const std::string& s) { std::string o; o.reserve(s.size()); for (size_t i = 0; i < s.size();) { if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '*') { const size_t e = s.find("*/", i + 2); i = e == std::string::npos ? s.size() : e + 2; o += ' '; } else o += s[i++]; } return o; }
static std::string trim(const std::string& s) { size_t a = s.find_first_not_of(" \t\r\n\f"), b = s.find_last_not_of(" \t\r\n\f"); return a == std::string::npos ? "" : s.substr(a, b - a + 1); }
static void parseDecls(const std::string& body, std::vector<Decl>& out) {
    size_t i = 0; const size_t N = body.size();
    while (i < N) { size_t e = i; int dp = 0, br = 0; char q = 0; while (e < N) { const char c = body[e]; if (q) { if (c == q) q = 0; } else if (c == '"' || c == '\'') q = c; else if (c == '(') dp++; else if (c == ')') dp--; else if (c == '{') br++; else if (c == '}') br--; else if (c == ';' && dp <= 0 && br <= 0) break; e++; }
        std::string d = body.substr(i, e - i); i = e + 1; const size_t c = d.find(':'); if (c == std::string::npos || d.find('{') != std::string::npos) continue;
        Decl x; x.p = trim(d.substr(0, c)); x.v = trim(d.substr(c + 1)); if (x.p.compare(0, 2, "--") != 0) x.p = lower(x.p); const size_t im = lower(x.v).rfind("!important"); if (im != std::string::npos) { x.imp = true; x.v = trim(x.v.substr(0, im)); } if (!x.p.empty() && !x.v.empty()) out.push_back(x); }
}
static bool parseCompound(const std::string& s, size_t& i, Comp& c) {
    const size_t N = s.size(); auto ident = [&]() { size_t b = i; while (i < N && (isalnum((unsigned char)s[i]) || s[i] == '-' || s[i] == '_' || (unsigned char)s[i] >= 0x80 || s[i] == '\\')) { if (s[i] == '\\' && i + 1 < N) i++; i++; } return s.substr(b, i - b); };
    bool any = false; if (i < N && s[i] == '*') { c.tag = "*"; i++; any = true; } else if (i < N && (isalpha((unsigned char)s[i]) || s[i] == '_')) { c.tag = lower(ident()); any = true; }
    while (i < N) { const char ch = s[i];
        if (ch == '#') { i++; c.id = ident(); any = true; } else if (ch == '.') { i++; c.cls.push_back(ident()); any = true; }
        else if (ch == '[') { const size_t e = s.find(']', i); if (e == std::string::npos) return false; std::string in = s.substr(i + 1, e - i - 1); i = e + 1; Comp::At a; size_t p = in.find_first_of("=~|^$*"); if (p == std::string::npos) a.n = lower(trim(in)); else { a.n = lower(trim(in.substr(0, p))); if (in[p] == '=') { a.op = '='; p++; } else { a.op = in[p]; p += 2; } std::string v = trim(in.substr(p)); size_t sp = v.find_last_of(" \t"); if (sp != std::string::npos && (v.back() == 'i' || v.back() == 's') && v.size() > 2 && (v[sp - 1] == '"' || v[sp - 1] == '\'')) v = trim(v.substr(0, sp)); if (v.size() >= 2 && (v[0] == '"' || v[0] == '\'')) v = v.substr(1, v.size() - 2); a.v = v; } c.at.push_back(a); any = true; }
        else if (ch == ':') { i++; bool dbl = false; if (i < N && s[i] == ':') { i++; dbl = true; } std::string nm = lower(ident()); std::string arg; if (i < N && s[i] == '(') { int dp = 1; size_t b = ++i; while (i < N && dp) { if (s[i] == '(') dp++; else if (s[i] == ')') dp--; i++; } arg = s.substr(b, i - b - 1); }
            if (dbl || nm == "before" || nm == "after" || nm == "first-line" || nm == "first-letter" || nm == "hover" || nm == "focus" || nm == "active" || nm == "visited" || nm == "focus-within" || nm == "focus-visible" || nm == "disabled" || nm == "target" || nm == "placeholder-shown" || nm == "selection" || nm == "marker" || nm == "placeholder" || nm == "-webkit-autofill" || nm == "backdrop" || nm == "invalid" || nm == "valid" || nm == "required" || nm == "optional") return false;
            if (nm == "first-child") c.pseudo |= PS_FIRST; else if (nm == "last-child") c.pseudo |= PS_LAST; else if (nm == "only-child") c.pseudo |= PS_ONLY; else if (nm == "root") c.pseudo |= PS_ROOT; else if (nm == "empty") c.pseudo |= PS_EMPTY; else if (nm == "link" || nm == "any-link") c.pseudo |= PS_LINK; else if (nm == "checked") c.pseudo |= PS_CHECKED; else if (nm == "enabled") {}
            else if (nm == "not") { size_t j = 0; Comp n; if (!parseCompound(trim(arg), j, n)) return false; c.nots.push_back(n); }
            else if (nm == "is" || nm == "where" || nm == "matches") { size_t j = 0; Comp n; std::string a1 = trim(arg.substr(0, arg.find(','))); if (!parseCompound(a1, j, n)) return false; if (!n.tag.empty() && c.tag.empty()) c.tag = n.tag; for (auto& x : n.cls) c.cls.push_back(x); if (!n.id.empty()) c.id = n.id; for (auto& x : n.at) c.at.push_back(x); }
            else if (nm == "nth-child" || nm == "nth-last-child") { std::string a = lower(trim(arg)); c.hasNth = true; c.nthLast = nm == "nth-last-child"; if (a == "odd") { c.nA = 2; c.nB = 1; } else if (a == "even") { c.nA = 2; c.nB = 0; } else { const size_t n = a.find('n'); if (n == std::string::npos) { c.nA = 0; c.nB = atoi(a.c_str()); } else { std::string as = trim(a.substr(0, n)); c.nA = as.empty() || as == "+" ? 1 : as == "-" ? -1 : atoi(as.c_str()); std::string bs = a.substr(n + 1); bs.erase(std::remove(bs.begin(), bs.end(), ' '), bs.end()); c.nB = bs.empty() ? 0 : atoi(bs.c_str()); } } }
            else if (nm == "first-of-type" || nm == "last-of-type" || nm == "only-of-type" || nm == "nth-of-type" || nm == "nth-last-of-type") { if (nm == "first-of-type") c.pseudo |= PS_FIRST; else if (nm == "last-of-type") c.pseudo |= PS_LAST; else return false; }
            else return false; any = true; }
        else break; }
    return any;
}
static bool parseSelector(const std::string& sIn, Sel& sel) {
    const std::string s = trim(sIn); size_t i = 0; char comb = ' '; sel.c.clear(); sel.spec = 0; if (s.empty()) return false;
    while (i < s.size()) { Comp c; if (!parseCompound(s, i, c)) return false; c.comb = comb; sel.c.push_back(c); sel.spec += (c.id.empty() ? 0 : 10000) + (int)(c.cls.size() + c.at.size() + (c.pseudo ? 1 : 0) + (c.hasNth ? 1 : 0) + c.nots.size()) * 100 + (c.tag.empty() || c.tag == "*" ? 0 : 1);
        bool sp = false; while (i < s.size() && isspace((unsigned char)s[i])) { i++; sp = true; } if (i >= s.size()) break; if (s[i] == '>' || s[i] == '+' || s[i] == '~') { comb = s[i++]; while (i < s.size() && isspace((unsigned char)s[i])) i++; } else if (sp) comb = ' '; else return false; }
    return !sel.c.empty();
}
static bool mediaOk(const std::string& qIn, int vw) {
    std::string q = lower(qIn); if (q.find("print") != std::string::npos && q.find("screen") == std::string::npos && q.find("all") == std::string::npos) return false; if (q.find("prefers-color-scheme: dark") != std::string::npos) return false; if (q.find("prefers-reduced-motion") != std::string::npos && q.find("reduce") != std::string::npos && q.find("no-preference") == std::string::npos) return false;
    if (q.find("hover: none") != std::string::npos || q.find("pointer: coarse") != std::string::npos) return false; size_t p = 0; bool ok = true; if (q.find("not all") == 0 || q.find("speech") == 0) return false;
    while ((p = q.find('(', p)) != std::string::npos) { const size_t e = q.find(')', p); if (e == std::string::npos) break; std::string f = q.substr(p + 1, e - p - 1); p = e + 1; const size_t c = f.find(':'); if (c == std::string::npos) continue; const std::string k = trim(f.substr(0, c)); const float v = (float)atof(f.c_str() + c + 1) * (f.find("em") != std::string::npos ? 16.f : 1.f);
        if (k == "max-width" && !(vw <= v)) ok = false; if (k == "min-width" && !(vw >= v)) ok = false; if (k == "max-device-width" && !(vw <= v)) ok = false; if (k == "min-device-width" && !(vw >= v)) ok = false; }
    return ok;
}
static void parseSheetRec(const std::string& css, Sheet& sh, int vw, int depth) {
    size_t i = 0; const size_t N = css.size(); if (depth > 4) return;
    auto block = [&](size_t& pos) { int dp = 1; size_t b = pos; char q = 0; while (pos < N && dp) { const char c = css[pos]; if (q) { if (c == q) q = 0; else if (c == '\\') pos++; } else if (c == '"' || c == '\'') q = c; else if (c == '{') dp++; else if (c == '}') dp--; pos++; } return css.substr(b, pos - b - (dp == 0 ? 1 : 0)); };
    while (i < N) { while (i < N && (isspace((unsigned char)css[i]) || css[i] == '}' || css[i] == ';')) i++; if (i >= N) break;
        if (css[i] == '@') { size_t j = i + 1; while (j < N && (isalnum((unsigned char)css[j]) || css[j] == '-')) j++; const std::string nm = lower(css.substr(i + 1, j - i - 1)); size_t b = j; while (b < N && css[b] != '{' && css[b] != ';') b++; const std::string pre = css.substr(j, b - j);
            if (b >= N) break; if (css[b] == ';') { i = b + 1; continue; } size_t pos = b + 1; const std::string inner = block(pos); i = pos;
            if (nm == "media") { if (mediaOk(pre, vw)) parseSheetRec(inner, sh, vw, depth + 1); } else if (nm == "layer" || nm == "document" || (nm == "supports" && lower(pre).find("not") == std::string::npos)) parseSheetRec(inner, sh, vw, depth + 1); continue; }
        size_t b = i; while (b < N && css[b] != '{') b++; if (b >= N) break; const std::string selText = css.substr(i, b - i); size_t pos = b + 1; const std::string body = block(pos); i = pos;
        std::vector<Decl> ds; parseDecls(body, ds); if (ds.empty()) continue;
        size_t s0 = 0; int dp = 0; for (size_t k = 0; k <= selText.size(); k++) { const char c = k < selText.size() ? selText[k] : ','; if (c == '(' || c == '[') dp++; if (c == ')' || c == ']') dp--; if (c == ',' && dp <= 0) { Rule r; if (parseSelector(selText.substr(s0, k - s0), r.sel)) { r.d = ds; r.order = sh.order++; sh.rules.push_back(std::move(r)); } s0 = k + 1; } } }
}
static void parseSheet(const std::string& css, Sheet& sh, int vw) { parseSheetRec(stripComments(css), sh, vw, 0); }
// ---- casamento ----
static int elemIndex(const Doc& d, int n, bool fromEnd, bool& only) { const Node& p = d.n[d.n[n].parent]; int idx = 0, tot = 0, my = 0; for (int k : p.kids) if (!d.n[k].isText) { tot++; if (k == n) my = tot; } only = tot == 1; idx = fromEnd ? tot - my + 1 : my; return idx; }
static int prevElem(const Doc& d, int n) { const Node& p = d.n[d.n[n].parent]; int prev = -1; for (int k : p.kids) { if (k == n) return prev; if (!d.n[k].isText) prev = k; } return -1; }
static bool matchComp(const Doc& d, int n, const Comp& c) {
    const Node& nd = d.n[n]; if (nd.isText) return false;
    if (!c.tag.empty() && c.tag != "*" && nd.tag != c.tag) return false; if (!c.id.empty() && attrS(nd, "id") != c.id) return false; for (const std::string& k : c.cls) if (!hasClass(nd, k)) return false;
    for (const Comp::At& a : c.at) { const std::string* v = attrOf(nd, a.n.c_str()); if (!v) return false; const std::string& x = *v;
        if (a.op == '=') { if (x != a.v) return false; } else if (a.op == '~') { if (!hasClass(nd, a.v) && a.n != "class") { bool f = false; size_t i = 0; while (i < x.size()) { while (i < x.size() && isspace((unsigned char)x[i])) i++; size_t j = i; while (j < x.size() && !isspace((unsigned char)x[j])) j++; if (x.compare(i, j - i, a.v) == 0 && j - i == a.v.size()) f = true; i = j; } if (!f) return false; } }
        else if (a.op == '^') { if (a.v.empty() || x.compare(0, a.v.size(), a.v) != 0) return false; } else if (a.op == '$') { if (a.v.empty() || x.size() < a.v.size() || x.compare(x.size() - a.v.size(), a.v.size(), a.v) != 0) return false; } else if (a.op == '*') { if (a.v.empty() || x.find(a.v) == std::string::npos) return false; } else if (a.op == '|') { if (x != a.v && x.compare(0, a.v.size() + 1, a.v + "-") != 0) return false; } }
    if (c.pseudo) { const bool root = nd.parent == d.root && (nd.tag == "html" || nd.parent == 0); bool only = false;
        if ((c.pseudo & PS_ROOT) && !(nd.tag == "html")) return false; if ((c.pseudo & PS_FIRST) && (nd.parent < 0 || elemIndex(d, n, false, only) != 1)) return false; if ((c.pseudo & PS_LAST) && (nd.parent < 0 || elemIndex(d, n, true, only) != 1)) return false; if ((c.pseudo & PS_ONLY) && (nd.parent < 0 || (elemIndex(d, n, false, only), !only))) return false;
        if ((c.pseudo & PS_EMPTY) && !nd.kids.empty()) return false; if ((c.pseudo & PS_LINK) && !(nd.tag == "a" && attrOf(nd, "href"))) return false; if ((c.pseudo & PS_CHECKED) && !attrOf(nd, "checked")) return false; (void)root; }
    if (c.hasNth) { if (nd.parent < 0) return false; bool only; const int idx = elemIndex(d, n, c.nthLast, only); if (c.nA == 0) { if (idx != c.nB) return false; } else { const int diff = idx - c.nB; if ((diff % c.nA != 0) || diff / c.nA < 0) return false; } }
    for (const Comp& x : c.nots) if (matchComp(d, n, x)) return false; return true;
}
static bool matchSel(const Doc& d, int n, const Sel& s, int k) {   // k = indice do composto que deve casar com n
    if (!matchComp(d, n, s.c[k])) return false; if (k == 0) return true; const char cb = s.c[k].comb;
    if (cb == '>') { const int p = d.n[n].parent; return p > 0 && matchSel(d, p, s, k - 1); }
    if (cb == '+') { const int p = prevElem(d, n); return p >= 0 && matchSel(d, p, s, k - 1); }
    if (cb == '~') { const Node& par = d.n[d.n[n].parent]; for (int sib : par.kids) { if (sib == n) break; if (!d.n[sib].isText && matchSel(d, sib, s, k - 1)) return true; } return false; }
    for (int p = d.n[n].parent; p > 0; p = d.n[p].parent) if (matchSel(d, p, s, k - 1)) return true; return false;
}
// ---- aplicar declaracoes ----
static std::string subVars(const std::string& v, const Style& st, int depth = 0) {
    if (v.find("var(") == std::string::npos || depth > 6) return v; std::string o; size_t i = 0;
    while (i < v.size()) { const size_t p = v.find("var(", i); if (p == std::string::npos) { o += v.substr(i); break; } o += v.substr(i, p - i); size_t e = p + 4; int dp = 1; while (e < v.size() && dp) { if (v[e] == '(') dp++; else if (v[e] == ')') dp--; e++; }
        std::string in = v.substr(p + 4, e - p - 5); const size_t c = in.find(','); std::string nm = trim(c == std::string::npos ? in : in.substr(0, c)), fb = c == std::string::npos ? "" : trim(in.substr(c + 1)); std::string val;
        if (st.vars) { auto it = st.vars->find(nm); if (it != st.vars->end()) val = it->second; } if (val.empty()) val = fb; o += subVars(val, st, depth + 1); i = e; }
    return o;
}
static void edge4(const std::string& v, const Style& st, Len* out, bool lenOnly = true) {
    std::vector<std::string> t = splitWs(v); if (t.empty() || t.size() > 4) return; Len L[4]; for (size_t i = 0; i < t.size(); i++) if (!parseLen(t[i], st, L[i])) return;
    const size_t n = t.size(); Len top = L[0], right = n > 1 ? L[1] : L[0], bottom = n > 2 ? L[2] : L[0], left = n > 3 ? L[3] : (n > 1 ? L[1] : L[0]); out[0] = top; out[1] = right; out[2] = bottom; out[3] = left; (void)lenOnly;
}
static float borderW(const std::string& w, const Style& st) { if (w == "thin") return std::max(1.f, gZoom); if (w == "medium") return 3 * gZoom; if (w == "thick") return 5 * gZoom; Len l; if (parseLen(w, st, l) && l.u == 'p') return l.v > 0 ? std::max(1.f, floorf(l.v + .5f)) : 0; return -1; }
static void parseBorder(const std::string& v, Style& st, int side /* -1 todos */) {
    float w = 3 * gZoom; uint32_t col = st.color; bool none = false, hasW = false; for (const std::string& t : splitWs(v)) { const std::string lt = lower(t); if (lt == "none" || lt == "hidden") none = true; else if (lt == "solid" || lt == "dashed" || lt == "dotted" || lt == "double" || lt == "groove" || lt == "ridge" || lt == "inset" || lt == "outset") {} else { const float bw = borderW(lt, st); if (bw >= 0) { w = bw; hasW = true; } else parseColor(t, col); } }
    (void)hasW; if (none) w = 0; for (int s = 0; s < 4; s++) if (side < 0 || side == s) { st.border[s] = w; st.bcol[s] = col; }
}
static void applyDecl(Style& st, const Style& par, const std::string& p, const std::string& vRaw, float rootPx) {
    const std::string v = lower(trim(vRaw)); Len L; uint32_t c;
    if (v == "inherit" && p != "display") { if (p == "color") st.color = par.color; else if (p == "font-size") st.fontPx = par.fontPx; else if (p == "background-color") st.bg = par.bg; return; }
    if (v == "initial" || v == "unset" || v == "revert") { if (p == "display") st.display = 2; return; }
    if (p == "display") { const std::string& x = v; st.display = x == "none" ? 0 : x == "block" || x == "flow-root" ? 1 : x == "inline" ? 2 : x == "inline-block" ? 3 : x == "list-item" ? 4 : x == "flex" ? 5 : x == "inline-flex" ? 6 : x == "table" || x == "inline-table" ? 7 : x == "table-row" ? 8 : x == "table-cell" ? 9 : x == "table-row-group" || x == "table-header-group" || x == "table-footer-group" ? 10 : x == "table-caption" ? 11 : x == "grid" ? 5 : x == "inline-grid" ? 6 : x == "contents" ? 2 : st.display; if (x == "grid" || x == "inline-grid") st.dir = 3; }
    else if (p == "color") { if (parseColor(v, c)) st.color = c; } else if (p == "background-color") { if (parseColor(v, c)) st.bg = c; }
    else if (p == "background") { for (const std::string& t : splitWs(v)) if (parseColor(t, c)) { st.bg = c; return; } if (v.find("url(") == std::string::npos && v.find("gradient") == std::string::npos) st.bg = 0; else { size_t k = v.find("rgb"); if (k == std::string::npos) k = v.find('#'); if (k != std::string::npos && v.find("gradient") != std::string::npos) { std::string t = v.substr(k); size_t e = t.find(')'); if (t[0] == 'r' && e != std::string::npos) t = t.substr(0, e + 1); else t = t.substr(0, t.find_first_of(" ,)")); if (parseColor(t, c)) st.bg = c; } } }
    else if (p == "font-size") { float px = -1; static const struct { const char* n; float px; } K[] = {{"xx-small", 9}, {"x-small", 10}, {"small", 13}, {"medium", 16}, {"large", 18}, {"x-large", 24}, {"xx-large", 32}, {"xxx-large", 48}}; for (const auto& k : K) if (v == k.n) px = k.px * gZoom;
        if (v == "smaller") px = par.fontPx / 1.2f; else if (v == "larger") px = par.fontPx * 1.2f; else if (px < 0 && parseLen(v, par, L, rootPx)) { if (L.u == '%') px = par.fontPx * L.v / 100.f; else if (L.u == 'p') px = L.v; } if (px >= 0) st.fontPx = std::max(0.f, px); }
    else if (p == "font-weight") { st.weight = v == "bold" || v == "bolder" ? 700 : v == "normal" || v == "lighter" ? 400 : atoi(v.c_str()) ? atoi(v.c_str()) : st.weight; } else if (p == "font-style") st.italic = v == "italic" || v == "oblique";
    else if (p == "font") { std::vector<std::string> t = splitWs(v); for (const std::string& x : t) { if (x == "italic" || x == "oblique") st.italic = true; else if (x == "bold" || x == "bolder") st.weight = 700; else if (isdigit((unsigned char)x[0]) || x[0] == '.') { std::string sz = x.substr(0, x.find('/')); applyDecl(st, par, "font-size", sz, rootPx); const size_t sl = x.find('/'); if (sl != std::string::npos) applyDecl(st, par, "line-height", x.substr(sl + 1), rootPx); break; } else { static const char* kw[] = {"xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large", "smaller", "larger"}; for (const char* k : kw) if (x == k) { applyDecl(st, par, "font-size", x, rootPx); break; } } } }
    else if (p == "line-height") { if (v == "normal") st.lineH = 0; else { char* e; const float x = strtof(v.c_str(), &e); if (*e == 0 && e != v.c_str()) st.lineH = x * st.fontPx; else if (parseLen(v, st, L, rootPx)) st.lineH = L.u == '%' ? st.fontPx * L.v / 100.f : L.v; } }
    else if (p == "text-align") st.align = v == "center" || v == "-webkit-center" ? 1 : v == "right" || v == "end" ? 2 : 0; else if (p == "vertical-align") st.valign = v == "middle" ? 1 : v == "bottom" || v == "text-bottom" ? 2 : 0;
    else if (p == "text-decoration" || p == "text-decoration-line") { st.ul = v.find("underline") != std::string::npos; st.strike = v.find("line-through") != std::string::npos; }
    else if (p == "text-transform") st.transform = v == "uppercase" ? 1 : v == "lowercase" ? 2 : v == "capitalize" ? 3 : 0; else if (p == "white-space") st.white = v == "pre" ? 1 : v == "nowrap" ? 2 : v == "pre-wrap" || v == "pre-line" || v == "break-spaces" ? 3 : 0;
    else if (p == "visibility") st.hidden = v == "hidden" || v == "collapse"; else if (p == "opacity") { st.opacity = (float)atof(v.c_str()); if (v.find('%') != std::string::npos) st.opacity /= 100.f; }
    else if (p == "margin") edge4(v, st, st.margin); else if (p == "padding") edge4(v, st, st.padding);
    else if (p.compare(0, 7, "margin-") == 0 || p.compare(0, 8, "padding-") == 0) { const bool m = p[0] == 'm'; const std::string s = p.substr(m ? 7 : 8); const int k = s == "top" ? 0 : s == "right" ? 1 : s == "bottom" ? 2 : s == "left" ? 3 : s == "inline-start" ? 3 : s == "inline-end" ? 1 : s == "block-start" ? 0 : s == "block-end" ? 2 : -1; if (k >= 0 && parseLen(v, st, L, rootPx)) (m ? st.margin : st.padding)[k] = L; }
    else if (p == "width" || p == "inline-size") { if (parseLen(v, st, L, rootPx)) st.width = L; } else if (p == "height" || p == "block-size") { if (parseLen(v, st, L, rootPx)) st.height = L; } else if (p == "min-width") { if (parseLen(v, st, L, rootPx)) st.minW = L; } else if (p == "max-width") { if (parseLen(v, st, L, rootPx)) st.maxW = L; }
    else if (p == "min-height") { if (parseLen(v, st, L, rootPx)) st.minH = L; } else if (p == "max-height") { if (parseLen(v, st, L, rootPx)) st.maxH = L; }
    else if (p == "border") parseBorder(v, st, -1); else if (p == "border-top") parseBorder(v, st, 0); else if (p == "border-right") parseBorder(v, st, 1); else if (p == "border-bottom") parseBorder(v, st, 2); else if (p == "border-left") parseBorder(v, st, 3);
    else if (p == "border-width") { std::vector<std::string> t = splitWs(v); for (int s = 0; s < 4 && !t.empty(); s++) { const std::string& x = t[t.size() == 1 ? 0 : t.size() == 2 ? s % 2 : t.size() == 3 ? (s == 3 ? 1 : s) : s]; const float w = borderW(lower(x), st); if (w >= 0) st.border[s] = w; } }
    else if (p == "border-color") { std::vector<std::string> t = splitWs(v); for (int s = 0; s < 4 && !t.empty(); s++) { const std::string& x = t[t.size() == 1 ? 0 : t.size() == 2 ? s % 2 : t.size() == 3 ? (s == 3 ? 1 : s) : s]; if (parseColor(x, c)) st.bcol[s] = c; } }
    else if (p == "border-style") { if (v == "none" || v == "hidden") for (float& b : st.border) b = 0; }
    else if (p.compare(0, 7, "border-") == 0 && p.size() > 7) { const size_t d = p.rfind('-'); const std::string side = p.substr(7, d - 7), what = p.substr(d + 1); const int k = side == "top" ? 0 : side == "right" ? 1 : side == "bottom" ? 2 : side == "left" ? 3 : -1;
        if (k >= 0 && what == "width") { const float w = borderW(v, st); if (w >= 0) st.border[k] = w; } else if (k >= 0 && what == "color") { if (parseColor(v, c)) st.bcol[k] = c; } else if (k >= 0 && what == "style" && (v == "none" || v == "hidden")) st.border[k] = 0; }
    else if (p == "list-style" || p == "list-style-type") { st.list = v.find("none") != std::string::npos ? 2 : v.find("decimal") != std::string::npos ? 1 : v.find("circle") != std::string::npos ? 3 : v.find("square") != std::string::npos ? 4 : v.find("disc") != std::string::npos ? 0 : st.list; }
    else if (p == "flex-direction") st.dir = v.compare(0, 6, "column") == 0 ? 1 : 0; else if (p == "flex-wrap") st.wrap = v == "wrap" || v == "wrap-reverse"; else if (p == "flex-flow") { st.dir = v.find("column") != std::string::npos ? 1 : 0; st.wrap = v.find("wrap") != std::string::npos && v.find("nowrap") == std::string::npos; }
    else if (p == "justify-content") st.justify = v == "center" ? 1 : v == "flex-end" || v == "end" || v == "right" ? 2 : v == "space-between" ? 3 : v == "space-around" || v == "space-evenly" ? 4 : 0; else if (p == "align-items") st.alignI = v == "center" ? 1 : v == "flex-end" || v == "end" ? 2 : v == "stretch" ? 3 : 0;
    else if (p == "flex-grow") st.grow = (float)atof(v.c_str()); else if (p == "flex-shrink") st.shrink = (float)atof(v.c_str()); else if (p == "flex-basis") { if (parseLen(v, st, L, rootPx)) st.flexBasis = L; }
    else if (p == "flex") { std::vector<std::string> t = splitWs(v); if (v == "none") { st.grow = 0; st.shrink = 0; } else if (v == "auto") { st.grow = 1; st.shrink = 1; } else { if (!t.empty() && (isdigit((unsigned char)t[0][0]) || t[0][0] == '.')) { st.grow = (float)atof(t[0].c_str()); if (t.size() == 1) { st.shrink = 1; st.flexBasis.u = 'p'; st.flexBasis.v = 0; } } if (t.size() > 1 && isdigit((unsigned char)t[1][0]) && t[1].find_first_not_of("0123456789.") == std::string::npos) st.shrink = (float)atof(t[1].c_str()); if (t.size() > 1 && parseLen(t.back(), st, L, rootPx) && L.u != 'a' && t.back().find_first_not_of("0123456789.") != std::string::npos) st.flexBasis = L; else if (t.size() == 3 && parseLen(t[2], st, L, rootPx)) st.flexBasis = L; } }
    else if (p == "gap" || p == "column-gap" || p == "grid-gap" || p == "row-gap") { std::vector<std::string> t = splitWs(v); if (!t.empty() && parseLen(t[0], st, L, rootPx)) { const float a = lenPx(L, 0); if (p == "row-gap") st.rowGap = a; else { st.rowGap = p == "gap" || p == "grid-gap" ? a : st.rowGap; st.gap = a; } if (t.size() > 1 && parseLen(t[1], st, L, rootPx)) st.gap = lenPx(L, 0); } }
    else if (p == "float") st.floatv = v == "left" ? 1 : v == "right" ? 2 : 0; else if (p == "position") st.pos = v == "absolute" ? 1 : v == "fixed" ? 2 : v == "sticky" ? 3 : v == "relative" ? 4 : 0;
    else if (p == "overflow" || p == "overflow-x" || p == "overflow-y") st.ovHidden = v == "hidden" || v == "clip"; else if (p == "top") { if (parseLen(v, st, L, rootPx)) st.top = L; } else if (p == "left" || p == "right") { if (parseLen(v, st, L, rootPx)) st.left = L; }
    else if (p == "grid-template-columns") { st.gridCols = 0; st.gridMin = 0; const size_t r = v.find("repeat("); if (r != std::string::npos && (v.find("auto-fill") != std::string::npos || v.find("auto-fit") != std::string::npos)) { const size_t m = v.find("minmax("); Len ml; if (m != std::string::npos && parseLen(v.substr(m + 7, v.find_first_of(",)", m + 7) - m - 7), st, ml, rootPx) && ml.u == 'p') st.gridMin = std::max(40.f, ml.v); else st.gridMin = 160 * gZoom; }
        else { int n = r != std::string::npos ? atoi(v.c_str() + r + 7) : (int)splitWs(v).size(); st.gridCols = std::max(1, std::min(12, n)); } }
    else if (p == "border-spacing") { if (parseLen(splitWs(v)[0], st, L, rootPx) && L.u == 'p') st.spacing = L.v; } else if (p == "border-collapse") { if (v == "collapse") st.spacing = 0; }
    else if (p == "box-sizing") st.bsBorder = v == "border-box";
    else if (p == "clip" && v.find("rect(0") != std::string::npos) st.ovHidden = true;
}
}   // namespace pw
