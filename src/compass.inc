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
// chamada uma vez por quadro, com o mundo em jogo. px/pz = posicao do jogador, yawDeg = yaw do jogo (0 olha para -Z; cresce para a esquerda)
static void compassUpdate(double px, double pz, float yawDeg) {
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
