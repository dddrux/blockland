// VBOs (OpenGL 1.5): a malha fica na VRAM e só é reenviada quando o chunk muda.
// Com DRI_PRIME/PRIME offload, arrays em RAM eram copiados pelo PCIe a cada quadro -> travadas.
static PFNGLGENBUFFERSPROC pGenBuffers; static PFNGLBINDBUFFERPROC pBindBuffer; static PFNGLBUFFERDATAPROC pBufferData;
static PFNGLBLENDCOLORPROC pBlendColor; static bool useVBO = false; static int vsyncMode = 0;
// ---- balde (Item Bucket do 1.8): vazio pega fonte de agua/lava (raio atravessa liquidos); cheio coloca uma fonte na celula da face mirada ----
static bool bucketUse(Slot& s, const float* eye, const float* dir) {
    if (!gBktEmpty || (s.id != gBktEmpty && s.id != gBktWater && s.id != gBktLava && !(gBktMilk && s.id == gBktMilk))) return false;
    if (s.id == gBktMilk) { gPoisonT = 0; if (!creative) s.id = gBktEmpty; return true; }   // patchBucket.2: beber leite remove o veneno e devolve o balde vazio
    RayHit r;
    if (rayCast(eye, dir, 5.f, r, false) && getB(r.x, r.y, r.z) == XB_DECOR && (getM(r.x, r.y, r.z) & 3) == 0) {   // patchBucket.3: caldeirao (meta bits 2-3 = nivel 0..3)
        const int cm = getM(r.x, r.y, r.z), lv = (cm >> 2) & 3;
        if (s.id == gBktWater) {
            if (lv < 3) { setBM(r.x, r.y, r.z, XB_DECOR, (cm & 3) | (3 << 2)); netSendBlock(r.x, r.y, r.z, XB_DECOR, (cm & 3) | (3 << 2)); if (!creative) s.id = gBktEmpty; }
            return true;
        }
        if (s.id == gBktEmpty && lv == 3) {
            setBM(r.x, r.y, r.z, XB_DECOR, cm & 3); netSendBlock(r.x, r.y, r.z, XB_DECOR, cm & 3);
            if (!creative) {
                if (s.n <= 1) { s.id = gBktWater; s.n = 1; }
                else { --s.n; if (addItem(gBktWater, 1) > 0) spawnDrop(eye[0], eye[1] - .3, eye[2], dir[0] * .2, .1, dir[2] * .2, gBktWater, 1, 10); }
            }
            return true;
        }
    }
    if (s.id == gBktEmpty) {
        if (gBktMilk) {   // patchBucket.1: balde vazio na vaca (adulta) = balde de leite (vale se a vaca estiver mais perto que o bloco)
            int cow = -1; float bt = 2.8f;
            for (int i = 0; i < (int)MOBS.size(); i++) {
                const MobEnt& e = MOBS[i]; if (e.dead || e.type != M_COW) continue;
                const float hw = MD[e.type].w * .5f + .1f, lo[3] = {(float)e.x - hw, (float)e.y - .1f, (float)e.z - hw}, hi[3] = {(float)e.x + hw, (float)e.y + MD[e.type].h + .1f, (float)e.z + hw};
                float t0 = 0.f, t1 = bt; bool ok = true;
                for (int a = 0; a < 3 && ok; a++) {
                    if (fabsf(dir[a]) < 1e-6f) { if (eye[a] < lo[a] || eye[a] > hi[a]) ok = false; continue; }
                    float ta = (lo[a] - eye[a]) / dir[a], tb = (hi[a] - eye[a]) / dir[a]; if (ta > tb) std::swap(ta, tb);
                    t0 = std::max(t0, ta); t1 = std::min(t1, tb); if (t0 > t1) ok = false;
                }
                if (ok && t0 < bt) { bt = t0; cow = i; }
            }
            if (cow >= 0) {
                const float tb = raySolidDistance(eye, dir, 2.8f);
                if (tb < 0.f || tb >= bt) {
                    if (!creative) {
                        if (s.n <= 1) { s.id = gBktMilk; s.n = 1; }
                        else { --s.n; if (addItem(gBktMilk, 1) > 0) spawnDrop(eye[0], eye[1] - .3, eye[2], dir[0] * .2, .1, dir[2] * .2, gBktMilk, 1, 10); }
                    }
                    return true;
                }
            }
        }
        if (!rayCast(eye, dir, 5.f, r, true)) return true;
        const int b = getB(r.x, r.y, r.z);
        const int full = b == WATER ? gBktWater : b == LAVA ? gBktLava : 0;   // so FONTES (fluxo nao enche o balde)
        if (!full) return true;
        setB(r.x, r.y, r.z, AIR); netSendBlock(r.x, r.y, r.z, AIR);
        if (!creative) {   // no criativo o balde continua vazio
            if (s.n <= 1) { s.id = full; s.n = 1; }
            else { --s.n; if (addItem(full, 1) > 0) spawnDrop(eye[0], eye[1] - .3, eye[2], dir[0] * .2, .1, dir[2] * .2, full, 1, 10); }
        }
        return true;
    }
    if (!rayCast(eye, dir, 5.f, r, false)) return true;   // balde cheio mira so em blocos solidos e usa a celula da face
    const int tx = r.x + r.nx, ty = r.y + r.ny, tz = r.z + r.nz;
    if (ty < 0 || ty >= CH) return true;
    const int tb = getB(tx, ty, tz);
    if (solid(tb)) return true;
    if (s.id == gBktWater && gDim == 1) {   // patchBucket.1: Nether (ultra quente): a agua evapora, o balde esvazia e nenhuma fonte e criada
        if (!creative) s.id = gBktEmpty;
        return true;
    }
    setB(tx, ty, tz, s.id == gBktWater ? WATER : LAVA); netSendBlock(tx, ty, tz, s.id == gBktWater ? WATER : LAVA);
    if (!creative) s.id = gBktEmpty;   // no criativo o balde continua cheio
    return true;
}
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
#if defined(__ANDROID__) || defined(BL_VULKAN)
typedef uint16_t QIdx; static const GLenum QIDX_GL = 0x1403; static const int QMAXV = 65532;   // GLES2/gl4es: indices de 16 bits (uint32 vira conversao na CPU a cada desenho); desenhos maiores sao divididos
#else
typedef uint32_t QIdx; static const GLenum QIDX_GL = 0x1405; static const int QMAXV = 0x7fffffff;
#endif
static void bindQuadIB(size_t quads) {
#if defined(__ANDROID__) || defined(BL_VULKAN)
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
static int gTrimX2 = 0x7fffffff, gTrimZ2 = 0;   // centro do jogador 2 (split screen): suas malhas tambem ficam na GPU
static void gpuTrim(int pcx, int pcz) {
    static void (*pDel)(GLsizei, const GLuint*) = (void (*)(GLsizei, const GLuint*))SDL_GL_GetProcAddress("glDeleteBuffers");
    if (!useVBO || !pDel) return;
    const int lim = (RAD + 4) * (RAD + 4); int cnt = 0; double bytes = 0;
    for (auto& kv : W) {
        Chunk* c = kv.second; if (!c->vbo && !c->wvbo && !c->gl && !c->glw) continue;
        const int dx = (int)(int32_t)(kv.first >> 32) - pcx, dz = (int)(int32_t)(uint32_t)kv.first - pcz;
        const int ex = (int)(int32_t)(kv.first >> 32) - gTrimX2, ez = (int)(int32_t)(uint32_t)kv.first - gTrimZ2;
        if (dx * dx + dz * dz > lim && (gTrimX2 == 0x7fffffff || ex * ex + ez * ez > lim)) {
            if (c->vbo) pDel(1, &c->vbo); if (c->wvbo) pDel(1, &c->wvbo);
            c->vbo = c->wvbo = 0; c->nv = c->nwv = c->ngv = 0; c->gl = c->glw = false; c->dirty = true;
            std::vector<V>().swap(c->mesh); std::vector<V>().swap(c->wmesh);
        } else { cnt++; bytes += (double)(c->nv + c->nwv) * sizeof(V); }
    }
    gGpuChunks = cnt; gGpuMB = bytes / 1048576.0;
}
// céu em degradê (zênite -> horizonte); o horizonte tem a mesma cor do fog, então o terreno distante some suavemente nele
static void drawSkyDome(const float* top, const float* hor) {
    static const float el[7] = {-90.f, 0.f, 8.f, 20.f, 38.f, 60.f, 90.f};
    glDisable(GL_TEXTURE_2D); glDisable(GL_FOG); glDisable(GL_DEPTH_TEST); glDisable(GL_ALPHA_TEST); glDepthMask(GL_FALSE);
    const int N = 24; const float R = 100.f;   // tem que ser < plano far (200), senão o domo é cortado e vira uma bola
    glBegin(GL_QUADS);
    for (int b = 0; b < 6; b++) for (int j = 0; j < N; j++) {
        const float a0 = j * 6.2831853f / N, a1 = (j + 1) * 6.2831853f / N;
        const float ea[4] = {el[b], el[b], el[b + 1], el[b + 1]}, aa[4] = {a0, a1, a1, a0};
        for (int k = 0; k < 4; k++) {
            const float e = ea[k] * .0174533f, f = ea[k] <= 0.f ? 0.f : powf(ea[k] / 90.f, .55f);
            glColor3f(hor[0] + (top[0] - hor[0]) * f, hor[1] + (top[1] - hor[1]) * f, hor[2] + (top[2] - hor[2]) * f);
            glVertex3f(R * cosf(e) * sinf(aa[k]), R * sinf(e), -R * cosf(e) * cosf(aa[k]));
        }
    }
    glEnd();
    glDepthMask(GL_TRUE); glEnable(GL_TEXTURE_2D);
}
// panorama do titulo: mundo gerado (seed fixa) visto de cima, girando devagar
#ifdef __ANDROID__
#define PANO_RAD 4   // celular: panorama do menu com 81 chunks (em vez de 289) = abre o menu bem mais rapido e usa menos RAM/GPU
#else
#ifdef BL_RPI
#define PANO_RAD 2   // Raspberry: panorama minimo (quase sem custo)
#else
#define PANO_RAD 8
#endif
#endif
static bool panoOn = false; static float panoT = 0.f;
static void panoInit() {
    resetWorldData(); WORLD_SEED = 0x5EED1234u; applyGm(1); gHard = false; RAD = PANO_RAD;   // panorama do menu: raio fixo (8 no PC, 4 no celular), nao importa a distancia configurada
#ifndef BL_RPI
    for (int i = 0; i < 3; i++) stream(0, 0, 100000, 100000);   // Raspberry: sem panorama (nao gera mundo no titulo)
#endif
    panoOn = true; panoT = 0.f;
}
static void drawPanorama(int w, int h, float dt) {
#ifdef BL_RPI
    panoT += dt; blViewport(w, h); glClearColor(.09f, .09f, .11f, 1.f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); return;   // Raspberry: fundo escuro liso, sem panorama
#endif
    panoT += dt; const float yawD = panoT * 2.f;
    int gy = SEA; for (int y = CH - 1; y >= 0; y--) if (solid(getB(8, y, 8))) { gy = y; break; }
    const float cy = (float)std::max(gy, SEA) + 6.f;
    blViewport(w, h);
    const float sky[4] = {.74f, .86f, 1.f, 1.f}, top[3] = {.27f, .50f, .90f};
    glClearColor(sky[0], sky[1], sky[2], 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const float n = .05f, t = n * tanf(42.f * .0174533f), r = t * w / (float)h;
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glFrustum(-r, r, -t, t, n, 700);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glRotatef(18.f, 1, 0, 0); glRotatef(yawD, 0, 1, 0);   // pitch 18 graus para baixo (era 8): olha mais de cima, como o menu oficial
    drawSkyDome(top, sky);
    glTranslatef(-8.5f, -cy, -8.5f);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.02f);
    glEnable(GL_FOG); glFogi(GL_FOG_MODE, GL_LINEAR); glFogf(GL_FOG_START, PANO_RAD * 4.75f); glFogf(GL_FOG_END, PANO_RAD * 13.5f); glFogfv(GL_FOG_COLOR, sky);   // neblina proporcional ao raio (some a borda do mundo)
    glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D); glColor3f(1, 1, 1);
    glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
    skyShaderOn(0.f, 1.f, 1.f);   // panorama do título: sempre meio-dia
    for (int pass = 0; pass < 2; pass++) {
        if (pass) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); skyShaderOn(1.f, 1.f, 1.f); }
        for (int dz = -RAD; dz <= RAD; dz++) for (int dx = -RAD; dx <= RAD; dx++) {
            Chunk* c = getC(dx, dz); if (!c) continue;
            const int cnt = pass ? c->nwv : c->nv; if (cnt <= 0) continue;
            if (useVBO) {
                if (c->gl || c->glw) uploadChunk(c);
                pBindBuffer(GL_ARRAY_BUFFER, pass ? c->wvbo : c->vbo);
                glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)offsetof(V, x)); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)offsetof(V, u)); glColorPointer(4, GL_FLOAT, sizeof(V), (const void*)offsetof(V, r));
            } else { std::vector<V>& m = pass ? c->wmesh : c->mesh; const V* v = m.data(); glVertexPointer(3, GL_FLOAT, sizeof(V), &v->x); glTexCoordPointer(2, GL_FLOAT, sizeof(V), &v->u); glColorPointer(4, GL_FLOAT, sizeof(V), &v->r); }
            glDrawArrays(GL_QUADS, 0, (GLsizei)cnt);
        }
    }
    skyShaderOff();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_FOG);
}
#include "shadows.inc"   // sombras do sol (F7)
static bool touchShadowOn() { return gShadowsOn; }   // usado pelo HUD touch (botao de luz acende quando as sombras estao ligadas)
#include "godrays.inc"   // god rays no nascer/por do sol (F9)
int main(int argc, char** argv) {
#ifndef _WIN32
    setenv("GIO_USE_VFS", "local", 1);
#endif   // evita o aviso/timeout do GVFS (GIO tentando falar com o daemon gvfsd)
#ifdef __ANDROID__
    androidPrepare();   // chdir p/ armazenamento do app + extrai assets do APK (antes do logInit)
#endif
    logInit();
    bool startFull = false; for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--fullscreen")) startFull = true;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { logFatal("Falha ao iniciar o SDL: %s", SDL_GetError()); return 1; }
    sndInit();   // audio.inc (sem dispositivo de som o jogo segue mudo)
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) logWarn("Controles indisponiveis: %s", SDL_GetError());   // sem isso o jogo segue so com teclado/mouse/toque
    { SDL_version sv; SDL_GetVersion(&sv); const char* vd = SDL_GetCurrentVideoDriver();
      logInfo("Ambiente: SDL %d.%d.%d, driver de video: %s, CPU: %d nucleos, RAM: %d MB", (int)sv.major, (int)sv.minor, (int)sv.patch, vd ? vd : "?", SDL_GetCPUCount(), SDL_GetSystemRAM());
      std::string av; for (int i = 1; i < argc; i++) { if (i > 1) av += ' '; av += argv[i]; }
      logInfo("Argumentos: %s", av.empty() ? "(nenhum)" : av.c_str()); }
#ifdef __ANDROID__
    androidGLAttrs();
#endif
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);   // stencil: máscara de água do bote
    SDL_Window* win = SDL_CreateWindow("Blockland", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, BL_WIN_W, BL_WIN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (startFull ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) | BL_ANDROID_WINFLAGS);
    if (!win) { fprintf(stderr, "janela: %s\n", SDL_GetError()); return 1; }
    SDL_SetWindowMinimumSize(win, BL_WIN_MINW, BL_WIN_MINH);   // trava o tamanho minimo: launcher e split nao quebram o layout em janela pequena
    gBlWin = win;
    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) { fprintf(stderr, "OpenGL: %s\n", SDL_GetError()); return 1; }
#ifdef __ANDROID__
    androidGLInit(); touchHud = true;   // gl4es pronto; controles mobile ja ligados (sem precisar do F8)
#endif
#if defined(__ANDROID__) || defined(BL_RPI)
    SDL_GL_SetSwapInterval(1);   // celular/Raspberry: vsync ligado (celular: FPS destravado esquenta e engasga): vsync ligado (FPS destravado esquenta o aparelho, causa throttling e engasgos)
#else
    SDL_GL_SetSwapInterval(0);   // FPS destravado (use /vsync 1 para ligar)
#endif
    { int fw = 0, fh = 0; SDL_GetWindowSize(win, &fw, &fh);
      logInfo("Janela criada: %dx%d%s", fw, fh, startFull ? " (tela cheia)" : "");
      logInfo("OpenGL: %s", (const char*)glGetString(GL_VERSION)); logInfo("GPU: %s (%s)", (const char*)glGetString(GL_RENDERER), (const char*)glGetString(GL_VENDOR)); }
    Uint32 stageLast = SDL_GetTicks();
    auto stageDone = [&](const char* what) { const Uint32 n = SDL_GetTicks(); logInfo("%s (%u ms)", what, (unsigned)(n - stageLast)); stageLast = n; };   // tempo de cada etapa da inicializacao
    splashInit(); splashStep(win, 0.f, .04f);   // janela ja aparece com a tela de loading; cada etapa abaixo avanca a barra
    initVBO(); logInfo("VBOs (OpenGL 1.5): %s", useVBO ? "disponiveis" : "INDISPONIVEIS (usando arrays em RAM)"); splashStep(win, .04f, .08f);
    initSkyShader();   // luz do mundo com ciclo dia/noite (GL 2.0); tem de vir antes de gerar qualquer malha
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    stageDone("Shaders de luz/ciclo dia-noite prontos");
    splashStep(win, .08f, .55f);
    applyMcPack();   // resource pack do Minecraft (assets/resourcepacks): UNICA fonte das texturas de blocos/itens (+ todos os seus blocos/itens como itens extras)
    stageDone("Resource pack carregado");
    splashStep(win, .55f, .65f);
    xbInit(); netherInit();   // modelos do pack (tocha, alavanca, bigorna, plantas): tem de vir antes de subir o atlas
    stageDone("Modelos de blocos prontos");
    splashStep(win, .65f, .75f);
    registerPackTab();
    loadMods();
    modulesInit();   // ganchos dos modulos (src/modules)
    stageDone("Abas do criativo, mods e modulos carregados");
    splashStep(win, .75f, .85f);
    bakeBiomeTiles();
    loadGUIAssets();
    stageDone("Texturas de bioma e interface carregadas");
    splashStep(win, .85f, .93f);
    initArmor(); initElytra();   // camadas de armadura (pack ativo ou cor do material)
    stageDone("Armaduras prontas");
    splashStep(win, .93f, .96f);   // ANTES de ligar o atlas: o splash faz bind da propria textura
    glGenTextures(1, &atlasTex);
    glBindTexture(GL_TEXTURE_2D, atlasTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    uploadAtlasMips();   // nivel 0 + 4 mipmaps por tile (acaba com o tremor/pisca da vegetacao a distancia)
    glEnable(GL_TEXTURE_2D); glDisable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.02f);   // recorte das folhas (como o Minecraft)

    // Skin padrão também fica em assets, assim pode ser trocada sem recompilar.
    if (!loadSkin("assets/skins/default.png")) { defaultSkin(); uploadSkin(); buildModel(); }
    gpLoad();   // mapeamento do controle (gamepad.cfg)
    profileLoad();   // perfil salvo (nome + skin escolhida); uma skin passada na linha de comando ainda vale mais
    const char* skinFile = netInit(argc, argv, win);
    if (argc > 1) {
        if (FILE* tf = fopen(skinFile, "rb")) { fclose(tf); loadSkin(skinFile); }
    }
    stageDone("Atlas de texturas enviado a GPU, skin e rede prontos");
    splashStep(win, .96f, .995f);
    printf("Controles: WASD mover | Shift agachar | Ctrl correr | Espaco pular | E inventario | 1-9/roda hotbar | F4 criativo/sobrevivencia | 2x Espaco voar (criativo; Espaco sobe, Shift desce) | T ou / chat e comandos (/ajuda, /gamemode 0|1) | F3 informacoes de depuracao | F5 camera | F6 bracos finos/normais | arraste um .png de skin na janela\n");


    int start[9] = {GRASS, DIRT, STONE, SAND, LOG, PLANKS, COBBLE, IT_WSWORD, IT_APPLE};
    for (int i = 0; i < 9; i++) inv[i] = {start[i], maxStack(start[i])};

    // ---- estado do jogador (Java: posX/prevPosX, motionX..., rotationYaw...) ----
    Phys P; gPvpP = &P;
    P.x = P.px = 8.5; P.z = P.pz = 8.5; P.y = P.py = 60; optLoad(); gfxTogglesSync(); RAD = rdChunks();
    for (int y = CH - 1; y >= 0; y--) if (getB(8, y, 8)) { P.y = P.py = y + 1.0; break; }
    float yaw = 0, pitch = 0;

    bool deadFree = false;   // mouse solto na tela de morte
    bool grab = true, invOpen = false, dbg = false, pauseOpen = false;   // pauseOpen: menu de pausa (Esc)
    bool& hideHud = gHideHud;   // F1 oculta HUD e mão (teclado e botão do toque compartilham o estado)
    int sel = 0, camMode = 0, lastHeld = -1, handItem = 0, mouseX = 0, mouseY = 0;
    float swing = 0, equip = 0, rep = 0, nameT = 0, msgT = 0;
    bool swinging = false;
    int mnX = 0, mnY = 0, mnZ = 0, mnHeld = -1; float mnT = 0, mnNeed = 1, mnCool = 0, mnSw = 0, mnPt = 0; bool mnOn = false, mnHv = true;   // mineração em sobrevivência
    // corpo/cabeça (EntityLivingBase.renderYawOffset)
    float bodyYaw = 0, prevBodyYaw = 0;
    // membros (limbSwing / limbSwingAmount) e balanço de câmera (cameraYaw/cameraPitch)
    float limbSwing = 0, limbAmt = 0, prevLimbAmt = 0;
    float camBobYaw = 0, prevCamBobYaw = 0, camBobPitch = 0, prevCamBobPitch = 0;
    float ageTicks = 0;
    float fovMod = 1.f, prevFovMod = 1.f, lastFov = 70.f;
    // sprint como no EntityPlayerSP
    int sprintToggleTimer = 0, jumpTicks = 0;
    bool lastSneakKey = false, lastFwdOk = false, lastJumpKey = false; int flyTimer = 0;
    // 3ª pessoa: distância nominal 4 blocos, sem suavização (como o Java).
    const float thirdPersonDistance = 4.f;
    double tickAcc = 0.0;
    std::string msg;
    GpIn gi1;   // entrada analogica do controle do jogador 1 neste quadro (gamepad.inc)
    relMouse(false); grab = false;   // abre no menu de titulo
    SaveInfo loadSi; int loadLogPct = 0; int loadCx = 0, loadCz = 0; float loadProg = 0.f; Uint32 loadT0 = 0, loadDoneT = 0, fadeT0 = 0; bool fading = false, quitPhase = false, titleQuick = false; Uint32 quitT0 = 0, titleT0 = 0;
    auto startWorld = [&]() {   // carrega (ou cria) o save gMenuDir
        panoOn = false; resetWorldData(); netEdits.clear(); netPend.clear(); remotes.clear(); RAD = rdChunks(); gCurDir = gMenuDir; SaveInfo si;
        { FILE* f = fopen(("saves/" + gCurDir + "/level.cfg").c_str(), "rb"); std::string t; if (f) { char b[512]; size_t r; while ((r = fread(b, 1, sizeof b, f)) > 0) t.append(b, r); fclose(f); }
          WORLD_SEED = (int64_t)strtoll(cfgGet(t, "seed").c_str(), nullptr, 10); applyGm(atoi(cfgGet(t, "mode").c_str())); gHard = atoi(cfgGet(t, "hardcore").c_str()) != 0;
          gWorldTime = atoll(cfgGet(t, "time").c_str()); gDayCycle = true;
          if (!cfgGet(t, "weather").empty()) { gWeather = std::max(0, std::min(2, atoi(cfgGet(t, "weather").c_str()))); gWxLeft = std::max(600, atoi(cfgGet(t, "wxleft").c_str())); gRain = gWeather > 0 ? 1.f : 0.f; gThunder = gWeather == 2 ? 1.f : 0.f; } else { gWeather = 0; gWxLeft = 30000; gRain = gThunder = 0.f; }   // patchWx.5: clima salvo   // hora do mundo (save sem "time" começa no amanhecer)
          if (!cfgGet(t, "px").empty()) { si.hasPos = true; si.px = atof(cfgGet(t, "px").c_str()); si.py = atof(cfgGet(t, "py").c_str()); si.pz = atof(cfgGet(t, "pz").c_str()); } }
        logInfo("Loading world '%s' (seed: %lld, modo: %s)", gCurDir.c_str(), (long long)WORLD_SEED, creative ? "Criativo" : "Sobrevivencia");
        loadSavedChunks(gCurDir); netherOnWorldLoad(gCurDir); boatsLoad(gCurDir); cartsLoad(gCurDir); chestsLoad(gCurDir); furnLoad(gCurDir);
        survRespawn(P); gRespHold = 0; P.flying = (gGm == 3);   // (ao carregar mundo a posicao vem do save/finishLoad: sem correcao de renascimento)
        if (!loadPlayer()) {   // primeira vez neste mundo: kit inicial; depois o inventario volta como foi salvo
            for (int i = 0; i < 36; i++) inv[i] = Slot(); cur = Slot();
            for (int i = 0; i < 9; i++) inv[i] = {start[i], maxStack(start[i])};
        }
        loadSi = si; loadCx = si.hasPos ? fdiv((int)floor(si.px), CW) : 0; loadCz = si.hasPos ? fdiv((int)floor(si.pz), CW) : 0; loadProg = 0.f; loadT0 = SDL_GetTicks(); loadDoneT = 0; loadLogPct = 0;
        gScreen = SC_LOAD; grab = false; relMouse(false);
    };
    auto finishLoad = [&]() {
        const SaveInfo& si = loadSi;
        safeSpawn(loadCx, loadCz, gSpX, gSpY, gSpZ);
        if (si.hasPos) {   // posicao salva; se estiver dentro de blocos, sobe ate achar espaco livre
            P.x = P.px = si.px; P.y = P.py = si.py; P.z = P.pz = si.pz;
            const int fx = (int)floor(si.px), fz = (int)floor(si.pz); int fy = (int)floor(si.py);
            if (solid(getB(fx, fy, fz)) || solid(getB(fx, fy + 1, fz))) { while (fy < CH - 3 && (solid(getB(fx, fy, fz)) || solid(getB(fx, fy + 1, fz)))) fy++; P.y = P.py = fy; }
        } else { P.x = P.px = gSpX; P.y = P.py = gSpY; P.z = P.pz = gSpZ; }
        yaw = pitch = 0; gHandBob[0] = gHandBob[1] = gHandBob[2] = gHandBob[3] = 0; invOpen = false; pauseOpen = false; grab = true; relMouse(true); gScreen = SC_PLAY; tickAcc = 0.0;
        if (gInvOwner == 2) { split2SwapInv(); gInvOwner = 0; }
        S2.init = false; if (gSplit) split2Spawn(P, yaw);   // mundo novo: jogador 2 nasce ao lado do jogador 1
        if (gJoin == NJ_INGAME) netGuestReady();
        if (gHostPending) { gHostPending = false; netStartHost(gNetPort); }
    };
#ifndef __ANDROID__
    SDL_StartTextInput();   // no Android o teclado so abre no chat
#endif
    chatAdd("Pressione T para o chat ou / para comandos (/ajuda).", 0xFFFF55);
    const float skyTop[3] = {.27f, .50f, .90f}, skyC[4] = {.74f, .86f, 1.f, 1.f}, uwC[4] = {.1f, .25f, .5f, 1.f};   // cores de DIA; o ciclo (updateSky) multiplica pelos fatores do MC
    float skyTopD[3] = {.27f, .50f, .90f}, skyHorD[4] = {.74f, .86f, 1.f, 1.f}, uwD[4] = {.1f, .25f, .5f, 1.f};
    Uint32 last = SDL_GetTicks();
    // posição da câmera/olho do frame anterior (usada para o raycast de mira)
    double viewEye[3] = {P.x, P.y + 1.62, P.z};
    panoInit(); gTitleNoBlack = true;   // panorama do titulo pronto ANTES do fim do loading e sem o fade preto proprio; botoes e titulo entram DEPOIS do splash sumir
    bool run = true;
    if (gCliHost) gHostPending = true;
    if (!gCliJoin.empty()) { std::string jh; int jp; if (srvParse(gCliJoin, jh, jp)) { netJoinBegin(jh, jp); pageGo("conectando"); } }
    else if (!gCliHost) launcherOpen();   // mini launcher antes do menu principal (nome, skin, Jogar)
    splashFinish(win);   // barra alcanca 100% e o overlay passa a sumir (fade 1 s) por cima do menu
    stageDone("Panorama do menu gerado");
    logInfo("Inicializacao concluida em %.3fs", SDL_GetTicks() / 1000.0);
    while (run) {
        SDL_Event e;
        int hh[3], pp[3];
        const Uint8* k = SDL_GetKeyboardState(nullptr);
        gTabHeld = (k[SDL_SCANCODE_TAB] && !chatOpen && !invOpen && !pauseOpen) || gTabTouch;
        gTabTouch = false; for (const TPt& q : tpts) if (q.on && q.kind == 3 && q.idx == TA_TAB && !gHideHud) gTabTouch = true;   // botao de toque: vale enquanto o dedo estiver nele   // lista de jogadores (Tab): so enquanto segura
        float cy = cosf(yaw * .0174533f), sy = sinf(yaw * .0174533f);
        float cp = cosf(pitch * .0174533f), sp_ = sinf(pitch * .0174533f);
        float eye[3] = {(float)viewEye[0], (float)viewEye[1], (float)viewEye[2]};
        float dir[3] = {-sy * cp, sp_, -cy * cp};
        bool has = ray(eye, dir, hh, pp);
        auto say = [&](const std::string& s) { chatAdd(s); };
        auto breakAt = [&](int bx, int by, int bz, bool harvest) {
            int b = getB(bx, by, bz);
            spawnBreak(bx, by, bz, b); sndEvtAt(sndBlockEvt(b, false), bx + .5, by + .5, bz + .5);
            setB(bx, by, bz, AIR); netSendBlock(bx, by, bz, AIR);
            if (isLogB(b) || isLeafB(b)) { keepLeaves.erase(k3(bx, by, bz)); leafNeighbors(bx, by, bz); }
            if (!creative && harvest) { blockDrops(bx, by, bz, b); survExh(.025f); survOreXp(b); }
            if (!creative && blockHardness(b) > 0.f) toolWear(inv[sel]);   // patchDur.1
        };
        bool openChestReq = false;   // pedido para abrir a tela de um bau (processado junto com a mesa)
        bool openTable = false;   // pedido para abrir a mesa de trabalho (processado depois do laço de eventos)
        auto pickPut = [&](int id, bool make, const std::string& nbt = std::string()) -> bool { return invPickPut(sel, id, make, nbt); };   // pick block: ver invPickPut (inventory_ui.inc)
        auto actMain = [&](int btn, bool rpt = false) {   // quebrar / colocar (rpt = repetição do botão segurado: sem alvo não balança o braço)
            if (gDead) return;
            if (gGm == 3) { if (btn == SDL_BUTTON_LEFT && !rpt) spectTeleport(P); return; }   // espectador não interage; clique esquerdo teleporta para um jogador
            if (btn == SDL_BUTTON_MIDDLE) {   // pick block (como no Java): item ja no hotbar => seleciona; na mochila => traz para o slot atual; criativo => cria; sobrevivencia => nada
                if (rpt) return;
                if (creative) {   // pick em entidade (criativo): ovo de spawn do mob mirado, como no Java (so os mobs que tem ovo no jogo)
                    const int mt = mobPickType(eye, dir, 6.f), egg = mt >= 0 ? mobEggItem(mt) : 0;
                    if (egg) {
                        pickPut(egg, true); return;
                    }
                    { const int bi = boatPick(eye, dir, 6.f), ci = cartPick(eye, dir, 6.f);   // bote / carrinho (sem ovo): item do proprio veiculo, como no Java
                      int it = 0; if (bi >= 0) it = boatItemId(BOATS[bi].type); else if (ci >= 0) it = cartItemId(CARTS[ci].type);
                      if (it) { pickPut(it, true); return; } }
                }
                if (!has) return;
                const int pb = getB(hh[0], hh[1], hh[2]);
                if (pb == AIR || isLiquid(pb)) return;
                const int pit = pickItemAt(hh[0], hh[1], hh[2]);   // estado (laje/escada/tocha...) vem do item do modelo, nao do generico
                if (pit <= 0) return;
                std::string pnbt; if (pb == XB_CHEST && (SDL_GetModState() & KMOD_CTRL)) pnbt = chestPickNbt(hh[0], hh[1], hh[2]);   // Ctrl + botao do meio: copia o conteudo do bau (BlockEntityTag); sem Ctrl, bau vazio (touch/controle nunca copiam)
                pickPut(pit, creative, pnbt); return;
            }
            if (btn == SDL_BUTTON_LEFT && !rpt && pvpAttack(eye, dir, inv[sel].id, !P.onGround && P.my < 0 && !P.inWater, P.sprint, creative ? 6.f : 3.f)) { meleeWear(inv[sel]); if (P.sprint) { P.mx *= .6; P.mz *= .6; } P.sprint = false; swinging = true; swing = 0; return; }   // bater em outro jogador
            if (btn == SDL_BUTTON_LEFT && !rpt && mobAttack(eye, dir, inv[sel].id, !P.onGround && P.my < 0 && !P.inWater, P.sprint, creative ? 6.f : 3.f)) { meleeWear(inv[sel]); if (P.sprint) { P.mx *= .6; P.mz *= .6; } P.sprint = false; swinging = true; swing = 0; return; }   // bater em mob
            if (btn == SDL_BUTTON_LEFT && !rpt && boatHit(eye, dir, inv[sel].id, creative, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // bater no bote
            if (btn == SDL_BUTTON_LEFT && !rpt && cartHit(eye, dir, inv[sel].id, creative, creative ? 6.f : 3.f, P)) { swinging = true; swing = 0; return; }   // bater no carrinho
            if (btn == SDL_BUTTON_RIGHT && !rpt && !gBoatRide && !gCartRide && cartInteract(eye, dir, inv[sel], creative, creative ? 6.f : 3.f, P)) { swinging = true; swing = 0; return; }   // montar / abastecer
            if (btn == SDL_BUTTON_RIGHT && !rpt && !gBoatRide && boatInteract(eye, dir, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // montar
            if (btn == SDL_BUTTON_RIGHT && !rpt && mobInteract(eye, dir, creative ? 6.f : 3.f)) { swinging = true; swing = 0; return; }   // aldeao: para e olha
            if (btn == SDL_BUTTON_RIGHT && !rpt) { Slot& bs = inv[sel]; const int bt = boatTypeOfItem(bs.id);   // item de bote: coloca na água (alcance 5)
                if (bt >= 0 && gGm != 2) { if (boatPlace(bt, eye, dir, yaw, 5.f)) { swinging = true; swing = 0; if (!creative && --bs.n <= 0) bs = Slot(); } return; } }
            if (btn == SDL_BUTTON_RIGHT && !rpt && rocketUse(P, inv[sel])) { swinging = true; swing = 0; return; }
            if (btn == SDL_BUTTON_RIGHT && !rpt && (inv[sel].id == IT_EGG || (pearlId() > 0 && inv[sel].id == pearlId())) && gGm != 3 && (!has || P.sneak || !eggBlockUsable(hh[0], hh[1], hh[2])) && eggThrow(P, yaw, pitch, inv[sel])) { swinging = true; swing = 0; return; }   // ovo: arremessa (ovo.inc/egg.inc)   // foguete de artificio: impulso em voo de elitra
            if (btn == SDL_BUTTON_RIGHT && survCanEat(inv[sel].id)) return;   // comer: segurar o botao (eatTick)
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && bucketUse(inv[sel], eye, dir)) { swinging = true; swing = 0; return; }   // balde
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && lilyUse(inv[sel], eye, dir)) { swinging = true; swing = 0; return; }   // vitoria-regia: mira na agua (raio atravessa liquidos)
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && chiselUse(inv[sel], eye, dir, P.sneak)) { swinging = true; swing = 0; return; }   // cinzel
            if (btn == SDL_BUTTON_RIGHT && !rpt && gGm != 2 && flintUse(inv[sel], eye, dir)) { swinging = true; swing = 0; return; }   // isqueiro: fogo / portal
            if (rpt && !has) return;
            if (btn == SDL_BUTTON_LEFT) { swinging = true; swing = 0; }   // só o botão esquerdo balança no ar; colocar bloco balança só se colocar de fato
            if (!has) return;
            if (btn == SDL_BUTTON_LEFT) {
                int b = getB(hh[0], hh[1], hh[2]);
                if (b == BEDROCK) return;
                if (creative) breakAt(hh[0], hh[1], hh[2], false);   // sobrevivência: quebra pelo tempo, no bloco de mineração abaixo
            } else if (btn == SDL_BUTTON_RIGHT) {
                if (!P.sneak && !rpt && monUse(hh[0], hh[1], hh[2])) { grab = false; relMouse(false); swinging = true; swing = 0; if (touchHud) touchReset(); return; }   // Monitor Touch: toque/clique no bloco abre o navegador
                if (getB(hh[0], hh[1], hh[2]) == XB_CHEST && !rpt && !(P.sneak && inv[sel].id)) {   // bau: abre a tela (agachado com item na mao coloca bloco, como no Java)
                    const int cu = chestOpenAt(hh[0], hh[1], hh[2]); if (cu) { if (cu == 1) openChestReq = true; swinging = true; swing = 0; return; } }
                if (gFurnId && getB(hh[0], hh[1], hh[2]) == gFurnId && !rpt && !(P.sneak && inv[sel].id)) {   // fornalha: abre a tela (agachado com item na mao coloca bloco)
                    const int fu = furnOpenAt(hh[0], hh[1], hh[2]); if (fu) { if (fu == 1) openChestReq = true; swinging = true; swing = 0; return; } }
                if (getB(hh[0], hh[1], hh[2]) == CTABLE && !P.sneak) { gTable = true; openTable = true; swinging = true; swing = 0; return; }   // como no Java: agachado coloca bloco, em pé abre a mesa
                if (gGm == 2) {   // aventura: só interage (alavanca, portas, botões); não coloca blocos
                    const int hb0 = getB(hh[0], hh[1], hh[2]);
                    if (xbIsLever(hb0) && !P.sneak && !rpt) { xbToggleLever(hh[0], hh[1], hh[2]); swinging = true; swing = 0; return; }
                    if (!P.sneak && !rpt && rsUse(hh[0], hh[1], hh[2])) { swinging = true; swing = 0; return; }
                    return; }
                { Slot& e0 = inv[sel]; const int mt = eggMob(e0.id);   // ovo de spawn
                  if (mt >= 0 && !rpt) { spawnMob(mt, pp[0] + .5, pp[1], pp[2] + .5, yaw + 180.f); swinging = true; swing = 0; if (!creative && --e0.n <= 0) e0 = Slot(); return; } }
                Slot& s = inv[sel];
                if (gEndFrameId && !rpt && getB(hh[0], hh[1], hh[2]) == gEndFrameId && !(getM(hh[0], hh[1], hh[2]) & 4) && isPk(s.id) && PK[s.id - PK0].file == "item/ender_eye") {   // encaixa o olho do Ender na moldura
                    setBM(hh[0], hh[1], hh[2], gEndFrameId, getM(hh[0], hh[1], hh[2]) | 4); netSendBlock(hh[0], hh[1], hh[2], gEndFrameId, getM(hh[0], hh[1], hh[2]));
                    tryOpenEnd(hh[0], hh[1], hh[2]); swinging = true; swing = 0; if (!creative && --s.n <= 0) s = Slot(); return; }
                { const int cu = cropUse(s, hh, pp, rpt); if (cu) { swinging = true; swing = 0; if (cu == 2 && !creative && --s.n <= 0) s = Slot(); return; } }   // patchCrop.1: enxada / semente
                { const int ru = railUse(s, hh, pp, rpt); if (ru) { if (ru == 2) { swinging = true; swing = 0; if (!creative && --s.n <= 0) s = Slot(); } return; } }   // trilho / carrinho (minecart.inc)
                { const int hb0 = getB(hh[0], hh[1], hh[2]);
                  if (xbIsLever(hb0) && !P.sneak && !rpt) { xbToggleLever(hh[0], hh[1], hh[2]); swinging = true; swing = 0; return; }   // liga/desliga a alavanca
                  if (!P.sneak && !rpt && rsUse(hh[0], hh[1], hh[2])) { swinging = true; swing = 0; return; }   // porta/alcapao/portao/repetidor/comparador
                  if (const int xb = xbItemBlock(s.id)) { { auto gv = gXbVar.find(s.id); gXbPlaceVar = gv != gXbVar.end() ? gv->second : 0; } if (!s.nbt.empty() && netSock >= 0 && !netHost) return;   /* convidado nao guarda conteudo de bau: nao coloca bau cheio (perderia os itens) */ gPlaceNbt = s.nbt; const bool plcd = !rpt && (isRs(xb) ? rsPlace(xb, rsItemVar(s.id), hh, pp, yaw, P.x, P.y, P.z) : xbPlace(xb, hh, pp, yaw, P.x, P.y, P.z)); gPlaceNbt.clear(); if (plcd) { swinging = true; swing = 0; if (!creative && --s.n <= 0) s = Slot(); } return; } }
                if (!s.id || !isBlk(s.id)) return;
                PlaceCtx pc{hh[0], hh[1], hh[2], 3, .5, .5, .5, yaw, pitch, P.sneak, P.x, P.y, P.z};
                if (gRay.hit && gRay.x == hh[0] && gRay.y == hh[1] && gRay.z == hh[2]) { pc.face = gRay.face; pc.fx = gRay.hx - hh[0]; pc.fy = gRay.hy - hh[1]; pc.fz = gRay.hz - hh[2]; }
                int tp[3]; if (!placeBlock(s.id, pc, tp)) return;
                sndEvtAt(sndBlockEvt(s.id, true), tp[0] + .5, tp[1] + .5, tp[2] + .5);   // colocar bloco: som de pedra mais grave (como o Java)
                swinging = true; swing = 0;
                if (isLeafB(s.id)) keepLeaves.insert(k3(tp[0], tp[1], tp[2]));
                if (!creative && --s.n <= 0) s = Slot();
            }
        };
        auto act = [&](int btn, bool rpt = false) {   // botao direito com a mao principal vazia usa o item da mao esquerda (colocar tocha/bloco)
            const bool oh = btn == SDL_BUTTON_RIGHT && gGm != 3 && !inv[sel].id && offh.id;
            if (oh) std::swap(inv[sel], offh);
            actMain(btn, rpt);
            if (oh) std::swap(inv[sel], offh);
        };
        auto setInv = [&](bool on) {
            if (on && gGm == 3) return;   // espectador não tem inventário
            invReset();
            if (!on) { closeInv(); if (gInvOwner == 2) { split2SwapInv(); gInvOwner = 0; } }   // inventario do jogador 2: devolve o conteudo do jogador 1 DEPOIS de guardar o crafting
            invOpen = on; grab = !on; relMouse(grab);
        };
        auto setPause = [&](bool on) {   // menu de pausa: solta o mouse e congela o jogo (so no modo solo, como o Java); fechar devolve o mouse
            pauseOpen = on; grab = !on; relMouse(grab);
            if (on) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); mouseX = ww / 2; mouseY = wh / 2; SDL_WarpMouseInWindow(win, mouseX, mouseY); }
        };
        auto setChat = [&](bool on, const char* pre, const char* skip) {   // abre/fecha o chat (solta o mouse enquanto digita)
            if (!on) gChatAs2 = false; chatOpen = on; chatIn = on ? pre : ""; chatSkip = on ? skip : ""; chatHistPos = -1; chatScroll = 0; chatCur = (size_t)-1; chatSelAll = false;
            grab = !on; relMouse(grab);
            if (on) SDL_StartTextInput();
#ifdef __ANDROID__
            else SDL_StopTextInput();   // fecha o teclado virtual ao sair do chat
#endif
        };
        while (SDL_PollEvent(&e)) {
            if (gpEvent(e)) continue;   // conexao/desconexao de controles
            if (e.type == SDL_KEYDOWN && !e.key.repeat && (e.key.keysym.sym == SDLK_F11 || (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)))) {   // tela cheia: F11 ou Alt+Enter (vale em qualquer tela/menu)
                const bool fs = (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                if (SDL_SetWindowFullscreen(win, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) == 0) logInfo(fs ? "Modo janela" : "Tela cheia ligada"); else logError("Falha ao alternar tela cheia: %s", SDL_GetError());
                continue;
            }
#ifdef __ANDROID__
            if (e.type == SDL_APP_WILLENTERBACKGROUND && !gScreen) saveCurWorld(P.x, P.y, P.z);   // Android pode matar o app em segundo plano
            if (e.type == SDL_APP_WILLENTERBACKGROUND) logSyncSession();   // atualiza o log datado antes de o sistema poder matar o app
            if (e.type == SDL_APP_TERMINATING) logFinalize("encerrado pelo sistema");
#endif
            if (e.type == SDL_QUIT) { if (!gScreen) saveCurWorld(P.x, P.y, P.z); run = false; }
            else if (monIsOpen()) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); monEvent(e, ww, wh); if (!monIsOpen()) { grab = true; relMouse(true); if (touchHud) touchReset(); } }   // Monitor Touch aberto: consome a entrada
            else if (gScreen) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); if (!splashActive()) { if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) sndEvtUi("ui.click"); menuEvent(e, ww, wh); } }   // overlay do splash bloqueia o menu (como no Minecraft)
            else if (pauseOpen && !gPage.empty()) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); if (e.type == SDL_MOUSEMOTION) { mouseX = e.motion.x; mouseY = e.motion.y; } pageEvent(e, ww, wh); }   // Opções dentro do menu de pausa
            else if (touchHud && grab && !invOpen && !chatOpen && [&] { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); return touchEvent(e, ww, wh); }()) {}   // HUD touch consumiu o evento
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST && grab) setPause(true);   // trocar de janela abre o menu de pausa (como o Java)
            else if (e.type == SDL_DROPFILE) { if (skinDropped(e.drop.file)) say("Skin carregada"); else say("Skin inválida"); SDL_free(e.drop.file); }
            else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_F3) { if (gGmSw) { gGmSw = false; if (GM_ORDER[gGmSelI] != gGm) setGm(GM_ORDER[gGmSelI]); } else if (!gF3Used) { dbg = !dbg; if (!dbg) msgT = 0; } }
            else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
#ifdef __ANDROID__
                if (k == SDLK_AC_BACK) k = SDLK_ESCAPE;   // botao Voltar = Esc
#endif
                if (chatOpen) { if (chatKey(k, e.key.keysym.mod, P)) setChat(false, "", ""); continue; }
                if (pauseOpen) { if (quitPhase) continue; if (k == SDLK_ESCAPE) setPause(false); else if (k == SDLK_F3) { dbg = !dbg; if (!dbg) msgT = 0; } continue; }   // no menu de pausa só Esc (volta) e F3
                if (!invOpen && grab && (k == SDLK_t || k == SDLK_SLASH)) { setChat(true, k == SDLK_SLASH ? "/" : "", k == SDLK_SLASH ? "/" : "t"); continue; }
                if (k == SDLK_ESCAPE) { if (gDead && !creative) continue; if (invOpen) setInv(false); else setPause(true); }
                if (k == SDLK_q && grab && !invOpen && !chatOpen && gGm != 3 && inv[sel].id) {   // soltar item (Ctrl+Q = pilha inteira)
                    int nn = (SDL_GetModState() & KMOD_CTRL) ? inv[sel].n : 1;
                    throwItem(viewEye, yaw, pitch, inv[sel].id, nn); inv[sel].n -= nn; if (inv[sel].n <= 0) inv[sel] = Slot();
                }
                if (k == SDLK_f && grab && !invOpen && !chatOpen && gGm != 3) std::swap(inv[sel], offh);   // trocar item entre as maos (como o F do Minecraft)
                if (k == SDLK_e && (grab || invOpen) && !(invOpen && invSearching())) setInv(!invOpen);
                if (invOpen && invSearching()) { if (k == SDLK_BACKSPACE) invSearchBack(); }
                if (k == SDLK_F4) { if (invOpen) setInv(false); if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F3]) { gF3Used = true; if (!gGmSw) { gGmSw = true; gGmSelI = 0; for (int i = 0; i < 4; i++) if (GM_ORDER[i] == gGm) gGmSelI = i; } gGmSelI = (gGmSelI + 1) & 3; } else setGameMode(!creative); }   // F3+F4: abre o seletor e cada F4 avanca; soltar F3 aplica (igual ao Java)
                if (k == SDLK_F3) gF3Used = false;   // o debug alterna ao SOLTAR o F3 sem combinacao (SDL_KEYUP abaixo)
                if (k == SDLK_F1) hideHud = !hideHud;
                if (k == SDLK_F5) { camMode = (camMode + 1) % 3; }
                if (k == SDLK_F7) shadowToggle(say);
                if (k == SDLK_F9) godToggle(say);
                if (k == SDLK_F12) { split2Enable(!gSplit, P, yaw); gSt["splitscr"] = gSplit ? 1 : 0; say(gSplit ? "Split screen ligado (jogador 2 usa o controle)" : "Split screen desligado"); }
                if (k == SDLK_F10) flareToggle(say);
                if (k == SDLK_F8) { touchHud = !touchHud; touchReset(); tvSneakT = false; relMouse(grab); say(touchHud ? "HUD touch ligado (F8 desliga)" : "HUD touch desligado"); }
                if (k == SDLK_F6) { slim = !slim; slimAuto = false; buildModel(); say(slim ? "Braços finos" : "Braços normais"); }
                if (invOpen && (k == SDLK_q || k == SDLK_f) && !invSearching()) {   // Q / Ctrl+Q / F sobre o slot apontado (survival e criativo, igual ao baú)
                    int w0, h0; SDL_GetWindowSize(win, &w0, &h0); const int hs = hitSlot(w0, h0, mouseX, mouseY);
                    if (k == SDLK_q) invDropKey(hs, (SDL_GetModState() & KMOD_CTRL) != 0); else invOffhandKey(hs);
                }
                if (k >= SDLK_1 && k <= SDLK_9 && !(invOpen && invSearching())) {
                    int n = k - SDLK_1;
                    int w0, h0; SDL_GetWindowSize(win, &w0, &h0);
                    int hs = invOpen ? hitSlot(w0, h0, mouseX, mouseY) : -1;
                    if (hs >= 0) invNumberKey(n, hs);
                    else if (!invOpen) sel = n;   // com o inventário aberto o Java ignora 1-9 fora de slot
                }
            } else if (e.type == SDL_TEXTINPUT) { if (chatOpen) chatText(e.text.text); else if (invOpen) invSearchText(e.text.text); else if (grab && !strcmp(e.text.text, "/")) setChat(true, "/", "/"); }
            else if (e.type == SDL_MOUSEWHEEL) { if (chatOpen) chatScroll = std::max(0, chatScroll + e.wheel.y * 2); else if (pauseOpen) {} else if (!invOpen) sel = ((sel - e.wheel.y) % 9 + 9) % 9; else invWheel(e.wheel.y); }
            else if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x; mouseY = e.motion.y;
                if (invOpen) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); invMouseMotion(ww, wh, mouseX, mouseY); }
                if (grab) {
                    yaw -= e.motion.xrel * .1f;
                    if (yaw > 180.f) yaw -= 360.f;
                    if (yaw < -180.f) yaw += 360.f;
                    pitch = std::max(-89.f, std::min(89.f, pitch - e.motion.yrel * .1f));
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (touchHud && (invOpen || chatOpen)) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); if (touchCloseRect(ww, wh).in((float)e.button.x, (float)e.button.y)) { if (invOpen) setInv(false); else setChat(false, "", ""); continue; } }   // X do modo celular
                if (gDead && !creative && !pauseOpen) {   // tela de morte: Renascer / Menu principal
                    int ww, wh; SDL_GetWindowSize(win, &ww, &wh);
                    if (e.button.button == SDL_BUTTON_LEFT && !quitPhase) { const int hb = deathHit(ww, wh, e.button.x, e.button.y); if (hb >= 0) sndEvtUi("ui.click"); if (hb == 0) gWantRespawn = true; else if (hb == 1) { quitPhase = true; quitT0 = SDL_GetTicks(); } }
                    continue;
                }
                if (pauseOpen) {   // só "Voltar para o jogo" funciona; os outros botões são placeholders
                    int ww, wh; SDL_GetWindowSize(win, &ww, &wh); if (e.button.button == SDL_BUTTON_LEFT && !quitPhase && pauseHit(ww, wh, e.button.x, e.button.y) != PB_NONE) sndEvtUi("ui.click");
                    if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_BACK && !quitPhase) setPause(false);
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_LAN && !quitPhase) { if (!netLanBusy() && netStartHost(gNetPort)) setPause(false); }
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_SPLIT && !quitPhase) { split2Enable(!gSplit, P, yaw); gSt["splitscr"] = gSplit ? 1 : 0; if (gSplit) setPause(false); }   // Esc > Tela dividida (jogador 2 usa o controle)
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_SPLITMODE && !quitPhase) { gSplitMode ^= 1; gSt["splitmode"] = gSplitMode; }   // Esc > layout do split
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_SKIN2 && !quitPhase) { gMX = e.button.x; gMY = e.button.y; pageGo("launcher"); }   // Esc > skin do jogador 2 (so com split ligado)
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_OPT && !quitPhase) { gMX = e.button.x; gMY = e.button.y; pageGo("opcoes"); }
                    else if (e.button.button == SDL_BUTTON_LEFT && pauseHit(ww, wh, e.button.x, e.button.y) == PB_SAVE && !quitPhase) { quitPhase = true; quitT0 = SDL_GetTicks(); }
                    continue;
                }
                if (invOpen) {
                    int ww, wh; SDL_GetWindowSize(win, &ww, &wh);
                    sndEvtUi("ui.click", .6f); invMouseDown(e.button.button, ww, wh, e.button.x, e.button.y, SDL_GetModState() & KMOD_SHIFT, SDL_GetTicks());
                    continue;
                }
                if (chatOpen) { if (oskActive() && e.button.button == SDL_BUTTON_LEFT) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); oskMouse(e.button.x, e.button.y, ww, wh); } continue; }   // chat: clique no teclado virtual
                if (!grab) { grab = true; relMouse(true); continue; }
                act(e.button.button); rep = 0;
            }
            else if (e.type == SDL_MOUSEBUTTONUP) { if (invOpen) invMouseUp(e.button.button, 0, 0, 0, 0); }
        }

        if (touchHud) {   // HUD touch: aplica o que os toques pediram neste quadro
            if (invOpen || chatOpen || !grab) touchReset();
            gTouchUseItem = grab && !gDead && ((!creative && foodValue(inv[sel].id) > 0 && gFood < 20) || (inv[sel].id == IT_BOW && bowHasArrow()));   // item que se usa segurando
            touchFrame(SDL_GetTicks());
            int tMob = -1; float d2[3] = {dir[0], dir[1], dir[2]}; bool haveD2 = false;   // tMob: IA do mob sob o dedo (2 = aldeão), -1 = nenhum
            {   // MCPE (sem controles divididos): quebrar/colocar/bater age sob o dedo, não no centro da tela
                int ww, wh; SDL_GetWindowSize(win, &ww, &wh);
                bool aim = touchAim; float ax = touchBrkX, ay = touchBrkY;
                if (touchPlace) { aim = true; ax = touchPlaceX; ay = touchPlaceY; }
                has = false;
                if (aim && ww > 0 && wh > 0) {
                    const float tn = tanf(lastFov * .5f * .0174533f), nx = (ax / ww * 2 - 1) * tn * ww / (float)wh, ny = (1 - ay / wh * 2) * tn;
                    const float rgt[3] = {cy, 0, -sy}, upv[3] = {sy * sp_, cp, cy * sp_};
                    for (int i = 0; i < 3; i++) d2[i] = dir[i] + rgt[i] * nx + upv[i] * ny;
                    const float l2 = sqrtf(d2[0] * d2[0] + d2[1] * d2[1] + d2[2] * d2[2]); for (float& v : d2) v /= l2;
                    has = ray(eye, d2, hh, pp); haveD2 = true; tMob = touchMobAim(eye, d2, creative ? 6.f : 3.f);
                }
            }
            auto actAt = [&](int btn) { float sv[3] = {dir[0], dir[1], dir[2]}; if (haveD2) for (int i = 0; i < 3; i++) dir[i] = d2[i]; act(btn); for (int i = 0; i < 3; i++) dir[i] = sv[i]; };   // age no ponto tocado (mobs/botes/blocos)
            if (touchBreak && tMob >= 0 && tMob != 2) {   // Bedrock: segurar sobre um mob = bater repetidamente (não minera)
                touchBreak = false; touchRing = false; touchBrkProg = 0;
                static Uint32 atkLast = 0; const Uint32 nt = SDL_GetTicks(); if (nt - atkLast > 500) { atkLast = nt; actAt(SDL_BUTTON_LEFT); }
            }
            if (touchUse && gEatOn && gEatT >= 0) { touchRing = true; touchBrkProg = std::max(.02f, gEatT / 32.f); }   // Bedrock: segurar com comida = comer em 1,6 s (eatTick); o anel enche enquanto come
            if (tDX != 0 || tDY != 0) {
                yaw -= tDX * .25f; if (yaw > 180.f) yaw -= 360.f; if (yaw < -180.f) yaw += 360.f;
                pitch = std::max(-89.f, std::min(89.f, pitch - tDY * .25f)); tDX = tDY = 0;
            }
            if (touchSel >= 0) { sel = touchSel; touchSel = -1; }
            if (tAct[TA_LIGHT]) shadowToggle(say);   // botao de luz (mobile) = F7: liga/desliga sombras
            if (tAct[TA_CHAT]) setChat(true, "", "");
            if (tAct[TA_F3]) { dbg = !dbg; if (!dbg) msgT = 0; }
            if (tAct[TA_PAUSE]) setPause(true);
            if (tAct[TA_INV]) setInv(true);
            if (tAct[TA_F1]) hideHud = !hideHud;   // F1: oculta o HUD (o botão F1 fica com opacidade baixa)
            if (tAct[TA_F5]) camMode = (camMode + 1) % 3;   // F5: 1ª pessoa > 3ª atrás > 3ª na frente
            if (tAct[TA_DROP] && gGm != 3 && inv[sel].id) { throwItem(viewEye, yaw, pitch, inv[sel].id, 1); if (--inv[sel].n <= 0) inv[sel] = Slot(); }   // Q
            if (tAct[TA_SWAP] && gGm != 3) std::swap(inv[sel], offh);   // F
            if (tAct[TA_SPRINT]) tvSprintL = !tvSprintL;
            if (tAct[TA_PICK]) actMain(SDL_BUTTON_MIDDLE);   // pick block (touch)
            for (bool& a : tAct) a = false;
            if (touchPlace) {   // toque rápido: em mob = bater (aldeão = interagir); senão colocar/interagir. Com comida/arco na mão só segurar usa o item (como o Bedrock)
                touchPlace = false;
                if (grab && !invOpen && !gTouchUseItem) actAt(tMob >= 0 && tMob != 2 ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);
            }
        }
        if (openChestReq) { openChestReq = false; if (!invOpen) setInv(true); }
        if (invOpen && gChest && chestFrame(P.x, P.y, P.z)) setInv(false);
        if (openTable) { openTable = false; if (!invOpen) setInv(true); else gTable = false; }   // fora do bloco touchHud: vale também com mouse/teclado
        {   // ---------- controle (gamepad.inc): le o estado; menus = cursor virtual; jogo = entrada do jogador 1 (e do 2 no split) ----------
            static Uint32 gpT = 0; const Uint32 tn = SDL_GetTicks(); const float gdt = gpT ? std::min(.1f, (tn - gpT) / 1000.f) : 0.f; gpT = tn;
            gSplitMode = gsi("splitmode", 0) & 1;
            if (gSkin2Dirty) { gSkin2Dirty = false; if (S2.skinTex) { glDeleteTextures(1, &S2.skinTex); S2.skinTex = 0; } if (S2.init) split2LoadSkin(); }
            if (!gScreen) split2Enable(gsi("splitscr", 0) != 0, P, yaw);   // toggle do launcher / F12
            gpPoll(); { static const float SN[5] = {.5f, .75f, 1.f, 1.5f, 2.f}; gGpSens = SN[std::max(0, std::min(4, gsi("gpsens", 2)))]; }
            gGpMouse = 0; gi1 = GpIn(); GpPad* pad1 = gpPadFor(1);
            const bool uiMode = gScreen || pauseOpen || invOpen || chatOpen || monIsOpen() || (gDead && !creative);
            if (gpCount() && uiMode) { if (!splashActive()) gpUiFrame(win, gdt); } else gpUiLeave();
            if (!uiMode) {
                for (GpPad& q : gPads) if (q.c && gpPressed(&q, GA_PAUSE)) { setPause(true); break; }
                if (pad1 && grab && !gDead && !pauseOpen) {
                    gi1 = gpRead(pad1);
                    yaw = wrap180(yaw - gi1.lx * GP_LOOK_DEG * gGpSens * gdt); pitch = std::max(-89.f, std::min(89.f, pitch - gi1.ly * GP_LOOK_DEG * gGpSens * gdt));
                    if (gpPressed(pad1, GA_SPRINT) && gi1.fwd > .5f) gGpSprint = true; else if (gi1.fwd < .3f) gGpSprint = false;
                    const int bA = gGpMap[GA_ATTACK], bU = gGpMap[GA_USE];
                    if (pad1->b[bA] && !pad1->pb[bA]) gpPushMouse(win, SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, mouseX, mouseY);   // vira o clique normal (act) no proximo quadro
                    if (!pad1->b[bA] && pad1->pb[bA]) gpPushMouse(win, SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, mouseX, mouseY);
                    if (pad1->b[bU] && !pad1->pb[bU]) gpPushMouse(win, SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, mouseX, mouseY);
                    if (!pad1->b[bU] && pad1->pb[bU]) gpPushMouse(win, SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT, mouseX, mouseY);
                    gGpMouse = (gi1.attack ? SDL_BUTTON(1) : 0) | (gi1.use ? SDL_BUTTON(3) : 0);
                    if (gpPressed(pad1, GA_INV)) setInv(true);
                    else if (gpPressed(pad1, GA_CHAT)) setChat(true, "", "");
                    if (gpPressed(pad1, GA_CAM)) camMode = (camMode + 1) % 3;
                    if (gpPressed(pad1, GA_NEXT) || gpBtnPressed(pad1, GB_RIGHT)) sel = (sel + 1) % 9;
                    if (gpPressed(pad1, GA_PREV) || gpBtnPressed(pad1, GB_LEFT)) sel = (sel + 8) % 9;
                    if (gpPressed(pad1, GA_DROP) && gGm != 3 && inv[sel].id) { throwItem(viewEye, yaw, pitch, inv[sel].id, 1); if (--inv[sel].n <= 0) inv[sel] = Slot(); }
                    if (gpPressed(pad1, GA_SWAP) && gGm != 3) std::swap(inv[sel], offh);
                    if (gpPressed(pad1, GA_PICK)) actMain(SDL_BUTTON_MIDDLE);   // pick block (controle)
                } else if (!pad1 || !grab) gGpSprint = false;
            }
            if (gSplit && split2Frame(gdt, !uiMode && grab, P) && !invOpen && gGm != 3) { split2SwapInv(); gInvOwner = 2; setInv(true); }   // jogador 2 pediu o inventario
        }
        if (gScreen != SC_PLAY) netUpdate(0.f);   // menu/loading: descoberta, handshake e keepalive (em jogo, netUpdate(dt) roda mais abaixo)
        if (gJoin == NJ_WELCOMED && (gScreen == SC_OTHER || gScreen == SC_TITLE)) { netPrepareGuestWorld(); gMenuDir = "_mp"; leavePages(); gMenuAct = 1; }
        if (gMenuAct == 1) startWorld(); else if (gMenuAct == 2) run = false;
        gMenuAct = 0;
        { static Uint32 pAuto = 0; const Uint32 tn = SDL_GetTicks(); if (gScreen == SC_PLAY && !gCurDir.empty() && tn - pAuto > 5000) { pAuto = tn; savePlayer(); } }   // autosave do jogador a cada 5 s
        if (gScreen == SC_LOAD) {   // carrega os chunks iniciais (raio 3 gerado e com malha) antes de mostrar o jogo
            int ww, wh; SDL_GetWindowSize(win, &ww, &wh);
            stream(loadCx, loadCz, 6, 4, 12.0);
            if (useVBO) for (auto& kv : W) if (kv.second->gl || kv.second->glw) uploadChunk(kv.second);   // sobe as malhas durante o loading
            int ok = 0, tot = 0; const int lr = std::min(3, std::max(1, RAD)); const float lrm = (RAD + .5f) * (RAD + .5f);   // so exige o que o stream realmente gera e malha (raio RAD): com RAD baixo o 7x7 fixo nunca fechava (travava em 40%)
            for (int dz = -lr; dz <= lr; dz++) for (int dx = -lr; dx <= lr; dx++) { if ((float)(dx * dx + dz * dz) > lrm) continue; tot++; Chunk* c = getC(loadCx + dx, loadCz + dz); if (c && !c->dirty) ok++; }
            if (tot == 0) { tot = 1; ok = 1; }
            { const int pct = ok * 100 / tot / 10 * 10; if (pct > loadLogPct) { loadLogPct = pct; logInfo("Preparing spawn area: %d%%", pct); } }
            const Uint32 nowL = SDL_GetTicks(); const float el = (nowL - loadT0) / 1000.f;   // barra: 1 s; depois "Pronto!" por 1 s
            if (!loadDoneT && ((ok == tot && el >= 1.f) || el >= 25.f)) loadDoneT = nowL;   // 25 s: nunca fica preso no carregamento
            loadProg = std::min((float)ok / tot, el); drawLoad(ww, wh, loadProg, loadDoneT != 0, 1.f); SDL_GL_SwapWindow(win);
            if (loadDoneT && nowL - loadDoneT >= 1000) { logInfo("Done (%.3fs)! Para ajuda, digite \"/ajuda\"", (nowL - loadT0) / 1000.0); for (auto& kv : W) kv.second->born = 0.f; finishLoad(); last = SDL_GetTicks(); loadDoneT = 0; fading = true; fadeT0 = last; }
            continue;
        }
        if (gScreen) { int ww, wh; SDL_GetWindowSize(win, &ww, &wh); const Uint32 now2 = SDL_GetTicks(); float tIn = 1e9f;
            if (gScreen == SC_TITLE) { RAD = PANO_RAD; if (!panoOn) { panoInit(); titleT0 = SDL_GetTicks(); gTitleNoBlack = false; } drawPanorama(ww, wh, std::min(.1f, (now2 - last) / 1000.f)); if (splashActive()) { gTitleNoBlack = true; titleT0 = SDL_GetTicks(); }   // enquanto o splash esta na tela o contador fica parado em 0
                tIn = (SDL_GetTicks() - titleT0) / 1000.f; if (!gTitleNoBlack) drawFadeBlack(ww, wh, 1.f - tIn / (titleQuick ? .12f : .5f)); gUiA = splashActive() ? 0.f : std::max(0.f, std::min(1.f, (tIn - (titleQuick ? .0f : .35f)) / (titleQuick ? .15f : .5f))); if (tIn > 2.f) titleQuick = false; }   // logo e botoes: fade-in 0,35 s DEPOIS do splash acabar   // fundo aparece primeiro; logo e botoes entram depois
            else { glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
            drawMenu(ww, wh); gUiA = 1.f; last = now2; splashOverlay(ww, wh); logGlErrors("Menu"); SDL_GL_SwapWindow(win); continue; }
        Uint32 now = SDL_GetTicks();
        { const Uint32 raw = now - last; static Uint32 lastSlow = 0;   // como o "Can't keep up!" do Minecraft: quadro muito longo (maximo 1 aviso a cada 5 s)
          if (raw > 500 && now - lastSlow > 5000) { lastSlow = now; logWarn("Can't keep up! O quadro demorou %u ms (limite de simulacao: 250 ms)", (unsigned)raw); } }
        float dt = std::min(.25f, (now - last) / 1000.f);
        last = now;
        dbgFrame(dt);
        if (pauseOpen && !netOn && !netHost && !netConn) dt = 0.f;   // pausa congela a simulação (mundo, ticks, braço); com outro jogador conectado o jogo continua
        if (grab && !invOpen) {   // segurar o botão repete a ação
            Uint32 mb = gpMouseState();
            if (touchHud) mb = touchBreak ? SDL_BUTTON(1) : 0;   // touch: só "segurar parado" quebra
            const bool hold = (mb & SDL_BUTTON(3)) || (creative && (mb & SDL_BUTTON(1)));   // em sobrevivência o botão esquerdo é tratado pela mineração abaixo
            if (hold) { rep += dt; if (rep > .3f) { rep = .2f; act((mb & SDL_BUTTON(1)) ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT, true); } } else rep = 0;
            // sobrevivência: segurar o botão esquerdo minera (tempo conforme dureza e ferramenta)
            bool mining = false;
            mnCool = std::max(0.f, mnCool - dt);
            if (!creative && gGm != 2 && !gDead && !chatOpen && (mb & SDL_BUTTON(1)) && has) {   // aventura não quebra blocos
                const int mb2 = getB(hh[0], hh[1], hh[2]);
                if (mb2 != AIR && mb2 != BEDROCK && !(mb2 == XB_PORTAL && !creative) && !isWater(mb2)) {
                    mining = true;
                    if (!mnOn || mnX != hh[0] || mnY != hh[1] || mnZ != hh[2] || mnHeld != inv[sel].id) {
                        mnOn = true; mnX = hh[0]; mnY = hh[1]; mnZ = hh[2]; mnHeld = inv[sel].id; mnT = 0;
                        mnNeed = breakTime(mb2, mnHeld, mnHv);
                    }
                    // braço: como o swingItem() a cada tick do 1.8.9, o balanço reinicia a cada 4 ticks (0,2 s) e só vai até ~metade
                    mnSw += dt;
                    if (!swinging || mnSw >= .2f) { swinging = true; swing = 0; mnSw = 0; }   // sem travar o balanço no meio (só reinicia a cada 0,2 s)
                    for (mnPt += dt; mnPt >= .05f; mnPt -= .05f) spawnHit(hh[0], hh[1], hh[2], pp[0] - hh[0], pp[1] - hh[1], pp[2] - hh[2], mb2);   // partículas na face batida (1/tick)
                    if (mnCool <= 0.f) {
                        mnT += dt;
                        if (mnT >= mnNeed) { breakAt(mnX, mnY, mnZ, mnHv); mnT = 0; mnCool = .25f; mnOn = false; }   // mining continua true: o timer do braço não zera entre blocos
                    }
                }
            }
            if (!mining) { mnOn = false; mnT = 0; mnSw = 0; mnPt = 0; }
        } else { mnOn = false; mnT = 0; mnSw = 0; mnPt = 0; }
        if (touchHud && !creative && !touchUse) touchBrkProg = (touchBreak && mnOn && mnNeed > 0) ? std::min(1.f, mnT / mnNeed) : 0.f;   // círculo enche conforme minera
        if (swinging) { swing += dt / .3f; if (swing >= 1) { swing = 0; swinging = false; } }
        int held = inv[sel].id;
        netUpdate(dt);
        if (held != lastHeld) { lastHeld = held; nameT = 2.f; }
        {   // ItemRenderer.updateEquippedItem (1.8.9): o item antigo desce (0,4/tick), troca ao passar de 0,1 e o novo sobe
            const float tgt = (held == handItem) ? 1.f : 0.f, step = 8.f * dt;
            equip += std::max(-step, std::min(step, tgt - equip));
            if (equip < .1f) handItem = held;
        }
        nameT = std::max(0.f, nameT - dt); msgT = std::max(0.f, msgT - dt);

        // ======================= TICK (20 por segundo, como o Java) =======================
        tickAcc += dt;
        int ticksThisFrame = 0;
        while (tickAcc >= 0.05 && ticksThisFrame < 5) {
            tickAcc -= 0.05; ++ticksThisFrame;
            if (gDead && !creative && !deadFree) { deadFree = true; if (invOpen) setInv(false); if (chatOpen) setChat(false, "", ""); grab = false; relMouse(false); int ww, wh; SDL_GetWindowSize(win, &ww, &wh); mouseX = ww / 2; mouseY = wh / 2; SDL_WarpMouseInWindow(win, mouseX, mouseY); }   // solta o mouse para clicar nos botões
            if (gWantRespawn) { survRespawn(P); if (deadFree) { deadFree = false; if (!pauseOpen && !chatOpen && !invOpen) { grab = true; relMouse(true); } } }
            handBobTick(yaw, pitch);
            tickMobs(P);
            tickDrops(P.x, P.y, P.z);
            if (gSplit) split2Tick(P, grab && !invOpen && !chatOpen && !pauseOpen);
            if (gP2ChatReq) { gP2ChatReq = false; if (gSplit && !chatOpen && !invOpen && !pauseOpen) { gChatAs2 = true; setChat(true, "", ""); } }   // GA_CHAT do controle do jogador 2   // jogador 2 (splitscreen.inc)
            runWorldTick();   // ticks agendados de bloco (blockupdate.inc)
            netherTick(P); if (gNetherTurn) { gNetherTurn = false; yaw = gNetherYaw; pitch = gNetherPitch; }    // fogo queima o jogador; portal do Nether (nether.inc)
            modulesTick(P);
            netSendState(P.x, P.y, P.z, yaw, pitch, inv[sel].id, swinging ? swing : 0.f, P.sneak, (gGm != 3 && isElytraItem(armor[1].id) ? 1 : 0) | (gGm != 3 && isElytraItem(armor[1].id) && P.elytra && !P.onGround ? 2 : 0)); netTick();
            // Entity.onEntityUpdate: guarda o estado anterior
            P.px = P.x; P.py = P.y; P.pz = P.z;
            P.prevDistWalked = P.distWalked;
            sndSyncVol(); sndTick(P.x, P.y + 1.62, P.z, yaw, P.distWalked, P.onGround && !P.sneak && !P.flying && !P.inWater && !P.noclip);   // ouvinte + passos + musica
            prevBodyYaw = bodyYaw; prevLimbAmt = limbAmt;
            prevCamBobYaw = camBobYaw; prevCamBobPitch = camBobPitch;
            prevFovMod = fovMod;
            ageTicks += 1.f;
            if (netOn && netHost && ++gTimeSyncCtr >= 20) { gTimeSyncCtr = 0; netSendTime(false); }   // hora do mundo para todos, 1x/s
            if (gDayCycle) ++gWorldTime;
            wxTick(P.x, P.y, P.z);   // patchWx.1   // relógio do mundo: 1 por tick (20/s), 24000 = 1 dia; para junto com o menu de pausa (dt = 0)
            if (sprintToggleTimer > 0) --sprintToggleTimer;
            P.inWater = inWaterNow(P); P.inLava = inLavaNow(P);
            if (P.inWater && !P.flying) waterPush(P);   // correnteza (Entity.handleWaterMovement)

            // MovementInputFromOptions
            const bool sneakKey = grab && (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT] || tvSneakT || gi1.sneak);
            const bool jumpKey = grab && !gDead && (k[SDL_SCANCODE_SPACE] || tvJump || gi1.jump);
            float moveForward = 0.f, moveStrafe = 0.f;   // strafe > 0 = direita
            if (grab && !gDead) {
                if (k[SDL_SCANCODE_W] || tvF) moveForward += 1.f;
                if (k[SDL_SCANCODE_S] || tvB) moveForward -= 1.f;
                if (k[SDL_SCANCODE_D] || tvR) moveStrafe += 1.f;
                if (k[SDL_SCANCODE_A] || tvL) moveStrafe -= 1.f;
                moveForward = std::max(-1.f, std::min(1.f, moveForward + gi1.fwd)); moveStrafe = std::max(-1.f, std::min(1.f, moveStrafe + gi1.str));   // stick analogico
            }
            if (sneakKey) { moveStrafe *= .3f; moveForward *= .3f; }
            if (gBowOn || gEatOn) { moveStrafe *= .2f; moveForward *= .2f; }   // carregando o arco: 20% da velocidade
            P.sneak = sneakKey;
            // Voo criativo: dois toques rapidos em ESPACO (7 ticks) ligam/desligam; Espaco sobe, Shift desce
            if (flyTimer > 0) --flyTimer;
            if (creative && !lastJumpKey && jumpKey) { if (flyTimer == 0) flyTimer = 7; else { P.flying = !P.flying; flyTimer = 0; } }
            lastJumpKey = jumpKey;
            if (!creative) P.flying = false;
            if (gGm == 3) P.flying = true;
            P.noclip = (gGm == 3);   // espectador atravessa blocos
            if (P.flying) { if (sneakKey) P.my -= .15; if (jumpKey) P.my += .15; }

            // EntityPlayerSP.onLivingUpdate: início/fim do sprint (Ctrl segurado ou toque duplo em W)
            const bool sprintKey = grab && (k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL] || tvSprint || gGpSprint);
            const float F08 = .8f;
            const bool canSprintFood = creative || P.flying || gFood > 6;   // Java: fome > 6 (mais de 3 pernis) ou pode voar
            if (P.onGround && !lastSneakKey && !lastFwdOk && moveForward >= F08 && !P.sprint && canSprintFood) {
                if (sprintToggleTimer <= 0 && !sprintKey) sprintToggleTimer = 7; else P.sprint = true;
            }
            if (!P.sprint && moveForward >= F08 && sprintKey && canSprintFood) P.sprint = true;
            if (P.sprint && (moveForward < F08 || P.collH || !canSprintFood)) P.sprint = false;
            lastSneakKey = sneakKey; lastFwdOk = moveForward >= F08;

            // EntityLivingBase.onLivingUpdate
            if (jumpTicks > 0) --jumpTicks;
            if (fabs(P.mx) < .005) P.mx = 0;
            if (fabs(P.my) < .005) P.my = 0;
            if (fabs(P.mz) < .005) P.mz = 0;
            if (jumpKey) {
                if (P.inWater || P.inLava) P.my += .04;
                else if (P.onGround && jumpTicks == 0) {
                    P.my = .42;                                  // jump()
                    survExh(P.sprint ? .8f : .2f);
                    if (P.sprint) { P.mx += -sy * .2; P.mz += -cy * .2; }   // impulso do pulo correndo
                    jumpTicks = 10;
                }
            } else jumpTicks = 0;
            moveStrafe *= .98f; moveForward *= .98f;
            // escada de mao (EntityLivingBase.moveEntityWithHeading): limita a velocidade, zera a queda; batendo na parede sobe a 0,2
            elytraBefore(P, jumpKey, yaw, pitch);
            const bool onLadder = !P.flying && climbBlk(getB((int)floor(P.x), (int)floor(P.y), (int)floor(P.z)));
            if (onLadder) { P.mx = std::max(-.15, std::min(.15, P.mx)); P.mz = std::max(-.15, std::min(.15, P.mz)); P.my = std::max(P.my, -.15); if (sneakKey && P.my < 0) P.my = 0; gFall = 0.f; }
            const double myPre = P.my;
            if (gTpHold > 0) { P.mx = P.my = P.mz = 0; gFall = 0.f; } else { unstuckPlayer(P); moveEntityWithHeading(P, yaw, moveStrafe, moveForward); }   // /tp: parado ate o terreno de destino carregar
            { static int slimeId = -1; if (slimeId < 0) { auto sl = gPkByFile.find("block/slime_block"); slimeId = sl != gPkByFile.end() ? sl->second : 0; }   // bloco de slime: quica (Block.onLanded) e anula a queda
              if (slimeId && myPre < -.1 && !sneakKey && P.onGround && getB((int)floor(P.x), (int)floor(P.y - .01), (int)floor(P.z)) == slimeId) { P.my = -myPre; P.onGround = false; gFall = 0.f; } }
            if (onLadder && P.collH) P.my = (.2 - .08) * .98;   // 0,2 e a gravidade do mesmo tick
            survTick(P);
            elytraAfter(P);
            gBowSlot = &inv[sel]; bowTick(P, grab && !invOpen && !chatOpen && !pauseOpen, inv[sel].id, yaw, pitch); eatTick(P, grab && !invOpen && !chatOpen && !pauseOpen, inv[sel]); arrowsTick(P); eggsTick(P); pearlApply(P); tntTick(P);
            if (P.onGround && P.flying) P.flying = false;

            // EntityPlayer.onLivingUpdate: balanço de câmera (view bobbing)
            {
                float f = (float)sqrt(P.mx * P.mx + P.mz * P.mz);
                float f1 = (float)(atan(-P.my * .2) * 15.0);
                if (f > .1f) f = .1f;
                if (!P.onGround) f = 0.f;
                if (P.onGround) f1 = 0.f;
                camBobYaw += (f - camBobYaw) * .4f;
                camBobPitch += (f1 - camBobPitch) * .8f;
            }

            // EntityLivingBase.onUpdate: corpo (renderYawOffset), cabeça e membros
            {
                const double d0 = P.x - P.px, d1 = P.z - P.pz;
                const float dsq = (float)(d0 * d0 + d1 * d1);
                float f1 = bodyYaw;
                if (dsq > .0025000002f) f1 = (float)(atan2(-d0, -d1) * 57.29577951308232);   // direção do movimento
                if (swinging) f1 = yaw;                                                         // atacando: corpo acompanha a mira
                // func_110146_f
                float f = wrap180(f1 - bodyYaw);
                bodyYaw += f * .3f;
                float h = wrap180(yaw - bodyYaw);
                if (h < -75.f) h = -75.f;
                if (h >= 75.f) h = 75.f;
                bodyYaw = yaw - h;
                if (h * h > 2500.f) bodyYaw += h * .2f;
                // evita saltos de 360 na interpolação
                while (bodyYaw - prevBodyYaw < -180.f) prevBodyYaw -= 360.f;
                while (bodyYaw - prevBodyYaw >= 180.f) prevBodyYaw += 360.f;

                float f6 = (float)sqrt(d0 * d0 + d1 * d1) * 4.f;
                if (f6 > 1.f) f6 = 1.f;
                limbAmt += (f6 - limbAmt) * .4f;
                limbSwing += limbAmt;
            }

            // EntityRenderer.updateRenderer: FOV do jogador (correr = x1.15)
            {
                const float target = P.sprint ? 1.15f : 1.f;
                fovMod += (target - fovMod) * .5f;
                fovMod = std::max(.1f, std::min(1.5f, fovMod));
            }

            // ----- bote (boat.inc): física dos botes e jogador montado -----
            {
                const bool inp = grab && !gDead;
                const bool bL = inp && (k[SDL_SCANCODE_A] || tvL || gi1.str < -.5f), bR = inp && (k[SDL_SCANCODE_D] || tvR || gi1.str > .5f), bF = inp && (k[SDL_SCANCODE_W] || tvF || gi1.fwd > .5f), bB = inp && (k[SDL_SCANCODE_S] || tvB || gi1.fwd < -.5f);
                if (cartTickAll(P, bF, sneakKey, yaw)) { bodyYaw = yaw; limbAmt = 0.f; gFall = 0.f; }
                if (boatTickAll(P, bL, bR, bF, bB, sneakKey)) {
                    const BoatEnt* rb = boatById(gBoatRide);
                    const float bo = rb ? boatYawO(*rb) : yaw;
                    bodyYaw = bo; limbAmt = 0.f; gFall = 0.f;
                    while (bodyYaw - prevBodyYaw < -180.f) prevBodyYaw -= 360.f;
                    while (bodyYaw - prevBodyYaw >= 180.f) prevBodyYaw += 360.f;
                }
            }
        }
        if (ticksThisFrame == 5) tickAcc = 0.0;   // não acumula atraso se o frame travou
        const float pt = (float)(tickAcc / 0.05);   // partialTicks
        if (const BoatEnt* rb = boatById(gBoatRide)) {   // camera acompanha o giro do bote (EntityBoat.updatePassenger: yaw += deltaRotation) POR QUADRO, com o yaw interpolado do bote; cabeca presa a 105 graus
            const float bo = boatYawLerp(*rb, pt);
            if (gBoatCamId != gBoatRide) { gBoatCamId = gBoatRide; gBoatCamLast = bo; }
            yaw = wrap180(yaw + wrap180(bo - gBoatCamLast)); gBoatCamLast = bo;
            float d = wrap180(yaw - bo); d = std::max(-105.f, std::min(105.f, d)); yaw = wrap180(bo + d);
        } else gBoatCamId = 0;

        // ===== viewports (split screen): o bloco de desenho abaixo roda 1x por viewport; na 2a passada o contexto do jogador 2 entra no lugar do jogador 1 =====
        int fw = 0, fh = 0; SDL_GetWindowSize(win, &fw, &fh);
        const bool dual = gSplit && !invOpen;   // com inventario aberto: 1 passada em tela cheia (do dono do inventario), para o mouse/cursor bater com o layout
        const int vpBeg = (gSplit && invOpen && gInvOwner == 2) ? 1 : 0, vpEnd = dual ? 2 : vpBeg + 1;
        DbgInfo dbgD; bool dbgHave = false;
        int s2Cam = 0, s2Boat = 0, s2Cart = 0, s2Hand = 0; float s2Eq = 1.f, s2Z[6] = {0, 0, 0, 0, 0, 0}, s2Fov = 1.f, s2Nm = 0.f, s2Mm = 0.f;
        auto swapView = [&]() {   // troca o contexto de VISAO entre jogador 1 e jogador 2 (chamado 2x: entra e sai)
            std::swap(P, S2.P); std::swap(yaw, S2.yaw); std::swap(pitch, S2.pitch); std::swap(sel, S2.sel); std::swap(bodyYaw, S2.bodyYaw); std::swap(prevBodyYaw, S2.prevBodyYaw);
            std::swap(limbSwing, S2.limbSwing); std::swap(limbAmt, S2.limbAmt); std::swap(prevLimbAmt, S2.prevLimbAmt); std::swap(swing, S2.swing); std::swap(swinging, S2.swinging);
            std::swap(mnOn, S2.mnOn); std::swap(mnT, S2.mnT); std::swap(mnNeed, S2.mnNeed); std::swap(mnX, S2.mnX); std::swap(mnY, S2.mnY); std::swap(mnZ, S2.mnZ);
            split2SwapInv(); if (S2.skinTex) std::swap(skinTex, S2.skinTex);
            split2SwapSurv();   // vida, fome, fogo, comer, arco do jogador 2 (HUD e maos dele)
            std::swap(camMode, s2Cam); std::swap(gBoatRide, gBoat2); std::swap(gCartRide, s2Cart); std::swap(handItem, s2Hand); std::swap(equip, s2Eq);
            std::swap(camBobYaw, s2Z[0]); std::swap(prevCamBobYaw, s2Z[1]); std::swap(camBobPitch, s2Z[2]); std::swap(prevCamBobPitch, s2Z[3]); std::swap(fovMod, s2Fov); std::swap(prevFovMod, s2Fov);
            std::swap(nameT, s2Nm); std::swap(msgT, s2Mm);
        };
        for (int vp = vpBeg; vp < vpEnd; vp++) {
        int vx = 0, vy = 0, w = fw, h = fh; if (dual) split2Rect(vp, fw, fh, vx, vy, w, h);
        gVpX = vx; gVpY = vy;
        if (vp == 1) { swapView(); held = inv[sel].id; handItem = held; equip = 1.f; }
        // ----- estado interpolado para renderização (igual ao Java: prev + (cur - prev) * partialTicks) -----
        const double rX = P.px + (P.x - P.px) * pt, rY = P.py + (P.y - P.py) * pt, rZ = P.pz + (P.z - P.pz) * pt;
        const float eyeH = P.sneak ? 1.54f : 1.62f;   // 1.8.9: agachar baixa o olho em 0.08
        const double eyeX = rX, eyeY = rY + eyeH, eyeZ = rZ;
        viewEye[0] = eyeX; viewEye[1] = eyeY; viewEye[2] = eyeZ;
        float renderBodyYaw = prevBodyYaw + (bodyYaw - prevBodyYaw) * pt;
        float renderYaw = yaw, renderPitch = pitch; gYawNow = yaw;
        const float renderLimbAmt = std::min(1.f, prevLimbAmt + (limbAmt - prevLimbAmt) * pt);
        const float renderLimb = limbSwing - limbAmt * (1.f - pt);
        const float fovHand = prevFovMod + (fovMod - prevFovMod) * pt;

        if (vp == 1) {   // mira do jogador 2 (o raio usa os olhos/angulos dele; o jogador 1 ja foi calculado no topo do quadro)
            eye[0] = (float)eyeX; eye[1] = (float)eyeY; eye[2] = (float)eyeZ; const float c2 = cosf(yaw * .0174533f), s2 = sinf(yaw * .0174533f), cp2 = cosf(pitch * .0174533f);
            dir[0] = -s2 * cp2; dir[1] = sinf(pitch * .0174533f); dir[2] = -c2 * cp2; has = ray(eye, dir, hh, pp); }
        gHeldSel = sel; gPlayerYaw = yaw;
        if (vp == vpBeg) {   // simulacao do mundo: so 1x por quadro (nao por viewport)
        for (Slot& ts : throwQ) throwItem(viewEye, yaw, pitch, ts.id, ts.n, ts.nbt);
        throwQ.clear();
        RAD = rdChunks();   // menu Video > Renderizacao
#ifdef __ANDROID__
        RAD = std::max(3, RAD - gAdaptCut);   // ajuste automatico de FPS (perf.inc)
#endif
        {   // orcamento ADAPTATIVO: usa a folga do quadro (alvo ~55 fps) em vez de 3 ms fixos => chunks carregam bem mais rapido quando ha folga
            // a espera da GPU (swap) e tempo ocioso da CPU: quando a GPU e o gargalo, carregar chunks ali nao custa FPS
            #ifdef __ANDROID__
            const double sBudget = std::max(1.5, std::min(4.0, 16.0 - gPfOtherMs));   // celular: no maximo 4 ms/quadro gerando/malhando chunks (antes ate 14 ms = picos de FPS)
#else
            const double sBudget = std::max(2.5, std::min(14.0, 16.0 - gPfOtherMs));
#endif
            gStreamBudgetMs = sBudget;   // alvo fixo ~60 fps (antes 0,9x o quadro: o orcamento crescia com o lag e prendia o jogo em ~40 fps)
#ifdef BL_RPI
            const int sFar = 3;   // Raspberry: a partir de 3 chunks a malha dispensa plantas e faces enterradas (menos vertices)
#else
            const int sFar = RAD <= 8 ? (1 << 20) : std::max(7, (int)(fogStartD() / 16.f * .6f));   // raio grande: longe da camera a malha dispensa plantas (a nevoa as esconde)
#endif
            { static int tpx = 0x7fffffff, tpz = 0, trad = 0; static Uint32 tt = 0; const int ccx = fdiv((int)floorf((float)P.x), CW), ccz = fdiv((int)floorf((float)P.z), CW), tn = (int)SDL_GetTicks();
              if (ccx != tpx || ccz != tpz || trad != RAD || tn - (int)tt > 2000) { tpx = ccx; tpz = ccz; trad = RAD; tt = (Uint32)tn; gTrimX2 = (gSplit && S2.init) ? fdiv((int)floor(S2.P.x), CW) : 0x7fffffff; gTrimZ2 = (gSplit && S2.init) ? fdiv((int)floor(S2.P.z), CW) : 0; gpuTrim(ccx, ccz); } }
            #ifdef __ANDROID__
            int sMaxGen = 3, sMaxMesh = 3; double sBud = sBudget;   // celular: poucos chunks por quadro (carrega um pouco mais devagar, sem engasgar)
#else
            int sMaxGen = 32, sMaxMesh = 32; double sBud = sBudget;
#endif
#ifdef __ANDROID__
            {   // celular: no maximo 1 chunk gerado e 1 malha por quadro (um chunk custa dezenas de ms na CPU do celular); se o quadro ja esta lento, so carrega a cada 3 quadros
                static int slowSkip = 0; const bool near0 = getC(fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW)) != nullptr && W.size() > 30;
                sMaxGen = 1; sMaxMesh = 1; sBud = std::min(sBud, 6.0);
                if (near0 && gPfFrameMs > 22.0 && (++slowSkip % 3) != 0) { sMaxGen = 0; sMaxMesh = 0; }
            }
#endif
            if (gTpHold > 0) sBud = std::max(sBud, 14.0);   // carregando terreno do /tp: orcamento maior, mas sempre limitado por quadro
            stream(fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW), sMaxGen, sMaxMesh, sBud, -sinf(yaw * .0174533f), -cosf(yaw * .0174533f), sFar);
            if (gSplit && S2.init) {   // jogador 2 longe do 1: carrega e malha o mundo em volta dele tambem (metade do orcamento)
                const int q2x = fdiv((int)floor(S2.P.x), CW), q2z = fdiv((int)floor(S2.P.z), CW), p1x = fdiv((int)floorf((float)P.x), CW), p1z = fdiv((int)floorf((float)P.z), CW);
                if (abs(q2x - p1x) > 2 || abs(q2z - p1z) > 2) stream(q2x, q2z, std::max(1, sMaxGen / 2), std::max(1, sMaxMesh / 2), sBud * .5, -sinf(S2.yaw * .0174533f), -cosf(S2.yaw * .0174533f), sFar); }
        }
        if (gTpHold > 0) { Chunk* hc = getC(fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW)); if ((hc && hc->lc && !hc->dirty) || ++gTpWait > 900) { gTpHold = 0; gKick = true; } }
        if (gNetTp.on) { if (gNetTp.dim >= 0 && gNetTp.dim != gDim) dimSwitch(gNetTp.dim); P.x = P.px = gNetTp.x; P.y = P.py = gNetTp.y; P.z = P.pz = gNetTp.z; P.mx = P.my = P.mz = 0; gTpHold = 1; gTpWait = 0; gNetTp.on = false; }   // /tp ou /puxar vindo do host
        if (gNetReseed) { Chunk* pc = getC(fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW));   // seed trocada pelo host: sobe o jogador para a superfície nova
            if (pc) { const int lx = (int)floorf((float)P.x) - fdiv((int)floorf((float)P.x), CW) * CW, lz = (int)floorf((float)P.z) - fdiv((int)floorf((float)P.z), CW) * CW;
                int y = CH - 1; while (y > 0 && (pc->b[idx(lx, y, lz)] == AIR || isWater(pc->b[idx(lx, y, lz)]))) y--;
                P.y = y + 1.0; P.py = P.y; P.my = 0; gNetReseed = false; } }   // orçamento ~3 ms/quadro: sem travadas ao gerar chunks
        { PerfScope perfSim(PS_SIM); simulateWater(dt, fdiv((int)floorf((float)P.x), CW), fdiv((int)floorf((float)P.z), CW)); tickLeaves(dt); }
        updateParts(dt);
        }   // fim: so 1x por quadro

        const bool headWater = isWater(getB((int)floor(eyeX), (int)floor(eyeY), (int)floor(eyeZ)));
        const bool headLava = isLava(getB((int)floor(eyeX), (int)floor(eyeY), (int)floor(eyeZ)));   // cabeca na lava: tela laranja, visao de ~2 blocos
        if (dual) glViewport(vx, vy, w, h); else blViewport(w, h);
        {   // ciclo dia/noite: cor do zênite, do horizonte/neblina e brilho da luz do céu deste quadro
            const float ly = (renderYaw + (camMode == 2 ? 180.f : 0.f)) * .0174533f, lpi = (camMode == 2 ? -renderPitch : renderPitch) * .0174533f;
            updateSky(pt, -sinf(ly) * cosf(lpi), skyTop, skyC, skyTopD, skyHorD); wxSkyApply(skyTopD, skyHorD);
            for (int q = 0; q < 3; q++) uwD[q] = uwC[q] * gSky.sunB;   // debaixo d'água: escurece com o brilho do sol
        }
        static const float lavaFogC[4] = {.60f, .10f, 0.f, 1.f};
        static const float netherFogC[4] = {.20f, .03f, .03f, 1.f}, endFogC[4] = {.094f, .075f, .094f, 1.f};   // End: 0xA080A0 * 0.15 (WorldProviderEnd.getFogColor)
        const float* sky = headLava ? lavaFogC : headWater ? uwD : gDim == 2 ? endFogC : gDim ? netherFogC : skyHorD;
        glStackHeal("inicio do quadro");
        glClearColor(sky[0], sky[1], sky[2], 1);
        glClearStencil(0); if (dual) { glEnable(GL_SCISSOR_TEST); glScissor(vx, vy, w, h); }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); if (dual) glDisable(GL_SCISSOR_TEST);
        float fov = (30.f + 80.f * gsf("fov", .5f)) * fovHand;   // slider Campo de Visao (30..110, padrao 70); correr aumenta
        if (headWater) fov = fov * 60.f / 70.f;     // dentro d'água o Java reduz o FOV
        lastFov = fov;
        float n = (RAD > 8 && !headWater) ? .09f : .05f, t = n * tanf(fov * .5f * .0174533f), r = t * w / (float)h;   // patchShader.2: render alto = near maior (menos z-fighting longe)
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glFrustum(-r, r, -t, t, n, 700);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        applyViewBob(P.distWalked, P.prevDistWalked, camBobYaw, prevCamBobYaw, camBobPitch, prevCamBobPitch, pt);
        // Câmera (EntityRenderer.orientCamera do 1.8.9):
        //  1ª pessoa: nos olhos.
        //  F5 (modo 1): ATRÁS do jogador, a 4 blocos ao longo de -olhar.
        //  F5 duplo (modo 2): NA FRENTE do jogador, olhando de volta (yaw+180, pitch invertido).
        //  A distância é limitada por 8 raycasts (±0.1) do pivô dos olhos até a posição desejada,
        //  na MESMA direção em que a câmera vai ser posicionada.
        double cam[3] = {eyeX, eyeY, eyeZ};
        float camYaw = renderYaw, camPitch = renderPitch;
        if (camMode) {
            const float radYaw = renderYaw * .01745329252f, radPitch = renderPitch * .01745329252f;
            const float lookX = -sinf(radYaw) * cosf(radPitch), lookY = sinf(radPitch), lookZ = -cosf(radYaw) * cosf(radPitch);
            // voo de elitra: o corpo deita ao redor dos pes, entao o pivo da camera passa a ser o centro do corpo deitado (nao os olhos), um pouco mais longe
            double pvX = eyeX, pvY = eyeY, pvZ = eyeZ; float camDist = thirdPersonDistance;
            if (P.elytra && !P.onGround && isElytraItem(armor[1].id) && gGm != 3) {
                const float ft = (float)P.elyTicks + pt, f1 = std::min(1.f, ft * ft / 100.f);
                pvX = rX + lookX * .9f * f1; pvY = rY + .9f * (1.f - f1) + lookY * .9f * f1 + .55f * f1; pvZ = rZ + lookZ * .9f * f1;
                camDist = thirdPersonDistance + 1.5f * f1;
            }
            const float camSign = camMode == 1 ? -1.f : 1.f;   // direção do pivô até a câmera
            const float toX = camSign * lookX, toY = camSign * lookY, toZ = camSign * lookZ;
            float d3 = camDist;
            for (int i = 0; i < 8; ++i) {
                const float ox8 = ((i & 1) * 2 - 1) * .1f, oy8 = (((i >> 1) & 1) * 2 - 1) * .1f, oz8 = (((i >> 2) & 1) * 2 - 1) * .1f;
                float ro[3] = {(float)pvX + ox8, (float)pvY + oy8, (float)pvZ + oz8};
                float rd[3] = {toX, toY, toZ};
                // escada (placa fina) nao bloqueia a camera: antes, ao subir/descer a escada a celula do proprio jogador contava como parede (distancia 0 = camera colada)
                float tHit = -1.f, t0 = 0.f;
                for (int sk = 0; sk < 6; ++sk) {
                    const float so[3] = {ro[0] + rd[0] * t0, ro[1] + rd[1] * t0, ro[2] + rd[2] * t0};
                    const float th = raySolidDistance(so, rd, camDist - t0);
                    if (th < 0.f) break;
                    const float hit = t0 + th;
                    const float px = so[0] + rd[0] * (th + .01f), py = so[1] + rd[1] * (th + .01f), pz = so[2] + rd[2] * (th + .01f);
                    const bool ladCell = getB((int)floorf(th <= 0.f ? so[0] : px), (int)floorf(th <= 0.f ? so[1] : py), (int)floorf(th <= 0.f ? so[2] : pz)) == XB_LADDER;
                    if (!ladCell) { tHit = hit; break; }
                    t0 = hit + (th <= 0.f ? .5f : .5f);   // pula a celula da escada e continua
                    if (t0 >= camDist) break;
                }
                if (tHit >= 0.f) {   // distância do ponto de impacto até o pivô (não até o canto deslocado)
                    float hx = ro[0] + rd[0] * tHit - (float)pvX, hy = ro[1] + rd[1] * tHit - (float)pvY, hz = ro[2] + rd[2] * tHit - (float)pvZ;
                    d3 = std::min(d3, sqrtf(hx * hx + hy * hy + hz * hz));
                }
            }
            cam[0] = pvX + toX * d3; cam[1] = pvY + toY * d3; cam[2] = pvZ + toZ * d3;
            if (camMode == 2) { camYaw = renderYaw + 180.f; camPitch = -renderPitch; }
        }
        const double shEc[3] = {rX, rY + 1.0, rZ};
#ifdef __ANDROID__
        { static bool tried = false; if (!tried && !headWater) { tried = true; if (gsi("shd.mob", 0) == 1 && !gShadowsOn) shadowToggle(say); } }   // celular: religa as sombras se o jogador as deixou ligadas da ultima vez (padrao: desligadas)
#endif
        shadowFrame(cam, headWater || headLava || gDim, shEc, [&](float R) {   // sombras (F7): terreno no mapa do sol; jogador + mobs no mapa das entidades (todo quadro)
            for (const MobEnt& e : MOBS) { if (fabs(e.x - rX) > R + 3.0 || fabs(e.z - rZ) > R + 3.0) continue; drawMob(e, pt); }
            Pose sp; sp.headYaw = wrap180(renderYaw - renderBodyYaw); sp.headPitch = renderPitch; sp.limb = renderLimb; sp.amp = renderLimbAmt;
            sp.atk = swinging ? swing : 0; sp.held = held; sp.held2 = gGm != 3 ? offh.id : 0; sp.bow = gBowOn ? (float)gBowT : -1.f; sp.eat = gEatOn ? (float)gEatT : -1.f; sp.age = ageTicks + pt; sp.sneak = P.sneak && !gBoatRide && !gCartRide; sp.sit = gBoatRide != 0 || gCartRide != 0; wornFromInv(sp); elytraPoseFill(sp, P, pt, renderYaw, renderPitch);
            glPushMatrix(); if (const BoatEnt* rbt = boatById(gBoatRide)) { const double sp[3] = {rX, rY, rZ}; boatRiderBegin(*rbt, pt, sp); } else glTranslatef((float)rX, (float)rY, (float)rZ); glRotatef(renderBodyYaw, 0, 1, 0); localDeathPose(pt); drawPlayer(sp); glPopMatrix();   // tambem em 1a pessoa: a sombra do corpo aparece no chao
            netDraw(pt);
        });
        if (!creative && gHurt > 10 && !gDead) { float f = (gHurt - 10 - pt) / 10.f; if (f > 0.f) { f = sinf(f * f * f * f * 3.14159265f); glRotatef(-gHurtAtYaw, 0, 1, 0); glRotatef(-f * 14.f, 0, 0, 1); glRotatef(gHurtAtYaw, 0, 1, 0); } }   // 1.8.9 hurtCameraEffect: a camera inclina para o lado do golpe
        if (gDead && !creative) glRotatef(40.f - 8000.f / (gDeadT + pt + 200.f), 0, 0, 1);   // 1.8.9 hurtCameraEffect: camera tomba de lado ao morrer
        glRotatef(-camPitch, 1, 0, 0); glRotatef(-camYaw, 0, 1, 0); godCaptureCam();
        if (!headWater && !headLava && !gDim) drawSkyFull(skyTopD, skyHorD);   // domo + faixa do nascer/pôr + sol + lua + estrelas
        glTranslatef(-(float)cam[0], -(float)cam[1], -(float)cam[2]);
        glGetFloatv(GL_MODELVIEW_MATRIX, gViewM); gCamW[0] = (float)cam[0]; gCamW[1] = (float)cam[1]; gCamW[2] = (float)cam[2]; gSkyCamW[0] = gCamW[0]; gSkyCamW[1] = gCamW[1]; gSkyCamW[2] = gCamW[2];
        { const double dEye[3] = {viewEye[0], viewEye[1], viewEye[2]}; dynLightUpdate(dEye, dir, sel, cam[0], cam[1], cam[2]); }   // luz dinamica (tocha na mao / no chao)
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
        glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.1f);   // (0,1 como o MC: com mipmap, 0,02 engordava as folhas ao longe) o HUD desliga o alpha test: religa para folhas/grama
        glEnable(GL_FOG); glHint(GL_FOG_HINT, GL_NICEST);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, (headWater || headLava) ? 0.f : fogStartD()); glFogf(GL_FOG_END, headLava ? 2.5f : headWater ? 24.f : gDim == 1 ? std::min(fogEndD(), 110.f) : fogEndD());
        glFogfv(GL_FOG_COLOR, sky);
        glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D);
        glColor3f(1, 1, 1);
        glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        int pcx = fdiv((int)floor(cam[0]), CW), pcz = fdiv((int)floor(cam[2]), CW);
        const float fxv = -sy, fzv = -cy;
        struct FadeC { Chunk* c; float f, x0, z0; }; std::vector<FadeC> fadeList;
        const float nowS = SDL_GetTicks() * .001f;
        const float cullT = tanf(fov * .5f * .0174533f) * (w / (float)h) *
#ifdef BL_RPI
            1.0f   // Raspberry: culling de visao sem folga extra
#else
            1.08f
#endif
            , cullK = sqrtf(1.f + cullT * cullT), cullFar = headWater ? 24.f : fogEndD();
        // Cache dos chunks visiveis (circulo de raio RAD, ordenado de perto p/ longe): so e refeito quando a camera troca de chunk ou o conjunto de
        // chunks muda. Antes eram 2 passes x (2R+1)^2 buscas no hash por quadro. NAO ha corte por angulo de visao: girar a camera nunca faz chunk sumir.
        static std::vector<StreamOff> discOffs; static int discR = -1;
        static std::vector<Chunk*> viewC; static std::vector<float> viewX, viewZ; static int viewPcx = 0x7fffffff, viewPcz = 0, viewR = -1; static unsigned viewEp = 0;
        if (discR != RAD) {
            discOffs.clear(); const float rm = RAD + .5f;
            for (int dz = -RAD - 1; dz <= RAD + 1; dz++) for (int dx = -RAD - 1; dx <= RAD + 1; dx++) { const int d2 = dx * dx + dz * dz; if ((float)d2 <= rm * rm) discOffs.push_back({dx, dz, d2, true, false}); }
            std::sort(discOffs.begin(), discOffs.end(), [](const StreamOff& a, const StreamOff& b) { return a.d2 < b.d2; }); discR = RAD;
        }
        if (viewPcx != pcx || viewPcz != pcz || viewR != RAD || viewEp != gEpoch) {
            viewC.clear(); viewX.clear(); viewZ.clear();
            for (const StreamOff& o : discOffs) if (Chunk* c = getC(pcx + o.dx, pcz + o.dz)) { viewC.push_back(c); viewX.push_back((pcx + o.dx) * CW + 8.f); viewZ.push_back((pcz + o.dz) * CW + 8.f); }
            viewPcx = pcx; viewPcz = pcz; viewR = RAD; viewEp = gEpoch;
        }
#ifdef BL_RPI
        const float kChM = 0.f, kSeM = 0.f, kSeH = 16.f;   // Raspberry: culling exato (sem folga) por chunk e por fatia vertical de 16 blocos
#else
        const float kChM = 4.f, kSeM = 3.f, kSeH = 18.f;
#endif
        static const bool cullEnv = getenv("BLOCKLAND_NOCULL") == nullptr; gCullOn = cullEnv;   // culling de visao LIGADO por padrao (BLOCKLAND_NOCULL=1 desliga)
        Frustum frs; if (cullEnv) { float Pm[16], Mm[16]; glGetFloatv(GL_PROJECTION_MATRIX, Pm); glGetFloatv(GL_MODELVIEW_MATRIX, Mm); frustumFrom(frs, Pm, Mm); }
        // Desenho de um chunk. Com culling: so as secoes verticais de 16 blocos que tocam o frustum (a malha e ordenada por Y, entao secoes
        // visiveis vizinhas viram UMA chamada de desenho). A GPU deixa de processar vertices de subsolo/ceu que estao fora da tela.
        auto bindDraw = [&](Chunk* c, bool wpass, int cnt, float x0, float z0) {
            const uint32_t* off = wpass ? c->wOff : c->sOff;
            const bool secs = cullEnv && c->secOk && off[16] == (uint32_t)cnt;
            if (useVBO) {
                if (c->gl || c->glw) uploadChunk(c);
                pBindBuffer(GL_ARRAY_BUFFER, wpass ? c->wvbo : c->vbo); bindQuadIB((size_t)cnt / 4);
            }
            auto range = [&](int a, int b) {   // vertices [a, b) da malha
                if (a < 0 || b <= a) return;   // a<0: nenhuma secao pendente (rs=-1); antes desenhava a partir do vertice -1 e embaralhava cor/posicao
                if (useVBO) {
                    const size_t o = (size_t)a * sizeof(V);
                    glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, x))); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, u))); glColorPointer(4, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, r)));
                    quadDrawVBO(o, b - a, true);
                } else {
                    std::vector<V>& mm = wpass ? c->wmesh : c->mesh; const V* v = mm.data();
                    glVertexPointer(3, GL_FLOAT, sizeof(V), &v->x); glTexCoordPointer(2, GL_FLOAT, sizeof(V), &v->u); glColorPointer(4, GL_FLOAT, sizeof(V), &v->r);
                    glDrawArrays(GL_QUADS, a, b - a);
                }
                gPfCalls++; gPfVerts += b - a;
            };
            if (!secs) { range(0, cnt); return; }
            // so as secoes verticais de 16 blocos que tocam o frustum; secoes visiveis vizinhas = uma chamada
            int rs = -1, re = 0;
            for (int sc = 0; sc < 16; sc++) {
                const int a = (int)off[sc], b = (int)off[sc + 1]; if (b <= a) continue;
                if (frustumOutside(frs, x0, sc * 16.f, z0, x0 + 16.f, sc * 16.f + kSeH, z0 + 16.f, kSeM)) { range(rs, re); rs = -1; continue; }
                if (rs >= 0 && re == a) re = b; else { range(rs, re); rs = a; re = b; }
            }
            range(rs, re);
        };
        auto drawChunks = [&](bool wpass) {
            fadeList.clear(); const float cfD = gRfPass ? std::min(cullFar, ANDROID_RF_FAR) : cullFar; const float lim = (cfD + 11.5f) * (cfD + 11.5f);   // reflexo da agua: so ate 64 blocos
            const size_t nView = viewC.size();
            for (size_t k = 0; k < nView; k++) {
                const size_t i = wpass ? nView - 1 - k : k;   // solidos: perto -> longe (early-z); agua translucida: longe -> perto (mistura correta)
                Chunk* c = viewC[i];
                if (c->born < 0.f) c->born = nowS;   // fade-in conta desde que o chunk existe (nao desde que entra no campo de visao: senao chunks atras/ao lado ressurgem ao girar)
                const int cnt = wpass ? c->nwv - c->ngv : c->nv;   // agua = parte inicial de wmesh; o vidro vem depois (drawGlass)
                if (cnt <= 0) continue;
                const float vx = viewX[i] - (float)cam[0], vz = viewZ[i] - (float)cam[2];
                if (vx * vx + vz * vz > lim) continue;   // so descarta o que a nevoa ja esconde
                if (cullEnv && frustumOutside(frs, viewX[i] - 8.f, 0.f, viewZ[i] - 8.f, viewX[i] + 8.f, (float)CH, viewZ[i] + 8.f, kChM)) continue;   // opcional: inteiramente fora da tela (margem 12 blocos)
                if (gRfPass && c->rborn < 0.f) c->rborn = nowS;   // patchRfl.1: o reflexo tem fade-in proprio (comeca quando o chunk aparece no reflexo, 1.6 s)
                float fade = gRfPass ? (nowS - std::max(c->born, c->rborn)) / 1.6f : (nowS - c->born) / 1.2f;
                if (fade < 1.f) {
                    fade = fade * fade * (3.f - 2.f * fade);
                    if (wpass) { if (fade < .6f) continue; }
                    else if (pBlendColor) { fadeList.push_back({c, fade, viewX[i] - 8.f, viewZ[i] - 8.f}); continue; }
                }
                if (!wpass) gPfDrawn++;
                bindDraw(c, wpass, cnt, viewX[i] - 8.f, viewZ[i] - 8.f);
            }
            if (!wpass && !fadeList.empty()) {   // chunks novos: mistura com o fundo (alpha constante crescente)
                glEnable(GL_BLEND); glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                for (auto& fc : fadeList) { pBlendColor(0.f, 0.f, 0.f, fc.f); gPfDrawn++; bindDraw(fc.c, false, fc.c->nv, fc.x0, fc.z0); }
                glDisable(GL_BLEND);
            }
        };
        auto drawGlass = [&]() {   // vidro colorido / gelo / slime: ultimos ngv vertices de wmesh; longe -> perto, blend, sem escrever profundidade
            const float lim = (cullFar + 11.5f) * (cullFar + 11.5f); const size_t nView = viewC.size();
            for (size_t k = 0; k < nView; k++) {
                const size_t i = nView - 1 - k; Chunk* c = viewC[i]; const int cnt = c->ngv; if (cnt <= 0) continue;
                const float vx = viewX[i] - (float)cam[0], vz = viewZ[i] - (float)cam[2]; if (vx * vx + vz * vz > lim) continue;
                if (cullEnv && frustumOutside(frs, viewX[i] - 8.f, 0.f, viewZ[i] - 8.f, viewX[i] + 8.f, (float)CH, viewZ[i] + 8.f, kChM)) continue;
                const int tot = c->nwv, base = tot - cnt;
                if (useVBO) {
                    if (c->gl || c->glw) uploadChunk(c);
                    pBindBuffer(GL_ARRAY_BUFFER, c->wvbo); bindQuadIB((size_t)tot / 4);
                    const size_t o = (size_t)base * sizeof(V);
                    glVertexPointer(3, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, x))); glTexCoordPointer(2, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, u))); glColorPointer(4, GL_FLOAT, sizeof(V), (const void*)(o + offsetof(V, r)));
                    quadDrawVBO(o, cnt, true);
                } else {
                    const V* v = c->wmesh.data() + base; glVertexPointer(3, GL_FLOAT, sizeof(V), &v->x); glTexCoordPointer(2, GL_FLOAT, sizeof(V), &v->u); glColorPointer(4, GL_FLOAT, sizeof(V), &v->r);
                    glDrawArrays(GL_QUADS, 0, cnt);
                }
                gPfCalls++; gPfVerts += cnt;
            }
        };
        gRfOn = false;
        if (!BL_LITE && gShaderPackOn && !headWater && !headLava && !gDim && gShadowShader) {   // reflexao planar da agua (HL2): mundo espelhado em meia resolucao
            float wy = 0.f;
            if (rfFindWater(cam[0], cam[1], cam[2], wy) && cam[1] > wy + 0.05) {
                GLint vp0[4]; glGetIntegerv(GL_VIEWPORT, vp0);
#ifdef __ANDROID__
                const int rdiv = 4;
#else
                const int rdiv = 2;
#endif
                const int rw = std::max(vp0[2] / rdiv, 64), rh = std::max(vp0[3] / rdiv, 64);
                if (rfEnsure(rw, rh)) {
                    gRfSW = vp0[2]; gRfSH = vp0[3];
                    static int rfFr = 0; bool rfSkip = false;
#ifdef __ANDROID__
                    rfSkip = (rfFr++ % 3) != 0;   // celular: refaz o reflexo a cada 3 quadros (reusa a textura anterior)
#endif
                    if (rfSkip) { pActTex(GL_TEXTURE0 + 6); glBindTexture(GL_TEXTURE_2D, gRfTex); pActTex(GL_TEXTURE0); gRfOn = true; } else {
                    glPushAttrib(GL_ALL_ATTRIB_BITS); glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);   // a passada do reflexo nao pode vazar estado para o mundo
                    pActTex(GL_TEXTURE0 + 6); glBindTexture(GL_TEXTURE_2D, 0); pActTex(GL_TEXTURE0);   // sem laco de feedback: a textura do reflexo nao fica ligada enquanto e o alvo
                    const bool fgW = glIsEnabled(GL_FOG), atW = glIsEnabled(GL_ALPHA_TEST);
                    pBindFB(GL_FRAMEBUFFER, gRfFbo); glViewport(0, 0, rw, rh);
                    glClearColor(sky[0], sky[1], sky[2], 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glMatrixMode(GL_MODELVIEW); glPushMatrix();
                    float Ms[16]; for (int q = 0; q < 16; q++) Ms[q] = gViewM[q]; Ms[12] = Ms[13] = Ms[14] = 0.f;
                    glLoadMatrixf(Ms); glScalef(1.f, -1.f, 1.f); if (!headWater && !headLava && !gDim) drawSkyFull(skyTopD, skyHorD);   // ceu/sol espelhados
                    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); glDepthFunc(GL_LEQUAL); glEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
                    if (fgW) glEnable(GL_FOG); if (atW) { glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.1f); }
                    glLoadMatrixf(gViewM); glTranslatef(0.f, wy, 0.f); glScalef(1.f, -1.f, 1.f); glTranslatef(0.f, -wy, 0.f);
                    const double pl[4] = {0.0, 1.0, 0.0, -(double)wy - 0.02}; glClipPlane(GL_CLIP_PLANE0, pl); glEnable(GL_CLIP_PLANE0);   // so o que esta acima da agua
                    if (cullEnv) { float Pm2[16], Mm2[16]; glGetFloatv(GL_PROJECTION_MATRIX, Pm2); glGetFloatv(GL_MODELVIEW_MATRIX, Mm2); frustumFrom(frs, Pm2, Mm2); }
                    const bool sdw = gShadowDraw; gShadowDraw = false; gRfPass = true;
                    skyShaderOn(0.f, gSkyScale, gSkyRB); drawChunks(false); skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
                    {   // o proprio personagem aparece no reflexo (tambem em 1a pessoa)
                        Pose p; p.headYaw = wrap180(renderYaw - renderBodyYaw); p.headPitch = renderPitch;
                        p.limb = renderLimb; p.amp = renderLimbAmt; p.atk = swinging ? swing : 0; p.held = held; p.held2 = gGm != 3 ? offh.id : 0; p.bow = gBowOn ? (float)gBowT : -1.f; p.eat = gEatOn ? (float)gEatT : -1.f;
                        p.age = ageTicks + pt; p.sneak = P.sneak && !gBoatRide && !gCartRide; p.sit = gBoatRide != 0 || gCartRide != 0; wornFromInv(p); elytraPoseFill(p, P, pt, renderYaw, renderPitch);
                        glPushMatrix(); if (const BoatEnt* rbt = boatById(gBoatRide)) { const double sp[3] = {rX, rY, rZ}; boatRiderBegin(*rbt, pt, sp); } else glTranslatef((float)rX, (float)rY, (float)rZ); glRotatef(renderBodyYaw, 0, 1, 0); localDeathPose(pt);
                        entLightAt(rX, rY + 1.0, rZ, gEntDim); entShaderOn(); p.hurt = (gHurt > 10 || (gDead && !creative)) ? 1.f : 0.f; drawPlayer(p); if (gFire > 0 && !creative && !gDead) drawFireWH(.45f, 1.8f);   // chamas no proprio modelo (3a pessoa/espelho)
                    skyShaderOff(); entDimReset(); glPopMatrix();   // patchShader.6: isto estava DENTRO do comentario acima (sem pop = pilha vazava 1 por quadro)
                        if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
                    }
                    gShadowDraw = sdw; gRfPass = false; glDisable(GL_CLIP_PLANE0);
                    glPopMatrix(); pBindFB(GL_FRAMEBUFFER, 0); glViewport(vp0[0], vp0[1], vp0[2], vp0[3]);
                    glPopClientAttrib(); glPopAttrib(); glMatrixMode(GL_MODELVIEW); pActTex(GL_TEXTURE0); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
                    if (cullEnv) { float Pm2[16], Mm2[16]; glGetFloatv(GL_PROJECTION_MATRIX, Pm2); glGetFloatv(GL_MODELVIEW_MATRIX, Mm2); frustumFrom(frs, Pm2, Mm2); }
                    pActTex(GL_TEXTURE0 + 6); glBindTexture(GL_TEXTURE_2D, gRfTex); pActTex(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTex);
                    if (!fgW) glDisable(GL_FOG); if (!atW) glDisable(GL_ALPHA_TEST); glClearColor(sky[0], sky[1], sky[2], 1);
                    gRfOn = true; glStackHeal("reflexo da agua");
                    }
                }
            }
        }
        torchWorldUpdate();   // tocha animada (mundo e item no chao)
        netherAnimUpdate();   // fogo e portal animados (nether.inc)
        compassUpdate(P.x, P.z, yaw);   // agulha da bussola (single e multiplayer: usa so posicao/yaw locais e o spawn fixo)
        skyShaderOn(0.f, gSkyScale, gSkyRB); { PerfDrawScope perfDs(PS_DRAW_SOLID); drawChunks(false); } gShadowDraw = false;
        if (!BOATS.empty() || !CARTS.empty() || !gSfPos.empty() || !gPAnim.empty()) {   // botes e carrinhos: antes da água (a placa "noWater" esconde a água dentro do casco); estado fixo, sem shader/arrays
            skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
            glDisable(GL_BLEND); drawBoats(pt); drawCarts(pt); glBindTexture(GL_TEXTURE_2D, atlasTex); drawSunflowers(pt, P.x, P.z); drawPistonAnims(); drawFalling(pt);   // bote/carrinho deixam a textura deles ligada: o girassol usa o atlas
            glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.1f); glColor4f(1.f, 1.f, 1.f, 1.f);   // drawBoats deixava a textura do bote ligada e o alpha test desligado: a água era desenhada com a textura errada
            glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        }
        {   // entidades opacas ANTES da agua/vidro/portal (eles nao escrevem profundidade: se viessem depois, cobriam a transparencia com a cor cheia)
            skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
            glDisable(GL_BLEND); glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D);
            torchFootShadow(rX, rY, rZ);
            { int nsh = 0; for (const MobEnt& me : MOBS) { if (me.dead || nsh >= 6) continue; const double ddx = me.x - rX, ddz = me.z - rZ; if (ddx * ddx + ddz * ddz > 400.0) continue; torchFootShadow(me.x, me.y, me.z, std::max(.5f, MD[me.type].w / .6f), MD[me.type].h); nsh++; } }   // patchEmber.1: mobs (6 mais proximos, ate 20 blocos)
            if (gDim == 1 && gNetherFx) netherEmbers(rX, rY, rZ, SDL_GetTicks() * .001f);
            wxDraw(rX, rY, rZ, SDL_GetTicks() * .001f);   // patchWx.1
            drawDrops(pt); drawPickups(pt, rX, rY, rZ); netDraw(pt); drawMobs(pt); arrowsDraw(pt); eggsDraw(pt); tntDraw(pt);
        if (dual && S2.init) split2DrawBody(split2PoseOf(S2.P, S2.yaw, S2.pitch, S2.prevBodyYaw, S2.bodyYaw, S2.limbSwing, S2.limbAmt, S2.prevLimbAmt, S2.swinging, S2.swing, S2.inv[S2.sel].id, S2.ageT, pt), S2.skinTex);   // o OUTRO jogador (na passada 2 o S2 guarda o jogador 1)
        if (camMode) {   // personagem em 3ª pessoa
            Pose p; p.headYaw = wrap180(renderYaw - renderBodyYaw); p.headPitch = renderPitch;
            p.limb = renderLimb; p.amp = renderLimbAmt; p.atk = swinging ? swing : 0; p.held = held; p.held2 = gGm != 3 ? offh.id : 0; p.bow = gBowOn ? (float)gBowT : -1.f; p.eat = gEatOn ? (float)gEatT : -1.f;
            p.age = ageTicks + pt; p.sneak = P.sneak && !gBoatRide && !gCartRide; p.sit = gBoatRide != 0 || gCartRide != 0; wornFromInv(p); elytraPoseFill(p, P, pt, renderYaw, renderPitch);
            if (const BoatEnt* rb = boatById(gBoatRide)) boatRowPose(p, *rb, pt);   // remar: fase do remo
            glPushMatrix(); if (const BoatEnt* rbt = boatById(gBoatRide)) { const double sp[3] = {rX, rY, rZ}; boatRiderBegin(*rbt, pt, sp); } else glTranslatef((float)rX, (float)rY, (float)rZ); glRotatef(renderBodyYaw, 0, 1, 0); localDeathPose(pt);
            entLightAt(rX, rY + 1.0, rZ, gEntDim);
            entShaderOn();
            p.hurt = (gHurt > 10 || (gDead && !creative)) ? 1.f : 0.f; drawPlayer(p); if (gFire > 0 && !creative && !gDead) drawFireWH(.45f, 1.8f);   // chamas no proprio modelo (3a pessoa/espelho)
            skyShaderOff();
            entDimReset();
            glPopMatrix();
        }
            glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.1f); glColor4f(1.f, 1.f, 1.f, 1.f);
            glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        }
        if (!headWater && !headLava && !gDim) {   // nuvens ANTES dos translucidos (agua/vidro): senao apareciam por cima do vidro, que nao escreve profundidade
            const GLboolean fgC = glIsEnabled(GL_FOG);
            skyShaderOff(); if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
            drawClouds(cam[0], cam[1], cam[2], nowS);
            if (!fgC) glDisable(GL_FOG);
            glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.1f); glColor4f(1.f, 1.f, 1.f, 1.f);
            glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
        }
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        skyShaderOn(1.f, gSkyScale, gSkyRB); glDepthMask(GL_FALSE);
        glEnable(GL_STENCIL_TEST); glStencilFunc(GL_NOTEQUAL, 1, 1); glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);   // água não é desenhada onde a máscara do bote marcou
        { PerfDrawScope perfDw(PS_DRAW_WATER); drawChunks(true); } glDisable(GL_STENCIL_TEST);
        skyShaderOff(); skyShaderOn(0.f, gSkyScale, gSkyRB); drawGlass(); skyShaderOff(); glDepthMask(GL_TRUE);   // vidro colorido por cima da agua (shader de bloco, nao o da agua)
        if (useVBO) pBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisable(GL_BLEND);
        glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisableClientState(GL_VERTEX_ARRAY);
        glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_TEXTURE_2D);
        if (has) {   // contorno da caixa de selecao real; gRay so vale se for a mesma celula mirada (o toque pode ter refeito o raio)
            if (gRay.hit && gRay.x == hh[0] && gRay.y == hh[1] && gRay.z == hh[2]) wireBox(gRay.box);
            else wireBox(AABB{(double)hh[0], (double)hh[1], (double)hh[2], hh[0] + 1.0, hh[1] + 1.0, hh[2] + 1.0});
        }
        if (mnOn && mnNeed > 0.f) { drawCrack(mnX, mnY, mnZ, std::min(9, (int)(mnT / mnNeed * 10.f))); glBindTexture(GL_TEXTURE_2D, atlasTex); }
        drawParts(camYaw, camPitch);
        gNetTagYaw = camYaw; gNetTagPitch = camPitch; gNetTagOn = true; netDrawTags(pt);
        if (dual && S2.init) netDrawTagLocal(vp == 0 ? p2Nick() : netName(), S2.P.px + (S2.P.x - S2.P.px) * pt, S2.P.py + (S2.P.y - S2.P.py) * pt, S2.P.pz + (S2.P.z - S2.P.pz) * pt, S2.P.sneak, false);   // nick do outro jogador local (na passada 2 o S2 guarda o jogador 1)
        gNetTagOn = false;   // nametags depois do mundo opaco (translucidas, sem escrever profundidade)
        modulesDrawWorld(pt);
        if (!headWater && !headLava && !gDim) drawGodRays();   // raios de luz: profundidade copiada ANTES do jogador/mao, igual em 1a e 3a pessoa
        if (!camMode) {         // braço / item em 1ª pessoa
            glDisable(GL_FOG);
            glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            applyViewBob(P.distWalked, P.prevDistWalked, camBobYaw, prevCamBobYaw, camBobPitch, prevCamBobPitch, pt);
            entLightAt(eyeX, eyeY, eyeZ, gEntDim);   // a mão também escurece à noite / em cavernas
            if (gGm != 3 && !hideHud) drawFireOverlay();   // jogador em chamas: fogo na tela (patch5.61)
            if (gGm != 3 && !hideHud) { glPushMatrix(); handBobApply(yaw, pitch, pt); drawHand(bowPullItem(handItem), swinging ? swing : 0, equip); glPopMatrix(); }
            if (gGm != 3 && !hideHud && offh.id) { glPushMatrix(); handBobApply(yaw, pitch, pt); drawHand(offh.id, 0.f, 1.f, -1); glPopMatrix(); }   // mao esquerda (secundaria)
            entDimReset();
        }
        drawNightOverlay();   // só sem shader: escurecimento simples da cena
        glBindTexture(GL_TEXTURE_2D, atlasTex);
        sanitizeInv();
        gNoCross = touchHud;
        gInvTouch = touchHud;
        const bool hudOff = hideHud && !invOpen && !pauseOpen && !chatOpen;
        if (!hudOff) drawHud(w, h, sel, invOpen, mouseX, mouseY, itemName(held), nameT, msg.c_str(), msgT);
        if (touchHud && !invOpen) drawTouch(w, h, hudOff);   // F1: só o botão F1 (fraco)
        if (!hudOff) { if (vp == vpBeg) { drawChat(w, h); drawTabList(w, h); drawGmSwitcher(w, h); } modulesDrawHud(w, h); }
        if (!hudOff && gSplit) drawSplitNick(w, h, vp == 1 ? p2Nick() : netName());   // nick do jogador de cada viewport
        if (touchHud && (invOpen || chatOpen)) drawTouchClose(w, h);
        if (dbg && vp == vpBeg) {   // dados do jogador deste viewport; desenhado depois do laco (por cima de tudo)
            DbgInfo d; d.x = rX; d.y = rY; d.z = rZ; d.speed = sqrt(P.mx * P.mx + P.mz * P.mz) * 20.0; d.yaw = yaw; d.pitch = pitch;
            d.hasT = has; d.tx = hh[0]; d.ty = hh[1]; d.tz = hh[2]; d.tid = has ? getB(hh[0], hh[1], hh[2]) : 0;
            d.flying = P.flying; d.sprint = P.sprint; d.sneak = P.sneak; d.ground = P.onGround; d.water = P.inWater; d.camMode = camMode; d.ww = fw; d.wh = fh;
            dbgD = d; dbgHave = true;
        }
        if (vp == 1) swapView();   // devolve o contexto do jogador 1
        }   // fim do laco de viewports
        gVpX = gVpY = 0;
        const int w = fw, h = fh;
        if (dual) {   // divisoria preta entre os dois viewports
            glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, w, h, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_TEXTURE_2D); glColor4f(0, 0, 0, 1);
            const float dx = gSplitMode == 1 ? w / 2.f - 1.f : 0.f, dy = gSplitMode == 1 ? 0.f : h / 2.f - 1.f, dw = gSplitMode == 1 ? 2.f : (float)w, dh = gSplitMode == 1 ? (float)h : 2.f;
            glBegin(GL_QUADS); glVertex2f(dx, dy); glVertex2f(dx + dw, dy); glVertex2f(dx + dw, dy + dh); glVertex2f(dx, dy + dh); glEnd();
            glEnable(GL_TEXTURE_2D); glEnable(GL_DEPTH_TEST);
        }
        if (pauseOpen) { if (gPage.empty()) drawPauseMenu(w, h, mouseX, mouseY); else drawPage(w, h, true); }
        else if (gDead && !creative) drawDeathScreen(w, h, mouseX, mouseY);
        if (dbgHave) drawDebug(dbgD, w, h);
        if (chatOpen && oskActive()) { uiOrtho(w, h); G = guiScaleFor(w, h); OX = OY = 0; oskDraw((w + G - 1) / G, (h + G - 1) / G, mouseX / G, mouseY / G); glEnable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); }   // teclado virtual do chat (controle)
        if (fading) { const float fa = 1.f - (SDL_GetTicks() - fadeT0) / 250.f; if (fa <= 0.f) fading = false; else drawLoad(w, h, 1.f, true, fa); }   // fade out do loading = fade in do HUD/mundo
        if (quitPhase) {   // fade out do jogo ate o preto; ao fechar salva e volta ao titulo (que entra com fade in)
            const float qa = (SDL_GetTicks() - quitT0) / 120.f; drawFadeBlack(w, h, qa);
            if (qa >= 1.f) { quitPhase = false; saveCurWorld(P.x, P.y, P.z); netStop(); gHostPending = false; setPause(false); resetWorldData(); gCurDir.clear(); panoOn = false; gScreen = SC_TITLE; titleQuick = true; grab = false; relMouse(false); fading = false; }
        }
        splashOverlay(w, h); glStackHeal("fim do quadro"); logGlErrors("Post render");
        { PerfScope perfSw(PS_SWAP); SDL_GL_SwapWindow(win); }
#ifdef BL_RPI
        glFinish();   // Raspberry: GPU nao enfileira quadros a frente (menos atraso entre apertar a tecla e ver na tela)
        { static Uint64 t0 = 0; const Uint64 fq = SDL_GetPerformanceFrequency(), per = fq / (gRpiSplit ? 30 : 60); const Uint64 n = SDL_GetPerformanceCounter();   // trava em 60 FPS mesmo se o vsync do driver falhar
          if (t0 && n - t0 < per) { const Uint32 ms = (Uint32)((per - (n - t0)) * 1000 / fq); if (ms > 1) SDL_Delay(ms - 1); } t0 = SDL_GetPerformanceCounter(); }
#endif
        perfFrameEnd(W.size());   // profiler: [PERF] a cada 5 s e [LAG] em quadros > 45 ms
    }
    logInfo("Stopping!");
    logInfo("Tempo de sessao: %.1f s", SDL_GetTicks() / 1000.0);
    modulesShutdown();
    netClose();
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    logFinalize("fechado pelo jogador");
    return 0;
}
