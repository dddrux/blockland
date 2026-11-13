// ================= malhas de blocos e itens (3D) =================
static const std::vector<XQuad>* rsIconQuads(int id);   // redstone.inc
static void entRotY(std::vector<XQuad>& qs, size_t from, int ay);   // entmodels.inc (incluido depois via redstone.inc)
static inline bool isModelIcon(int id) { return !isCubeIcon(id) && rsIconQuads(id) != nullptr; }   // portao/placa/botao/alcapao...: item com malha propria (nao tem sprite)
static int propHeldKind(int id);   // props.inc (patchMod.12): 2 arma de fogo, 1 arma branca/ferramenta
static void propGunXf(int id, bool fp, float sd);   // propsys.inc: transformacao de mao (display do modelo) p/ arma 3D
static bool propIsFlat(int id);   // propsys.inc (patchProp.7)
static inline bool isBlockLike(int id) { return isCubeIcon(id) || isModelIcon(id) || (isProp(id) && !propIsFlat(id)); }   // posa de bloco (mao/chao/3a pessoa)
// ---- cor padrao dos icones de vegetacao (o pack traz a textura em cinza; o jogo pinta com a cor do bioma) ----
static float gIconTint[3] = {1.f, 1.f, 1.f};
static float gGlintC[3] = {1.f, 1.f, 1.f}, gGlintA = 1.f;   // patchEnch.2: passada do brilho de encantamento (multiplica a cor do icone; 1 = normal)
static bool iconTintRGB(int id, float* c) {   // cores do inventario do Minecraft 1.12: grama (0,5/1,0), folhagem padrao 0x48B518, abeto 0x619961, bétula 0x80A755
    std::string f; if (isPk(id)) f = PK[id - PK0].file;
    { auto gi = gXbItem.find(id); if (gi != gXbItem.end()) { if (gi->second == XB_VINE) f = "block/vine"; else if (gi->second == XB_LILY) f = "block/lily_pad"; } }   // vale mesmo se o arquivo do item tiver outro nome (ex. item/vine)   // patch5.75: vinha/vitoria-regia sao itens XB (nao PK): sem isso ficavam sem cor no inventario
    auto has = [&](const char* s) { return f.find(s) != std::string::npos; };
    float r = 0, g = 0, b = 0;
    if (id == LEAVES || leafKind(id) || has("_leaves") || has("leaves_")) {
        if (has("spruce")) { r = .38f; g = .60f; b = .38f; } else if (has("birch")) { r = .50f; g = .66f; b = .33f; } else { r = .28f; g = .71f; b = .09f; }
    } else if (has("grass_block_top") || f == "block/short_grass" || f == "block/grass" || f == "block/fern" || has("tall_grass") || has("large_fern") || has("double_plant_grass") || has("double_plant_fern") || f == "block/tallgrass") { r = .48f; g = .74f; b = .35f; }
    else if (f == "block/vine") { r = .28f; g = .71f; b = .09f; }
    else if (f == "block/lily_pad") { r = .13f; g = .50f; b = .19f; }
    else return false;
    c[0] = r; c[1] = g; c[2] = b; return true;
}
static int iconSpriteTile(int id) {   // girassol: o item do pack aponta para a cabeca vista de cima; no inventario aparece a flor (frente)
    if (isPk(id) && PK[id - PK0].file.find("sunflower") != std::string::npos && PK[id - PK0].file != "block/sunflower_front") { auto it = gPkByFile.find("block/sunflower_front"); if (it != gPkByFile.end() && isPk(it->second)) return PK[it->second - PK0].tile; }
    return itemTile(id);
}
static std::unordered_map<int, std::vector<V>> meshCache;   // por id (blocos, itens e itens do pack >= PK0)
static void tileUV(int t, float& u0, float& v0, float& u1, float& v1) { u0 = (t % ATL) / (float)ATL; v0 = (t / ATL) / (float)ATLR; u1 = u0 + 1.f / ATL; v1 = v0 + 1.f / ATLR; }
static void qa(std::vector<V>& o, const float p[4][3], float u0, float v0, float u1, float v1, float s) {
    float uv[4][2] = {{u0,v1},{u1,v1},{u1,v0},{u0,v0}};
    for (int k = 0; k < 4; k++) o.push_back({p[k][0], p[k][1], p[k][2], uv[k][0], uv[k][1], s, s, s, 1.f});
}
static const std::vector<V>& meshOf(int id) {   // espaço [0,1]^3
    auto cached = meshCache.find(id); if (cached != meshCache.end()) return cached->second;
    std::vector<V>& o = meshCache[id];
    float u0, v0, u1, v1;
    if (isCubeIcon(id)) {
        static const float Cu[6][4][3] = {{{1,0,1},{1,0,0},{1,1,0},{1,1,1}},{{0,0,0},{0,0,1},{0,1,1},{0,1,0}},{{0,1,1},{1,1,1},{1,1,0},{0,1,0}},
                                          {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},{{0,0,1},{1,0,1},{1,1,1},{0,1,1}},{{1,0,0},{0,0,0},{0,1,0},{1,1,0}}};
        static const float SHc[6] = {.8f, .8f, 1.f, .5f, .65f, .65f};
        const float hh = pkHeight(id);   // 1.12: mesa de encantamento 12/16, moldura do End 13/16
        for (int f = 0; f < 6; f++) {
            tileUV(f == 5 && pkFront(id) >= 0 ? pkFront(id) : iconTile(id, f == 2 ? 0 : f == 3 ? 2 : 1), u0, v0, u1, v1);
            float pq[4][3]; for (int k = 0; k < 4; k++) { pq[k][0] = Cu[f][k][0]; pq[k][1] = Cu[f][k][1] * hh; pq[k][2] = Cu[f][k][2]; }
            if (id == gCactusId && id && (f < 2 || f > 3)) for (int k = 0; k < 4; k++) {   // cacto (modelo 1.12): as 4 laterais ficam 1/16 para dentro; topo e base cheios
                if (f == 0) pq[k][0] = 15.f / 16; else if (f == 1) pq[k][0] = 1.f / 16; else if (f == 4) pq[k][2] = 15.f / 16; else pq[k][2] = 1.f / 16; }
            if (f != 2 && f != 3) v0 += (1.f - hh) * (v1 - v0);   // laterais: corta o alto da textura (como o uv [0,4,16,16] do modelo)
            qa(o, pq, u0, v0, u1, v1, SHc[f]);
            float tc[3]; if (iconTintRGB(id, tc) && (f == 2 || !(isPk(id) && PK[id - PK0].file.find("grass_block") != std::string::npos))) for (size_t k = o.size() - 4; k < o.size(); k++) { o[k].r *= tc[0]; o[k].g *= tc[1]; o[k].b *= tc[2]; }
        }
        return o;
    }
    if (const std::vector<XQuad>* mq = rsIconQuads(id)) {   // item com modelo 3D: usa a malha do proprio bloco (como no inventario)
        const float ex = .02f / ATW, ey = .02f / (16.f * ATLR);
        std::vector<XQuad> rq = *mq;   // copia: o giro vale so para a mao/chao/3a pessoa (o icone do inventario usa a malha original)
        { auto gi = gXbItem.find(id); const int xb = gi != gXbItem.end() ? gi->second : 0;
          if (xb == XB_CHEST || xb == XB_SIGN || xb == XB_BED) entRotY(rq, 0, 90); }   // -90 graus em Y: trinco/frente (bau, placa) e cabeceira (cama) ficam certos na mao
        for (const XQuad& q : rq) {
            float e1[3], e2[3], n[3]; for (int i = 0; i < 3; i++) { e1[i] = q.p[1][i] - q.p[0][i]; e2[i] = q.p[3][i] - q.p[0][i]; }
            n[0] = e1[1] * e2[2] - e1[2] * e2[1]; n[1] = e1[2] * e2[0] - e1[0] * e2[2]; n[2] = e1[0] * e2[1] - e1[1] * e2[0];
            const float sh = fabsf(n[1]) >= std::max(fabsf(n[0]), fabsf(n[2])) ? 1.f : (fabsf(n[2]) >= fabsf(n[0]) ? .78f : .55f);
            tileUV(q.tile, u0, v0, u1, v1);
            bool big = false; for (int k = 0; k < 4; k++) if (q.u[k] > 1.001f || q.v[k] > 1.001f) big = true;   // modelos de entidade (bau, cama, placa, shulker): textura 64x64 espalhada por 4x4 tiles, u/v ate 4 (como no chunk)
            for (int k = 0; k < 4; k++) {
                if (big) { o.push_back({q.p[k][0], q.p[k][1], q.p[k][2], u0 + q.u[k] / ATL, v0 + q.v[k] / ATLR, sh, sh, sh, 1.f}); continue; }
                const float fu = std::min(std::max(q.u[k], 0.f), 1.f), fv = std::min(std::max(q.v[k], 0.f), 1.f);
                o.push_back({q.p[k][0], q.p[k][1], q.p[k][2], u0 + fu / ATL + (fu < .5f ? ex : -ex), v0 + fv / ATLR + (fv < .5f ? ey : -ey), sh, sh, sh, 1.f});
            }
        }
        return o;
    }
    int t = iconSpriteTile(id); tileUV(t, u0, v0, u1, v1);
    const float zf = 8.5f / 16, zb = 7.5f / 16;
    float F[4][3] = {{0,0,zf},{1,0,zf},{1,1,zf},{0,1,zf}}, B[4][3] = {{1,0,zb},{0,0,zb},{0,1,zb},{1,1,zb}};
    { const float iu = .1f / ATW, iv = .1f / (16.f * ATLR);   // patchItemEdge.2: inset de 0,1 texel nas faces do sprite; a borda do quad amostrava o tile vizinho do atlas (linha em volta do item no OpenGL)
      qa(o, F, u0 + iu, v0 + iv, u1 - iu, v1 - iv, 1.f); qa(o, B, u1 - iu, v0 + iv, u0 + iu, v1 - iv, 1.f); }
    auto op = [&](int x, int y) { if (x < 0 || y < 0 || x > 15 || y > 15) return false;
        return atlas[((t / ATL * 16 + y) * ATW + (t % ATL * 16 + x)) * 4 + 3] > 128; };
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        if (!op(x, y)) continue;
        float cu = u0 + (x + .5f) / 16 / ATL, cv = v0 + (y + .5f) / 16 / ATLR, X0 = x / 16.f, X1 = (x + 1) / 16.f, Y1 = 1 - y / 16.f, Y0 = 1 - (y + 1) / 16.f;
        auto side = [&](float a0,float a1,float a2,float b0,float b1,float b2,float c0,float c1,float c2,float d0,float d1,float d2,float s) {
            float p[4][3] = {{a0,a1,a2},{b0,b1,b2},{c0,c1,c2},{d0,d1,d2}};
            for (int k = 0; k < 4; k++) o.push_back({p[k][0], p[k][1], p[k][2], cu, cv, s, s, s, 1.f});
        };
        if (!op(x - 1, y)) side(X0,Y0,zb, X0,Y0,zf, X0,Y1,zf, X0,Y1,zb, .8f);
        if (!op(x + 1, y)) side(X1,Y0,zf, X1,Y0,zb, X1,Y1,zb, X1,Y1,zf, .8f);
        if (!op(x, y - 1)) side(X0,Y1,zf, X1,Y1,zf, X1,Y1,zb, X0,Y1,zb, .9f);
        if (!op(x, y + 1)) side(X0,Y0,zb, X1,Y0,zb, X1,Y0,zf, X0,Y0,zf, .6f);
    }
    { float tc[3]; if (iconTintRGB(id, tc)) for (V& v : o) { v.r *= tc[0]; v.g *= tc[1]; v.b *= tc[2]; } }
    return o;
}
static float gEntHurt = 0.f;   // 0..1: avermelha a textura da entidade (hit estilo 1.8.9); usado por jogadores
// DIAG temporario (linhas no ceu): BL_DIAG=nohand,nodrops,nochunks,nosky (separado por virgula) desliga cada etapa para isolar a origem
static bool diagOff(const char* k) { static std::string v; static bool init = false; if (!init) { init = true; const char* e = getenv("BL_DIAG"); if (e) v = std::string(",") + e + ","; } return !v.empty() && v.find(std::string(",") + k + ",") != std::string::npos; }
static void drawMesh(const std::vector<V>& m0) {
    if (m0.empty()) return;
    // gEntDim = luz do local (ciclo dia/noite, cavernas); só vale para entidades do mundo (a GUI deixa em 1,1,1 e não copia nada)
    static std::vector<V> dimmed; const std::vector<V>* ms = &m0;
    if (gEntDim[0] != 1.f || gEntDim[1] != 1.f || gEntDim[2] != 1.f || gEntHurt > 0.f) { dimmed = m0; const float hg = 1.f - .62f * gEntHurt;   // hurt: verde/azul caem (RendererLivingEntity: overlay vermelho 0.4)
        for (V& v : dimmed) { v.r *= gEntDim[0]; v.g *= gEntDim[1] * hg; v.b *= gEntDim[2] * hg; } ms = &dimmed; }
    const std::vector<V>& m = *ms;
    glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(V), &m[0].x); glTexCoordPointer(2, GL_FLOAT, sizeof(V), &m[0].u); glColorPointer(4, GL_FLOAT, sizeof(V), &m[0].r);
    glDrawArrays(GL_QUADS, 0, (GLsizei)m.size());
    glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
}
static bool gGlintOn = false;   // patchEnch.2: quem chama liga antes de desenhar um item encantado (chao, mao); drawMeshG faz a 2a passada aditiva roxa
static void drawMeshG(const std::vector<V>& m) {
    drawMesh(m); if (!gGlintOn || m.empty()) return;
    const float t = SDL_GetTicks() * .001f, k = .5f + .5f * sinf(t * 2.4f);
    const float sd[3] = {gEntDim[0], gEntDim[1], gEntDim[2]}, sh = gEntHurt; const bool bl = rdIsEnabled(GL_BLEND) != 0;
    gEntDim[0] = .42f + .18f * k; gEntDim[1] = .16f + .10f * k; gEntDim[2] = .85f; gEntHurt = 0.f;
    rdEnable(GL_BLEND); rdBlendFunc(GL_SRC_ALPHA, GL_ONE); rdDepthMask(GL_FALSE); rdDepthFunc(GL_LEQUAL);
    drawMesh(m);
    rdDepthMask(GL_TRUE); rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); if (!bl) rdDisable(GL_BLEND);
    gEntDim[0] = sd[0]; gEntDim[1] = sd[1]; gEntDim[2] = sd[2]; gEntHurt = sh;
}
// transformações de exibição (mesmos valores do Minecraft: first/third person, bloco / item / ferramenta)
static void displayXf(int id, bool fp) {
    float t[3] = {0,0,0}, r[3] = {0,0,0}, s;
    if (isProp(id) && !propIsFlat(id) && propHeldKind(id) == 2) { propGunXf(id, fp, 1.f); return; }   // arma 3D de mod: nao e bloco
    if (isBlockLike(id)) { if (fp) { r[1] = 45; s = .4f; } else { r[0] = 75; r[1] = 45; t[1] = 2.5f; s = .375f; } }
    else if (fp) { r[1] = -90; r[2] = 25; t[0] = 1.13f; t[1] = 3.2f; t[2] = 1.13f; s = .68f; }
    else if (id == IT_BOW || (isPk(id) && PK[id - PK0].file.compare(0, 13, "item/bow_pull") == 0)) { r[0] = -80; r[1] = 260; r[2] = -40; t[0] = -1; t[1] = -2; t[2] = 2.5f; s = .9f; }   // arco: display thirdperson_righthand do pack (empunhadura na mao, curva para a frente)
    else if (isTool(id) || (isProp(id) && propHeldKind(id) >= 1)) { r[1] = -90; r[2] = 55; t[1] = 4; t[2] = .5f; s = .85f; }
    else { t[1] = 3; t[2] = 1; s = .55f; }
    rdTranslatef(t[0] / 16, t[1] / 16, t[2] / 16);
    rdRotatef(r[0], 1, 0, 0); rdRotatef(r[1], 0, 1, 0); rdRotatef(r[2], 0, 0, 1);   // ItemTransformVec3.apply (1.12.2): translate, rotate X, Y, Z, scale (para o arco tambem)
    rdScalef(s, s, s); rdTranslatef(-.5f, -.5f, -.5f);
}
static int bowPullItemT(int id, int t) {   // bow.json (1.12.2): pulling=1 -> bow_pulling_0; pull>=0,65 (13 ticks) -> _1; pull>=0,9 (18 ticks) -> _2. Vale em 1a e 3a pessoa
    if (gRodId && id == gRodId && gBobOn && gRodCastId) return gRodCastId;   // BUG-019: vara com boia lancada
    if (id != IT_BOW || t < 0) return id;
    const int k = t >= 18 ? 2 : t >= 13 ? 1 : 0;
    static int pid[3] = {-1, -1, -1}; static bool scanned = false;
    if (!scanned) { scanned = true; for (int q = 0; q < 3; q++) { const std::string f = std::string("item/bow_pulling_") + (char)('0' + q); for (int i = 0; i < (int)PK.size(); i++) if (PK[i].file == f) { pid[q] = PK0 + i; break; } } }
    return pid[k] >= 0 ? pid[k] : id;
}
static void propDrawUnit(int id);   // propsys.inc
static void drawHeldRaw(int id) {   // desenha o item no sistema de coordenadas atual (sem displayXf): quem chama posiciona
    if (!id) return;
    glBindTexture(GL_TEXTURE_2D, atlasTex);
    rdEnable(GL_BLEND);
    rdBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, .3f);
    if (isProp(id)) propDrawUnit(id); else   // patchProp.2: prop na mao = modelo reduzido
    drawMeshG(meshOf(id));
    rdDisable(GL_ALPHA_TEST);
    rdDisable(GL_BLEND);
}
static void drawHeld(int id, bool fp) {
    if (!id) return;
    rdPushMatrix(); displayXf(id, fp); drawHeldRaw(id); rdPopMatrix();
}
