// ---------- decoração do terreno (v32): cavernas 3D, veios de minério, árvores sem cortes, cactos ----------
// Tudo é função só da seed + coordenadas (determinístico) => host e convidados geram o mesmo mundo.
static std::unordered_map<int64_t, std::vector<std::array<int, 4>>> gSpill;   // blocos de árvore que cruzam a borda para chunks ainda não gerados
static int gTcx = 0, gTcz = 0;                                                 // chunk em geração (referência de treePut)
static int pkFind(const char* f) {
    auto it = gPkByFile.find(f); if (it != gPkByFile.end()) return it->second;
    if (strncmp(f, "item/", 5) == 0) {   /* patchFix.1: gPkByFile so indexa "block/..."; itens se acham varrendo PK (cache refeito quando PK muda) */
        static std::unordered_map<std::string, int> ic; static size_t cn = (size_t)-1;
        if (cn != PK.size()) { cn = PK.size(); ic.clear(); for (size_t i = 0; i < PK.size(); i++) if (PK[i].file.compare(0, 5, "item/") == 0) ic[PK[i].file] = PK0 + (int)i; }
        auto j = ic.find(f); if (j != ic.end()) return j->second;
    }
    return 0;
}

// coloca um bloco de árvore; se cair fora do chunk, escreve no vizinho (se já existe) ou guarda para quando ele for gerado
static void treePut(Chunk* c, int x, int y, int z, BT id, bool leaf, int meta = 0) {   // meta: estado da vinha (bits 0..4); so grava onde o bloco foi colocado
    if (y < 0 || y >= CH) return;
    if (x >= 0 && x < CW && z >= 0 && z < CW) { BT& b = c->b[idx(x, y, z)]; if (b == AIR || (!leaf && isLeafB(b))) { b = id; if (meta) c->m[idx(x, y, z)] = (uint8_t)meta; } return; }
    const int ox = fdiv(x, CW), oz = fdiv(z, CW), lx = x - ox * CW, lz = z - oz * CW, tx = gTcx + ox, tz = gTcz + oz;
    if (Chunk* n = getC(tx, tz)) {
        BT& b = n->b[idx(lx, y, lz)];
        if (b == AIR || (!leaf && isLeafB(b))) {
            b = id; if (meta) n->m[idx(lx, y, lz)] = (uint8_t)meta; if (n->maxY >= 0 && y > n->maxY) n->maxY = y; n->dirty = true; gKick = true;
            for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) if (dx || dz) dirtyC(tx + dx, tz + dz);
        }
    } else gSpill[key(tx, tz)].push_back({lx, y, lz, (int)id | (leaf ? 0x10000 : 0) | (meta << 20)});
}
static void applySpill(Chunk* c, int cx, int cz) {
    auto it = gSpill.find(key(cx, cz)); if (it == gSpill.end()) return;
    for (auto& s : it->second) { BT& b = c->b[idx(s[0], s[1], s[2])]; const bool leaf = (s[3] & 0x10000) != 0; if (b == AIR || (!leaf && isLeafB(b))) { b = (BT)(s[3] & 0xFFFF); if (s[3] >> 20) c->m[idx(s[0], s[1], s[2])] = (uint8_t)(s[3] >> 20); } }
    gSpill.erase(it);
}

// ---------- cavernas e ravinas: porte exato de MapGenCaves / MapGenRavine / MapGenBase do 1.12.2 ----------
struct JR2 : JRng {   // java.util.Random completo (nextLong/nextFloat)
    explicit JR2(int64_t v = 0) : JRng(v) {}
    int64_t nextLong() { const int64_t a = (int64_t)next(32); return (int64_t)((uint64_t)a << 32) + (int64_t)next(32); }
    float nextFloat() { return (float)next(24) / (float)(1 << 24); }
};
static float gSinT[65536]; static bool gSinOk = false;   // MathHelper.SIN_TABLE
static inline float mhSin(float f) { if (!gSinOk) { gSinOk = true; for (int i = 0; i < 65536; i++) gSinT[i] = (float)sin((double)i * M_PI * 2.0 / 65536.0); } return gSinT[(int)(f * 10430.378f) & 65535]; }
static inline float mhCos(float f) { mhSin(0.f); return gSinT[(int)(f * 10430.378f + 16384.0f) & 65535]; }
struct CarveCtx { Chunk* c; int cx, cz; const uint8_t* bio; };
static inline BT& cvB(CarveCtx& k, int x, int y, int z) { return k.c->b[idx(x, y, z)]; }
static void cvDig(CarveCtx& k, int x, int y, int z, bool foundTop) {   // MapGenCaves.digBlock
    const int bio = k.bio[z * 16 + x]; const bool sandy = bio == B_DESERT || bio == B_BEACH;
    const BT top = sandy ? (BT)SAND : bio == B_TUNDRA ? (BT)SNOWBLK : (BT)GRASS, fill = sandy ? (BT)SAND : (BT)DIRT;
    BT& s = cvB(k, x, y, z); const BT up = cvB(k, x, y + 1, z);
    const bool rep = s == STONE || s == DIRT || s == GRASS || s == SANDSTONE || s == SNOWBLK || ((s == SAND || s == GRAVEL) && !isWater(up));
    if (!(rep || s == top || s == fill)) return;
    if (y - 1 < 10) { s = (BT)LAVA; return; }
    s = AIR;
    if (up == SAND) cvB(k, x, y + 1, z) = (BT)SANDSTONE;
    if (foundTop && cvB(k, x, y - 1, z) == fill) cvB(k, x, y - 1, z) = top;
}
static bool cvIsTop(CarveCtx& k, int x, int y, int z) {
    const int bio = k.bio[z * 16 + x]; const BT b = cvB(k, x, y, z);
    if (bio == B_DESERT || bio == B_BEACH) return b == GRASS;
    return b == (bio == B_TUNDRA ? SNOWBLK : GRASS) || b == SNOWBLK;
}
static void cvTunnel(CarveCtx& k, bool ravine, int64_t seed, double x, double y, double z, float width, float yaw, float pitch, int j2, int k2, double scale) {
    const double d0 = k.cx * 16 + 8, d1 = k.cz * 16 + 8; float f = 0, f1 = 0; JR2 r(seed);
    if (k2 <= 0) { const int i = 8 * 16 - 16; k2 = i - r.nextInt(i / 4); }
    bool flag2 = false; if (j2 == -1) { j2 = k2 / 2; flag2 = true; }
    float rs[256]; int jj = 0; bool fl = false;
    if (ravine) { float f2 = 1.0f; for (int i = 0; i < 256; i++) { if (i == 0 || r.nextInt(3) == 0) f2 = 1.0f + r.nextFloat() * r.nextFloat(); rs[i] = f2 * f2; } }
    else { jj = r.nextInt(k2 / 2) + k2 / 4; fl = r.nextInt(6) == 0; }
    for (; j2 < k2; ++j2) {
        double d2 = 1.5 + (double)(mhSin((float)j2 * (float)M_PI / (float)k2) * width), d3 = d2 * scale;
        if (ravine) { d2 *= (double)r.nextFloat() * .25 + .75; d3 *= (double)r.nextFloat() * .25 + .75; }
        const float f3 = mhCos(pitch), f4 = mhSin(pitch);
        x += (double)(mhCos(yaw) * f3); y += (double)f4; z += (double)(mhSin(yaw) * f3);
        if (ravine) { pitch *= 0.7f; pitch += f1 * 0.05f; yaw += f * 0.05f; f1 *= 0.8f; f *= 0.5f; }
        else { pitch *= fl ? 0.92f : 0.7f; pitch += f1 * 0.1f; yaw += f * 0.1f; f1 *= 0.9f; f *= 0.75f; }
        f1 += (r.nextFloat() - r.nextFloat()) * r.nextFloat() * 2.0f; f += (r.nextFloat() - r.nextFloat()) * r.nextFloat() * 4.0f;
        if (!ravine && !flag2 && j2 == jj && width > 1.0f && k2 > 0) {
            cvTunnel(k, false, r.nextLong(), x, y, z, r.nextFloat() * .5f + .5f, yaw - (float)M_PI / 2.f, pitch / 3.0f, j2, k2, 1.0);
            cvTunnel(k, false, r.nextLong(), x, y, z, r.nextFloat() * .5f + .5f, yaw + (float)M_PI / 2.f, pitch / 3.0f, j2, k2, 1.0);
            return;
        }
        if (!(flag2 || r.nextInt(4) != 0)) continue;
        const double d4 = x - d0, d5 = z - d1, d6 = (double)(k2 - j2), d7 = (double)(width + 2.0f + 16.0f);
        if (d4 * d4 + d5 * d5 - d6 * d6 > d7 * d7) return;
        if (!(x >= d0 - 16.0 - d2 * 2.0 && z >= d1 - 16.0 - d2 * 2.0 && x <= d0 + 16.0 + d2 * 2.0 && z <= d1 + 16.0 + d2 * 2.0)) continue;
        int xa = (int)floor(x - d2) - k.cx * 16 - 1, xb = (int)floor(x + d2) - k.cx * 16 + 1, ya = (int)floor(y - d3) - 1, yb = (int)floor(y + d3) + 1;
        int za = (int)floor(z - d2) - k.cz * 16 - 1, zb = (int)floor(z + d2) - k.cz * 16 + 1;
        if (xa < 0) xa = 0; if (xb > 16) xb = 16; if (ya < 1) ya = 1; if (yb > 248) yb = 248; if (za < 0) za = 0; if (zb > 16) zb = 16;
        bool ocean = false;
        for (int i = xa; !ocean && i < xb; ++i) for (int j = za; !ocean && j < zb; ++j) for (int m = yb + 1; !ocean && m >= ya - 1; --m) {
            if (m >= 0 && m < 256) { if (isWater(cvB(k, i, m, j))) ocean = true; if (m != ya - 1 && i != xa && i != xb - 1 && j != za && j != zb - 1) m = ya; }
        }
        if (!ocean) {
            for (int i = xa; i < xb; ++i) { const double e1 = ((double)(i + k.cx * 16) + .5 - x) / d2;
                for (int j = za; j < zb; ++j) { const double e2 = ((double)(j + k.cz * 16) + .5 - z) / d2; bool top = false;
                    if (e1 * e1 + e2 * e2 >= 1.0) continue;
                    for (int m = yb; m > ya; --m) { const double e3 = ((double)(m - 1) + .5 - y) / d3;
                        const bool in = ravine ? ((e1 * e1 + e2 * e2) * (double)rs[m - 1] + e3 * e3 / 6.0 < 1.0) : (e3 > -0.7 && e1 * e1 + e3 * e3 + e2 * e2 < 1.0);
                        if (!in) continue;
                        if (cvIsTop(k, i, m, j)) top = true;
                        cvDig(k, i, m, j, top);
                    } } }
            if (flag2) break;
        }
    }
}
static void carveCaves(Chunk* c, int cx, int cz, const uint8_t* bio) {   // MapGenBase.generate: vizinhança 17x17 de chunks, semente por chunk
    CarveCtx k{c, cx, cz, bio}; JR2 r; const int64_t ws = (int64_t)WORLD_SEED;
    for (int ravine = 0; ravine < 2; ravine++)   // cavernas primeiro, depois ravinas (ordem do ChunkGeneratorOverworld)
    for (int i = cx - 8; i <= cx + 8; i++) for (int j = cz - 8; j <= cz + 8; j++) {
        r.setSeed(ws); const int64_t a = r.nextLong(), b = r.nextLong();
        r.setSeed((int64_t)((uint64_t)(int64_t)i * (uint64_t)a ^ (uint64_t)(int64_t)j * (uint64_t)b ^ (uint64_t)ws));
        if (ravine) {
            if (r.nextInt(50) != 0) continue;
            const double x = (double)(i * 16 + r.nextInt(16)), y = (double)(r.nextInt(r.nextInt(40) + 8) + 20), z = (double)(j * 16 + r.nextInt(16));
            const float f = r.nextFloat() * (float)M_PI * 2.f, f1 = (r.nextFloat() - .5f) * 2.f / 8.f, f2 = (r.nextFloat() * 2.f + r.nextFloat()) * 2.f;
            cvTunnel(k, true, r.nextLong(), x, y, z, f2, f, f1, 0, 0, 3.0);
            continue;
        }
        int n = r.nextInt(r.nextInt(r.nextInt(15) + 1) + 1); if (r.nextInt(7) != 0) n = 0;
        for (int t = 0; t < n; t++) {
            const double x = (double)(i * 16 + r.nextInt(16)), y = (double)r.nextInt(r.nextInt(120) + 8), z = (double)(j * 16 + r.nextInt(16));
            int m = 1;
            if (r.nextInt(4) == 0) { const int64_t sd = r.nextLong(); const float rw = 1.0f + r.nextFloat() * 6.0f;   // addRoomTunnel
                cvTunnel(k, false, sd, x, y, z, rw, 0.f, 0.f, -1, -1, 0.5); m += r.nextInt(4); }
            for (int l = 0; l < m; l++) {
                const float f = r.nextFloat() * (float)M_PI * 2.f, f1 = (r.nextFloat() - .5f) * 2.f / 8.f; float f2 = r.nextFloat() * 2.f + r.nextFloat();
                if (r.nextInt(10) == 0) f2 *= r.nextFloat() * r.nextFloat() * 3.0f + 1.0f;
                cvTunnel(k, false, r.nextLong(), x, y, z, f2, f, f1, 0, 0, 1.0);
            }
        }
    }
}

// ---------- população: minérios (WorldGenMinable), discos de areia/argila/cascalho (WorldGenSand/Clay), esmeraldas ----------
// Como no MC, o chunk P é populado com a região [P*16+8, P*16+23]; o chunk C recebe as populações de P em {C-1,C}^2, recortadas em C.
// Semente de população idêntica à do MC. (Lagos e masmorras não são portados; o MC gasta sorteios neles, então as POSIÇÕES não batem 1:1.)
static std::unordered_map<int64_t, std::array<int16_t, 256>> gFloor;   // altura do chão (sólido/água) por coluna, de chunks vizinhos
static const std::array<int16_t, 256>& floorOf(int cx, int cz) {
    auto it = gFloor.find(key(cx, cz)); if (it != gFloor.end()) return it->second;
    static BT sb[CW * CH * CW]; uint8_t bo[256]; int tp[256]; terrain112(cx, cz, sb, bo, tp);
    std::array<int16_t, 256> a; for (int i = 0; i < 256; i++) a[i] = (int16_t)tp[i];
    return gFloor.emplace(key(cx, cz), a).first->second;
}
static void veinGen(Chunk* c, int cx, int cz, JR2& r, BT ore, int n, int px, int py, int pz, const BT* host, int nh) {
    const float f = r.nextFloat() * (float)M_PI;
    const double d0 = (double)((float)(px + 8) + mhSin(f) * (float)n / 8.0f), d1 = (double)((float)(px + 8) - mhSin(f) * (float)n / 8.0f);
    const double d2 = (double)((float)(pz + 8) + mhCos(f) * (float)n / 8.0f), d3 = (double)((float)(pz + 8) - mhCos(f) * (float)n / 8.0f);
    const double d4 = (double)(py + r.nextInt(3) - 2), d5 = (double)(py + r.nextInt(3) - 2);
    for (int i = 0; i < n; i++) {
        const float f1 = (float)i / (float)n;
        const double d6 = d0 + (d1 - d0) * (double)f1, d7 = d4 + (d5 - d4) * (double)f1, d8 = d2 + (d3 - d2) * (double)f1;
        const double d9 = r.nextDouble() * (double)n / 16.0, d10 = (double)(mhSin((float)M_PI * f1) + 1.0f) * d9 + 1.0, d11 = d10;
        const int x0 = (int)floor(d6 - d10 / 2.0), y0 = (int)floor(d7 - d11 / 2.0), z0 = (int)floor(d8 - d10 / 2.0);
        const int x1 = (int)floor(d6 + d10 / 2.0), y1 = (int)floor(d7 + d11 / 2.0), z1 = (int)floor(d8 + d10 / 2.0);
        for (int xx = std::max(x0, cx * 16); xx <= std::min(x1, cx * 16 + 15); xx++) {
            const double e1 = ((double)xx + .5 - d6) / (d10 / 2.0); if (e1 * e1 >= 1.0) continue;
            for (int yy = std::max(y0, 0); yy <= std::min(y1, 255); yy++) {
                const double e2 = ((double)yy + .5 - d7) / (d11 / 2.0); if (e1 * e1 + e2 * e2 >= 1.0) continue;
                for (int zz = std::max(z0, cz * 16); zz <= std::min(z1, cz * 16 + 15); zz++) {
                    const double e3 = ((double)zz + .5 - d8) / (d10 / 2.0); if (e1 * e1 + e2 * e2 + e3 * e3 >= 1.0) continue;
                    BT& b = c->b[idx(xx - cx * 16, yy, zz - cz * 16)]; for (int h = 0; h < nh; h++) if (b == host[h]) { b = ore; break; }
                }
            }
        }
    }
}
static void diskGen(Chunk* c, int cx, int cz, JR2& r, int wx, int wz, BT blk, int radius, bool clay) {   // WorldGenSand / WorldGenClay no ponto (wx, topo+1)
    const int fx = fdiv(wx, CW), fz = fdiv(wz, CW); const int fl = floorOf(fx, fz)[(wz - fz * 16) * 16 + (wx - fx * 16)];
    const int py = fl + 1; if (py >= SEA) return;   // só se o ponto for água
    const int rr = r.nextInt(radius - 2) + 2, yd = clay ? 1 : 2;
    for (int x = std::max(wx - rr, cx * 16); x <= std::min(wx + rr, cx * 16 + 15); x++) for (int z = std::max(wz - rr, cz * 16); z <= std::min(wz + rr, cz * 16 + 15); z++) {
        if ((x - wx) * (x - wx) + (z - wz) * (z - wz) > rr * rr) continue;
        for (int y = py - yd; y <= py + yd; y++) { if (y < 0 || y > 255) continue; BT& b = c->b[idx(x - cx * 16, y, z - cz * 16)]; if (b == DIRT || (!clay && b == GRASS) || (clay && b == CLAY)) b = blk; }
    }
}
// ---------- lagos de água/lava: WorldGenLakes do 1.12.2 (forma idêntica; testes de obstrução só dentro do chunk) ----------
static void lakeGen(Chunk* c, int cx, int cz, JR2& r, int px, int py, int pz, bool lava, int bio) {
    int x0 = px - 8, z0 = pz - 8, y = py;
    { const int fx = fdiv(x0, CW), fz = fdiv(z0, CW); const int top = floorOf(fx, fz)[(z0 - fz * 16) * 16 + (x0 - fx * 16)]; if (y > top) y = top; }   // desce pelo ar até o chão
    if (y <= 4) return; y -= 4;
    static bool f[2048]; memset(f, 0, sizeof f);
    const int n = r.nextInt(4) + 4;
    for (int j = 0; j < n; j++) {
        const double d0 = r.nextDouble() * 6.0 + 3.0, d1 = r.nextDouble() * 4.0 + 2.0, d2 = r.nextDouble() * 6.0 + 3.0;
        const double d3 = r.nextDouble() * (16.0 - d0 - 2.0) + 1.0 + d0 / 2.0, d4 = r.nextDouble() * (8.0 - d1 - 4.0) + 2.0 + d1 / 2.0, d5 = r.nextDouble() * (16.0 - d2 - 2.0) + 1.0 + d2 / 2.0;
        for (int l = 1; l < 15; l++) for (int i = 1; i < 15; i++) for (int k = 1; k < 7; k++) {
            const double a = (l - d3) / (d0 / 2.0), b = (k - d4) / (d1 / 2.0), e = (i - d5) / (d2 / 2.0);
            if (a * a + b * b + e * e < 1.0) f[(l * 16 + i) * 8 + k] = true;
        }
    }
    auto at = [&](int l, int i, int k) -> BT* { const int wx = x0 + l - cx * 16, wz = z0 + i - cz * 16, wy = y + k; return (wx < 0 || wx > 15 || wz < 0 || wz > 15 || wy < 0 || wy > 255) ? nullptr : &c->b[idx(wx, wy, wz)]; };
    auto liq = [](BT b) { return isWater(b) || b == (BT)LAVA; };
    for (int l = 0; l < 16; l++) for (int i = 0; i < 16; i++) for (int k = 0; k < 8; k++) {
        const bool fl = !(f[(l * 16 + i) * 8 + k]) && ((l < 15 && f[((l + 1) * 16 + i) * 8 + k]) || (l > 0 && f[((l - 1) * 16 + i) * 8 + k]) || (i < 15 && f[(l * 16 + i + 1) * 8 + k]) || (i > 0 && f[(l * 16 + i - 1) * 8 + k]) || (k < 7 && f[(l * 16 + i) * 8 + k + 1]) || (k > 0 && f[(l * 16 + i) * 8 + k - 1]));
        if (!fl) continue; BT* b = at(l, i, k); if (!b) continue;
        if (k >= 4 && liq(*b)) return;
        if (k < 4 && *b == AIR) return;   // borda do fundo precisa ser sólida (ou líquido)
    }
    const BT liquid = lava ? (BT)LAVA : (BT)WATER;
    for (int l = 0; l < 16; l++) for (int i = 0; i < 16; i++) for (int k = 0; k < 8; k++) if (f[(l * 16 + i) * 8 + k]) if (BT* b = at(l, i, k)) *b = k >= 4 ? (BT)AIR : liquid;
    for (int l = 0; l < 16; l++) for (int i = 0; i < 16; i++) for (int k = 4; k < 8; k++) if (f[(l * 16 + i) * 8 + k]) {   // terra exposta vira grama
        BT* b = at(l, i, k - 1); BT* up = at(l, i, k); if (b && up && *b == DIRT && *up == AIR && !lava) *b = (BT)GRASS; }
    if (lava) for (int l = 0; l < 16; l++) for (int i = 0; i < 16; i++) for (int k = 0; k < 8; k++) {   // borda de pedra ao redor da lava
        const bool fl = !(f[(l * 16 + i) * 8 + k]) && ((l < 15 && f[((l + 1) * 16 + i) * 8 + k]) || (l > 0 && f[((l - 1) * 16 + i) * 8 + k]) || (i < 15 && f[(l * 16 + i + 1) * 8 + k]) || (i > 0 && f[(l * 16 + i - 1) * 8 + k]) || (k < 7 && f[(l * 16 + i) * 8 + k + 1]) || (k > 0 && f[(l * 16 + i) * 8 + k - 1]));
        if (fl && (k < 4 || r.nextInt(2) != 0)) { BT* b = at(l, i, k); if (b && *b != AIR && !liq(*b)) *b = (BT)STONE; } }
    (void)bio;
}

// ---------- masmorras: WorldGenDungeons (MonsterRoom) do 1.12.2; sem baú/spawner (blocos inexistentes no jogo) ----------
static void dungeonGen(Chunk* c, int cx, int cz, JR2& r, int px, int py, int pz) {
    const int j = r.nextInt(2) + 2, k = -j - 1, l = j + 1, k1 = r.nextInt(2) + 2, l1 = -k1 - 1, i2 = k1 + 1;
    auto at = [&](int x, int y, int z) -> BT* { const int wx = x - cx * 16, wz = z - cz * 16; return (wx < 0 || wx > 15 || wz < 0 || wz > 15 || y < 0 || y > 255) ? nullptr : &c->b[idx(wx, y, wz)]; };
    auto solid = [](BT b) { return b != AIR && !isWater(b) && b != (BT)LAVA && !isLeafB(b); };
    int cnt = 0;
    for (int a = k; a <= l; a++) for (int b = -1; b <= 4; b++) for (int d = l1; d <= i2; d++) {
        BT* p = at(px + a, py + b, pz + d); if (!p) continue;
        if ((b == -1 || b == 4) && !solid(*p)) return;
        if ((a == k || a == l || d == l1 || d == i2) && b == 0 && *p == AIR) { BT* u = at(px + a, py + 1, pz + d); if (u && *u == AIR) cnt++; }
    }
    if (cnt < 1 || cnt > 5) return;
    for (int a = k; a <= l; a++) for (int b = 3; b >= -1; b--) for (int d = l1; d <= i2; d++) {
        BT* p = at(px + a, py + b, pz + d); if (!p) continue;
        if (a != k && b != -1 && d != l1 && a != l && b != 4 && d != i2) *p = AIR;
        else { BT* dn = at(px + a, py + b - 1, pz + d); if (py + b >= 0 && dn && !solid(*dn)) *p = AIR;
               else if (solid(*p)) *p = (b == -1 && r.nextInt(4) != 0) ? (BT)MOSSY : (BT)COBBLE; }
    }
}

static void populateChunk(Chunk* c, int cx, int cz) {
    const BT gr = (BT)pkFind("block/granite"), di = (BT)pkFind("block/diorite"), an = (BT)pkFind("block/andesite"), em = (BT)pkFind("block/emerald_ore");
    std::vector<BT> host{(BT)STONE}; for (BT v : {gr, di, an}) if (v) host.push_back(v);
    const int ws = (int)host.size(); const int64_t seed = (int64_t)WORLD_SEED;
    floorOf(cx, cz);
    for (int pz = cz - 1; pz <= cz; pz++) for (int px = cx - 1; px <= cx; px++) {
        JR2 r(seed); const int64_t k = r.nextLong() / 2 * 2 + 1, l = r.nextLong() / 2 * 2 + 1;   // ChunkGeneratorOverworld.populate
        const int64_t ps = (int64_t)((uint64_t)((int64_t)px * k) + (uint64_t)((int64_t)pz * l)) ^ seed; r.setSeed(ps);
        const int ox = px * 16, oz = pz * 16;
        { const int pb = biomeAt((float)(ox + 16), (float)(oz + 16));   // lagos antes da decoração, como em ChunkGeneratorOverworld.populate
          if (pb != B_DESERT && r.nextInt(4) == 0) lakeGen(c, cx, cz, r, ox + r.nextInt(16) + 8, r.nextInt(256), oz + r.nextInt(16) + 8, false, pb);
          if (r.nextInt(8) == 0) { const int lx = ox + r.nextInt(16) + 8, ly = r.nextInt(r.nextInt(248) + 8), lz = oz + r.nextInt(16) + 8; if (ly < SEA || r.nextInt(10) == 0) lakeGen(c, cx, cz, r, lx, ly, lz, true, pb); } }
        for (int t = 0; t < 8; t++) { const int dx = ox + r.nextInt(16) + 8, dy = r.nextInt(256), dz = oz + r.nextInt(16) + 8; dungeonGen(c, cx, cz, r, dx, dy, dz); }   // masmorras (8 tentativas), depois dos lagos
        auto ore1 = [&](BT id, int cnt, int size, int y0, int y1) { if (!id) { for (int t = 0; t < cnt; t++) { r.nextInt(16); r.nextInt(y1 - y0); r.nextInt(16); r.nextFloat(); r.nextInt(3); r.nextInt(3); for (int q = 0; q < size; q++) r.nextDouble(); } return; }
            for (int t = 0; t < cnt; t++) { const int x = ox + r.nextInt(16), y = r.nextInt(y1 - y0) + y0, z = oz + r.nextInt(16); veinGen(c, cx, cz, r, id, size, x, y, z, host.data(), ws); } };
        ore1((BT)DIRT, 10, 33, 0, 256); ore1((BT)GRAVEL, 8, 33, 0, 256); ore1(gr, 10, 33, 0, 80); ore1(di, 10, 33, 0, 80); ore1(an, 10, 33, 0, 80);
        ore1((BT)COALORE, 20, 17, 0, 128); ore1((BT)IRONORE, 20, 9, 0, 64); ore1((BT)GOLDORE, 2, 9, 0, 32); ore1((BT)REDSTONEORE, 8, 8, 0, 16); ore1((BT)DIAMONDORE, 1, 8, 0, 16);
        { const int x = ox + r.nextInt(16), y = r.nextInt(16) + r.nextInt(16) + 0, z = oz + r.nextInt(16); veinGen(c, cx, cz, r, (BT)LAPISORE, 7, x, y, z, host.data(), ws); }   // genStandardOre2(1, lápis 7, centro 16, espalhamento 16)
        for (int t = 0; t < 3; t++) { const int x = ox + r.nextInt(16) + 8, z = oz + r.nextInt(16) + 8; diskGen(c, cx, cz, r, x, z, (BT)SAND, 7, false); }
        { const int x = ox + r.nextInt(16) + 8, z = oz + r.nextInt(16) + 8; diskGen(c, cx, cz, r, x, z, (BT)CLAY, 4, true); }
        { const int x = ox + r.nextInt(16) + 8, z = oz + r.nextInt(16) + 8; diskGen(c, cx, cz, r, x, z, (BT)GRAVEL, 6, false); }
        {   // nascentes de água (50) e lava (20) em pedra, como WorldGenLiquids (sorteio próprio; o MC usa o mesmo fluxo, mas as posições já não batem 1:1)
            JR2 q(ps ^ 0x5F10D5LL);
            auto spring = [&](int wx, int y, int wz, BT liquid) {
                const int x = wx - cx * 16, z = wz - cz * 16; if (x < 1 || x > 14 || z < 1 || z > 14 || y < 2 || y > 250) return;
                if (c->b[idx(x, y + 1, z)] != STONE || c->b[idx(x, y - 1, z)] != STONE) return;
                const BT m = c->b[idx(x, y, z)]; if (m != AIR && m != STONE) return;
                int st = 0, air = 0; const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
                for (int i = 0; i < 4; i++) { const BT n = c->b[idx(x + dx[i], y, z + dz[i])]; if (n == STONE) st++; else if (n == AIR) air++; }
                if (st == 3 && air == 1) c->b[idx(x, y, z)] = liquid; };
            for (int t = 0; t < 50; t++) { const int x = ox + q.nextInt(16) + 8, y = q.nextInt(q.nextInt(248) + 8), z = oz + q.nextInt(16) + 8; spring(x, y, z, (BT)WATER); }
            for (int t = 0; t < 20; t++) { const int x = ox + q.nextInt(16) + 8, y = q.nextInt(q.nextInt(q.nextInt(240) + 8) + 8), z = oz + q.nextInt(16) + 8; spring(x, y, z, (BT)LAVA); }
        }
        if (px == cx && pz == cz && em && biomeAt((float)(ox + 16), (float)(oz + 16)) == B_MOUNTAINS) {   // BiomeHills: esmeraldas 3..8 pontos soltos em pedra
            JR2 e(ps ^ 0x5DEECE66DLL); const int n = 3 + e.nextInt(6);
            for (int t = 0; t < n; t++) { const int x = e.nextInt(16), y = e.nextInt(28) + 4, z = e.nextInt(16); BT& b = c->b[idx(x, y, z)]; if (b == STONE) b = em; }
        }
    }
}
