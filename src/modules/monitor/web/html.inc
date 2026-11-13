// ---------- PIE Web: analisador de HTML (tolerante, estilo HTML5 simplificado) ----------
// Incluido por web/web.inc DENTRO de namespace mod_monitor. Gera uma arvore de nos (Doc). Sem JavaScript.
namespace pw {
struct Attr { std::string k, v; };
struct Node { bool isText = false; std::string tag, text; std::vector<Attr> at; int parent = -1; std::vector<int> kids; int sty = -1; };
struct Doc { std::vector<Node> n; std::string title, baseUrl; int root = 0; };
static std::string lower(std::string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
static const std::string* attrOf(const Node& n, const char* k) { for (const Attr& a : n.at) if (a.k == k) return &a.v; return nullptr; }
static std::string attrS(const Node& n, const char* k) { const std::string* p = attrOf(n, k); return p ? *p : std::string(); }
static bool hasClass(const Node& n, const std::string& c) { const std::string* p = attrOf(n, "class"); if (!p) return false; size_t i = 0; const std::string& s = *p; while (i < s.size()) { while (i < s.size() && isspace((unsigned char)s[i])) i++; size_t j = i; while (j < s.size() && !isspace((unsigned char)s[j])) j++; if (j - i == c.size() && s.compare(i, j - i, c) == 0) return true; i = j; } return false; }

// ---- texto: UTF-8 -> so o que a fonte 5x7 desenha (ASCII + acentos do portugues); resto vira o equivalente mais proximo ----
static std::string sanitize(const std::string& s) {
    static const char* L1 = "AAAAAAACEEEEIIIIDNOOOOO*OUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";   // 0xC0..0xFF sem acento
    std::string o; o.reserve(s.size());
    auto put = [&](int cp) { if (cp < 0x80) { o += (char)cp; return; } o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3F)); };
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = (unsigned char)s[i]; int cp = c, n = 1;
        if (c >= 0xF0 && i + 3 < s.size()) { cp = ((c & 7) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F); n = 4; }
        else if (c >= 0xE0 && i + 2 < s.size()) { cp = ((c & 15) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F); n = 3; }
        else if (c >= 0xC0 && i + 1 < s.size()) { cp = ((c & 31) << 6) | (s[i + 1] & 0x3F); n = 2; }
        else if (c >= 0x80) { i++; continue; }
        i += n;
        if (cp < 0x80) { if (cp == '\t' || cp == '\n' || cp == '\r' || cp == '\f' || cp == ' ' || (cp >= 32 && cp < 127)) o += (char)cp; continue; }
        bool known = false; for (const AccM& a : ACC) if (a.cp == cp) known = true;
        if (known) { put(cp); continue; }
        switch (cp) {
            case 0xA0: case 0x2002: case 0x2003: case 0x2009: case 0x200A: case 0x202F: o += ' '; continue;
            case 0x200B: case 0x200C: case 0x200D: case 0xFEFF: case 0xAD: continue;
            case 0x2018: case 0x2019: case 0x201A: case 0x2032: o += '\''; continue; case 0x201C: case 0x201D: case 0x201E: case 0x2033: o += '"'; continue;
            case 0x2013: case 0x2014: case 0x2212: case 0x2010: case 0x2011: o += '-'; continue; case 0x2026: o += "..."; continue; case 0x2022: case 0x25CF: case 0xB7: case 0x25AA: o += '*'; continue;
            case 0xAB: o += "<<"; continue; case 0xBB: case 0x203A: o += ">>"; continue; case 0x2039: o += '<'; continue; case 0xA9: o += "(c)"; continue; case 0xAE: o += "(R)"; continue; case 0x2122: o += "TM"; continue; case 0x20AC: o += "EUR"; continue;
            case 0xD7: o += 'x'; continue; case 0xF7: o += '/'; continue; case 0xB0: o += 'o'; continue; case 0xBA: o += 'o'; continue; case 0xAA: o += 'a'; continue; case 0xB2: o += '2'; continue; case 0xB3: o += '3'; continue; case 0xB9: o += '1'; continue;
            case 0xBD: o += "1/2"; continue; case 0xBC: o += "1/4"; continue; case 0xBE: o += "3/4"; continue; case 0xA3: o += "GBP"; continue; case 0xA5: o += "JPY"; continue; case 0xA2: o += 'c'; continue; case 0xA7: o += "S."; continue; case 0xB6: o += "P."; continue;
            case 0x2190: o += "<-"; continue; case 0x2192: o += "->"; continue; case 0x2191: o += '^'; continue; case 0x2193: o += 'v'; continue; case 0x2713: case 0x2714: o += 'v'; continue; case 0x2715: case 0x2716: o += 'x'; continue; case 0x2605: case 0x2606: o += '*'; continue; case 0x25BC: case 0x25BE: o += 'v'; continue; case 0x25B2: o += '^'; continue; default: break; }
        if (cp >= 0xC0 && cp <= 0xFF) { o += L1[cp - 0xC0]; continue; }
        if (cp >= 0x100 && cp <= 0x17F) { static const char* LA = "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiJjJjKkkLlLlLlLlLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs"; if (cp - 0x100 < (int)strlen(LA)) { o += LA[cp - 0x100]; continue; } }
        if (cp >= 0x2500 && cp <= 0x259F) { o += '|'; continue; }
        o += '?';
    }
    return o;
}
static void appendUtf8Raw(std::string& o, int cp) { if (cp < 0x80) o += (char)cp; else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 63)); } else if (cp < 0x10000) { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 63)); o += (char)(0x80 | (cp & 63)); } else { o += (char)(0xF0 | (cp >> 18)); o += (char)(0x80 | ((cp >> 12) & 63)); o += (char)(0x80 | ((cp >> 6) & 63)); o += (char)(0x80 | (cp & 63)); } }
static std::string decodeEntities(const std::string& s) {
    if (s.find('&') == std::string::npos) return s;
    static const struct { const char* n; int cp; } E[] = {{"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''}, {"nbsp", 0xA0}, {"copy", 0xA9}, {"reg", 0xAE}, {"trade", 0x2122}, {"hellip", 0x2026}, {"mdash", 0x2014}, {"ndash", 0x2013}, {"lsquo", 0x2018}, {"rsquo", 0x2019}, {"ldquo", 0x201C}, {"rdquo", 0x201D},
        {"laquo", 0xAB}, {"raquo", 0xBB}, {"middot", 0xB7}, {"bull", 0x2022}, {"times", 0xD7}, {"divide", 0xF7}, {"deg", 0xB0}, {"euro", 0x20AC}, {"pound", 0xA3}, {"yen", 0xA5}, {"cent", 0xA2}, {"sect", 0xA7}, {"para", 0xB6}, {"larr", 0x2190}, {"rarr", 0x2192}, {"uarr", 0x2191}, {"darr", 0x2193},
        {"aacute", 0xE1}, {"agrave", 0xE0}, {"acirc", 0xE2}, {"atilde", 0xE3}, {"auml", 0xE4}, {"eacute", 0xE9}, {"egrave", 0xE8}, {"ecirc", 0xEA}, {"iacute", 0xED}, {"oacute", 0xF3}, {"ocirc", 0xF4}, {"otilde", 0xF5}, {"ouml", 0xF6}, {"uacute", 0xFA}, {"uuml", 0xFC}, {"ccedil", 0xE7}, {"ntilde", 0xF1},
        {"Aacute", 0xC1}, {"Agrave", 0xC0}, {"Acirc", 0xC2}, {"Atilde", 0xC3}, {"Eacute", 0xC9}, {"Ecirc", 0xCA}, {"Iacute", 0xCD}, {"Oacute", 0xD3}, {"Ocirc", 0xD4}, {"Otilde", 0xD5}, {"Uacute", 0xDA}, {"Ccedil", 0xC7}, {"Ntilde", 0xD1}, {"shy", 0xAD}, {"zwnj", 0x200C}, {"ensp", 0x2002}, {"emsp", 0x2003}, {"thinsp", 0x2009}};
    std::string o; o.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] != '&') { o += s[i]; continue; }
        const size_t e = s.find(';', i); if (e == std::string::npos || e - i > 10) { o += '&'; continue; }
        const std::string nm = s.substr(i + 1, e - i - 1); int cp = -1;
        if (nm.size() > 1 && nm[0] == '#') cp = (nm[1] == 'x' || nm[1] == 'X') ? (int)strtol(nm.c_str() + 2, nullptr, 16) : atoi(nm.c_str() + 1);
        else for (const auto& x : E) if (nm == x.n) { cp = x.cp; break; }
        if (cp <= 0 || cp > 0x10FFFF) { o += '&'; continue; } appendUtf8Raw(o, cp); i = e;
    }
    return o;
}
static std::string latin1ToUtf8(const std::string& s) { std::string o; for (unsigned char c : s) appendUtf8Raw(o, c); return o; }
static bool isVoidTag(const std::string& t) { static const char* V[] = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr", "keygen", "command"}; for (const char* v : V) if (t == v) return true; return false; }
static bool closesP(const std::string& t) { static const char* V[] = {"address", "article", "aside", "blockquote", "center", "details", "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "form", "h1", "h2", "h3", "h4", "h5", "h6", "header", "hgroup", "hr", "main", "menu", "nav", "ol", "p", "pre", "section", "table", "ul", "li", "dd", "dt"}; for (const char* v : V) if (t == v) return true; return false; }
static void parseHtml(const std::string& htmlIn, Doc& d) {
    d.n.clear(); d.title.clear(); Node r; r.tag = "#root"; d.n.push_back(r); d.root = 0; std::string html = htmlIn;
    { std::string head = lower(html.substr(0, 3000)); const size_t c = head.find("charset"); if (c != std::string::npos) { std::string cs = head.substr(c + 7, 24); if (cs.find("8859-1") != std::string::npos || cs.find("1252") != std::string::npos || cs.find("latin1") != std::string::npos) html = latin1ToUtf8(html); } }
    if (html.size() >= 3 && (unsigned char)html[0] == 0xEF && (unsigned char)html[1] == 0xBB && (unsigned char)html[2] == 0xBF) html.erase(0, 3);
    std::vector<int> st{0}; const size_t N = html.size(); size_t i = 0; bool inTitle = false;
    auto add = [&](Node nd) { nd.parent = st.back(); d.n.push_back(std::move(nd)); const int id = (int)d.n.size() - 1; d.n[st.back()].kids.push_back(id); return id; };
    auto closeTo = [&](const std::string& tag) { for (int k = (int)st.size() - 1; k > 0; k--) if (d.n[st[k]].tag == tag) { st.resize(k); return; } };
    auto openIn = [&](const char* tag, const char* stopA, const char* stopB) { for (int k = (int)st.size() - 1; k > 0; k--) { const std::string& t = d.n[st[k]].tag; if (t == tag) return k; if (t == stopA || t == stopB || t == "table" || t == "body") return -1; } return -1; };
    while (i < N) {
        if (html[i] != '<') { size_t e = html.find('<', i); if (e == std::string::npos) e = N; std::string tx = html.substr(i, e - i); i = e; if (inTitle) { d.title += tx; continue; }
            tx = sanitize(decodeEntities(tx)); if (tx.empty()) continue; if (st.size() == 1) { bool ws = true; for (char c : tx) if (!isspace((unsigned char)c)) ws = false; if (ws) continue; }
            Node t; t.isText = true; t.text = tx; add(t); continue; }
        if (html.compare(i, 4, "<!--") == 0) { const size_t e = html.find("-->", i + 4); i = e == std::string::npos ? N : e + 3; continue; }
        if (i + 1 < N && (html[i + 1] == '!' || html[i + 1] == '?')) { const size_t e = html.find('>', i); i = e == std::string::npos ? N : e + 1; continue; }
        const bool endTag = i + 1 < N && html[i + 1] == '/'; size_t p = i + 1 + (endTag ? 1 : 0); size_t q = p; while (q < N && (isalnum((unsigned char)html[q]) || html[q] == '-' || html[q] == ':' || html[q] == '_')) q++;
        if (q == p) { Node t; t.isText = true; t.text = "<"; add(t); i++; continue; }   // "<" solto
        std::string tag = lower(html.substr(p, q - p)); Node el; el.tag = tag; bool selfClose = false;
        while (q < N && html[q] != '>') {   // atributos
            while (q < N && (isspace((unsigned char)html[q]) || html[q] == '/')) { if (html[q] == '/' && q + 1 < N && html[q + 1] == '>') selfClose = true; q++; } if (q >= N || html[q] == '>') break;
            size_t a = q; while (q < N && !isspace((unsigned char)html[q]) && html[q] != '=' && html[q] != '>' && !(html[q] == '/' && q + 1 < N && html[q + 1] == '>')) q++; std::string k = lower(html.substr(a, q - a)); std::string v;
            while (q < N && isspace((unsigned char)html[q])) q++;
            if (q < N && html[q] == '=') { q++; while (q < N && isspace((unsigned char)html[q])) q++; if (q < N && (html[q] == '"' || html[q] == '\'')) { const char qc = html[q++]; size_t b = q; while (q < N && html[q] != qc) q++; v = html.substr(b, q - b); if (q < N) q++; }
                else { size_t b = q; while (q < N && !isspace((unsigned char)html[q]) && html[q] != '>') q++; v = html.substr(b, q - b); } }
            if (!k.empty() && !attrOf(el, k.c_str())) el.at.push_back({k, sanitize(decodeEntities(v))}); }
        i = q < N ? q + 1 : N;
        if (endTag) { if (tag == "body" || tag == "html") continue; if (tag == "title") inTitle = false; if (tag == "br") { Node b; b.tag = "br"; add(b); } else if (tag != "p" || openIn("p", "", "") >= 0) closeTo(tag); else { Node pp; pp.tag = "p"; int id = add(pp); st.push_back(id); st.pop_back(); } continue; }
        if (tag == "title") { inTitle = true; continue; }
        if (tag == "script" || tag == "style" || tag == "textarea" || tag == "noscript_raw") {   // texto cru ate o fechamento
            size_t e = N; for (size_t k = i; k + 2 < N; k++) if (html[k] == '<' && html[k + 1] == '/' && lower(html.substr(k + 2, tag.size())) == tag) { e = k; break; }
            Node t; t.isText = true; t.text = html.substr(i, e - i); if (tag == "textarea") t.text = sanitize(decodeEntities(t.text)); const int id = add(el); d.n[id].kids.push_back((int)d.n.size()); t.parent = id; d.n.push_back(t);
            const size_t ge = html.find('>', e); i = ge == std::string::npos ? N : ge + 1; continue; }
        if (tag == "html" || tag == "head" || tag == "body") { if (tag == "body" || tag == "html") { for (const Attr& a : el.at) if (!attrOf(d.n[st.back()], a.k.c_str())) d.n[st.back()].at.push_back(a); } if (tag == "body" && st.size() > 1 && d.n[st.back()].tag == "head") st.pop_back(); if (tag == "body") { bool has = false; for (int k : st) if (d.n[k].tag == "body") has = true; if (has) continue; } else { bool has = false; for (int k : st) if (d.n[k].tag == tag) has = true; if (has) continue; } }
        // fechamentos implicitos
        if (closesP(tag)) { const int k = openIn("p", "", ""); if (k > 0) st.resize(k); }
        if (tag == "li") { const int k = openIn("li", "ul", "ol"); if (k > 0) st.resize(k); }
        if (tag == "dd" || tag == "dt") { int k = openIn("dd", "dl", ""); if (k < 0) k = openIn("dt", "dl", ""); if (k > 0) st.resize(k); }
        if (tag == "tr") { int k = openIn("tr", "table", ""); if (k > 0) st.resize(k); }
        if (tag == "td" || tag == "th") { int k = openIn("td", "tr", "table"); if (k < 0) k = openIn("th", "tr", "table"); if (k > 0) st.resize(k); }
        if (tag == "thead" || tag == "tbody" || tag == "tfoot") { int k = openIn("tbody", "table", ""); if (k < 0) k = openIn("thead", "table", ""); if (k < 0) k = openIn("tfoot", "table", ""); if (k > 0) st.resize(k); }
        if (tag == "option") { const int k = openIn("option", "select", ""); if (k > 0) st.resize(k); }
        if (tag == "head" ) { if (st.size() > 1 && d.n[st.back()].tag == "head") continue; }
        if ((tag == "div" || tag == "p" || tag == "span" || tag == "a" || tag == "section") && st.size() > 120) continue;   // profundidade absurda
        const int id = add(el); if (!isVoidTag(tag) && !selfClose) st.push_back(id);
        if (tag == "base") { const std::string h = attrS(d.n[id], "href"); if (!h.empty()) d.baseUrl = h; }
    }
    { std::string t; bool sp = false; for (char c : sanitize(decodeEntities(d.title))) { if (isspace((unsigned char)c)) { sp = true; continue; } if (sp && !t.empty()) t += ' '; sp = false; t += c; } d.title = t.substr(0, 120); }
}
}   // namespace pw
