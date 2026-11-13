// ---------- propsys.inc (patchProp.2): props do Decocraft etc. como ENTIDADES colocaveis ----------
// Os chunks guardam 1 byte por bloco, entao o prop nao e um bloco: e uma entidade estatica (PropEnt) numa celula do mundo,
// com rotacao em passos de 22,5 graus. Aqui: malha (a partir de props.inc), icone 3D do inventario, desenho no mundo,
// colocar (botao direito), remover (botao esquerdo), colisao (caixa do modelo) e salvar/carregar (saves/<mundo>/props.dat).
// Sem rede ainda (cada jogador ve os proprios props); sem comportamento por `beh` (Prop.3).
struct PropEnt { int id = 0; float x = 0, y = 0, z = 0; int rot = 0; bool coll = true; AABB box{0, 0, 0, 0, 0, 0}; std::vector<AABB> cb; uint8_t nb = 0; uint8_t on = 1; int vid = 0; };   // vid (patchConv.3): item-variante por vizinhos (0 = o proprio id)   // on: lampada ligada (SwitchableLamp)   // nb: bit f = tem vizinho do MESMO prop (mesma rotacao) colado na face MUNDIAL f (0=-X..5=+Z): o vidro nao desenha essa face (culling do vidro, como o MC)   // cb = caixas de colisao/selecao por parte do modelo (patchProp.3); box = uniao delas
static void propNetAdd(const PropEnt& e); static void propNetDel(const PropEnt& e); static void propNetSwap(const PropEnt& o, const PropEnt& n); static void propNetBed(); static bool propSeatTaken(const PropEnt& e); static std::string propKey(int id);   // patchProp.6: rede (fim do arquivo)
static bool gPropCollOff = false;   // true: propCollBoxes nao devolve nada (testa so os blocos)
static std::vector<PropEnt> gProps;
struct PropQuad { float p[4][3]; float uv[4][2]; int page = -1, tex = -1; float shade = 1.f; int8_t face = 0; bool glass = false; mutable int8_t trc = -1; };   // trc (patchProp.6): -1 = ainda nao avaliada; 1 = a FACE tem texels translucidos (vai na passada do vidro); 0 = opaca/recorte   // tex = indice em PropPack::tex (para saber se e translucida)   // page -1 = sem textura (cinza)
struct PropIconQ { float sx[4], sy[4]; float uv[4][2]; int page = -1; float shade = 1.f, alpha = 1.f; };
struct PropMesh { std::vector<PropQuad> q; std::vector<PropIconQ> icon; bool built = false, iconBuilt = false, flat = false; float alpha = 1.f; };   // patchProp.7: flat = icone plano de item (1 caixa fina 1x1: Techguns/cgm) -> sprite 2D no slot e pose de item vanilla   // alpha < 1: item de vidro (translucent.txt)
static std::unordered_map<int, PropMesh> gPropMesh;   // por id de item
static std::vector<std::vector<GLuint>> gPropTex;     // [pacote][pagina]; 0 = ainda nao carregada
static std::vector<std::vector<int8_t>> gPropTexBad;  // 1 = falhou ao carregar (nao tenta de novo)
static std::vector<std::unordered_map<int, std::vector<uint8_t>>> gPropTexA;   // patchProp.6: [pacote][textura translucida] -> classe do alfa por texel (0 vazio, 1 parcial, 2 opaco), para decidir a translucidez POR FACE
static std::vector<std::unordered_map<int, std::array<float, 3>>> gPropTexAvg;   // patchProp.part: cor media (0..1) por textura, calculada quando a pagina sobe para a GPU (particulas ao quebrar)
static std::vector<std::vector<int8_t>> gPropTrans;   // [pacote][textura]: 1 = tem texels translucidos (vidro): desenha com blend; so vale depois da pagina carregada
static const float PROP_ROT_STEP = 22.5f;

// textura de uma pagina do atlas do pacote (carrega so quando algum prop daquela pagina aparece: 4096x4096 = 64 MB de VRAM cada)
static GLuint propPageTex(int pack, int page) {
    if (pack < 0 || pack >= (int)gPropPacks.size()) return 0;
    const PropPack& pk = gPropPacks[pack];
    if (page < 0 || page >= (int)pk.pages.size()) return 0;
    if (gPropTex.size() < gPropPacks.size()) { gPropTex.resize(gPropPacks.size()); gPropTexBad.resize(gPropPacks.size()); }
    if (gPropTex[pack].size() < pk.pages.size()) { gPropTex[pack].resize(pk.pages.size(), 0); gPropTexBad[pack].resize(pk.pages.size(), 0); }
    if (gPropTex[pack][page]) return gPropTex[pack][page];
    if (gPropTexBad[pack][page]) return 0;
    GLuint t = 0; std::string path = pk.pages[page];
    if (!std::filesystem::exists(path)) path = pk.dir + "/" + pk.pages[page];   // props.bin guarda so o nome do arquivo
    std::vector<uint8_t> rgba; int w = 0, h = 0;
    if (!loadPNG(path.c_str(), rgba, w, h)) { gPropTexBad[pack][page] = 1; logWarn("props: pagina %s nao carregou", path.c_str()); return 0; }
    if (gPropTrans.size() < gPropPacks.size()) gPropTrans.resize(gPropPacks.size());
    if (gPropTrans[pack].size() < pk.tex.size()) gPropTrans[pack].assign(pk.tex.size(), 0);
    for (size_t i = 0; i < pk.tex.size(); i++) { const PropTex& pt = pk.tex[i]; if (pt.ref || pt.page != page) continue;   // translucida = >= 25% dos texels visiveis com alfa parcial (borda suave de textura nao conta)
        int vis = 0, par = 0; for (int yy = pt.y; yy < pt.y + pt.h && yy < h; yy++) for (int xx = pt.x; xx < pt.x + pt.w && xx < w; xx++) { const uint8_t a = rgba[((size_t)yy * w + xx) * 4 + 3]; if (a > 0) { vis++; if (a < 250) par++; } }
        gPropTrans[pack][i] = vis > 0 && par * 4 >= vis ? 1 : 0;
        if (gPropTrans[pack][i]) { if (gPropTexA.size() < gPropPacks.size()) gPropTexA.resize(gPropPacks.size()); std::vector<uint8_t>& ca = gPropTexA[pack][(int)i]; ca.assign((size_t)pt.w * pt.h, 0);
            for (int yy = 0; yy < pt.h && pt.y + yy < h; yy++) for (int xx = 0; xx < pt.w && pt.x + xx < w; xx++) { const uint8_t a = rgba[((size_t)(pt.y + yy) * w + pt.x + xx) * 4 + 3]; ca[(size_t)yy * pt.w + xx] = a == 0 ? 0 : a < 250 ? 1 : 2; } } }
    if (gPropTexAvg.size() < gPropPacks.size()) gPropTexAvg.resize(gPropPacks.size());
    for (size_t i = 0; i < pk.tex.size(); i++) { const PropTex& pt = pk.tex[i]; if (pt.ref || pt.page != page) continue;
        double sr = 0, sg = 0, sb = 0; int n = 0; const int st = std::max(1, std::min(pt.w, pt.h) / 16);
        for (int yy = pt.y; yy < pt.y + pt.h && yy < h; yy += st) for (int xx = pt.x; xx < pt.x + pt.w && xx < w; xx += st) { const uint8_t* q = &rgba[((size_t)yy * w + xx) * 4]; if (q[3] < 128) continue; sr += q[0]; sg += q[1]; sb += q[2]; n++; }
        if (n) gPropTexAvg[pack][(int)i] = {(float)(sr / n / 255.0), (float)(sg / n / 255.0), (float)(sb / n / 255.0)}; }
    glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    gPropTex[pack][page] = t; logInfo("props: pagina de textura %s carregada", path.c_str());
    return t;
}
// pagina de textura de um slot do item (-1 = textura por referencia/ausente: desenha cinza)
static int propSlotRegion(const PropPack& pk, const PropItem& it, int slot, float& ox, float& oy, float& sx, float& sy, int* texIdx = nullptr) {
    if (slot < 0 || slot >= (int)it.tex.size()) return -1;
    int t = it.tex[slot]; if (t < 0) t = -1 - t;
    if (t < 0 || t >= (int)pk.tex.size()) return -1;
    const PropTex& pt = pk.tex[t]; if (pt.ref || pt.page < 0 || pt.page >= (int)pk.pageW.size()) return -1;
    const float W = (float)pk.pageW[pt.page], H = (float)pk.pageH[pt.page];
    ox = pt.x / W; oy = pt.y / H; sx = pt.w / W; sy = pt.h / H; if (texIdx) *texIdx = t; return pt.page;
}
static bool propQuadTrans(int pack, const PropQuad& q) {   // chamar depois de propPageTex(pack, q.page). patchProp.6: por FACE (antes era pela textura inteira: moldura opaca de uma mesa de vidro ia para a passada do vidro e sumia/oscilava)
    if (!(q.page >= 0 && q.tex >= 0 && pack < (int)gPropTrans.size() && q.tex < (int)gPropTrans[pack].size() && gPropTrans[pack][q.tex])) return false;
    if (q.trc >= 0) return q.trc != 0;
    q.trc = 1;
    if (pack >= (int)gPropTexA.size()) return true;
    auto it = gPropTexA[pack].find(q.tex); if (it == gPropTexA[pack].end() || q.page >= (int)gPropPacks[pack].pageW.size()) return true;
    const PropTex& pt = gPropPacks[pack].tex[q.tex]; const float W = (float)gPropPacks[pack].pageW[q.page], H = (float)gPropPacks[pack].pageH[q.page];
    float u0 = 1e9f, u1 = -1e9f, v0 = 1e9f, v1 = -1e9f; for (int k = 0; k < 4; k++) { u0 = std::min(u0, q.uv[k][0]); u1 = std::max(u1, q.uv[k][0]); v0 = std::min(v0, q.uv[k][1]); v1 = std::max(v1, q.uv[k][1]); }
    int x0 = (int)floorf(u0 * W - pt.x + .001f), x1 = (int)ceilf(u1 * W - pt.x - .001f), y0 = (int)floorf(v0 * H - pt.y + .001f), y1 = (int)ceilf(v1 * H - pt.y - .001f);
    x0 = std::max(0, std::min(pt.w - 1, x0)); y0 = std::max(0, std::min(pt.h - 1, y0)); x1 = std::max(x0 + 1, std::min(pt.w, x1)); y1 = std::max(y0 + 1, std::min(pt.h, y1));
    int vis = 0, par = 0; const std::vector<uint8_t>& ca = it->second;
    for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++) { const uint8_t c = ca[(size_t)yy * pt.w + xx]; if (c) { vis++; if (c == 1) par++; } }
    q.trc = (vis > 0 && par * 20 >= vis) ? 1 : 0;   // >= 5% dos texels visiveis da face com alfa parcial
    return q.trc != 0;
}
// regras de translucent.txt (por pacote): {trecho minusculo, opacidade}
static std::vector<std::vector<std::pair<std::string, float>>> gPropGlass; static bool gPropGlassLoaded = false;
static float propGlassAlpha(int pack, const std::string& name) {
    if (!gPropGlassLoaded) { gPropGlassLoaded = true; gPropGlass.resize(gPropPacks.size());
        for (size_t p = 0; p < gPropPacks.size(); p++) { FILE* f = fopen((gPropPacks[p].dir + "/translucent.txt").c_str(), "r"); if (!f) continue; char ln[512];
            while (fgets(ln, sizeof ln, f)) { std::string t = ln; while (!t.empty() && (t.back() == '\n' || t.back() == '\r' || t.back() == ' ')) t.pop_back(); if (t.empty() || t[0] == '#') continue;
                const size_t sp = t.find_last_of(' '); if (sp == std::string::npos) continue; const float a = (float)atof(t.c_str() + sp + 1); std::string k = t.substr(0, sp); while (!k.empty() && k.back() == ' ') k.pop_back();
                for (char& c : k) c = (char)tolower((unsigned char)c); if (!k.empty() && a > 0.f && a < 1.f) gPropGlass[p].push_back({k, a}); }
            fclose(f); } }
    if (pack < 0 || pack >= (int)gPropGlass.size()) return 1.f;
    std::string n = name; for (char& c : n) c = (char)tolower((unsigned char)c);
    for (auto& r : gPropGlass[pack]) if (n.find(r.first) != std::string::npos) return r.second;
    return 1.f;
}
static PropMesh* propMesh(int id) {
    int pack = 0; const PropItem* it = propItem(id, &pack); if (!it) return nullptr;
    PropMesh& m = gPropMesh[id];
    if (m.built) return &m;
    m.built = true; m.alpha = propGlassAlpha(pack, it->name); const PropPack& pk = gPropPacks[pack]; const PropModel& md = pk.models[it->model];
    m.flat = md.boxes.size() == 1 && md.bb[5] - md.bb[2] <= .07f && md.bb[3] - md.bb[0] >= .5f && md.bb[4] - md.bb[1] >= .5f;
    m.q.reserve(md.boxes.size() * 3);
    for (const PropBox& b : md.boxes) for (int f = 0; f < 6; f++) {
        const PropFace& pf = b.f[f]; if (pf.slot < 0) continue;
        float ox = 0, oy = 0, sx = 1, sy = 1; int ti = -1; const int page = propSlotRegion(pk, *it, pf.slot, ox, oy, sx, sy, &ti);
        float qd[4][5]; propsFaceQuad(b, f, qd);
        PropQuad q; q.face = (int8_t)f; q.page = page; q.tex = ti; q.shade = f == 3 ? 1.f : f == 2 ? .5f : (f == 0 || f == 1) ? .66f : .82f;
        for (int k = 0; k < 4; k++) { q.p[k][0] = qd[k][0]; q.p[k][1] = qd[k][1]; q.p[k][2] = qd[k][2]; q.uv[k][0] = ox + qd[k][3] * sx; q.uv[k][1] = oy + qd[k][4] * sy; }
        if (m.alpha < 1.f) {   // vidro: so as faces GRANDES de caixas finas (a vidraca); moldura/cabo continuam opacos
            const int ax = f >> 1, o1 = (ax + 1) % 3, o2 = (ax + 2) % 3;
            if (b.g[3 + ax] * 2 <= .08f && b.g[3 + o1] * 2 * b.g[3 + o2] * 2 >= .12f) q.glass = true; }
        m.q.push_back(q);
    }
    return &m;
}
// icone: projecao isometrica igual a dos cubos (cx0 + hw*(x-z), y + 4u*(x+z) - 8u*y), so faces voltadas para o observador (+X,+Y,+Z), ordenadas de tras para frente.
// Coordenadas em "unidades de 1/16 do slot" (u = s/16 na hora de desenhar); modelo centrado e ampliado para caber em 15x15.
static void propBuildIcon(PropMesh& m) {
    m.iconBuilt = true; m.icon.clear();
    if (m.flat) {   // item plano: a face +Z de frente, como o sprite do item vanilla (nao a laje em perspectiva isometrica)
        float mn[2] = {1e9f, 1e9f}, mx[2] = {-1e9f, -1e9f};
        for (const PropQuad& q : m.q) for (int k = 0; k < 4; k++) for (int a = 0; a < 2; a++) { mn[a] = std::min(mn[a], q.p[k][a]); mx[a] = std::max(mx[a], q.p[k][a]); }
        const float ext = std::max(mx[0] - mn[0], mx[1] - mn[1]); if (ext < 1e-4f) return;
        const float kf = 16.f / ext, cx = (mn[0] + mx[0]) * .5f, cy = (mn[1] + mx[1]) * .5f;
        for (int pass = 0; pass < 2; pass++) { bool any = false;
            for (const PropQuad& q : m.q) { if (q.face != (pass == 0 ? 5 : 4)) continue; any = true;
                PropIconQ o; o.page = q.page; o.shade = 1.f; o.alpha = m.alpha;
                for (int k = 0; k < 4; k++) { o.sx[k] = 8.f + (q.p[k][0] - cx) * kf; o.sy[k] = 8.f - (q.p[k][1] - cy) * kf; o.uv[k][0] = q.uv[k][0]; o.uv[k][1] = q.uv[k][1]; }
                m.icon.push_back(o); }
            if (any) break; }
        return;
    }
    struct T { float d; int i; }; std::vector<T> ord; std::vector<char> vis(m.q.size(), 0);
    float mn[3] = {1e9f, 1e9f, 1e9f}, mx[3] = {-1e9f, -1e9f, -1e9f};
    for (size_t i = 0; i < m.q.size(); i++) { const PropQuad& q = m.q[i];
        for (int k = 0; k < 4; k++) for (int a = 0; a < 3; a++) { mn[a] = std::min(mn[a], q.p[k][a]); mx[a] = std::max(mx[a], q.p[k][a]); }
        float e1[3], e2[3]; for (int a = 0; a < 3; a++) { e1[a] = q.p[1][a] - q.p[0][a]; e2[a] = q.p[3][a] - q.p[0][a]; }
        const float n0 = -(e1[1] * e2[2] - e1[2] * e2[1]), n1 = -(e1[2] * e2[0] - e1[0] * e2[2]), n2 = -(e1[0] * e2[1] - e1[1] * e2[0]);   // normal para fora (cantos TL,TR,BR,BL)
        if (n0 + n1 + n2 > 1e-9f) vis[i] = 1; }
    if (mn[0] > mx[0]) return;
    const float c[3] = {(mn[0] + mx[0]) * .5f, (mn[1] + mx[1]) * .5f, (mn[2] + mx[2]) * .5f}, hw = 6.93f;
    float a0 = 1e9f, a1 = -1e9f, d0 = 1e9f, d1 = -1e9f;
    for (const PropQuad& q : m.q) for (int k = 0; k < 4; k++) {
        const float dx = q.p[k][0] - c[0], dy = q.p[k][1] - c[1], dz = q.p[k][2] - c[2], a = hw * (dx - dz), d = 4 * (dx + dz) - 8 * dy;
        a0 = std::min(a0, a); a1 = std::max(a1, a); d0 = std::min(d0, d); d1 = std::max(d1, d); }
    const float fk = std::min(15.f / std::max(a1 - a0, 1e-3f), 15.f / std::max(d1 - d0, 1e-3f)), fa = (a0 + a1) * .5f, fd = (d0 + d1) * .5f;
    for (size_t i = 0; i < m.q.size(); i++) if (vis[i]) { const PropQuad& q = m.q[i]; float s = 0; for (int k = 0; k < 4; k++) s += q.p[k][0] + q.p[k][1] + q.p[k][2]; ord.push_back({s, (int)i}); }
    std::stable_sort(ord.begin(), ord.end(), [](const T& a, const T& b) { return a.d < b.d; });
    m.icon.reserve(ord.size());
    for (const T& t : ord) { const PropQuad& q = m.q[t.i]; PropIconQ o; o.page = q.page; o.shade = q.shade; o.alpha = m.alpha;
        for (int k = 0; k < 4; k++) {
            const float dx = q.p[k][0] - c[0], dy = q.p[k][1] - c[1], dz = q.p[k][2] - c[2];
            o.sx[k] = 8.f + fk * (hw * (dx - dz) - fa); o.sy[k] = 8.f + fk * (4 * (dx + dz) - 8 * dy - fd); o.uv[k][0] = q.uv[k][0]; o.uv[k][1] = q.uv[k][1]; }
        m.icon.push_back(o); }
}
static void propDrawIcon(int id, float x, float y, float s) {
    PropMesh* m = propMesh(id); if (!m) return;
    if (!m->iconBuilt) propBuildIcon(*m);
    int pack = 0; propItem(id, &pack);
    const float u = s / 16.f; int cur = -2;
    rdEnable(GL_TEXTURE_2D); rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .02f); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // vidro translucido no icone
    bool open = false;
    for (const PropIconQ& q : m->icon) {
        GLuint tex = q.page >= 0 ? propPageTex(pack, q.page) : 0; const int key = tex ? (int)tex : -1;
        if (key != cur) { if (open) { glEnd(); open = false; } cur = key; if (tex) { rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); } else rdDisable(GL_TEXTURE_2D); }
        if (!open) { glBegin(GL_QUADS); open = true; }
        const float sh = q.shade; if (tex) glColor4f(sh, sh, sh, q.alpha); else glColor4f(.55f * sh, .55f * sh, .55f * sh, q.alpha);
        for (int k = 0; k < 4; k++) { if (tex) glTexCoord2f(q.uv[k][0], q.uv[k][1]); glVertex2f(x + q.sx[k] * u, y + q.sy[k] * u); }
    }
    if (open) glEnd();
    glColor4f(1, 1, 1, 1); rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, atlasTex);
}

// ---------- mundo ----------
static void propRotBox(const PropItem& it, const PropModel& md, float cx, float cy, float cz, int rot, AABB& o) {
    const float th = rot * PROP_ROT_STEP * .0174533f, cs = cosf(th), sn = sinf(th);
    float x0 = 1e9f, x1 = -1e9f, z0 = 1e9f, z1 = -1e9f;
    for (int i = 0; i < 4; i++) { const float lx = (i & 1) ? md.bb[3] : md.bb[0], lz = (i & 2) ? md.bb[5] : md.bb[2];
        const float wx = lx * cs + lz * sn, wz = -lx * sn + lz * cs; x0 = std::min(x0, wx); x1 = std::max(x1, wx); z0 = std::min(z0, wz); z1 = std::max(z1, wz); }
    o = AABB{cx + x0, cy + md.bb[1], cz + z0, cx + x1, cy + md.bb[4], cz + z1};
}
static void propQrot(const float* q, float& x, float& y, float& z) {   // mesmo qrot de conversor/modconv/pack.py (w,x,y,z)
    const float w = q[0], qx = q[1], qy = q[2], qz = q[3];
    const float tx = 2 * (qy * z - qz * y), ty = 2 * (qz * x - qx * z), tz = 2 * (qx * y - qy * x);
    const float rx = x + w * tx + (qy * tz - qz * ty), ry = y + w * ty + (qz * tx - qx * tz), rz = z + w * tz + (qx * ty - qy * tx);
    x = rx; y = ry; z = rz;
}
static PropEnt propMake(int id, float x, float y, float z, int rot) {
    PropEnt e; e.id = id; e.x = x; e.y = y; e.z = z; e.rot = rot & 15; if (propConnOf(id)) e.rot = 0;   // patchConv.3: item conectavel segue a grade, sem giro (variante ja vem girada)
    int pack = 0; const PropItem* it = propItem(id, &pack);
    if (!it) return e;
    if (it->beh == "Faucet" || it->beh == "ShowerHead" || it->beh == "ShowerHandles") e.on = 0;   // patchProp.10: agua so corre depois de ligar
    const PropModel& md = gPropPacks[pack].models[it->model];
    const float th = e.rot * PROP_ROT_STEP * .0174533f, cs = cosf(th), sn = sinf(th);
    struct CB { double vol; AABB b; }; std::vector<CB> tmp;
    for (const PropBox& bx : md.boxes) {
        float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
        for (int k = 0; k < 8; k++) {
            float px = (k & 1) ? bx.g[3] : -bx.g[3], py = (k & 2) ? bx.g[4] : -bx.g[4], pz = (k & 4) ? bx.g[5] : -bx.g[5];
            propQrot(bx.g + 6, px, py, pz); px += bx.g[0]; py += bx.g[1]; pz += bx.g[2];
            const float wx = px * cs + pz * sn, wz = -px * sn + pz * cs;
            lo[0] = std::min(lo[0], wx); hi[0] = std::max(hi[0], wx); lo[1] = std::min(lo[1], py); hi[1] = std::max(hi[1], py); lo[2] = std::min(lo[2], wz); hi[2] = std::max(hi[2], wz);
        }
        if (hi[0] - lo[0] < .1f && hi[1] - lo[1] < .1f && hi[2] - lo[2] < .1f) continue;   // fiapo (corrente, pavio): nao vira colisao
        for (int a = 0; a < 3; a++) if (hi[a] - lo[a] < .0625f) { if (a == 1) hi[1] = lo[1] + .0625f; else { const float m = (lo[a] + hi[a]) * .5f; lo[a] = m - .03125f; hi[a] = m + .03125f; } }   // painel de espessura zero (janela, placa): engrossa p/ colidir e receber o raio
        tmp.push_back({(double)(hi[0] - lo[0]) * (hi[1] - lo[1]) * (hi[2] - lo[2]), AABB{x + lo[0], y + lo[1], z + lo[2], x + hi[0], y + hi[1], z + hi[2]}});
    }
    if (tmp.size() > 24) { std::sort(tmp.begin(), tmp.end(), [](const CB& a, const CB& b) { return a.vol > b.vol; }); tmp.resize(24); }
    for (const CB& c : tmp) e.cb.push_back(c.b);
    if (!e.cb.empty()) { e.box = e.cb[0]; for (const AABB& c : e.cb) { e.box.x0 = std::min(e.box.x0, c.x0); e.box.y0 = std::min(e.box.y0, c.y0); e.box.z0 = std::min(e.box.z0, c.z0); e.box.x1 = std::max(e.box.x1, c.x1); e.box.y1 = std::max(e.box.y1, c.y1); e.box.z1 = std::max(e.box.z1, c.z1); } }
    else propRotBox(*it, md, x, y, z, e.rot, e.box);
    e.coll = !it->nocoll && !e.cb.empty() && (e.box.y1 - e.box.y0) > .08f && (e.box.x1 - e.box.x0) > .04 && (e.box.z1 - e.box.z0) > .04;
    return e;
}
static int propWorldFace(int f, int rot) {   // face local -> face mundial (rot em passos de 22,5; so multiplos de 90 dao resultado exato; senao -1)
    if (rot & 3) return -1;
    static const int quarter[4][6] = {{0, 1, 2, 3, 4, 5}, {5, 4, 2, 3, 0, 1}, {1, 0, 2, 3, 5, 4}, {4, 5, 2, 3, 1, 0}};   // 90 graus: +X vira -Z, +Z vira +X ...
    return quarter[(rot >> 2) & 3][f];
}
static void propLightRebuild() {   // luz do prop: item com light>0 emite (lampada so se ligada); refaz a malha dos chunks vizinhos quando muda
    std::vector<std::array<int, 4>> nv;
    for (const PropEnt& e : gProps) { const PropItem* it = propItem(e.id); if (!it || it->light <= 0 || (it->beh == "SwitchableLamp" && !e.on)) continue;
        nv.push_back({(int)floorf(e.x), (int)floorf(e.y + .01f), (int)floorf(e.z), std::min(15, it->light)}); }
    if (nv == gPropLightV) return;
    auto dirtyAround = [&](const std::vector<std::array<int, 4>>& v) { for (const auto& L : v) { const int cx = L[0] >> 4, cz = L[2] >> 4; for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) dirtyC(cx + dx, cz + dz); } };
    dirtyAround(gPropLightV); gPropLightV = nv; dirtyAround(gPropLightV);
}
// patchConv.3: estado por vizinhos. Mascara N=1 E=2 S=4 W=8 (N = -Z, E = +X). Vizinho = prop do mesmo grupo na celula ao lado
// (ou bloco solido, se a regra tiver a flag s). vid = item-variante da mascara; colisao e malha passam a ser as da variante.
static int propConnMask(const std::unordered_map<uint64_t, std::vector<int>>& cell, const PropEnt& e, const PropConnR& r) {
    static const int DX[4] = {0, 1, 0, -1}, DZ[4] = {-1, 0, 1, 0}; int m = 0;
    const int cx = (int)floorf(e.x), cy = (int)floorf(e.y + .01f), cz = (int)floorf(e.z);
    for (int d = 0; d < 4; d++) {
        const int nx = cx + DX[d], nz = cz + DZ[d]; bool ok = false;
        auto it = cell.find(((uint64_t)(uint32_t)(nx + 0x1000000) << 32) ^ ((uint64_t)(uint32_t)(nz + 0x1000000) << 8) ^ (uint64_t)(uint32_t)(cy + 512));
        if (it != cell.end()) for (int j : it->second) { const PropConnR* o = propConnOf(gProps[j].id); if (o && o->grp == r.grp) { ok = true; break; } }
        if (!ok && r.solid && cy >= 0 && cy < CH && getC(fdiv(nx, CW), fdiv(nz, CW)) && solid(getB(nx, cy, nz))) ok = true;
        if (ok) m |= 1 << d;
    }
    return m;
}
static void propConnAll() {
    if (gConn.empty() || gProps.empty()) return;
    std::unordered_map<uint64_t, std::vector<int>> cell;
    for (int i = 0; i < (int)gProps.size(); i++) if (propConnOf(gProps[i].id)) { const PropEnt& e = gProps[i];
        cell[((uint64_t)(uint32_t)((int)floorf(e.x) + 0x1000000) << 32) ^ ((uint64_t)(uint32_t)((int)floorf(e.z) + 0x1000000) << 8) ^ (uint64_t)(uint32_t)((int)floorf(e.y + .01f) + 512)].push_back(i); }
    for (auto& kv : cell) for (int i : kv.second) {
        PropEnt& e = gProps[i]; const PropConnR* r = propConnOf(e.id); if (!r) continue;
        const int v = r->var[propConnMask(cell, e, *r)]; const int nv = (v && v != e.id) ? v : 0;
        if (nv == e.vid) continue;
        const PropEnt t = propMake(nv ? nv : e.id, e.x, e.y, e.z, 0); e.vid = nv; e.cb = t.cb; e.box = t.box; e.coll = t.coll;
    }
}
static void propNbAll() {   // recalcula o culling do vidro (chamar ao colocar/remover/carregar)
    std::unordered_map<int, std::vector<int>> by;
    for (int i = 0; i < (int)gProps.size(); i++) { gProps[i].nb = 0; PropMesh* m = propMesh(gProps[i].id); if (m && m->alpha < 1.f && !(gProps[i].rot & 3)) by[gProps[i].id].push_back(i); }
    for (auto& kv : by) for (int i : kv.second) for (int j : kv.second) { if (i == j || gProps[i].rot != gProps[j].rot) continue;
        for (int d = 0; d < 6; d++) if (fabsf(gProps[j].x - gProps[i].x - FACE_N[d][0]) < .02f && fabsf(gProps[j].y - gProps[i].y - FACE_N[d][1]) < .02f && fabsf(gProps[j].z - gProps[i].z - FACE_N[d][2]) < .02f) gProps[i].nb |= 1 << d; }
    propConnAll();
    for (PropEnt& e : gProps) { const int ov = propOpenOf(e.id); if (!ov || propConnOf(e.id)) continue;   // patchOpen.1: aberto = variante open=true (malha e colisao)
        const int nv = e.on ? 0 : ov; if (nv == e.vid) continue; const PropEnt t = propMake(nv ? nv : e.id, e.x, e.y, e.z, e.rot); e.vid = nv; e.cb = t.cb; e.box = t.box; e.coll = t.coll; }
    propLightRebuild();
}
static void propCollBoxes(const AABB& r, std::vector<AABB>& out) {   // chamado por collBoxes (physics.inc)
    if (gProps.empty()) return;
    if (gPropCollOff) return;
    for (const PropEnt& e : gProps) if (e.coll && r.intersects(e.box)) for (const AABB& c : e.cb) if (r.intersects(c)) out.push_back(c);
}
static int propRotFor(int id, float yawDeg) {
    int rot = (int)floorf(yawDeg / PROP_ROT_STEP + .5f); rot &= 15;   // a frente (+Z do modelo) aponta para o jogador
    int pk = 0; const PropItem* bi = propItem(id, &pk);
    if (bi) { const PropModel& bm = gPropPacks[pk].models[bi->model];   // prop de bloco inteiro (janela, parede...): so 4 direcoes, para encaixar na grade
        const float w = bm.bb[3] - bm.bb[0], d = bm.bb[5] - bm.bb[2]; if (w > .9f && d > .9f && w < 1.1f && d < 1.1f) rot = ((rot + 2) / 4 * 4) & 15; }
    return rot;
}
static float propLiftOf(int id) { int pk = 0; const PropItem* it = propItem(id, &pk); return it ? std::max(0.f, -gPropPacks[pk].models[it->model].bb[1]) : 0.f; }   // modelo que desce abaixo do y=0: apoia no chao
static bool propOverlap(const AABB& a, const AABB& b, double m) { return a.x0 + m < b.x1 && a.x1 - m > b.x0 && a.y0 + m < b.y1 && a.y1 - m > b.y0 && a.z0 + m < b.z1 && a.z1 - m > b.z0; }
// cabe aqui? (limites do mundo, blocos, outros props, jogador). Props sem colisao so nao podem duplicar o mesmo prop no mesmo ponto.
static bool propFits(const PropEnt& e, double px, double py, double pz) {
    if (e.box.y0 < 0 || e.box.y1 > CH) return false;
    if (!getC(fdiv((int)floorf(e.x), CW), fdiv((int)floorf(e.z), CW))) return false;
    for (const PropEnt& o : gProps) if (o.id == e.id && o.rot == e.rot && fabsf(o.x - e.x) < .02f && fabsf(o.y - e.y) < .02f && fabsf(o.z - e.z) < .02f) return false;
    if (!e.coll) return true;
    const AABB pl{px - .3, py, pz - .3, px + .3, py + 1.8, pz + .3};
    for (const AABB& c : e.cb) {
        const AABB s{c.x0 + .01, c.y0 + .01, c.z0 + .01, c.x1 - .01, c.y1 - .01, c.z1 - .01};
        if (propOverlap(c, pl, 0)) return false;
        std::vector<AABB> l; gPropCollOff = true; collBoxes(s, l); gPropCollOff = false; if (!l.empty()) return false;   // dentro de bloco
        for (const PropEnt& o : gProps) if (o.coll && propOverlap(e.box, o.box, .01)) for (const AABB& oc : o.cb) if (propOverlap(c, oc, .02)) return false;
    }
    return true;
}
// patchProp.5b3: props de parede (fino em Z, encostado numa borda: janelas/portas encostam em +Z, quadros/placas/cortinas em -Z).
// Mirando o lado de um bloco: escolhe entre as 4 direcoes a que deixa a borda de tras rente ao plano da parede e encosta o prop nela.
static int propWallSide(int id) {   // +1 = encosta em +Z, -1 = em -Z, 0 = nao e prop de parede
    int pk = 0; const PropItem* it = propItem(id, &pk); if (!it) return 0;
    const float* bb = gPropPacks[pk].models[it->model].bb; const float w = bb[3] - bb[0], h = bb[4] - bb[1], d = bb[5] - bb[2];
    if (d > .35f || std::max(w, h) < .6f) return 0;
    if (bb[2] > -.02f && bb[5] > .2f) return 1;
    if (bb[5] < .02f && bb[2] < -.2f) return -1;
    return 0;
}
static bool propWallPlace(int id, const int* t, int face, int defRot, int& rotOut, float& xo, float& zo) {   // face = lado do bloco mirado (0=-X,1=+X,4=-Z,5=+Z); t = celula onde o prop fica
    if (!(face == 0 || face == 1 || face == 4 || face == 5) || !propWallSide(id)) return false;
    const int ax = (face < 2) ? 0 : 2, sg = FACE_N[face][ax]; const float lift = propLiftOf(id);
    const double plane = (ax == 0 ? t[0] : t[2]) + (sg > 0 ? 0.0 : 1.0); const float cx = t[0] + .5f, cz = t[2] + .5f;
    int best = -1; double bg = 1e9;
    for (int r = 0; r < 16; r += 4) {
        const PropEnt e = propMake(id, cx, t[1] + lift, cz, r); const double edge = sg > 0 ? (ax == 0 ? e.box.x0 : e.box.z0) : (ax == 0 ? e.box.x1 : e.box.z1);
        const double g = std::fabs(edge - plane), dr = std::min(std::abs(r - defRot), 16 - std::abs(r - defRot)) * .001;   // empate: a mais proxima da direcao do jogador
        if (g + dr < bg) { bg = g + dr; best = r; }
    }
    const PropEnt e = propMake(id, cx, t[1] + lift, cz, best); const double edge = sg > 0 ? (ax == 0 ? e.box.x0 : e.box.z0) : (ax == 0 ? e.box.x1 : e.box.z1);
    rotOut = best; xo = cx; zo = cz; if (ax == 0) xo += (float)(plane - edge); else zo += (float)(plane - edge);
    return true;
}
static std::string propKey(int id); static bool propContainerOpen(int i); static void propContainerRemoved(const PropEnt& e); static bool gPropOpenTable = false; static bool gPropOpenChest = false;   // chest.inc (patchProp.5c)
static float propSnap(float v) { return floorf(v * 8.f + .5f) / 8.f; }   // 1/8 de bloco
// fx/fz validos (nao NaN): posicao livre (1/8 de bloco) no ponto mirado, em vez do centro da celula (agachado)
static bool propIsFlat(int id);
// patchBug.2: bloco inteiro de mod (modelo ~1x1x1, fora do Decocraft) segue a GRADE: sempre no centro da celula, nunca em posicao livre; icone plano de mod (so textura) nao e posicionavel
static bool propGridBlock(int id) {
    int pk = 0; const PropItem* bi = propItem(id, &pk); if (!bi || gPropPacks[pk].mod == "decocraft") return false;
    if (propConnOf(id)) return true;   // patchConv.3: mesa/cerca/vidraca ligam por celula
    const PropModel& bm = gPropPacks[pk].models[bi->model]; const float w = bm.bb[3] - bm.bb[0], h = bm.bb[4] - bm.bb[1], d = bm.bb[5] - bm.bb[2];
    return w > .9f && d > .9f && w < 1.1f && d < 1.1f && h > .9f && h < 1.1f;
}
static bool propModFlat(int id) {   // so o SPRITE de item gerado pelo conversor (laje centrada em z, espessura ~0,06); coisa fina de verdade (escada de ferro encostada na parede, espessura 0) segue posicionavel
    int pk = 0; const PropItem* bi = propItem(id, &pk); if (!bi || gPropPacks[pk].mod == "decocraft" || !propIsFlat(id)) return false;
    const PropModel& bm = gPropPacks[pk].models[bi->model]; return fabsf(bm.bb[2] + bm.bb[5]) < .01f && bm.bb[5] - bm.bb[2] > .04f;
}
static bool propPlace(int id, int hx, int hy, int hz, int face, float yawDeg, double px, double py, double pz, double fx = NAN, double fz = NAN) {
    if (!isProp(id) || face < 0 || face > 5) return false;
    if (propModFlat(id)) return false;
    if (propGridBlock(id)) { fx = NAN; fz = NAN; }
    int t[3] = {hx + FACE_N[face][0], hy + FACE_N[face][1], hz + FACE_N[face][2]};
    if (xbReplaceable(getB(hx, hy, hz))) { t[0] = hx; t[1] = hy; t[2] = hz; }
    if (t[1] < 0 || t[1] >= CH || !getC(fdiv(t[0], CW), fdiv(t[2], CW))) return false;
    if (!blockReplaceable(getB(t[0], t[1], t[2]))) return false;
    int rot = propRotFor(id, yawDeg); const float lift = propLiftOf(id);
    float x = t[0] + .5f, z = t[2] + .5f;
    if (!std::isnan(fx) && !std::isnan(fz)) { x = std::min<float>(t[0] + .95f, std::max<float>(t[0] + .05f, propSnap((float)fx))); z = std::min<float>(t[2] + .95f, std::max<float>(t[2] + .05f, propSnap((float)fz))); }
    if (!(t[0] == hx && t[1] == hy && t[2] == hz)) { int wr = 0; float wx = x, wz = z; if (propWallPlace(id, t, face, rot, wr, wx, wz)) { rot = wr; if (face < 2) x = wx; else z = wz; } }   // parede: gira e encosta (a posicao livre ao longo da parede fica)
    const PropEnt e = propMake(id, x, t[1] + lift, z, rot);
    if (!propFits(e, px, py, pz)) return false;
    gProps.push_back(e); propNbAll(); propNetAdd(e); return true;
}
// raio x caixas de cada parte: indice do prop mais proximo (<= maxD) ou -1; tOut = distancia; nAxis/nSign = face atingida (eixo 0..2, sinal da normal)
static int propRay(const float* eye, const float* dir, float maxD, float* tOut = nullptr, int* nAxis = nullptr, int* nSign = nullptr) {
    int best = -1; float bt = maxD; int bax = 1, bsg = 1;
    for (int i = 0; i < (int)gProps.size(); i++) {
        const PropEnt& e = gProps[i];
        { const AABB& b = e.box; const double lo[3] = {b.x0, b.y0, b.z0}, hi[3] = {b.x1, b.y1, b.z1}; double t0 = 0, t1 = bt; bool ok = true;   // descarte rapido pela caixa total
          for (int a = 0; a < 3 && ok; a++) { if (fabsf(dir[a]) < 1e-7f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
              double ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb); t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false; }
          if (!ok) continue; }
        static thread_local std::vector<AABB> one; one.assign(1, e.box);
        const std::vector<AABB>& bl = e.cb.empty() ? one : e.cb;
        for (const AABB& b : bl) {
            const double lo[3] = {b.x0, b.y0, b.z0}, hi[3] = {b.x1, b.y1, b.z1};
            double t0 = 0, t1 = bt; int ax = 1, sg = 1; bool ok = true;
            for (int a = 0; a < 3 && ok; a++) {
                if (fabsf(dir[a]) < 1e-7f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
                double ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; int s0 = -1; if (ta > tb) { std::swap(ta, tb); s0 = 1; }
                if (ta > t0) { t0 = ta; ax = a; sg = s0; } t1 = std::min(t1, tb); if (t0 > t1) ok = false; }
            if (ok && t0 < bt) { bt = (float)t0; best = i; bax = ax; bsg = sg; }
        }
    }
    if (best >= 0) { if (tOut) *tOut = bt; if (nAxis) *nAxis = bax; if (nSign) *nSign = bsg; }
    return best;
}
static int propPick(const float* eye, const float* dir, float maxD, float* tOut = nullptr) { return propRay(eye, dir, maxD, tOut); }
// colocar mirando um PROP (empilhar): 0 = nao mira prop (cai no bloco), 1 = mirou mas nao coube, 2 = colocou.
// Em cima: livre no ponto mirado (1/8 de bloco) sobre a parte atingida; ao lado: encostado na parte, no mesmo nivel da base.
static int propPlaceOnProp(int id, const float* eye, const float* dir, float reach, float yawDeg, double px, double py, double pz, bool freeRot = false) {   // freeRot (agachado): giro livre pelo yaw do jogador, sem herdar o do prop de baixo
    if (!isProp(id) || gProps.empty()) return 0;
    const float lim = gRay.hit ? std::min(reach, (float)gRay.t) : reach;
    float t = 0; int ax = 1, sg = 1; const int i = propRay(eye, dir, lim, &t, &ax, &sg); if (i < 0) return 0;
    const float h[3] = {eye[0] + dir[0] * t, eye[1] + dir[1] * t, eye[2] + dir[2] * t};
    const PropEnt o = gProps[i]; const int rot = propRotFor(id, freeRot ? yawDeg : o.rot * PROP_ROT_STEP); const float lift = propLiftOf(id);   // patchMod.2: empilhado/encostado herda o giro do prop de baixo (antes usava o yaw do jogador e ficava torto)
    if (ax == 1 && sg < 0) return 1;   // por baixo nao
    PropEnt e = propMake(id, 0, 0, 0, rot);
    const float cxo = (e.box.x0 + e.box.x1) * .5f, czo = (e.box.z0 + e.box.z1) * .5f, hw = (e.box.x1 - e.box.x0) * .5f, hd = (e.box.z1 - e.box.z0) * .5f;
    float x, y, z;
    if (ax == 1) { x = propSnap(h[0]); z = propSnap(h[2]); y = h[1] + lift; }
    else { y = o.y - propLiftOf(o.id) + lift; x = ax == 0 ? h[0] + sg * hw - cxo : propSnap(h[0]) - cxo; z = ax == 2 ? h[2] + sg * hd - czo : propSnap(h[2]) - czo; x = propSnap(x); z = propSnap(z); }
    e = propMake(id, x, y, z, rot);
    if (!propFits(e, px, py, pz)) return 1;
    gProps.push_back(e); propNbAll(); propNetAdd(e); return 2;
}
static bool modPropDrop(int id, double x, double y, double z);   // pack/modrules.inc (patchMod.15)
// particulas ao quebrar prop: cor media das texturas do item (se a pagina ainda nao subiu, cinza-madeira), tile neutro (pedra) tingido
static void propBreakFx(const PropEnt& e) {
    if (parts.size() > 1500) return;
    int pack = 0; const PropItem* it = propItem(e.vid ? e.vid : e.id, &pack); if (!it) return;
    float c[3] = {.55f, .45f, .32f}; int n = 0; float a[3] = {0, 0, 0};
    if (pack < (int)gPropTexAvg.size()) for (int t : it->tex) { const int ti = t < 0 ? -1 - t : t; auto f = gPropTexAvg[pack].find(ti); if (f == gPropTexAvg[pack].end()) continue; for (int k = 0; k < 3; k++) a[k] += f->second[k]; n++; }
    if (n) for (int k = 0; k < 3; k++) c[k] = a[k] / n;
    int tile = 0; float junk[3]; partLook(STONE, false, e.x, e.z, tile, junk);
    const AABB& b = e.box; const float br = lightNear((int)floorf(e.x), (int)floorf(e.y + .5f), (int)floorf(e.z));
    const float W = std::max(.2f, (float)(b.x1 - b.x0)), H = std::max(.2f, (float)(b.y1 - b.y0)), D = std::max(.2f, (float)(b.z1 - b.z0));
    const int cnt = std::min(48, std::max(8, (int)(W * H * D * 24.f)));
    auto R = []() { return rand() / (float)RAND_MAX; };
    for (int i = 0; i < cnt; i++) { const float dx = R(), dy = R(), dz = R();
        addPart((float)(b.x0 + dx * W), (float)(b.y0 + dy * H), (float)(b.z0 + dz * D), dx - .5f, dy - .5f, dz - .5f, tile, c, br, 1.f, 1.f); }
}
static std::map<int, float> gPropHard;                      // patchMec.1: id -> dureza (mecanicas.txt, lida do bytecode)
static std::map<int, int> gPropTool;                        // patchMec.5b: id -> classe da ferramenta exigida (1 picareta, 2 machado, 3 pa); ausente = picareta
static std::map<int, float> gPropRes;                       // patchMec.5b: id -> resistencia a explosao (setResistance do bytecode; ausente = imune como antes)
static std::map<int, int> gPropLvl;                         // patchMec.5: id -> nivel de colheita (setHarvestLevel): 1 madeira/pedra... 2 ferro, 3 diamante
static std::map<int, std::string> gPropAct;                 // patchMec.2: id -> acoes de onBlockActivated lidas do bytecode (gui,sentar,estado,som,...)
static std::map<int, std::vector<std::pair<std::string, std::string>>> gPropActArg;   // patchForge.4: id -> (acao, argumento) de acoes.txt (mensagem = texto, som = ns:nome)
static std::map<int, std::string> gPropMeth;                // patchMec.1: id -> metodos sobrescritos no Java (onBlockActivated,...)
static float gAimEye[3] = {0, 0, 0}, gAimDir[3] = {0, 0, 1}; static bool gAimOk = false;   // mira do quadro (frame_begin) para a mineracao de prop
static int gPpX = -1; static float gPpT = 0;                // prop sendo minerado (indice) e tempo
static float propHardOf(int id) { auto it = gPropHard.find(id); return it == gPropHard.end() ? -1.f : it->second; }
static void propRemoveAt(int i, bool creative) {
    const PropEnt e = gProps[i]; gProps.erase(gProps.begin() + i); propNbAll(); propContainerRemoved(e); propNetDel(e); propBreakFx(e);
    if (!creative && !modPropDrop(e.id, e.x, e.y, e.z)) spawnDrop(e.x, e.y + .5, e.z, 0, .15, 0, e.id, 1, 10);   // patchMod.15: loot table do mod, senao o proprio item
}
static void propExplode(double cx, double cy, double cz, float pw) {   // patchMec.5b: explosao quebra props com resistencia conhecida (MC: (res*3/5+.3)*.3 contra a forca restante)
    for (int i = (int)gProps.size() - 1; i >= 0; i--) {
        auto r = gPropRes.find(gProps[i].id); if (r == gPropRes.end()) continue;
        const double dx = gProps[i].x - cx, dy = gProps[i].y + .5 - cy, dz = gProps[i].z - cz, d = sqrt(dx * dx + dy * dy + dz * dz);
        if (pw * (.7f + frand() * .6f) - d <= (r->second * 3.f / 5.f + .3f) * .3f) continue;
        propRemoveAt(i, rand() % std::max(1, (int)pw) != 0);
    }
}
static bool propHit(const float* eye, const float* dir, float reach, bool creative) {   // botao esquerdo: remove o prop mirado
    const float lim = gRay.hit ? std::min(reach, (float)gRay.t) : reach;
    const int i = propPick(eye, dir, lim); if (i < 0) return false;
    if (!creative && propHardOf(gProps[i].id) > 0.f) return false;   // patchMec.1: com dureza, em sobrevivencia quebra segurando (propMineTick)
    propRemoveAt(i, creative); return true;
}
static float breakTime(int b, int held, bool& harvest);
static bool propMineTick(float dt, int heldId) {   // patchMec.1: segurar o botao esquerdo num prop com dureza; true = mirando um (anima o braco)
    if (!gAimOk) { gPpX = -1; gPpT = 0; return false; }
    const float lim = gRay.hit ? std::min(4.5f, (float)gRay.t) : 4.5f;
    const int i = propPick(gAimEye, gAimDir, lim); if (i < 0) { gPpX = -1; gPpT = 0; return false; }
    const float h = propHardOf(gProps[i].id); if (h <= 0.f) { gPpX = -1; gPpT = 0; return false; }
    if (i != gPpX) { gPpX = i; gPpT = 0; }
    static const float SPD[5] = {2.f, 4.f, 6.f, 12.f, 8.f}; int tk = 0, tr = 0; toolKind(heldId, tk, tr);
    static const int LV[5] = {0, 1, 2, 0, 3}; auto lv = gPropLvl.find(gProps[i].id); const int nv = lv == gPropLvl.end() ? 0 : lv->second;
    auto tl = gPropTool.find(gProps[i].id); const int rk = tl == gPropTool.end() ? 1 : tl->second;   // patchMec.5b: axe/shovel (antes so picareta contava)
    const bool hv = nv <= 0 || (tk == rk && LV[tr < 5 ? tr : 0] >= nv);   // patchMec.5: exige picareta do nivel (vanilla: sem nivel = x5 e sem drop)
    const float speed = tk ? SPD[tr < 5 ? tr : 0] : 1.f, need = h * (hv ? 1.5f : 5.f) / speed;
    gPpT += dt; if (gPpT >= need) { propRemoveAt(i, !hv); gPpX = -1; gPpT = 0; }
    return true;
}
// arma 3D na mao: display do modelo (MrCrayfish: fp trans [-3.5,8.5,0] s .8; tp trans [0,6,-7] s 1; cano = +Z). Inclui o recentro/normalizacao de propDrawUnit
static void propGunXf(int id, bool fp, float sd) {
    PropMesh* m = propMesh(id); float mn[3] = {1e9f, 1e9f, 1e9f}, mx[3] = {-1e9f, -1e9f, -1e9f};
    if (m) for (const PropQuad& q : m->q) for (int k = 0; k < 4; k++) for (int a = 0; a < 3; a++) { mn[a] = std::min(mn[a], q.p[k][a]); mx[a] = std::max(mx[a], q.p[k][a]); }
    if (!m || m->q.empty()) { rdTranslatef(0, .2f, 0); rdScalef(.5f, .5f, .5f); rdTranslatef(-.5f, -.5f, -.5f); return; }
    const float ext = std::max(.01f, std::max(mx[0] - mn[0], std::max(mx[1] - mn[1], mx[2] - mn[2])));
    const float tx = fp ? -3.5f * sd : 0.f, ty = fp ? 8.5f : 6.f, tz = fp ? 0.f : -7.f, s = fp ? .8f : 1.f;
    rdTranslatef(tx / 16, ty / 16, tz / 16); rdScalef(s, s, s);
    rdTranslatef((mn[0] + mx[0]) * .5f, (mn[1] + mx[1]) * .5f - .5f, (mn[2] + mx[2]) * .5f); rdScalef(ext, ext, ext); rdTranslatef(-.5f, -.5f, -.5f);
}
static int propArmorSlot(int id) { const PropItem* it = propItem(id); return it ? it->aslot : -1; }
static int propArmorPts(int id) { const PropItem* it = propItem(id); return it && it->aslot >= 0 ? it->apts : 0; }
// textura da camada (0 = capacete/peitoral/botas, 1 = calca) lida de <mod>/armor/<nome>.png; a malha de armadura usa UV de 64x64 -> completa a altura (64x32 vira 64x64)
static GLuint propArmorTex(int id, int layer) {
    int pack = 0; const PropItem* c = propItem(id, &pack); if (!c || c->aslot < 0) return 0; PropItem* it = const_cast<PropItem*>(c); layer = layer ? 1 : 0;
    if (it->aGl[layer]) return it->aGl[layer]; if (it->aTry[layer]) return 0; it->aTry[layer] = 1;
    const std::string& f = it->atex[layer].empty() ? it->atex[0] : it->atex[layer]; if (f.empty()) return 0;
    std::vector<uint8_t> px; int w = 0, h = 0; if (!loadPNG((gPropPacks[pack].dir + "/armor/" + f + ".png").c_str(), px, w, h) || w < 8 || h < 8) return 0;
    if (h < w) { px.resize((size_t)w * w * 4, 0); h = w; }
    GLuint t = 0; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data()); it->aGl[layer] = t; return t;
}
static bool propIsFlat(int id) { PropMesh* m = propMesh(id); return m && m->flat; }   // patchProp.7
// modelo reduzido a caixa unitaria 0..1 (mao, item solto no chao); usa a cor/luz ja definida pelo chamador
static void propDrawUnit(int id) {
    PropMesh* m = propMesh(id); if (!m || m->q.empty()) return;
    int pack = 0; propItem(id, &pack);
    float mn[3] = {1e9f, 1e9f, 1e9f}, mx[3] = {-1e9f, -1e9f, -1e9f};
    for (const PropQuad& q : m->q) for (int k = 0; k < 4; k++) for (int a = 0; a < 3; a++) { mn[a] = std::min(mn[a], q.p[k][a]); mx[a] = std::max(mx[a], q.p[k][a]); }
    const float ext = std::max(mx[0] - mn[0], std::max(mx[1] - mn[1], mx[2] - mn[2])), kk = ext > 1e-4f ? 1.f / ext : 1.f;
    const float hg = 1.f - .62f * gEntHurt;
    rdEnable(GL_TEXTURE_2D); int cur = -2; bool open = false;
    for (const PropQuad& q : m->q) {
        GLuint tex = q.page >= 0 ? propPageTex(pack, q.page) : 0; const int key = tex ? (int)tex : -1;
        if (key != cur) { if (open) { glEnd(); open = false; } cur = key; if (tex) { rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); } else rdDisable(GL_TEXTURE_2D); }
        if (!open) { glBegin(GL_QUADS); open = true; }
        const float g = (tex ? 1.f : .55f) * q.shade; glColor3f(g * gEntDim[0], g * gEntDim[1] * hg, g * gEntDim[2] * hg);   // patchProp.7: luz do local (ciclo dia/noite, caverna) + dano, igual a drawMesh
        for (int k = 0; k < 4; k++) { if (tex) glTexCoord2f(q.uv[k][0], q.uv[k][1]);
            glVertex3f(.5f + (q.p[k][0] - (mn[0] + mx[0]) * .5f) * kk, .5f + (q.p[k][1] - (mn[1] + mx[1]) * .5f) * kk, .5f + (q.p[k][2] - (mn[2] + mx[2]) * .5f) * kk); }
    }
    if (open) glEnd();
    glColor4f(1, 1, 1, 1); rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, atlasTex);
}
// ---------- comportamentos (patchProp.4) ----------
// beh do item: "SwitchableLamp" (liga/desliga), "Door:N" / "LinkedModel:N" (troca pelo modelo do item deco_N: porta aberta/fechada).
static int propFromKey(const std::string& k);
static const double PROP_SIT_DROP = .586;   // patchProp.6: pes = topo do assento - 0,586 (quadril do modelo a 0,703 dos pes; a coxa, de 4 px, assenta no tampo)
static void propSeatFor(const PropEnt& e, double* o, float* yawOut = nullptr) {   // assento = topo da maior caixa horizontal entre 0,25 e 0,9 de altura (senao, metade da altura); o = onde ficam os PES do jogador sentado
    double best = 0, top = e.y + (e.box.y1 - e.y) * .5, cx = e.x, cz = e.z, dm = .4;
    for (const AABB& b : e.cb) { const double h = b.y1 - e.y, ar = (b.x1 - b.x0) * (b.z1 - b.z0); if (h >= .25 && h <= .9 && ar > best) { best = ar; top = b.y1; cx = (b.x0 + b.x1) / 2; cz = (b.z0 + b.z1) / 2; dm = std::min(b.x1 - b.x0, b.z1 - b.z0); } }
    const double th = e.rot * PROP_ROT_STEP * .0174533, back = std::min(.15, dm * .25);   // o quadril fica um pouco para tras do centro do assento (as pernas passam da borda da frente)
    o[0] = cx - sin(th) * back; o[1] = top - PROP_SIT_DROP; o[2] = cz - cos(th) * back;
    if (yawOut) { float y = e.rot * PROP_ROT_STEP + 180.f; while (y >= 180.f) y -= 360.f; *yawOut = y; }   // o jogador olha para a frente (+Z) do modelo; bodyYaw = giro + 180
}
static bool propSeatTaken(const PropEnt& e) {   // outro jogador (rede) ja sentado neste assento
    double o[3]; propSeatFor(e, o);
    for (const auto& kv : remotes) { const RemotePl& r = kv.second; if (r.init && r.sit && std::fabs(r.x - o[0]) < .45 && std::fabs(r.z - o[2]) < .45 && std::fabs(r.y - o[1]) < .6) return true; }
    return false;
}
static void propSeatStand(Phys& P, const PropEnt& f) {   // levanta: ao lado/na frente da cadeira se couber, senao em cima
    const double fy = f.y - propLiftOf(f.id) + .01, th = f.rot * PROP_ROT_STEP * .0174533, cx = (f.box.x0 + f.box.x1) / 2, cz = (f.box.z0 + f.box.z1) / 2;
    static const double ang[4] = {0, 1.5707963, -1.5707963, 3.1415927};   // frente, direita, esquerda, tras
    for (double d : {.8, 1.0}) for (double a : ang) {
        const double x = cx + sin(th + a) * d, z = cz + cos(th + a) * d; const AABB pl{x - .3, fy, z - .3, x + .3, fy + 1.8, z + .3};
        std::vector<AABB> l; collBoxes(pl, l); if (!l.empty()) continue;
        P.x = P.px = x; P.y = P.py = fy; P.z = P.pz = z; P.mx = P.my = P.mz = 0; return; }
    P.x = P.px = cx; P.y = P.py = f.box.y1 + .05; P.z = P.pz = cz; P.mx = P.my = P.mz = 0;
}
static bool propSeatTick(Phys& P, bool leave) {   // por tick: segura o jogador no assento; false = em pe (saiu, agachou/pulou ou o prop sumiu)
    if (!gSeated) return false;
    const PropEnt* f = nullptr;
    for (const PropEnt& e : gProps) if (std::fabs(e.x - gSeatPx) < 1e-4 && std::fabs(e.y - gSeatPy) < 1e-4 && std::fabs(e.z - gSeatPz) < 1e-4) { f = &e; break; }
    if (!f || leave) { gSeated = false; if (f) propSeatStand(P, *f); else { P.y = P.py = P.y + PROP_SIT_DROP; P.mx = P.my = P.mz = 0; } return false; }
    double o[3]; propSeatFor(*f, o, &gSeatYaw);
    P.x = P.px = o[0]; P.y = P.py = o[1]; P.z = P.pz = o[2]; P.mx = P.my = P.mz = 0; P.onGround = true; P.sprint = false; return true;
}
// patchProp.11b: dormir na cama de prop. Usar a cama de noite deita o jogador (preso, tela escurece); apos 5 s pula a noite; agachar/pular acorda antes.
static float gSleepA = 0;   // gSleepT/gSleepYaw ficam em net.inc (patchNet.2)
 static double gSleepPx = 0, gSleepPy = 0, gSleepPz = 0;
static bool propSleepTick(Phys& P, bool leave) { return sleepTick(P, leave); }   // patchSleep.1: logica unificada (cama de prop e de bloco, todos os jogadores) em sleep.inc
static void modPropSound(int id, double x, double y, double z);   // patchSnd.mod1 (modrules.inc)
static void modPropActArgs(int id, double x, double y, double z);   // patchForge.4 (modrules.inc)
static int propUse(const float* eye, const float* dir, float reach) {   // botao direito num prop sem agachar: 0 = nada, 1 = usou
    if (gProps.empty()) return 0;
    const float lim = gRay.hit ? std::min(reach, (float)gRay.t) : reach;
    const int i = propRay(eye, dir, lim); if (i < 0) return 0;
    int pack = 0; const PropItem* it = propItem(gProps[i].id, &pack); if (!it) return 0;
    modPropSound(gProps[i].id, gProps[i].x, gProps[i].y, gProps[i].z);   // patchSnd.mod1
    modPropActArgs(gProps[i].id, gProps[i].x, gProps[i].y, gProps[i].z);
    if (!propKey(gProps[i].id).empty() && modulesPropUse(propKey(gProps[i].id), (int)floorf(gProps[i].x), (int)floorf(gProps[i].y), (int)floorf(gProps[i].z))) return 1;   // patchJVM.11: Block.onBlockActivated do mod (AOT)   // patchForge.4: mensagem/som que o onBlockActivated do mod manda
    if (propOpenOf(gProps[i].id)) { gProps[i].on ^= 1; propNbAll(); propNetSwap(gProps[i], gProps[i]); if (it->beh != "Container") return 1; }   // patchOpen.1: portao abre/fecha (bau com tampa: alterna e abre o inventario)
    if (it->beh == "SwitchableLamp") { gProps[i].on ^= 1; propNetSwap(gProps[i], gProps[i]); propLightRebuild(); return 1; }
    if (it->beh == "Faucet" || it->beh == "ShowerHead" || it->beh == "ShowerHandles") {
        gProps[i].on ^= 1; propNetSwap(gProps[i], gProps[i]);
        if (it->beh == "ShowerHandles") {   // patchProp.13: o registro liga/desliga os chuveiros proximos (3 blocos na horizontal, 3 na vertical)
            const PropEnt h = gProps[i];
            for (size_t k = 0; k < gProps.size(); k++) { if ((int)k == i) continue; PropEnt& q = gProps[k];
                if (fabs(q.x - h.x) > 3 || fabs(q.z - h.z) > 3 || fabs(q.y - h.y) > 3) continue;
                const PropItem* qi = propItem(q.id); if (!qi || qi->beh != "ShowerHead" || q.on == h.on) continue;
                q.on = h.on; propNetSwap(q, q); }
        }
        return 1; }
    if (it->beh == "Decobench") { gPropOpenTable = true; return 1; }   // patchProp.15: Bancada de Artesanato abre a mesa de trabalho 3x3
    if (it->beh == "Container") { if (propContainerOpen(i)) { gPropOpenChest = true; return 1; } return 0; }
    if (it->beh == "Chair") { if (gSeated || propSeatTaken(gProps[i])) return 1; gSeated = true; gSeatPx = gProps[i].x; gSeatPy = gProps[i].y; gSeatPz = gProps[i].z; return 1; }
    if (it->beh == "Bed") { const PropEnt& e = gProps[i]; gSpX = e.x; gSpY = e.box.y1 + .1; gSpZ = e.z; if (sleepCanStart(e.x, e.y, e.z)) { sleepBegin(1); gSleepPx = e.x; gSleepPy = e.y; gSleepPz = e.z; gSleepYaw = e.rot * PROP_ROT_STEP; } return 1; }   // spawn sempre; de noite deita (propSleepTick pula a noite)
    { auto ac = gPropAct.find(gProps[i].id); if (ac != gPropAct.end() && it->beh.empty() && ac->second.find("sentar") != std::string::npos) {   // patchMec.2: o Java manda sentar (SeatUtil.sitOnBlock) e o perfil nao deu beh Chair
        if (gSeated || propSeatTaken(gProps[i])) return 1; gSeated = true; gSeatPx = gProps[i].x; gSeatPy = gProps[i].y; gSeatPz = gProps[i].z; return 1; } }
    const size_t c = it->beh.find(':');
    if (c != std::string::npos && (it->beh.compare(0, c, "Door") == 0 || it->beh.compare(0, c, "LinkedModel") == 0)) {
        const int nid = propFromKey(gPropPacks[pack].mod + "/deco_" + it->beh.substr(c + 1)); if (!nid) return 0;
        const PropEnt o = gProps[i]; PropEnt e = propMake(nid, o.x, o.y - propLiftOf(o.id) + propLiftOf(nid), o.z, o.rot); e.on = o.on; gProps[i] = e; propNbAll(); propNetSwap(o, e); return 1; }
    return 0;
}
static void drawProps(float pt, double cpx, double cpz, int only = -1) {   // only: 0 = so opacos, 1 = so translucidos (vidro; vem DEPOIS das nuvens), -1 = os dois
    if (gProps.empty()) return;
    std::vector<std::pair<double, int>> vis;   // props proximos, do mais longe para o mais perto (a passada translucida precisa dessa ordem)
    for (int i = 0; i < (int)gProps.size(); i++) { const double dx = gProps[i].x - cpx, dz = gProps[i].z - cpz, d = dx * dx + dz * dz; if (d > 80.0 * 80.0) continue;
        { const AABB& b = gProps[i].box; if (gFrsOn && (b.x1 > b.x0 || b.y1 > b.y0) && frustumOutside(gFrs, (float)b.x0 - 1.f, (float)b.y0 - 1.f, (float)b.z0 - 1.f, (float)b.x1 + 1.f, (float)b.y1 + 1.f, (float)b.z1 + 1.f, 0.f)) continue; }   // patchRpiCull.1: prop fora da tela
        vis.push_back({-d, i}); }
    if (vis.empty()) return;
    std::sort(vis.begin(), vis.end());
    rdEnable(GL_TEXTURE_2D);
    for (int pass = 0; pass < 2; pass++) {
        if (only >= 0 && pass != only) continue;   // 0 = opaco/recorte (escreve profundidade); 1 = translucido (blend, sem escrever profundidade)
        if (pass == 0) { rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .1f); }
        else { rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .02f); rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); rdDepthMask(GL_FALSE); }
        for (const auto& vv : vis) {
            const PropEnt& e = gProps[vv.second];
            PropMesh* m = propMesh(e.vid ? e.vid : e.id); if (!m) continue;
            int pack = 0; propItem(e.vid ? e.vid : e.id, &pack);
            float lt[3]; entLightAt(e.x, e.y + .5, e.z, lt);
            rdPushMatrix(); rdTranslatef(e.x, e.y, e.z); if (e.rot) rdRotatef(e.rot * PROP_ROT_STEP, 0, 1, 0);
            int cur = -2; bool open = false;
            for (const PropQuad& q : m->q) {
                GLuint tex = q.page >= 0 ? propPageTex(pack, q.page) : 0;
                const bool tr = q.glass || (tex && propQuadTrans(pack, q)); if (tr != (pass == 1)) continue;
                if (q.glass && e.nb) { const int wf = propWorldFace(q.face, e.rot); if (wf >= 0 && (e.nb >> wf & 1)) { const int ax = q.face >> 1; const float c = q.p[0][ax]; if (ax == 1 ? (c <= .02f || c >= .98f) : fabsf(c) >= .45f) continue; } }   // culling do vidro: face colada em vidro igual
                const int key = tex ? (int)tex : -1;
                if (key != cur) { if (open) { glEnd(); open = false; } cur = key; if (tex) { rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); } else rdDisable(GL_TEXTURE_2D); }
                if (!open) { glBegin(GL_QUADS); open = true; }
                const float g = tex ? 1.f : .55f; const float dk = e.on ? 1.f : .35f; glColor4f(lt[0] * q.shade * g * dk, lt[1] * q.shade * g * dk, lt[2] * q.shade * g * dk, q.glass ? m->alpha : 1.f);
                for (int k = 0; k < 4; k++) { if (tex) glTexCoord2f(q.uv[k][0], q.uv[k][1]); glVertex3f(q.p[k][0], q.p[k][1], q.p[k][2]); }
            }
            if (open) glEnd();
            rdPopMatrix();
        }
        if (pass == 1) { rdDepthMask(GL_TRUE); rdDisable(GL_BLEND); }
    }
    rdDisable(GL_ALPHA_TEST); glColor4f(1, 1, 1, 1); rdEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, atlasTex);
}

// ---------- salvar / carregar (chave estavel = mod/id do item, nao o id numerico) ----------
static std::string propKey(int id) { int pack = 0; const PropItem* it = propItem(id, &pack); return it ? gPropPacks[pack].mod + "/" + it->id : std::string(); }
static int propFromKey(const std::string& k) {
    for (size_t p = 0; p < gPropPacks.size(); p++) { const std::string pre = gPropPacks[p].mod + "/"; if (k.rfind(pre, 0) != 0) continue;
        const std::string nm = k.substr(pre.size()); const auto& its = gPropPacks[p].items;
        for (size_t j = 0; j < its.size(); j++) if (its[j].id == nm) { const int id = PROP0 + gPropBase[p] + (int)j; return isProp(id) ? id : 0; } }
    return 0;
}
static void propsWorldSave(const std::string& dir) {
    const std::string path = "saves/" + dir + "/props.dat";
    if (gProps.empty()) { remove(path.c_str()); return; }
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fprintf(f, "PROPS1\n");
    for (const PropEnt& e : gProps) { const std::string k = propKey(e.id); if (!k.empty()) fprintf(f, "p %s %.3f %.3f %.3f %d %d\n", k.c_str(), e.x, e.y, e.z, e.rot, (int)e.on); }
    fclose(f);
}
static void propsWorldLoad(const std::string& dir) {   // chamar depois de resetWorldData()
    gProps.clear();
    FILE* f = fopen(("saves/" + dir + "/props.dat").c_str(), "rb"); if (!f) return; char ln[1024];
    while (fgets(ln, sizeof ln, f)) { char key[600]; float x, y, z; int rot, on = 1;
        if (ln[0] == 'p' && sscanf(ln, "p %599s %f %f %f %d %d", key, &x, &y, &z, &rot, &on) >= 5) { const int id = propFromKey(key); if (id) { PropEnt pe = propMake(id, x, y, z, rot); pe.on = on ? 1 : 0; gProps.push_back(pe); } } }
    fclose(f); propNbAll();
}

// ---------- patchProp.6: rede (NP_PROP) ----------
// Mesmo modelo dos blocos: quem faz a acao aplica local e manda; o host repassa aos outros convidados. Props sao identificados por (item, posicao, giro).
// op 1 = colocar (n + entradas: chave, x, y, z, giro, ligada); op 2 = remover (entrada); op 3 = trocar (chave antiga, chave nova, x, y, z da antiga, giro, ligada: porta/lampada); op 4 = cama (convidado pede ao host pular a noite).
enum { PROPNET_ADD = 1, PROPNET_DEL = 2, PROPNET_SWAP = 3, PROPNET_BED = 4 };
static void propNetStr(NBuf& b, const std::string& t) { const int l = (int)std::min<size_t>(t.size(), 120); b.put<uint8_t>((uint8_t)l); for (int i = 0; i < l; i++) b.put<uint8_t>((uint8_t)t[i]); }
static std::string propNetGetStr(NBuf& b) { const int l = b.get<uint8_t>(); std::string r; for (int i = 0; i < l; i++) r += (char)b.get<uint8_t>(); return r; }
static void propNetEnt(NBuf& b, const PropEnt& e) { propNetStr(b, propKey(e.id)); b.put<float>(e.x); b.put<float>(e.y); b.put<float>(e.z); b.put<uint8_t>((uint8_t)e.rot); b.put<uint8_t>(e.on); }
// patchProp.8: UDP nao confirma; ADD/DEL/SWAP sao idempotentes, entao cada op e reenviada mais 2 vezes (150 ms). Op nova na mesma celula (x,z,giro) descarta as pendentes antigas (evita ressuscitar prop removido).
struct PropRep { NBuf b; int left; float x, z; int rot; };
static std::vector<PropRep> gPropRep;
static void propRepQueue(const NBuf& b, float x, float z, int rot) {
    for (size_t i = gPropRep.size(); i-- > 0;) if (gPropRep[i].rot == (rot & 15) && fabsf(gPropRep[i].x - x) < .02f && fabsf(gPropRep[i].z - z) < .02f) gPropRep.erase(gPropRep.begin() + i);
    if (gPropRep.size() < 400) gPropRep.push_back({b, 2, x, z, rot & 15});
}
static void propNetRepeatTick(Uint32 now) {
    static Uint32 last = 0; if (now - last < 150) return; last = now;
    if (!netOn) { gPropRep.clear(); return; }
    for (size_t i = 0; i < gPropRep.size();) { NBuf c = gPropRep[i].b; netBroadcast(c); if (--gPropRep[i].left <= 0) gPropRep.erase(gPropRep.begin() + i); else i++; }
}
static void propNetAdd(const PropEnt& e) { if (!netOn || propKey(e.id).empty()) return; NBuf b = netMk(NP_PROP); b.put<uint8_t>(PROPNET_ADD); b.put<uint8_t>(1); propNetEnt(b, e); netBroadcast(b); propRepQueue(b, e.x, e.z, e.rot); }
static void propNetDel(const PropEnt& e) { if (!netOn || propKey(e.id).empty()) return; NBuf b = netMk(NP_PROP); b.put<uint8_t>(PROPNET_DEL); propNetEnt(b, e); netBroadcast(b); propRepQueue(b, e.x, e.z, e.rot); }
static void propNetSwap(const PropEnt& o, const PropEnt& n) {
    if (!netOn || propKey(o.id).empty() || propKey(n.id).empty()) return;
    NBuf b = netMk(NP_PROP); b.put<uint8_t>(PROPNET_SWAP); propNetStr(b, propKey(o.id)); propNetStr(b, propKey(n.id));
    b.put<float>(o.x); b.put<float>(o.y); b.put<float>(o.z); b.put<uint8_t>((uint8_t)o.rot); b.put<uint8_t>(n.on); netBroadcast(b); propRepQueue(b, o.x, o.z, o.rot);
}
static void propNetBed() { if (!netOn) return; NBuf b = netMk(NP_PROP); b.put<uint8_t>(PROPNET_BED); netBroadcast(b); }
static int propFind(int id, float x, float y, float z, int rot) {
    for (int i = 0; i < (int)gProps.size(); i++) { const PropEnt& o = gProps[i]; if (o.id == id && o.rot == (rot & 15) && fabsf(o.x - x) < .02f && fabsf(o.y - y) < .02f && fabsf(o.z - z) < .02f) return i; }
    return -1;
}
static bool propPosOk(float x, float y, float z) { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && fabsf(x) < 3e7f && fabsf(z) < 3e7f && y > -64.f && y < 4096.f; }   // patchProp.11: pacote truncado/corrompido nao pode criar prop com NaN/infinito (quebrava luz/colisao dos outros jogadores)
static void propNetRecv(NBuf& b, int sender) {
    const int op = b.get<uint8_t>();
    if (op == PROPNET_ADD) {
        int n = b.get<uint8_t>(); bool ch = false;
        while (n-- > 0 && b.r < b.n) {
            const std::string k = propNetGetStr(b); const float x = b.get<float>(), y = b.get<float>(), z = b.get<float>(); const int rot = b.get<uint8_t>(), on = b.get<uint8_t>(); const int id = propFromKey(k);
            if (!id || !propPosOk(x, y, z) || propFind(id, x, y, z, rot) >= 0 || gProps.size() >= 20000) continue;
            PropEnt e = propMake(id, x, y, z, rot); e.on = on ? 1 : 0; gProps.push_back(e); ch = true; }
        if (ch) propNbAll();
    } else if (op == PROPNET_DEL) {
        const std::string k = propNetGetStr(b); const float x = b.get<float>(), y = b.get<float>(), z = b.get<float>(); const int rot = b.get<uint8_t>(); b.get<uint8_t>();
        if (!propPosOk(x, y, z)) return; const int id = propFromKey(k), i = id ? propFind(id, x, y, z, rot) : -1; if (i < 0) return;
        const PropEnt e = gProps[i]; gProps.erase(gProps.begin() + i); propNbAll(); propContainerRemoved(e); propBreakFx(e);
    } else if (op == PROPNET_SWAP) {
        const std::string ko = propNetGetStr(b), kn = propNetGetStr(b); const float x = b.get<float>(), y = b.get<float>(), z = b.get<float>(); const int rot = b.get<uint8_t>(), on = b.get<uint8_t>();
        if (!propPosOk(x, y, z)) return; const int oid = propFromKey(ko), nid = propFromKey(kn), i = oid ? propFind(oid, x, y, z, rot) : -1; if (i < 0 || !nid) return;
        if (nid == oid) { gProps[i].on = on ? 1 : 0; propLightRebuild(); return; }
        const PropEnt o = gProps[i]; PropEnt e = propMake(nid, o.x, o.y - propLiftOf(o.id) + propLiftOf(nid), o.z, o.rot); e.on = on ? 1 : 0; gProps[i] = e; propNbAll();
    } else if (op == PROPNET_BED) {
        // patchSleep.1: ignorado - o host decide sozinho quando TODOS dormiram (sleep.inc: sleepAllReady/sleepSkipNight)
    }
}
static void propNetSyncPeer(NetPeer& p) {   // convidado acabou de entrar: manda todos os props existentes (ate 24 por pacote)
    for (size_t i = 0; i < gProps.size();) {
        NBuf b = netMk(NP_PROP); b.put<uint8_t>(PROPNET_ADD); const size_t at = b.n; b.put<uint8_t>(0); int c = 0;
        while (i < gProps.size() && c < 24 && b.n + 140 < (int)sizeof b.d) { if (!propKey(gProps[i].id).empty()) { propNetEnt(b, gProps[i]); c++; } i++; }
        b.d[at] = (uint8_t)c; if (c) p.q.push_back(b);
    }
}

// patchProp.10: efeitos de prop (20 Hz, perto do jogador): ParticleEmitter = fumaca fina saindo do topo (castical); Faucet/ShowerHead ligados = gotas de agua caindo.
static void propFxTick(const Phys& P) {
    static unsigned tk = 0; tk++;
    if (gProps.empty()) return;
    int n = 0;
    for (const PropEnt& e : gProps) {
        if (fabs(e.x - P.x) > 24 || fabs(e.z - P.z) > 24) continue;
        const PropItem* it = propItem(e.id); if (!it || it->beh.empty()) continue;
        const bool smoke = it->beh == "ParticleEmitter", water = (it->beh == "Faucet" || it->beh == "ShowerHead") && e.on;
        if (!smoke && !water) continue;
        if (++n > 40 || PUFFS.size() > 600) break;
        if (smoke && (tk + (unsigned)(e.x * 7 + e.z * 13)) % 6 != 0) continue;
        Puff p; const float th = e.rot * PROP_ROT_STEP * .0174533f, fx = sinf(th) * .15f, fz = cosf(th) * .15f;
        const float top = (float)e.box.y1;
        if (smoke) { p.x = e.x + fx * .3f; p.y = top; p.z = e.z + fz * .3f; p.vx = p.vz = 0; p.vy = .01f; p.s = .5f; p.c = .85f; p.max = 24; p.kind = 0; }
        else if (it->beh == "ShowerHead") {   // patchProp.16: cone de gotas saindo da parte de baixo da cabeca (varias por tick), caem ate o chao
            for (int k = 0; k < 3; k++) { Puff q = p; const float a = frand() * 6.2832f, r = frand() * .06f;
                q.x = e.x + fx * .5f + cosf(a) * r; q.z = e.z + fz * .5f + sinf(a) * r; q.y = (float)e.box.y0 + .1f;
                q.vx = cosf(a) * r * .5f + fx * .06f; q.vz = sinf(a) * r * .5f + fz * .06f; q.vy = -.02f; q.s = .3f; q.c = 1.f; q.max = 60; q.kind = 1;
                q.px = q.x; q.py = q.y; q.pz = q.z; q.age = 0; PUFFS.push_back(q); }
            continue; }
        else { p.x = e.x + fx + (frand() - .5f) * .03f; p.z = e.z + fz + (frand() - .5f) * .03f;   // torneira: fio de agua saindo do bico, reto para baixo
            p.y = (float)e.box.y0 + (float)(e.box.y1 - e.box.y0) * .7f; p.vx = p.vz = 0; p.vy = 0; p.s = .35f; p.c = 1.f; p.max = 30; p.kind = 1; }
        p.px = p.x; p.py = p.y; p.pz = p.z; p.age = 0; PUFFS.push_back(p);
    }
}
