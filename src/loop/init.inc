// inicializacao: SDL/janela/GL, pack, mods, modulos, atlas, estado do jogador (antes do laco)
// FRAGMENTO de main() (incluido dentro da funcao em main_loop.inc; mesmo escopo/variaveis). Movido em patchOrg.4, texto identico.
    keymapLoad();   // BUG-015
#ifndef __ANDROID__
#endif
    perfModeLoad();   // patchPerf.1: modo desempenho (precisa vir antes de atlas/malhas)
#ifndef _WIN32
    setenv("GIO_USE_VFS", "local", 1);
#endif   // evita o aviso/timeout do GVFS (GIO tentando falar com o daemon gvfsd)
#ifdef __ANDROID__
    androidPrepare();   // chdir p/ armazenamento do app + extrai assets do APK (antes do logInit)
#endif
    logInit();
    for (int i = 1; i < argc; i++) { if (!strcmp(argv[i], "--benchmark")) gBenchOn = true; else if (!strcmp(argv[i], "--pregen") && i + 1 < argc) gPregenArg = atoi(argv[++i]);   /* patchPregen.1 */ else if (!strcmp(argv[i], "--frames") && i + 1 < argc) gBenchFrames = std::max(1, atoi(argv[i + 1])); else if (!strcmp(argv[i], "--smoke-report") && i + 1 < argc) gBenchOut = argv[i + 1]; }   // HZ-008
    bool startFull = true; for (int i = 1; i < argc; i++) { if (!strcmp(argv[i], "--fullscreen")) startFull = true; else if (!strcmp(argv[i], "--windowed")) startFull = false; }   // patchBoat3.1: inicia em tela cheia (--windowed abre em janela)
#if defined(__linux__) && !defined(__ANDROID__) && !defined(BL_RPI)
    setenv("LIBDECOR_PLUGIN_DIR", "/nonexistent", 0); if (getenv("WAYLAND_DISPLAY")) setenv("SDL_VIDEODRIVER", "wayland", 0); else if (getenv("DISPLAY")) setenv("SDL_VIDEODRIVER", "x11", 0);   // patchWayland.1: Wayland nativo quando ha sessao Wayland (libdecor segue desligado abaixo, evita o abort do GTK); so cai em X11 se nao houver WAYLAND_DISPLAY; SDL_VIDEODRIVER=x11 forca X11
    setenv("SDL_VIDEO_WAYLAND_ALLOW_LIBDECOR", "0", 0);
#if !defined(__ANDROID__) && !defined(_WIN32)
    setenv("mesa_glthread", "false", 0);   // patchGLPerf.1: Mesa glthread + glGet*/FBO do quadro (matrizes, viewport, sombras) = sincroniza a thread a cada chamada (15-60 ms parados em pos-sombras->pre-reflexo). Reativar: mesa_glthread=true
#endif   // patchLinux.1: o libdecor-gtk le a chave 'antialiasing' do GNOME (removida nos novos) e aborta com SIGABRT; sem libdecor o jogo abre. Quem quiser: SDL_VIDEO_WAYLAND_ALLOW_LIBDECOR=1
#endif
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { logFatal("Falha ao iniciar o SDL: %s", SDL_GetError()); return 1; }
    {   // patchGenSlice.1: geracao de chunk em etapas por quadro: ligada no Pi e no Android (um chunk de uma vez trava 90-150 ms); BLOCKLAND_GENSLICE=1 liga / =0 desliga em qualquer plataforma
        gGenSlice = true;   // patchSeam.5: ligado em todas as plataformas (no PC um chunk gerado de uma vez travava 60-95 ms)
        if (const char* gs = getenv("BLOCKLAND_GENSLICE")) gGenSlice = gs[0] == '1';
    }
    sndInit();   // audio.inc (sem dispositivo de som o jogo segue mudo)
#ifdef BL_RPI
    sndPrewarm(); gSndAsync = true;   // patchRpiOpt.7: dai em diante, sons novos sao decodificados em thread de fundo
#endif
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) logWarn("Controles indisponiveis: %s", SDL_GetError());   // sem isso o jogo segue so com teclado/mouse/toque
    { SDL_version sv; SDL_GetVersion(&sv); const char* vd = SDL_GetCurrentVideoDriver();
      logInfo("Ambiente: SDL %d.%d.%d, driver de video: %s, CPU: %d nucleos, RAM: %d MB", (int)sv.major, (int)sv.minor, (int)sv.patch, vd ? vd : "?", SDL_GetCPUCount(), SDL_GetSystemRAM());
      std::string av; for (int i = 1; i < argc; i++) { if (i > 1) av += ' '; av += argv[i]; }
      logInfo("Argumentos: %s", av.empty() ? "(nenhum)" : av.c_str()); }
#ifdef __ANDROID__
    androidGLAttrs();
#endif
    vkChooseApi(argc, argv);   // patchVkRT.1: build unica: options.cfg renderer= decide OpenGL/Vulkan ANTES de criar a janela
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);   // stencil: máscara de água do bote
#if !defined(__ANDROID__) && !defined(BL_RPI)
    gfxLoadOpts(); if (BL_VKON) gAA = 0; if (gAA > 0 && !gPerf) { SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1); SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, gAA); } else gAA = 0;   // patchGfx.1: MSAA (options.cfg aa=0/2/4/8)
#endif
    SDL_Window* win = SDL_CreateWindow("Blockland", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, BL_WIN_W, BL_WIN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (startFull ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) | BL_ANDROID_WINFLAGS);
#if !defined(__ANDROID__) && !defined(BL_RPI)
    if (!win && gAA > 0) { logWarn("MSAA %dx indisponivel, abrindo sem antisserrilhado", gAA); gAA = 0; SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0); SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        win = SDL_CreateWindow("Blockland", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, BL_WIN_W, BL_WIN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (startFull ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)); }
#endif
#ifdef BL_XP
    if (!win) {   // patchXP.1: Chrome9 as vezes recusa 24/8 bits: tenta 16 bits de profundidade e sem stencil (o stencil so serve a mascara de agua do bote), depois cor 5-6-5
        static const int tent[2][3] = {{16, 0, 8}, {16, 0, 5}};
        for (int t = 0; t < 2 && !win; t++) {
            logWarn("Janela OpenGL recusada (%s); tentando profundidade %d, stencil %d, cor %d", SDL_GetError(), tent[t][0], tent[t][1], tent[t][2]);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, tent[t][0]); SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, tent[t][1]);
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE, tent[t][2]); SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, tent[t][2] == 5 ? 6 : 8); SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, tent[t][2]);
            win = SDL_CreateWindow("Blockland", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, BL_WIN_W, BL_WIN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (startFull ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)); }
    }
#endif
    if (!win) { fprintf(stderr, "janela: %s\n", SDL_GetError());
#ifdef BL_VULKAN
        if (BL_VKON) vkFailRestart(SDL_GetError());
#endif
        return 1; }
    SDL_SetWindowMinimumSize(win, BL_WIN_MINW, BL_WIN_MINH);   // trava o tamanho minimo: launcher e split nao quebram o layout em janela pequena
    gBlWin = win;
    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) { fprintf(stderr, "%s: %s\n", BL_VKON ? "Vulkan" : "OpenGL", SDL_GetError());
#ifdef BL_VULKAN
        if (BL_VKON) vkFailRestart(SDL_GetError());
#endif
        return 1; }
#ifdef __ANDROID__
    androidGLInit(); touchHud = true;   // gl4es pronto; controles mobile ja ligados (sem precisar do F8)
#endif
#if defined(__ANDROID__) || defined(BL_RPI)
    SDL_GL_SetSwapInterval(1);   // celular/Raspberry: vsync ligado (celular: FPS destravado esquenta e engasga): vsync ligado (FPS destravado esquenta o aparelho, causa throttling e engasgos)
#else
    // patchGfx.1: PC com vsync ADAPTATIVO (tearing so se cair abaixo da taxa do monitor); sem suporte, vsync normal. Quadro estavel = bem mais suave; /vsync 0 destrava
    SDL_GL_SetSwapInterval(0); gVsyncOn = false;   // patchFPS.1: PC destravado por padrao (maximo de FPS e menos input lag); VSync so se escolhido em Video ou /vsync 1
#if defined(__linux__) && !defined(__ANDROID__) && !defined(BL_RPI) && defined(SDL_DROPBEGIN)
    if (BL_NOVK) { const char* vd = SDL_GetCurrentVideoDriver(); const char* ev = getenv("BLOCKLAND_VSYNC");   // BLOCKLAND_VSYNC=1 mantem o vsync do driver
      if (vd && !strcmp(vd, "wayland") && !(ev && ev[0] == '1')) { SDL_DisplayMode dm; SDL_GetCurrentDisplayMode(0, &dm); gSoftHz = std::max(30, std::min(240, dm.refresh_rate > 0 ? dm.refresh_rate : 60)); SDL_GL_SetSwapInterval(0); gSoftPace = true; gVsyncOn = false; logInfo("Wayland: vsync do driver trocado por limitador de %d FPS (BLOCKLAND_VSYNC=1 desfaz)", gSoftHz); } }
#endif
    if (BL_NOVK && gAA > 0) rdEnable(GL_MULTISAMPLE);
#endif
    { int fw = 0, fh = 0; SDL_GetWindowSize(win, &fw, &fh);
      logInfo("Janela criada: %dx%d%s", fw, fh, startFull ? " (tela cheia)" : "");
      logInfo("OpenGL: %s", (const char*)glGetString(GL_VERSION));
#ifdef BL_XP
      logInfo("Build Windows XP (BL_XP): modo desempenho, sem MSAA/aniso/sombras/godrays, indices 16 bits, shader GLSL 1.10 enxuto; extensoes: %s", glGetString(GL_EXTENSIONS) ? (const char*)glGetString(GL_EXTENSIONS) : "?");
#endif
       logInfo("GPU: %s (%s)", (const char*)glGetString(GL_RENDERER), (const char*)glGetString(GL_VENDOR)); }
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
    modulesInit(); dsc::init();   // ganchos dos modulos (src/modules)
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
    rdEnable(GL_TEXTURE_2D); rdDisable(GL_CULL_FACE);
    rdEnable(GL_ALPHA_TEST); rdAlphaFunc(GL_GREATER, 0.02f);   // recorte das folhas (como o Minecraft)

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
    printf("Controles: WASD mover | Shift agachar | Ctrl correr | Espaco pular | E inventario | 1-9/roda hotbar | F3+F4 modo de jogo | 2x Espaco voar (criativo; Espaco sobe, Shift desce) | T ou / chat e comandos (/ajuda, /gamemode 0|1) | F3 informacoes de depuracao | F5 camera | arraste um .png de skin na janela\n");


    int start[9] = {GRASS, DIRT, STONE, SAND, LOG, PLANKS, COBBLE, IT_WSWORD, IT_APPLE};
    (void)start; for (int i = 0; i < 36; i++) inv[i] = Slot();   // patchGm.2: mundo novo nasce sem itens (igual ao Minecraft)

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
        panoOn = false; gPanoGen = false; resetWorldData(); netEdits.clear(); netPend.clear(); remotes.clear(); remotesX.clear(); RAD = rdChunks(); gCurDir = gMenuDir; SaveInfo si;
        { FILE* f = fopen(("saves/" + gCurDir + "/level.cfg").c_str(), "rb"); std::string t; if (f) { char b[512]; size_t r; while ((r = fread(b, 1, sizeof b, f)) > 0) t.append(b, r); fclose(f); }
          WORLD_SEED = (int64_t)strtoll(cfgGet(t, "seed").c_str(), nullptr, 10); applyGm(atoi(cfgGet(t, "mode").c_str())); gHard = atoi(cfgGet(t, "hardcore").c_str()) != 0;
          gWorldTime = atoll(cfgGet(t, "time").c_str()); gDayCycle = true;
          if (!cfgGet(t, "weather").empty()) { gWeather = std::max(0, std::min(2, atoi(cfgGet(t, "weather").c_str()))); gWxLeft = std::max(600, atoi(cfgGet(t, "wxleft").c_str())); gRain = gWeather > 0 ? 1.f : 0.f; gThunder = gWeather == 2 ? 1.f : 0.f; } else { gWeather = 0; gWxLeft = 30000; gRain = gThunder = 0.f; }   // patchWx.5: clima salvo   // hora do mundo (save sem "time" começa no amanhecer)
          if (!cfgGet(t, "px").empty()) { si.hasPos = true; si.px = atof(cfgGet(t, "px").c_str()); si.py = atof(cfgGet(t, "py").c_str()); si.pz = atof(cfgGet(t, "pz").c_str()); } }
        logInfo("Loading world '%s' (seed: %lld, modo: %s)", gCurDir.c_str(), (long long)WORLD_SEED, creative ? "Criativo" : "Sobrevivencia");
        loadSavedChunks(gCurDir); netherOnWorldLoad(gCurDir); boatsLoad(gCurDir); cartsLoad(gCurDir); chestsLoad(gCurDir); furnLoad(gCurDir); brewLoad(gCurDir); propsWorldLoad(gCurDir); signsLoad(gCurDir); dragonWorldLoad(gCurDir);
        modulesWorldLoad(gCurDir);   // patchMod.2
        survRespawn(P); gRespHold = 0; P.flying = (gGm == 3);   // (ao carregar mundo a posicao vem do save/finishLoad: sem correcao de renascimento)
        if (!loadPlayer()) {   // primeira vez neste mundo: kit inicial; depois o inventario volta como foi salvo
            for (int i = 0; i < 36; i++) inv[i] = Slot(); cur = Slot();
            // patchGm.2: sem kit inicial (inventario vazio como no Minecraft)
        }
        loadSi = si; loadCx = si.hasPos ? fdiv((int)floor(si.px), CW) : 0; loadCz = si.hasPos ? fdiv((int)floor(si.pz), CW) : 0; loadProg = 0.f; loadT0 = SDL_GetTicks(); loadDoneT = 0; loadLogPct = 0;
        gScreen = SC_LOAD; grab = false; relMouse(false);
    };
    auto finishLoad = [&]() {
        const SaveInfo& si = loadSi;
        if (!spawnLoad()) {   // patchFix.3: o spawn do mundo e FIXO (centro do mundo, salvo em spawn.dat); antes era recalculado perto da ultima posicao salva => renascia onde morreu
            if (si.hasPos) for (int dz = -4; dz <= 4; dz++) for (int dx = -4; dx <= 4; dx++) if (!getC(dx, dz)) gen(dx, dz);   // garante os chunks da origem (safeSpawn le blocos carregados)
            safeSpawn(0, 0, gSpX, gSpY, gSpZ); spawnSave();
        }
        if (si.hasPos) {   // posicao salva; se estiver dentro de blocos, sobe ate achar espaco livre
            P.x = P.px = si.px; P.y = P.py = si.py; P.z = P.pz = si.pz;
            const int fx = (int)floor(si.px), fz = (int)floor(si.pz); int fy = (int)floor(si.py);
            if (solid(getB(fx, fy, fz)) || solid(getB(fx, fy + 1, fz))) { while (fy < CH - 3 && (solid(getB(fx, fy, fz)) || solid(getB(fx, fy + 1, fz)))) fy++; P.y = P.py = fy; }
        } else { P.x = P.px = gSpX; P.y = P.py = gSpY; P.z = P.pz = gSpZ; }
        yaw = pitch = 0; gHandBob[0] = gHandBob[1] = gHandBob[2] = gHandBob[3] = 0; invOpen = false; pauseOpen = false; grab = true; relMouse(true); gScreen = SC_PLAY; tickAcc = 0.0;
        if (gInvOwner == 2) { split2SwapInvOwner(); gInvOwner = 0; }
        for (int xk = 0; xk < 3; xk++) gSX[xk].init = false; if (gSplit) for (int xk = 0; xk < gSplitN - 1; xk++) { SxSel q(xk); split2Spawn(P, yaw); }   // mundo novo: jogador 2 nasce ao lado do jogador 1
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
    if (!gPerf && (!gLoadBgTex || !gIntroBgTex)) {   // patchIntro.5: 1a execucao: gera UMA vez duas paisagens diferentes do menu (abertura e geracao de mundo), borra e guarda em cache/
        const bool svL = gSplashLoading; gSplashLoading = false;   // sem quadros do loading no meio da captura
        struct Job { unsigned seed; const char* path; GLuint* tex; } jobs[2] = { {0xA11CE5E5u, "cache/terreno_abertura.png", &gIntroBgTex}, {0xB10C1A4Du, "cache/terreno_load.png", &gLoadBgTex} };
        for (auto& j : jobs) if (!*j.tex) { gPanoSeedBase = j.seed; panoInit(); int w2, h2; SDL_GetWindowSize(win, &w2, &h2); RAD = PANO_RAD; drawPanorama(w2, h2, 0.f); blurScreen(w2, h2); blurScreen(w2, h2); bgCapture(w2, h2, j.path, *j.tex); }
        gPanoSeedBase = 0x5EED2026u; gSplashLoading = svL; }
    panoBegin(); gTitleNoBlack = true;   /* patchBoot.1: panorama gerado em fatias de 8 ms no proprio titulo (antes: ~376 ms bloqueando a abertura); fundo em cache cobre ate ficar pronto */   // panorama do titulo pronto ANTES do fim do loading e sem o fade preto proprio; botoes e titulo entram DEPOIS do splash sumir
    bool run = true;
    if (gCliHost) gHostPending = true;
    if (!gCliJoin.empty()) { std::string jh; int jp; if (srvParse(gCliJoin, jh, jp)) { netJoinBegin(jh, jp); pageGo("conectando"); } }
    else if (!gCliHost) { if (getenv("BL_PE")) touchHud = true; skinScan(); /* launcher antigo removido: nome e skins na tela de skins */ }   // patchPE.2: modo PE (Android/Rpi/PC com F8 ou BL_PE=1) nao tem launcher: nome/skin ficam no cabide do titulo
    splashHold(win);   // patchIntro.2: abertura unica de 5 s (carga + barra) antes do titulo
    splashFinish(win);   // barra alcanca 100% e o overlay passa a sumir (fade 1 s) por cima do menu
    stageDone("Panorama do menu gerado");
    logInfo("Inicializacao concluida em %.3fs", SDL_GetTicks() / 1000.0);
    ctlInit(argc, argv);
