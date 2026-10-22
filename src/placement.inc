// ================= colocacao de blocos (ItemBlock.onItemUse) =================
// Um unico caminho para colocar qualquer bloco do inventario: acha a celula-alvo, recusa o que nao cabe, calcula o METADADO (orientacao)
// a partir da face clicada / ponto de impacto / direcao do jogador, confere a sobreposicao com o jogador usando a FORMA REAL do bloco novo
// e grava com setBM (que avisa os vizinhos). Blocos com regra propria registram uma PlaceFn (blockPlaceSet); eixo (gBlkAxis) ja vem pronto.
struct PlaceCtx {
    int hx, hy, hz;          // bloco clicado
    int face;                // face clicada: 0 -X, 1 +X, 2 -Y, 3 +Y, 4 -Z, 5 +Z (mesma convencao de RayHit)
    double fx, fy, fz;       // ponto de impacto DENTRO da celula clicada (0..1): meio-bloco (laje, escada, alcapao)
    float yaw, pitch; bool sneak;
    double px, py, pz;       // pes do jogador (sobreposicao)
};
typedef bool (*PlaceFn)(int id, const PlaceCtx& c, int x, int y, int z, int& meta);   // false = recusa; preenche meta
static uint8_t gPlaceSel[65536]; static std::vector<PlaceFn> gPlaceFns(1);
static void blockPlaceSet(int id, PlaceFn f) { if ((unsigned)id < 65536u) { gPlaceFns.push_back(f); gPlaceSel[id] = (uint8_t)(gPlaceFns.size() - 1); } }

// direcao horizontal em que o jogador olha: 0 N(-Z), 1 L(+X), 2 S(+Z), 3 O(-X). yaw 0 olha para -Z (convencao do jogo)
static inline int facingFromYaw(float yawDeg) {
    const float dx = -sinf(yawDeg * .0174533f), dz = -cosf(yawDeg * .0174533f);
    return fabsf(dx) > fabsf(dz) ? (dx > 0 ? 1 : 3) : (dz > 0 ? 2 : 0);
}
static inline int axisFromFace(int face) { return face < 2 ? 1 : face < 4 ? 0 : 2; }   // meta de eixo: 0 Y, 1 X, 2 Z
static inline bool blockReplaceable(int id) { return id == AIR || isLiquid(id) || xbReplaceable(id); }   // o que ceda lugar ao colocar (ar, liquido, mato)

static std::vector<Phys*> pistonAll();   // splitscreen.inc: gPvpP + jogadores extras do split
// Minecraft (World.checkNoEntityCollision): nao se coloca bloco com colisao onde ha jogador, mob, bote ou carrinho; nada e empurrado.
// Itens no chao, flechas e espectadores nao impedem.
static bool fallEntBlocks(const AABB& bx);   // falling.inc: areia/cascalho caindo e TNT acesa
static bool entBlocksBox(const AABB& bx) {
    if (fallEntBlocks(bx)) return true;
    for (Phys* p : pistonAll()) if (p && !p->noclip && AABB{p->x - PW, p->y, p->z - PW, p->x + PW, p->y + PH, p->z + PW}.intersects(bx)) return true;
    for (const auto& kv : remotes) { const RemotePl& r = kv.second; if (!r.init || r.gm == 3 || r.deathT >= 0) continue; const double h = r.sneak ? 1.65 : 1.8; if (AABB{r.x - .3, r.y, r.z - .3, r.x + .3, r.y + h, r.z + .3}.intersects(bx)) return true; }
    for (const MobEnt& e : MOBS) if (!e.dead && AABB{e.x - mobW(e) * .5, e.y, e.z - mobW(e) * .5, e.x + mobW(e) * .5, e.y + mobH(e), e.z + mobW(e) * .5}.intersects(bx)) return true;
    for (const BoatEnt& b : BOATS) if (!b.dead && AABB{b.x - .75, b.y, b.z - .75, b.x + .75, b.y + .6, b.z + .75}.intersects(bx)) return true;
    for (const CartEnt& c : CARTS) if (!c.dead && AABB{c.x - .49, c.y, c.z - .49, c.x + .49, c.y + .7, c.z + .49}.intersects(bx)) return true;
    return false;
}
static bool placeBlock(int id, const PlaceCtx& c, int* outPos = nullptr) {
    if (!id || c.face < 0 || c.face > 5) return false;
    int t[3] = {c.hx + FACE_N[c.face][0], c.hy + FACE_N[c.face][1], c.hz + FACE_N[c.face][2]};
    if (xbReplaceable(getB(c.hx, c.hy, c.hz))) { t[0] = c.hx; t[1] = c.hy; t[2] = c.hz; }   // mirar no mato: substitui ele mesmo
    if (t[1] < 0 || t[1] >= CH || !getC(fdiv(t[0], CW), fdiv(t[2], CW))) return false;
    if (!blockReplaceable(getB(t[0], t[1], t[2]))) return false;
    int meta = 0;
    if (gBlkAxis[id]) meta = axisFromFace(c.face);
    if (gBlkFacing[id]) meta = (facingFromYaw(c.yaw) + 2) & 3;   // a frente aponta para o jogador (oposto de para onde ele olha), como abobora/fornalha no Minecraft
    if (const uint8_t ps = gPlaceSel[id]) if (gPlaceFns[ps] && !gPlaceFns[ps](id, c, t[0], t[1], t[2], meta)) return false;
    // nao coloca dentro do jogador: testa as caixas de colisao do bloco NOVO (laje no alto do pe nao colide, cubo sim)
    AABB tmp[SH_MAXB]; const int n = blockBoxesId(id, t[0], t[1], t[2], false, tmp, meta);
    const AABB pl{c.px - .3, c.py, c.pz - .3, c.px + .3, c.py + 1.8, c.pz + .3};
    for (int i = 0; i < n; i++) if (pl.intersects(tmp[i])) return false;
    for (int i = 0; i < n; i++) if (entBlocksBox(tmp[i])) return false;   // outros jogadores, mobs, botes e carrinhos tambem
    setBM(t[0], t[1], t[2], id, meta); netSendBlock(t[0], t[1], t[2], id, meta);
    if (outPos) { outPos[0] = t[0]; outPos[1] = t[1]; outPos[2] = t[2]; }
    return true;
}
