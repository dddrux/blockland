// ================= god rays (raios de luz do sol, F9 liga/desliga) =================
// Depois de desenhar o mundo, copia a profundidade da cena para uma textura e desenha UM quadro de tela cheia (shader gGodProg em sky_cycle.inc) que,
// para cada pixel, marcha em direcao ao sol contando quanto do caminho e ceu aberto: arvores, montanhas, nuvens e a mao bloqueiam, o resto vira
// feixes dourados, somados (blend aditivo). So aparecem com o sol baixo (nascer/por do sol): fora disso o passo inteiro e pulado (custo zero).
#ifdef __ANDROID__
static bool gGodOn = false,   // celular: raios de luz (tela cheia, 22 amostras) ficam desligados por padrao
#else
static bool gGodOn = true,
#endif
 gGodFail = false, gGodChk = false; static GLuint gGodTex = 0; static int gGodW = 0, gGodH = 0;
static float gGodCamM[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, gGodCamP[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
static inline float godSmooth(float a, float b, float x) { x = clampf2((x - a) / (b - a), 0.f, 1.f); return x * x * (3.f - 2.f * x); }
static void godCaptureCam() { glGetFloatv(GL_MODELVIEW_MATRIX, gGodCamM); glGetFloatv(GL_PROJECTION_MATRIX, gGodCamP); }   // chamado logo apos girar a camera (so rotacao)
#ifdef __ANDROID__
static bool gFlareOn = false;   // celular: lens flare (tela cheia + copia de profundidade) desligado
#else
static bool gFlareOn = true;
#endif
static float gFlI = 0.f; static Uint32 gFlT = 0;   // lens flare ao olhar o sol (F10); gFlI = intensidade suavizada (olho se adapta)
static void drawGodRays() {
    const Uint32 nowT = SDL_GetTicks(); const float dt = gFlT ? fminf((float)(nowT - gFlT) * .001f, .25f) : 0.f; gFlT = nowT;
    gShaderPackOn = !BL_NO_FANCY && (gsi("shd.sel", 0) > 0 || gShadowsOn);
#ifdef BL_GLES_NATIVE
    gnFancy = !BL_NO_FANCY && gsi("shd.sel", 0) > 0 && !gShadowsOn;   // sem sombras no backend nativo: so a escolha explicita do pacote liga a agua realista
#endif   // F7 liga junto agua/sol realistas;   // celular: nunca liga agua/sol realista
    {   // sol atras de nuvem: oclusao analitica suavizada (o buffer de profundidade nao pega nuvens longe/finas e escurecia a tela a toa)
        const float t2 = gSky.angle * 6.2831853f, sd3[3] = {-sinf(t2), cosf(t2), 0.f}; const double cm[3] = {gCamW[0], gCamW[1], gCamW[2]};
        const float tg = gShaderPackOn ? cloudSunOcc(cm, sd3) : 0.f; gSunOcc += (tg - gSunOcc) * (1.f - expf(-dt / .30f)); if (gSunOcc < .002f) gSunOcc = 0.f;
    }
    const bool godOK = gShaderPackOn && gGodOn && gFxOn && gGodProg, flOK = gShaderPackOn && gFlareOn && gFlProg;
    if (gGodFail || (!godOK && !flOK)) { gFlI = 0.f; return; }
    const float th = gSky.angle * 6.2831853f, sn = sinf(th), cs = cosf(th);   // sol = (-sin, cos, 0)
    const float g = godSmooth(-.10f, .02f, cs) * (1.f - godSmooth(.22f, .60f, cs));   // forte com o sol baixo; some ao meio-dia e de noite
    const float* M = gGodCamM; const float* P = gGodCamP; const float sx = -sn, sy = cs;
    const float ex = M[0] * sx + M[4] * sy, ey = M[1] * sx + M[5] * sy, ez = M[2] * sx + M[6] * sy;   // direcao do sol no espaco da camera
    const float facing = godSmooth(0.f, .35f, -ez); if (facing < .02f) { gFlI = 0.f; return; }   // sol atras da camera: sem raios nem flare
    const float cw = P[3] * ex + P[7] * ey + P[11] * ez; if (cw < .01f) { gFlI = 0.f; return; }
    const float su = (P[0] * ex + P[4] * ey + P[8] * ez) / cw * .5f + .5f, sv = (P[1] * ex + P[5] * ey + P[9] * ez) / cw * .5f + .5f;
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp); if (vp[2] < 8 || vp[3] < 8) return;
    const float asp = (float)vp[2] / (float)vp[3];
    const bool godNow = godOK && g >= .02f;
    {   // flare: so com o sol no campo de visao (perto do centro da tela) e acima do horizonte; sobe rapido, desce devagar
        const float dc = hypotf((su - .5f) * asp, sv - .5f);
        const float tgt = flOK ? (1.f - godSmooth(.10f, .85f, dc)) * facing * godSmooth(-.04f, .08f, cs) : 0.f;
        gFlI += (tgt - gFlI) * (1.f - expf(-dt / (tgt > gFlI ? .15f : .40f))); if (gFlI < .005f) gFlI = 0.f;
    }
    const bool flNow = flOK && gFlI >= .01f;
    if (!godNow && !flNow) return;
    if (!gGodTex) glGenTextures(1, &gGodTex);
    glBindTexture(GL_TEXTURE_2D, gGodTex);
    if (gGodW != vp[2] || gGodH != vp[3]) {   // (re)cria a textura de profundidade no tamanho da janela
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, vp[2], vp[3], 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gGodW = vp[2]; gGodH = vp[3];
    }
    if (!gGodChk) while (glGetError() != GL_NO_ERROR) {}
    if (godNow) glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp[0], vp[1], vp[2], vp[3]);
    else {   // so o flare: ele le a profundidade apenas ao redor do sol -> copia so um quadrado pequeno (antes: tela inteira todo quadro = travada ao olhar o sol)
        const int R = 48; int cx = (int)(su * vp[2]), cy = (int)(sv * vp[3]); cx = std::max(R, std::min(vp[2] - R, cx)); cy = std::max(R, std::min(vp[3] - R, cy));
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, cx - R, cy - R, vp[0] + cx - R, vp[1] + cy - R, 2 * R, 2 * R);
    }
    if (!gGodChk) { gGodChk = true; if (glGetError() != GL_NO_ERROR) { gGodFail = true; logWarn("God rays: copia de profundidade nao suportada (MSAA?); desligados"); glBindTexture(GL_TEXTURE_2D, atlasTex); return; } logInfo("God rays: prontos"); }
    const bool fog = glIsEnabled(GL_FOG), alpha = glIsEnabled(GL_ALPHA_TEST);
    skyShaderOff(); glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE); glDisable(GL_FOG); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    float sc3[3]; sunTransmit(cs, sc3); const float cr = sc3[0], cg = sc3[1] * .96f + .04f, cb = sc3[2] * .9f + .06f;   // cor real do sol pela atmosfera
    auto quad = [] { glBegin(GL_QUADS); glTexCoord2f(0, 0); glVertex2f(-1, -1); glTexCoord2f(1, 0); glVertex2f(1, -1); glTexCoord2f(1, 1); glVertex2f(1, 1); glTexCoord2f(0, 1); glVertex2f(-1, 1); glEnd(); };
    if (godNow) {
        pUseProgram(gGodProg); pUniform2f(uGdSun, su, sv); pUniform3f(uGdCol, cr, cg, cb);
        pUniform1f(uGdI, g * facing * .62f * (1.f + 1.4f * gSunOcc * (1.f - gSunOcc)));   // mais raios quando a nuvem esta so parcialmente na frente
        pUniform1f(uGdAsp, asp);
        quad();
    }
    if (flNow) {   // lens flare + escurecimento: dst = dst * alfa(1 - escuro) + cor do flare
        glBlendFunc(GL_ONE, GL_SRC_ALPHA);
        pUseProgram(gFlProg); pUniform2f(uFlSun, su, sv); pUniform3f(uFlCol, cr, cg, cb);
        pUniform1f(uFlI, gFlI); pUniform1f(uFlOcc, gSunOcc); pUniform1f(uFlAsp, asp);
        quad();
    }
    pUseProgram(0);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND); glDepthMask(GL_TRUE); glEnable(GL_DEPTH_TEST); if (fog) glEnable(GL_FOG); if (alpha) glEnable(GL_ALPHA_TEST);
    glBindTexture(GL_TEXTURE_2D, atlasTex);
}
template <class F> static void godToggle(F& say) {
    if (!gGodProg || gGodFail) { say("God rays indisponiveis nesta placa/driver (veja logs/latest.log)"); return; }
    gGodOn = !gGodOn; say(gGodOn ? "God rays: ligados (F9 desliga)" : "God rays: desligados");
}
template <class F> static void flareToggle(F& say) {
    if (!gFlProg || gGodFail) { say("Lens flare indisponivel nesta placa/driver (veja logs/latest.log)"); return; }
    gFlareOn = !gFlareOn; say(gFlareOn ? "Lens flare do sol: ligado (F10 desliga)" : "Lens flare do sol: desligado");
}
