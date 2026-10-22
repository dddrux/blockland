// ---------- bussola (igual ao Minecraft 1.12): a agulha aponta para o spawn do mundo e balanca com inercia ----------
// A textura do item (item/compass_00) e o tile do atlas; a cada quadro troca-se o conteudo desse tile por um dos 32 quadros
// (compass_00..31 do pack). Assim inventario, hotbar, mao e item no chao mostram a mesma agulha, sem tocar no render dos itens.
// Quadro 16 = alvo a frente; quadros crescem no sentido horario (24 = alvo a direita), como no vanilla.
// Multiplayer: o spawn e constante (mesmo para host e clientes, que usam a mesma seed); cada jogador usa a propria posicao e yaw.
// Nao existem outras dimensoes no jogo (no vanilla fora da superficie a agulha gira aleatoria).
static const double COMPASS_SPAWN_X = 8.5, COMPASS_SPAWN_Z = 8.5;   // = ponto inicial do mundo novo (a cama NAO muda o alvo, como no vanilla)
static std::vector<std::array<uint8_t, 1024>> gCpFrames;
static int gCpTile = -1; static size_t gCpPkN = 0;
static double gCpAngle = 0, gCpVel = 0; static uint32_t gCpLast = 0;

static void compassUploadTile(int tile) {   // envia o tile (nivel 0 e mips, com a mesma regra de tiles de (16>>l) texels do atlas)
    GLint prev = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev); glBindTexture(GL_TEXTURE_2D, atlasTex);
    const int tx = tile % ATL, ty = tile / ATL; const uint8_t* base = atlas;
    for (int l = 0; l <= ATLAS_MIPS; l++) {
        const int s = 16 >> l, k = 16 / s; if (s < 1) break;
        std::vector<uint8_t> o((size_t)s * s * 4);
        for (int y = 0; y < s; y++) for (int x = 0; x < s; x++) {
            float r = 0, g = 0, b = 0, a = 0;
            for (int j = 0; j < k; j++) for (int i = 0; i < k; i++) {
                const uint8_t* p = base + (((size_t)(ty * 16 + y * k + j)) * ATW + (tx * 16 + x * k + i)) * 4;
                r += p[0] * p[3]; g += p[1] * p[3]; b += p[2] * p[3]; a += p[3];
            }
            uint8_t* q = &o[((size_t)y * s + x) * 4];
            if (a > 0) { q[0] = (uint8_t)(r / a + .5f); q[1] = (uint8_t)(g / a + .5f); q[2] = (uint8_t)(b / a + .5f); }
            else q[0] = q[1] = q[2] = 0;
            q[3] = (uint8_t)(a / (k * k) + .5f);
        }
        glTexSubImage2D(GL_TEXTURE_2D, l, tx * s, ty * s, s, s, GL_RGBA, GL_UNSIGNED_BYTE, o.data());
    }
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev);
}
// patchGfx.2: Graficos Rapidos = folhas OPACAS (texels transparentes viram verde escuro, como o MC "fast"); Bonitos = textura original.
// Troca o conteudo dos tiles de folha no atlas (e nos mips) e reenvia; guarda o original para voltar.
struct LeafBak { std::array<uint8_t, 1024> orig, cur; };
static std::map<int, LeafBak> gLeafBak; static int gLeafFastState = -1;
static void leafGfxApply(bool fast) {
    if (!atlasTex) return;
#ifdef BL_RPI
    fast = true;   // patchRpiOpt.1: Pi = folhas sempre opacas
#endif
    std::set<int> tiles; for (int f = 0; f < 3; f++) tiles.insert(blkTile(LEAVES, f));
    for (size_t i = 0; i < PK.size(); i++) if (leafKind(PK0 + (int)i)) for (int f = 0; f < 3; f++) tiles.insert(blkTile(PK0 + (int)i, f));
    if ((int)fast == gLeafFastState) {   // patchRpiOpt.1: a cada ~1 s confere se o atlas foi recarregado (pack/mods) e as folhas voltaram a ter alpha
        static int chk = 0; if (!fast || (++chk % 60) != 0) return; bool bad = false;
        for (int tile : tiles) { if (tile < 0 || tile >= ATL * ATLR) continue; const int tx = tile % ATL, ty = tile / ATL; for (int y = 0; y < 16 && !bad; y++) for (int x = 0; x < 16; x++) if (atlas[(((size_t)(ty * 16 + y)) * ATW + tx * 16 + x) * 4 + 3] != 255) { bad = true; break; } if (bad) break; }
        if (!bad) return;
    }
    for (int tile : tiles) {
        if (tile < 0 || tile >= ATL * ATLR) continue;
        const int tx = tile % ATL, ty = tile / ATL; std::array<uint8_t, 1024> now;
        for (int y = 0; y < 16; y++) memcpy(&now[y * 64], &atlas[(((size_t)(ty * 16 + y)) * ATW + tx * 16) * 4], 64);
        auto it = gLeafBak.find(tile);
        if (it == gLeafBak.end() || now != it->second.cur) { LeafBak b; b.orig = now; b.cur = now; it = gLeafBak.insert_or_assign(tile, b).first; }   // tile novo ou recarregado pelo pack: guarda o original
        std::array<uint8_t, 1024> out = it->second.orig;
        if (fast) {
            float r = 0, g = 0, b = 0; int n = 0; for (int i = 0; i < 256; i++) if (out[i * 4 + 3] > 128) { r += out[i * 4]; g += out[i * 4 + 1]; b += out[i * 4 + 2]; n++; }
            const uint8_t dr = n ? (uint8_t)(r / n * .45f) : 0, dg = n ? (uint8_t)(g / n * .45f) : 0, db = n ? (uint8_t)(b / n * .45f) : 0;
            for (int i = 0; i < 256; i++) { uint8_t* q = &out[i * 4]; if (q[3] <= 128) { q[0] = dr; q[1] = dg; q[2] = db; } q[3] = 255; }
        }
        for (int y = 0; y < 16; y++) memcpy(&atlas[(((size_t)(ty * 16 + y)) * ATW + tx * 16) * 4], &out[y * 64], 64);
        it->second.cur = out; compassUploadTile(tile);
    }
    gLeafFastState = (int)fast;
}
// chamada uma vez por quadro, com o mundo em jogo. px/pz = posicao do jogador, yawDeg = yaw do jogo (0 olha para -Z; cresce para a esquerda)
static void compassUpdate(double px, double pz, float yawDeg) {
    leafGfxApply(gLeafFast);   // patchGfx.2: garante o atlas das folhas certo (idempotente)
    static size_t scanN = (size_t)-1; static int tileFound = -1;   // gPkByFile so tem "block/..."; itens do pack se acham varrendo PK
    if (scanN != PK.size()) { scanN = PK.size(); tileFound = -1; for (const PkItem& e : PK) if (e.file == "item/compass_00") { tileFound = e.tile; break; } }
    if (tileFound < 0) return;
    const int tile = tileFound;
    if (tile != gCpTile || PK.size() != gCpPkN || gCpFrames.empty()) {   // (re)carrega os 32 quadros do pack ativo
        gCpTile = tile; gCpPkN = PK.size(); gCpFrames.clear();
        for (int i = 0; i < 32; i++) { std::array<uint8_t, 1024> f; char nm[64]; snprintf(nm, sizeof nm, "item/compass_%02d.png", i);
            if (!packTileRel(nm, f.data())) { gCpFrames.clear(); break; } gCpFrames.push_back(f); }
    }
    if (gCpFrames.size() != 32) return;
    const double yr = yawDeg * 3.14159265358979 / 180.0, fx = -sin(yr), fz = -cos(yr), rx = cos(yr), rz = -sin(yr);   // frente e direita (planta)
    const double dx = COMPASS_SPAWN_X - px, dz = COMPASS_SPAWN_Z - pz;
    const double target = atan2(dx * rx + dz * rz, dx * fx + dz * fz);   // angulo horario do alvo em relacao a frente
    const uint32_t now = SDL_GetTicks(); if (!gCpLast || now - gCpLast > 2000) { gCpLast = now; gCpAngle = target; gCpVel = 0; }
    int steps = 0; while (now - gCpLast >= 50 && steps < 10) {   // 20 ticks/s, mesma inercia do vanilla (aceleracao .1, atrito .8)
        gCpLast += 50; steps++;
        double d = target - gCpAngle; while (d < -3.14159265358979) d += 6.28318530717959; while (d >= 3.14159265358979) d -= 6.28318530717959;
        gCpVel += d * .1; gCpVel *= .8; gCpAngle += gCpVel;
    }
    if (steps == 10) gCpLast = now;
    int fr = (int)floor((gCpAngle / 6.28318530717959 + .5) * 32.0) % 32; if (fr < 0) fr += 32;
    uint8_t* dst = atlas + ((size_t)(tile / ATL) * 16 * ATW + (size_t)(tile % ATL) * 16) * 4; const uint8_t* src = gCpFrames[fr].data();
    bool same = true; for (int y = 0; y < 16 && same; y++) same = memcmp(dst + (size_t)y * ATW * 4, src + y * 64, 64) == 0;
    if (same) return;
    for (int y = 0; y < 16; y++) memcpy(dst + (size_t)y * ATW * 4, src + y * 64, 64);
    compassUploadTile(tile);
}

// ---------- relogio (patchClock.1): 64 quadros item/clock_00..63 do pack; o quadro vem da hora do mundo como no 1.12 (ItemClock):
// angulo celeste = f + (1 - (cos(f*pi)+1)/2 - f)/3, f = tempo/24000 - 0,25; quadro 0 = meio-dia (sol no alto). Fora da superficie gira sem sentido (como no vanilla).
static std::vector<std::array<uint8_t, 1024>> gClkFrames; static int gClkTile = -1; static size_t gClkPkN = 0;
static int clockFrame(long long worldTime, int dim, uint32_t ticksMs) {
    if (dim != 0) return (int)((ticksMs / 100u) % 64u);
    double f = (double)(((worldTime % 24000) + 24000) % 24000) / 24000.0 - .25; if (f < 0) f += 1.0;
    const double ang = f + ((1.0 - (cos(f * 3.14159265358979) + 1.0) / 2.0) - f) / 3.0;
    int fr = (int)floor(ang * 64.0 + 1e-6) % 64; return fr < 0 ? fr + 64 : fr;   // 1e-6: evita 31,99999 por arredondamento em meia-noite
}
static void clockUpdate(long long worldTime, int dim) {
    static size_t scanN = (size_t)-1; static int tileFound = -1;
    if (scanN != PK.size()) { scanN = PK.size(); tileFound = -1; for (const PkItem& e : PK) if (e.file == "item/clock_00") { tileFound = e.tile; break; } }
    if (tileFound < 0) return;
    const int tile = tileFound;
    if (tile != gClkTile || PK.size() != gClkPkN || gClkFrames.empty()) {
        gClkTile = tile; gClkPkN = PK.size(); gClkFrames.clear();
        for (int i = 0; i < 64; i++) { std::array<uint8_t, 1024> f; char nm[64]; snprintf(nm, sizeof nm, "item/clock_%02d.png", i);
            if (!packTileRel(nm, f.data())) { gClkFrames.clear(); break; } gClkFrames.push_back(f); }
    }
    if (gClkFrames.size() != 64) return;
    uint8_t* dst = atlas + ((size_t)(tile / ATL) * 16 * ATW + (size_t)(tile % ATL) * 16) * 4; const uint8_t* src = gClkFrames[clockFrame(worldTime, dim, SDL_GetTicks())].data();
    bool same = true; for (int y = 0; y < 16 && same; y++) same = memcmp(dst + (size_t)y * ATW * 4, src + y * 64, 64) == 0;
    if (same) return;
    for (int y = 0; y < 16; y++) memcpy(dst + (size_t)y * ATW * 4, src + y * 64, 64);
    compassUploadTile(tile);
}

// tocha colocada no mundo / item no chao: o tile da tocha troca de quadro (textura animada block/torch.png do pack), igual a bussola:
// a malha dos chunks nao muda, so o conteudo do tile no atlas (nivel 0 + mips). Cobre as 5 variantes (em pe + 4 de parede).
static void torchWorldUpdate() {
    if (gTorchAnimN <= 0 || (int)gTorchPx.size() < gTorchAnimN) return;
    const int f = TORCH_SEQ[(SDL_GetTicks() / 150u) % 18u] % gTorchAnimN; const uint8_t* src = gTorchPx[f].data();
    int done[5], nd = 0;
    for (int k = 0; k < 5; k++) for (const XQuad& q : gXM[XB_TORCH + k - XB0]) {
        const int tile = q.tile; if (tile < 0) continue; bool dup = false; for (int i = 0; i < nd; i++) dup = dup || done[i] == tile; if (dup || nd >= 5) continue; done[nd++] = tile;
        uint8_t* dst = atlas + ((size_t)(tile / ATL) * 16 * ATW + (size_t)(tile % ATL) * 16) * 4;
        bool same = true; for (int y = 0; y < 16 && same; y++) same = memcmp(dst + (size_t)y * ATW * 4, src + y * 64, 64) == 0;
        if (same) continue;
        for (int y = 0; y < 16; y++) memcpy(dst + (size_t)y * ATW * 4, src + y * 64, 64);
        compassUploadTile(tile);
    }
}
