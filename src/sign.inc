// patchSign.1: texto das placas (4 linhas). Editor ao colocar/clicar com o botao direito; texto 3D na tabua (fonte 5x7, preto, 1 unidade = 1/96 bloco
// como o TileEntitySignRenderer do 1.12.2); salvar em saves/<mundo>/signs.dat. SEM rede ainda (texto local; ver PENDENCIAS).
struct SignT { int x, y, z; std::array<std::string, 4> l; };
static std::map<int64_t, SignT> gSignTxt;
static bool gSignEd = false; static int gSignX = 0, gSignY = 0, gSignZ = 0, gSignLine = 0;
static int gSignWatch[4] = {0, 0, 0, 0};   // x y z + quadros restantes: placa recem-colocada abre o editor
static inline int64_t signKeyOf(int x, int y, int z) { return ((int64_t)(x & 0x3FFFFFF) << 38) | ((int64_t)(z & 0x3FFFFFF) << 12) | (int64_t)(y & 0xFFF); }
static void signOpen(int x, int y, int z) {
    SignT& s = gSignTxt[signKeyOf(x, y, z)]; s.x = x; s.y = y; s.z = z;
    gSignEd = true; gSignX = x; gSignY = y; gSignZ = z; gSignLine = 0;
}
static void signNetPut(NBuf& b, int x, int y, int z, const std::array<std::string, 4>* l) {
    b.put<int32_t>(x); b.put<int32_t>(y); b.put<int32_t>(z);
    for (int i = 0; i < 4; i++) { const std::string s = l ? (*l)[i] : std::string(); const int n = (int)std::min<size_t>(s.size(), 100); b.put<uint8_t>((uint8_t)n); for (int k = 0; k < n; k++) b.put<uint8_t>((uint8_t)s[k]); }
}
static void signNetRecv(NBuf& b) {   // outro jogador concluiu uma placa (linhas vazias = removeu o texto)
    SignT s; s.x = b.get<int32_t>(); s.y = b.get<int32_t>(); s.z = b.get<int32_t>(); bool any = false;
    for (int i = 0; i < 4; i++) { const int n = b.get<uint8_t>(); for (int k = 0; k < n && b.r < b.n; k++) s.l[i] += (char)b.get<uint8_t>(); if (!s.l[i].empty()) any = true; }
    if (any) gSignTxt[signKeyOf(s.x, s.y, s.z)] = s; else gSignTxt.erase(signKeyOf(s.x, s.y, s.z));
}
static void signNetSyncPeer(NetPeer& p) { int c = 0; for (auto& kv : gSignTxt) { if (c++ >= 200) break; NBuf b = netMk(NP_SIGN); signNetPut(b, kv.second.x, kv.second.y, kv.second.z, &kv.second.l); p.q.push_back(b); } }
static void signDone() {
    gSignEd = false; auto it = gSignTxt.find(signKeyOf(gSignX, gSignY, gSignZ));
    if (it != gSignTxt.end()) { bool any = false; for (auto& s : it->second.l) if (!s.empty()) any = true;
        if (netOn) { NBuf b = netMk(NP_SIGN); signNetPut(b, gSignX, gSignY, gSignZ, any ? &it->second.l : nullptr); netBroadcast(b); }
        if (!any) gSignTxt.erase(it); }
}
static bool gSignSel = false;   // patchRpiOpt.7: Ctrl+A na placa
static void signText(const char* t) {
    auto it = gSignTxt.find(signKeyOf(gSignX, gSignY, gSignZ)); if (it == gSignTxt.end() || !t) return;
    std::string& l = it->second.l[gSignLine & 3]; if (gSignSel) { l.clear(); gSignSel = false; } const std::string n = l + t;
    if (textWidth(n.c_str(), 1.f) <= 90.f) l = n;   // 90 unidades cabem na tabua (como o Minecraft)
}
static void signKey(int k) {
    auto it = gSignTxt.find(signKeyOf(gSignX, gSignY, gSignZ)); if (it == gSignTxt.end()) { gSignEd = false; return; }
    std::string& l = it->second.l[gSignLine & 3];
    if (k == SDLK_a && (SDL_GetModState() & KMOD_CTRL)) gSignSel = !l.empty();
    else if (k == SDLK_BACKSPACE && gSignSel) { l.clear(); gSignSel = false; }
    else if (k == SDLK_BACKSPACE) { while (!l.empty() && (l.back() & 0xC0) == 0x80) l.pop_back(); if (!l.empty()) l.pop_back(); }
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_DOWN) { if (gSignLine >= 3 && k != SDLK_DOWN) signDone(); else gSignLine = std::min(3, gSignLine + 1); }
    else if (k == SDLK_UP) gSignLine = std::max(0, gSignLine - 1);
    else if (k == SDLK_ESCAPE) signDone();
}
static GLuint gSignUITex = 0; static bool gSignUITried = false;
static GLuint signUITex() {   // patchSignUI.1: textura REAL da placa (entity/sign.png do pack, 64x32) para o editor, como o GuiEditSign
    if (!gSignUITried) { gSignUITried = true; std::vector<uint8_t> px; int w = 0, h = 0;
        if ((packRGBA("entity/sign.png", px, w, h) || packRGBA("entity/signs/oak.png", px, w, h)) && w > 0 && h > 0) {
            glGenTextures(1, &gSignUITex); glBindTexture(GL_TEXTURE_2D, gSignUITex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data()); } }
    return gSignUITex;
}
static void signQuadUV(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1) {   // u/v em pixels da textura 64x32
    glTexCoord2f(u0 / 64.f, v0 / 32.f); glVertex2f(x0, y0); glTexCoord2f(u1 / 64.f, v0 / 32.f); glVertex2f(x1, y0);
    glTexCoord2f(u1 / 64.f, v1 / 32.f); glVertex2f(x1, y1); glTexCoord2f(u0 / 64.f, v1 / 32.f); glVertex2f(x0, y1);
}
static void signDrawUI(int w, int h) {
    if (!gSignEd) return;
    auto it = gSignTxt.find(signKeyOf(gSignX, gSignY, gSignZ)); if (it == gSignTxt.end()) return;
    const float S = std::max(2.f, floorf(h / 170.f)), bx = w * .5f - 48.f * S, by = h * .5f - 46.f * S;   // tabua 24x12 px do modelo = 96x48 unidades de texto
    glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0, 0, .55f); glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd();
    const GLuint tx = signUITex(); const int m = getM(gSignX, gSignY, gSignZ) % 20; const bool wall = m >= 16;
    if (tx) {
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tx); glColor4f(1, 1, 1, 1); glBegin(GL_QUADS);
        signQuadUV(bx, by, bx + 96 * S, by + 48 * S, 2, 2, 26, 14);   // frente da tabua
        if (!wall) signQuadUV(w * .5f - 4.f * S, by + 48 * S, w * .5f + 4.f * S, by + 104 * S, 2, 16, 4, 30);   // poste (so placa de chao)
        glEnd(); glDisable(GL_TEXTURE_2D);
    } else { glColor4f(.64f, .48f, .27f, 1); glBegin(GL_QUADS); glVertex2f(bx, by); glVertex2f(bx + 96 * S, by); glVertex2f(bx + 96 * S, by + 48 * S); glVertex2f(bx, by + 48 * S); glEnd(); }   // pack sem a textura: retangulo
    glEnable(GL_TEXTURE_2D);
    const char* t1 = "Edite o texto da placa"; text(w * .5f - textWidth(t1, S) * .5f, by - 14 * S, S, t1, 1, 1, 1, 0);
    const char* t2 = "Enter: proxima linha   Esc: concluir"; text(w * .5f - textWidth(t2, S * .6f) * .5f, by + (wall ? 54 : 110) * S, S * .6f, t2, .85f, .85f, .85f, 0);
    for (int i = 0; i < 4; i++) {
        std::string s = it->second.l[i]; if (i == gSignLine && ((int)(SDL_GetTicks() / 400) & 1)) s += "_";
        text(w * .5f - textWidth(s.c_str(), S) * .5f, by + (i * 10 + 4) * S, S, s.c_str(), 0, 0, 0, 0);
    }
    glEnable(GL_DEPTH_TEST);
}
static void signTick(const float* eye, const float* dir) {   // por quadro: abre o editor para a placa recem-colocada
    if (gSignWatch[3] > 0) { gSignWatch[3]--; if (!gSignEd && getB(gSignWatch[0], gSignWatch[1], gSignWatch[2]) == XB_SIGN) { signOpen(gSignWatch[0], gSignWatch[1], gSignWatch[2]); gSignWatch[3] = 0; } }
    (void)eye; (void)dir;
}
static bool signUse(const float* eye, const float* dir, bool sneak) {   // botao direito numa placa existente
    if (sneak || gSignEd) return false; RayHit r;
    if (!rayCast(eye, dir, 5.f, r, false) || getB(r.x, r.y, r.z) != XB_SIGN) return false;
    signOpen(r.x, r.y, r.z); return true;
}
static void signWatchPlace(const float* eye, const float* dir) {   // chamado ao clicar com a placa na mao
    RayHit r; if (!rayCast(eye, dir, 5.f, r, false)) return;
    gSignWatch[0] = r.x + r.nx; gSignWatch[1] = r.y + r.ny; gSignWatch[2] = r.z + r.nz; gSignWatch[3] = 12;   // quadros para o bloco aparecer
}
static void signDrawWorld(double cx, double cy, double cz) {
    if (gSignTxt.empty()) return;
    { std::vector<int64_t> gone; for (auto& kv : gSignTxt) if (getB(kv.second.x, kv.second.y, kv.second.z) != XB_SIGN) gone.push_back(kv.first); for (int64_t k : gone) gSignTxt.erase(k); }   // placa quebrada perde o texto
    if (gSignTxt.empty()) return;
    std::vector<int64_t> dead; glDisable(GL_TEXTURE_2D); glDisable(GL_CULL_FACE); glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-2.f, -2.f); glColor4f(0, 0, 0, 1);
    for (auto& kv : gSignTxt) {
        const SignT& s = kv.second;
        if (getB(s.x, s.y, s.z) != XB_SIGN) { dead.push_back(kv.first); continue; }
        const double dx = s.x + .5 - cx, dy = s.y - cy, dz = s.z + .5 - cz; if (dx * dx + dy * dy + dz * dz > 24.0 * 24.0) continue;
        const int m = getM(s.x, s.y, s.z) % 20; const bool wall = m >= 16; const int d = wall ? m - 16 : (m & 15);
        const float ay = wall ? 180.f + 90.f * (d & 3) : 22.5f * d, mdz = wall ? -7.f : 0.f, mdy = wall ? -5.f : 0.f;
        glPushMatrix(); glTranslatef(s.x + .5f, (float)s.y, s.z + .5f); glRotatef(-ay, 0, 1, 0);
        glTranslatef(0.f, (13.333f + mdy) / 16.f, (8.667f + mdz + .12f - 8.f) / 16.f); glScalef(1.f / 96.f, -1.f / 96.f, 1.f / 96.f);
        glBegin(GL_QUADS);
        for (int i = 0; i < 4; i++) { const char* str = s.l[i].c_str(); float cxu = -textWidth(str, 1.f) * .5f; const float oy = i * 10 - 19.f;
            for (const char* p = str; *p;) { int cp = u8next(p), gf, ga; glyphMetrics(cp, gf, ga); glyphQuads(cp, cxu - gf, oy, 1.f); cxu += ga; } }
        glEnd(); glPopMatrix();
    }
    for (int64_t k : dead) gSignTxt.erase(k);
    glDisable(GL_POLYGON_OFFSET_FILL); glEnable(GL_TEXTURE_2D);
}
static void signsSave(const std::string& dir) {
    const std::string path = "saves/" + dir + "/signs.dat";
    if (gSignTxt.empty()) { remove(path.c_str()); return; }
    FILE* f = fopen(path.c_str(), "wb"); if (!f) return; fprintf(f, "SIGNS1\n");
    for (auto& kv : gSignTxt) { const SignT& s = kv.second; fprintf(f, "s %d %d %d", s.x, s.y, s.z); for (int i = 0; i < 4; i++) { fputc('\t', f); for (unsigned char c : s.l[i]) { if (c == '\\' || c == '\t' || c == '\n' || c == '\r') fprintf(f, "\\x%02x", c); else fputc(c, f); } } fputc('\n', f); }
    fclose(f);
}
static void signsLoad(const std::string& dir) {
    gSignTxt.clear(); gSignEd = false;
    FILE* f = fopen(("saves/" + dir + "/signs.dat").c_str(), "rb"); if (!f) return; char ln[1024];
    while (fgets(ln, sizeof ln, f)) {
        int x, y, z, n = 0; if (ln[0] != 's' || sscanf(ln, "s %d %d %d%n", &x, &y, &z, &n) < 3) continue;
        SignT s; s.x = x; s.y = y; s.z = z; const char* p = ln + n; int li = 0; std::string cur;
        for (; *p && *p != '\n' && *p != '\r' && li < 4; p++) {
            if (*p == '\t') { if (li > 0 || !cur.empty()) { if (li > 0) s.l[li - 1] = cur; } li++; cur.clear(); continue; }
            if (p[0] == '\\' && p[1] == 'x' && p[2] && p[3]) { cur += (char)strtol(std::string(p + 2, 2).c_str(), nullptr, 16); p += 3; continue; }
            cur += *p;
        }
        if (li >= 1 && li <= 4) s.l[li - 1] = cur;
        gSignTxt[signKeyOf(x, y, z)] = s;
    }
    fclose(f);
}
