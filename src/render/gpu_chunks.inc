// VBOs/GPU de chunks (movido de main_loop.inc em patchOrg.4; texto identico): initVBO, quadDrawVBO, gpuTrim, chunkUnload
// VBOs (OpenGL 1.5): a malha fica na VRAM e só é reenviada quando o chunk muda.
// Com DRI_PRIME/PRIME offload, arrays em RAM eram copiados pelo PCIe a cada quadro -> travadas.
static PFNGLGENBUFFERSPROC pGenBuffers; static PFNGLBINDBUFFERPROC pBindBuffer; static PFNGLBUFFERDATAPROC pBufferData;
static PFNGLBLENDCOLORPROC pBlendColor; static bool useVBO = false; static int vsyncMode = 0;
static void initVBO() {
    pGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
    pBindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
    pBufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
    pBlendColor = (PFNGLBLENDCOLORPROC)SDL_GL_GetProcAddress("glBlendColor");
    useVBO = pGenBuffers && pBindBuffer && pBufferData;
}
// Quadros desenhados como TRIANGULOS INDEXADOS (buffer de indices unico e compartilhado, 4 vertices por quad em vez de 6): o driver nao precisa
// converter GL_QUADS a cada chamada (custo de CPU) e a GPU reaproveita vertices ja transformados (cache), como o Sodium/Iris fazem.
#ifdef __ANDROID__
#define ANDROID_RF_FAR 32.f
#else
#define ANDROID_RF_FAR 64.f
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
static GLuint gQuadIB = 0; static size_t gQuadIBCap = 0;
static int gPiPhase = 0;   // patchRpiPerf.4 (Pi): 0 = tudo, 1 = so cubos opacos (shader sem discard), 2 = so o 2o trecho (recortados)
#if defined(__ANDROID__) || defined(BL_VULKAN) || defined(BL_RPI) || defined(BL_XP)   // patchXP.1: Chrome9 tambem so rende bem com indices de 16 bits; patchRpiPerf.1: Pi (VC4) so tem indices de 16 bits: com uint32 o Mesa converte na CPU a cada desenho
typedef uint16_t QIdx; static const GLenum QIDX_GL = 0x1403; static const int QMAXV = 65532;   // GLES2/gl4es: indices de 16 bits (uint32 vira conversao na CPU a cada desenho); desenhos maiores sao divididos
#else
typedef uint32_t QIdx; static const GLenum QIDX_GL = 0x1405; static const int QMAXV = 0x7fffffff;
#endif
static void bindQuadIB(size_t quads) {
#if defined(__ANDROID__) || defined(BL_VULKAN) || defined(BL_RPI) || defined(BL_XP)
    if (!gQuadIB) quads = (size_t)(QMAXV / 4);   // buffer fixo (16383 quads), criado uma vez
    if (!gQuadIB || quads > gQuadIBCap) {
        const size_t cap = (size_t)(QMAXV / 4);
#else
    if (quads > gQuadIBCap || !gQuadIB) {
        const size_t cap = std::max(quads, gQuadIBCap * 3 / 2 + 8192);
#endif
        std::vector<QIdx> ix(cap * 6);
        for (size_t q = 0; q < cap; q++) { const QIdx b = (QIdx)(q * 4); QIdx* p = &ix[q * 6]; p[0] = b; p[1] = b + 1; p[2] = b + 2; p[3] = b; p[4] = b + 2; p[5] = b + 3; }
        if (!gQuadIB) pGenBuffers(1, &gQuadIB);
        pBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gQuadIB); pBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(ix.size() * sizeof(QIdx)), ix.data(), GL_STATIC_DRAW); gQuadIBCap = cap;
    } else pBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gQuadIB);
}
// desenha nv vertices (quads) a partir do byte o do VBO ligado; cor opcional. Divide em pedacos se o indice for de 16 bits.
static void quadDrawVBO(size_t o, int nv, bool color) {
    for (int d = 0; d < nv; d += QMAXV) {
        const int n = std::min(nv - d, QMAXV); const size_t oo = o + (size_t)d * sizeof(V);
        glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)(oo + offsetof(V, x))); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)(oo + offsetof(V, u)));
        if (color) glColorPointer(4, GL_FLOAT, sizeof(V), (const void*)(oo + offsetof(V, r)));
        glDrawElements(GL_TRIANGLES, (GLsizei)(n / 4 * 6), QIDX_GL, (const void*)0);
    }
}
static void uploadChunk(Chunk* c) {   // sólidos e água sobem separados (água muda muito mais que o terreno)
    PerfScope perfUp(PS_UPLOAD);
    if (c->gl) {
        if (!c->vbo) pGenBuffers(1, &c->vbo);
        pBindBuffer(GL_ARRAY_BUFFER, c->vbo); pBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(c->mesh.size() * sizeof(V)), c->mesh.data(), GL_STATIC_DRAW);
        std::vector<V>().swap(c->mesh); c->gl = false;   // libera a RAM
    }
    if (c->glw) {
        if (!c->wvbo) pGenBuffers(1, &c->wvbo);
        pBindBuffer(GL_ARRAY_BUFFER, c->wvbo); pBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(c->wmesh.size() * sizeof(V)), c->wmesh.data(), GL_STATIC_DRAW);
        std::vector<V>().swap(c->wmesh); c->glw = false;
    }
}
// Libera da VRAM as malhas dos chunks distantes (alem de RAD+4): os dados de blocos continuam na RAM (o save nao muda); ao voltar, a malha e refeita.
// Antes todo chunk visitado ficava na GPU para sempre => a memoria de video enchia e o quadro ficava cada vez mais lento.
static int gTrimX2[3] = {0x7fffffff, 0x7fffffff, 0x7fffffff}, gTrimZ2[3] = {0, 0, 0};   // centro do jogador 2 (split screen): suas malhas tambem ficam na GPU
static void gpuTrim(int pcx, int pcz) {
    static void (*pDel)(GLsizei, const GLuint*) = (void (*)(GLsizei, const GLuint*))SDL_GL_GetProcAddress("glDeleteBuffers");
    if (!useVBO || !pDel) return;
    const int tl = RAD + (oldUnload() ? 4 : 2); const int lim = tl * tl; int cnt = 0; double bytes = 0;
    for (auto& kv : W) {
        Chunk* c = kv.second; if (!c->vbo && !c->wvbo && !c->gl && !c->glw) continue;
        const int dx = (int)(int32_t)(kv.first >> 32) - pcx, dz = (int)(int32_t)(uint32_t)kv.first - pcz;
        bool nearX = false; for (int q = 0; q < 3; q++) if (gTrimX2[q] != 0x7fffffff) { const int ex = (int)(int32_t)(kv.first >> 32) - gTrimX2[q], ez = (int)(int32_t)(uint32_t)kv.first - gTrimZ2[q]; if (ex * ex + ez * ez <= lim) nearX = true; }
        if (dx * dx + dz * dz > lim && !nearX) {
            if (c->vbo) pDel(1, &c->vbo); if (c->wvbo) pDel(1, &c->wvbo);
            c->vbo = c->wvbo = 0; c->nv = c->nwv = c->ngv = 0; c->gl = c->glw = false; c->dirty = true;
            std::vector<V>().swap(c->mesh); std::vector<V>().swap(c->wmesh);
        } else { cnt++; bytes += (double)(c->nv + c->nwv) * sizeof(V); }
    }
    gGpuChunks = cnt; gGpuMB = bytes / 1048576.0;
}
// patchUnload.1: DESCARREGA chunks longe de todos os jogadores (alem de RAD+6): codifica blocos/meta em savedC/savedM (RLE, ~10x menor que o chunk),
// libera VBOs, luz e malhas, e tira de W. Antes todo chunk visitado ficava na RAM para sempre (e em varreduras de W). Ao voltar, gen() decodifica de savedC (igual a carregar do save);
// o save grava W + savedC (chunks descarregados nao se perdem). Max `maxN` por chamada para nao engasgar. Retorna quantos descarregou.
static int chunkUnload(int pcx, int pcz, int maxN) {
    static void (*pDel)(GLsizei, const GLuint*) = (void (*)(GLsizei, const GLuint*))SDL_GL_GetProcAddress("glDeleteBuffers");
    if (gPanoGen || gGenJob || maxN <= 0) return 0;
    const int ul = RAD + (oldUnload() ? 6 : 3); const int lim = ul * ul; const size_t N = (size_t)CW * CH * CW;
    std::vector<std::array<int, 2>> others;   // jogador 2/3/4 (tela dividida) e convidados: nada perto deles e descarregado
    for (int q = 0; q < 3; q++) if (gTrimX2[q] != 0x7fffffff) others.push_back({gTrimX2[q], gTrimZ2[q]});
    for (auto& r : remotes) if (r.second.init) others.push_back({fdiv((int)floor(r.second.x), CW), fdiv((int)floor(r.second.z), CW)});
    std::vector<int64_t> del;
    for (auto& kv : W) {
        const int cx = (int)(int32_t)(kv.first >> 32), cz = (int)(int32_t)(uint32_t)kv.first, dx = cx - pcx, dz = cz - pcz;
        if (dx * dx + dz * dz <= lim) continue;
        bool near = false; for (auto& o : others) { const int ex = cx - o[0], ez = cz - o[1]; if (ex * ex + ez * ez <= lim) { near = true; break; } }
        if (!near) { del.push_back(kv.first); if ((int)del.size() >= maxN) break; }
    }
    for (int64_t k : del) {
        auto it = W.find(k); if (it == W.end()) continue; Chunk* c = it->second;
        savedC[k] = savedEncB(c->b, N); savedM[k] = savedEncM(c->m, N);
        if (pDel) { if (c->vbo) pDel(1, &c->vbo); if (c->wvbo) pDel(1, &c->wvbo); }
        delete[] c->lc; delete c; W.erase(it);
    }
    if (!del.empty()) { getCReset(); gEpoch++; gKick = true; }   // gEpoch: invalida o cache de chunks visiveis e os fantasmas (que guardam Chunk*)
    return (int)del.size();
}
// céu em degradê (zênite -> horizonte); o horizonte tem a mesma cor do fog, então o terreno distante some suavemente nele
