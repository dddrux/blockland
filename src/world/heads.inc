// patchHead.1: cabecas de mob colocaveis (esqueleto, Wither, zumbi, creeper, Steve). Cada cabeca = 4 faces 16x16 recortadas da textura de entidade do pack
// (64x32/64x64: topo (8,0), base (16,0), lado (0,8), rosto (8,8) em unidades de 8px). As faces entram como itens "block/X_skull_{front,top,side,bottom}" ANTES de pkBuildProps:
// a regra X_top + X_side + FaceDef (pkblocks.inc) cria o bloco com rosto virado para o jogador. Itens-face ficam ocultos (pkIsFaceOnly).
static void headRegister(int cap) {
    // {nome, textura, tamanho de referencia da textura, lado do rosto (px de referencia), rosto (x,y), topo, lado, base}; HEAD_N (blockshape.inc) tem a mesma ordem
    struct HD { const char* n; const char* t; int ref, sz, f[2], tp[2], sd[2], bt[2]; };
    static const HD H[] = {
        {"skeleton", "entity/skeleton/skeleton", 64, 8, {8, 8}, {8, 0}, {0, 8}, {16, 0}}, {"wither_skeleton", "entity/skeleton/wither_skeleton", 64, 8, {8, 8}, {8, 0}, {0, 8}, {16, 0}},
        {"zombie", "entity/zombie/zombie", 64, 8, {8, 8}, {8, 0}, {0, 8}, {16, 0}}, {"creeper", "entity/creeper/creeper", 64, 8, {8, 8}, {8, 0}, {0, 8}, {16, 0}},
        {"steve", "entity/steve", 64, 8, {8, 8}, {8, 0}, {0, 8}, {16, 0}},
        {"dragon", "entity/enderdragon/dragon", 256, 16, {128, 46}, {128, 30}, {112, 46}, {144, 30}}};   // dragao: faces do icone/ancora = cabeca superior (upperhead, texOffs 112,30 16x16x16); o modelo 3D usa a textura inteira (entmodels.inc: entDragonHead)
    static const char* const FN[4] = {"front", "top", "side", "bottom"};
    for (const HD& h : H) {
        std::vector<uint8_t> px; int w = 0, ht = 0;
        if (!packRGBA(std::string(h.t) + ".png", px, w, ht) || w < h.ref || ht < w / 2) continue;
        if ((int)PK.size() + 4 >= cap) return;
        const int u = h.sz * w / h.ref; const int XY[4][2] = {{h.f[0], h.f[1]}, {h.tp[0], h.tp[1]}, {h.sd[0], h.sd[1]}, {h.bt[0], h.bt[1]}};
        for (int k = 0; k < 4; k++) {
            const int ox = XY[k][0] * w / h.ref, oy = XY[k][1] * w / h.ref;
            std::vector<uint8_t> c((size_t)u * u * 4);
            for (int y = 0; y < u; y++) memcpy(&c[(size_t)y * u * 4], &px[((size_t)(oy + y) * w + ox) * 4], (size_t)u * 4);
            uint8_t t[1024]; scaleRGBA(c, u, u, t, 16, 16); for (int i = 0; i < 256; i++) t[i * 4 + 3] = 255;
            PkItem it; it.file = std::string("block/") + h.n + "_skull_" + FN[k]; it.name = pkTitle(std::string(h.n) + "_skull_" + FN[k]); it.cube = false; it.stack = 64; it.tile = PKTILE0 + (int)PK.size();
            putTile16(it.tile, t); PK.push_back(std::move(it));
        }
    }
}
