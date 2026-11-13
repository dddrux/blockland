// patchChunks.2: snapshot 3x3 de chunks para montar a malha FORA da thread principal. Incluido por world.inc logo depois de build().
// snapTake (main): copia b/m/lc dos 9 chunks; o alvo (centro) leva tambem os escalares. meshRun (qualquer thread): build() sobre a copia. snapApply (main): devolve o resultado ao chunk real.
struct ChunkSnap {
    Chunk* c[9] = {nullptr}; int cx = 0, cz = 0; bool waterOnly = false, far = false; int lod = 0; bool done = false; uint32_t uid = 0;
    ~ChunkSnap() { for (int i = 0; i < 9; i++) if (c[i]) { delete[] c[i]->lc; delete c[i]; c[i] = nullptr; } }
};
static ChunkSnap* snapTake(int cx, int cz, bool waterOnly, bool far, int lod) {
    Chunk* real = getC(cx, cz); if (!real) return nullptr;
    ChunkSnap* s = new ChunkSnap(); s->cx = cx; s->cz = cz; s->uid = real->uid; s->waterOnly = waterOnly; s->far = far; s->lod = lod;
    for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) {
        Chunk* r = getC(cx + dx, cz + dz); if (!r) continue;
        Chunk* t = new Chunk();
        if (r == real) { std::vector<V> m0, w0; m0.swap(r->mesh); w0.swap(r->wmesh); *t = *r; m0.swap(r->mesh); w0.swap(r->wmesh); }   // escalares do alvo sem copiar as malhas grandes
        else { memcpy(t->b, r->b, sizeof(t->b)); memcpy(t->m, r->m, sizeof(t->m)); t->maxY = r->maxY; }
        t->lc = nullptr; if (r->lc) { t->lc = new uint8_t[CW * CH * CW]; memcpy(t->lc, r->lc, (size_t)CW * CH * CW); }
        s->c[(dz + 1) * 3 + (dx + 1)] = t;
    }
    return s;
}
static void meshRun(ChunkSnap* s) {   // thread de trabalho OU main (teste): so enxerga o snapshot
    for (int i = 0; i < 9; i++) tlSnapC[i] = s->c[i];
    tlSnapCx = s->cx; tlSnapCz = s->cz; tlSnapOn = true; gMeshFar = s->far; gMeshLod = s->lod;
    build(s->cx, s->cz, s->c[4], s->waterOnly);
    gMeshFar = false; gMeshLod = 0; tlSnapOn = false; for (int i = 0; i < 9; i++) tlSnapC[i] = nullptr;
}
static void snapApply(ChunkSnap* s, Chunk* real) {   // main: copia so o que build() escreve
    Chunk* t = s->c[4]; if (!t || !real) return;
    real->wmesh.swap(t->wmesh); real->wOff[0] = t->wOff[0]; memcpy(real->wOff, t->wOff, sizeof(t->wOff));
    real->ngv = t->ngv; real->hasTrans = t->hasTrans; real->nwv = t->nwv; real->glw = t->glw; real->secOk = t->secOk;
    if (s->waterOnly) return;
    real->mesh.swap(t->mesh); memcpy(real->sOff, t->sOff, sizeof(t->sOff)); memcpy(real->xOff, t->xOff, sizeof(t->xOff));
    memcpy(real->conn, t->conn, sizeof(t->conn)); real->connOk = t->connOk; real->cubeN = t->cubeN; real->nv = t->nv; real->gl = t->gl;
    real->farM = t->farM; real->lodL = t->lodL; real->maxY = t->maxY;
    delete[] real->lc; real->lc = t->lc; t->lc = nullptr;   // luz em cache: o chunk real passa a ter a nova
}
// patchChunks.3b: malha async. meshSubmit (main): limpa dirty, tira snapshot e manda o job; o worker so toca na copia. meshDrain (main): aplica os prontos.
// Edicao de bloco durante o job volta a marcar dirty no chunk real (apply nao mexe em dirty) => nova malha; resultado de chunk que sumiu (uid) e descartado.
static std::atomic<int> gMeshBusy{0}; static std::mutex gMeshMu; static std::vector<ChunkSnap*> gMeshDone;
static bool meshAsyncOn() {
    static int on = -1; if (on < 0) { const char* e = getenv("BL_SYNC_MESH"); on = (e && *e == '1') ? 0 : 1; }
    if (!on) return false; if (!jobPool().ready()) jobPool().start(0); return jobPool().ready();
}
static int meshAsyncCap() { return std::max(2, jobPool().nth.load() * 3); }   // fila curta: o jogador muda de lugar e o que esta na fila deixa de ser urgente
static bool meshSubmit(int cx, int cz, Chunk* c, bool waterOnly, bool far, int lod) {
    ChunkSnap* s = snapTake(cx, cz, waterOnly, far, lod); if (!s) return false;
    c->dirty = false; c->wdirty = false; c->busy = true; gMeshBusy++;
    jobPool().submit([s] {
        static thread_local bool tlW = (perfMarkWorker(), true); (void)tlW;
        meshRun(s); s->done = true;
        { std::lock_guard<std::mutex> lk(gMeshMu); gMeshDone.push_back(s); }
    });
    return true;
}
static void meshDrain(int maxN) {
    if (!gMeshBusy.load()) return;
    std::vector<ChunkSnap*> v; { std::lock_guard<std::mutex> lk(gMeshMu); if (gMeshDone.empty()) return; const size_t n = std::min((size_t)std::max(1, maxN), gMeshDone.size()); v.assign(gMeshDone.begin(), gMeshDone.begin() + n); gMeshDone.erase(gMeshDone.begin(), gMeshDone.begin() + n); }
    for (ChunkSnap* s : v) {
        Chunk* real = getC(s->cx, s->cz);
        if (real && real->uid == s->uid) { snapApply(s, real); real->busy = false; gKick = true; }
        delete s; gMeshBusy--;
    }
}
