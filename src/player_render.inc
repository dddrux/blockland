// ================= desenho do personagem =================
struct ElySm { float x = .2617994f, y = 0.f, z = -.2617994f; uint32_t last = 0; };   // suavizado das asas (um por jogador)
struct Pose { ElySm* elySm = nullptr; float hurt = 0; float headYaw = 0, headPitch = 0, limb = 0, amp = 0, atk = 0, age = 0; float bow = -1.f, eat = -1.f; int held = 0, held2 = 0; bool sleep = false; bool elyWorn = false, elyFly = false; float elyTicks = 0.f, elyMot[3] = {0, 0, 0}, elyLook[3] = {0, 0, -1}; bool sneak = false, sit = false, oar = false; float row[2] = {0, 0}, rowAmp[2] = {0, 0}; int worn[4] = {0, 0, 0, 0}; };   // worn = ids de armadura (capacete, peitoral, calcas, botas)
static void wornFromInv(Pose& p) { for (int i = 0; i < 4; i++) p.worn[i] = armor[i].id; p.elyWorn = isElytraItem(armor[1].id); }
static void elytraPoseFill(Pose& p, const Phys& P, float pt, float yawDeg, float pitchDeg) {   // estado de voo do jogador local para o modelo
    p.elyWorn = gGm != 3 && isElytraItem(armor[1].id); p.elyFly = p.elyWorn && P.elytra && !P.onGround;
    p.elyTicks = (float)P.elyTicks + pt; p.elyMot[0] = (float)P.mx; p.elyMot[1] = (float)P.my; p.elyMot[2] = (float)P.mz;
    const float yr = yawDeg * .0174533f, pr = pitchDeg * .0174533f; p.elyLook[0] = -sinf(yr) * cosf(pr); p.elyLook[1] = sinf(pr); p.elyLook[2] = -cosf(yr) * cosf(pr);
}
// ModelElytra.setRotationAngles: angulos das asas (suavizados 10% por quadro de 60 fps) e altura do pivo
static void elytraAngles(const Pose& p, float& fx, float& fy, float& fz, float& fpy) {
    static ElySm local; ElySm& S = p.elySm ? *p.elySm : local; float& sx = S.x; float& sy = S.y; float& sz = S.z; uint32_t& last = S.last;
    float f = .2617994f, f1 = -.2617994f, f2 = 0.f, f3 = 0.f;
    if (p.elyFly) { float f4 = 1.f;
        if (p.elyMot[1] < 0.f) { const double L = sqrt((double)p.elyMot[0] * p.elyMot[0] + (double)p.elyMot[1] * p.elyMot[1] + (double)p.elyMot[2] * p.elyMot[2]); if (L > 1e-9) f4 = 1.f - (float)pow(-p.elyMot[1] / L, 1.5); }
        f = f4 * .34906584f + (1.f - f4) * f; f1 = f4 * -1.5707964f + (1.f - f4) * f1;
    } else if (p.sneak) { f = .6981317f; f1 = -.7853982f; f2 = 3.f; f3 = .08726646f; }
    const uint32_t now = SDL_GetTicks(); const float k = last ? 1.f - powf(.9f, (float)std::min<uint32_t>(100u, now - last) * .06f) : 1.f; last = now;
    sx += (f - sx) * k; sy += (f3 - sy) * k; sz += (f1 - sz) * k; fx = sx; fy = sy; fz = sz; fpy = f2;
}
static void part(const std::vector<V>& m, float px, float py, float pz, float rx = 0, float ry = 0, float rz = 0) {
    glPushMatrix(); glTranslatef(px / 16, py / 16, pz / 16);
    if (rz != 0) glRotatef(rz, 0, 0, 1);   // mesma ordem do ModelRenderer: Z, Y, X
    if (ry != 0) glRotatef(ry, 0, 1, 0);
    if (rx != 0) glRotatef(rx, 1, 0, 0);
    glScalef(1 / 16.f, 1 / 16.f, 1 / 16.f); drawMesh(m); glPopMatrix();
}
// Pose = ModelBiped.setRotationAngles do 1.8.9. Os ângulos são calculados no referencial do
// modelo Java (y para baixo, frente = -Z, radianos) e convertidos na hora de desenhar:
// nosso X/Y = -(X/Y do Java), nosso Z = Z do Java, x/y dos pivôs também espelhados.
static void drawHeldBody(int id);   // item na mao em 3a pessoa (tocha = modelo 3D animado)
static void drawPlayer(const Pose& p) {   // origem = pés, olhando para -Z
    const float PI_ = 3.14159265f, R2D = 57.2957795f;
    glBindTexture(GL_TEXTURE_2D, skinTex); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f);
    gEntHurt = p.hurt;   // flash vermelho (so o corpo/armadura; o item na mao volta ao normal)
    glPushMatrix();
    if (p.elyFly) {   // RenderPlayer.applyRotations: o corpo deita na direcao do olhar (gira em torno dos pes) e inclina conforme a diferenca entre movimento e olhar
        glRotatef(p.headYaw, 0, 1, 0);
        const float ft = p.elyTicks, f1 = std::min(1.f, std::max(0.f, ft * ft / 100.f));
        glRotatef(f1 * (-90.f + p.headPitch), 1, 0, 0);
        const double d0 = (double)p.elyMot[0] * p.elyMot[0] + (double)p.elyMot[2] * p.elyMot[2], d1 = (double)p.elyLook[0] * p.elyLook[0] + (double)p.elyLook[2] * p.elyLook[2];
        if (d0 > 0 && d1 > 0) { const double d2 = std::max(-1.0, std::min(1.0, ((double)p.elyMot[0] * p.elyLook[0] + (double)p.elyMot[2] * p.elyLook[2]) / (sqrt(d0) * sqrt(d1))));
            const double d3 = (double)p.elyMot[0] * p.elyLook[2] - (double)p.elyMot[2] * p.elyLook[0];
            glRotatef((float)((d3 > 0 ? 1.0 : d3 < 0 ? -1.0 : 0.0) * acos(d2)) * R2D, 0, 1, 0); }
    }
    glScalef(.9375f, .9375f, .9375f);
    if (p.sleep) { glTranslatef(0, .1f, 0); glRotatef(-90.f, 1, 0, 0); }   // patchProp.14: deitado de costas, cabeca para -Z do corpo
    if (p.sneak) glTranslatef(0, -.2f, 0);   // ModelBiped.render: translate(0, 0.2, 0) ao agachar (antes o corpo ficava flutuando)

    const float ph = p.limb * .6662f;
    float armRx = cosf(ph + PI_) * 2.f * p.amp * .5f, armLx = cosf(ph) * 2.f * p.amp * .5f;
    float legRx = cosf(ph) * 1.4f * p.amp,            legLx = cosf(ph + PI_) * 1.4f * p.amp;
    const bool elyPose = p.elyFly && p.elyTicks > 4.f;   // ModelBiped (1.12): em voo (> 4 ticks) os membros quase nao balancam e a cabeca olha 45 graus para cima
    if (elyPose) { float ef = (p.elyMot[0] * p.elyMot[0] + p.elyMot[1] * p.elyMot[1] + p.elyMot[2] * p.elyMot[2]) / .2f; ef = ef * ef * ef; if (ef < 1.f) ef = 1.f; armRx /= ef; armLx /= ef; legRx /= ef; legLx /= ef; }
    const float hPitch = elyPose ? 45.f : p.headPitch, hYaw = p.elyFly ? 0.f : p.headYaw;
    float armRy = 0, armLy = 0, armRz = 0, armLz = 0, bodyX = 0, bodyY = 0, lean = 0;   // lean = inclinação do tronco inteiro (corpo+braços+cabeça) em torno do QUADRIL (remar)
    float armRpx = -5, armLpx = 5, armRpz = 0, armLpz = 0;
    float legPy = 12, legPz = 0, headPy = 0;
    const float headX = -hPitch / R2D;   // Java: positivo = olhando para baixo

    // ModelBiped.isRiding (montado no bote): braços -36° e pernas para a frente, abertas 18° (Y) / 4,5° (Z)
    const float sitRy = p.sit ? PI_ / 10.f : 0.f, sitRz = p.sit ? .07853982f : 0.f;
    if (p.sit) { armRx -= PI_ / 5.f; armLx -= PI_ / 5.f; legRx = legLx = -1.4137167f; }
    if (p.held) armRx = armRx * .5f - PI_ / 10.f;
    if (p.eat >= 0.f && p.held) armRx = -1.7f + sinf(p.eat * 1.4f) * .1f;   // comendo: braco levanta a comida ate a boca e balanca
    if (p.held2) armLx = armLx * .5f - PI_ / 10.f;   // segunda mao: braco esquerdo levanta como o direito
    if (p.sit) for (int i = 0; i < 2; i++) {   // remar: a mão segue o CABO do remo (alavanca no toleteira: a pá vai para a popa => o cabo vai para a proa). Mesma fase/fórmula do ModelBoat (boat.inc)
        const float a = std::min(1.f, p.rowAmp[i]); if (a <= 0.f) continue; const float rt = p.row[i], sd = i ? -1.f : 1.f;
        const float sweep = sinf(1.f - rt);                      // >0 = cabo para a popa (mão para trás); <0 = mão empurra para a proa
        const float deep = -sinf(-rt);                           // 1 = pá funda na água (cabo sobe), -1 = pá fora d'água (cabo desce)
        (i ? armLx : armRx) += (sweep * .42f - deep * .2f) * a;  // ombro: vai e vem com o cabo, sobe um pouco quando a pá afunda
        (i ? armLz : armRz) += sd * (.07f + .05f * (1.f - deep)) * a;   // cabos ficam perto do corpo: quase sem abrir
        (i ? armLy : armRy) += -sd * (.08f + .1f * sweep) * a;   // mão converge ao puxar
        lean -= sinf(1.f - rt - .45f) * .15f * a;                // tronco (2 braços somam .3 rad = ~17 graus): vai à frente ao empurrar, recua ao puxar; adianta o braço (o corpo inicia a remada)
    }
    if (p.atk > 0) {
        const float s = p.atk;
        bodyY = sinf(sqrtf(s) * PI_ * 2.f) * .2f;
        armRpz = sinf(bodyY) * 5.f; armRpx = -cosf(bodyY) * 5.f;
        armLpz = -sinf(bodyY) * 5.f; armLpx = cosf(bodyY) * 5.f;
        armRy += bodyY; armLy += bodyY; armLx += bodyY;
        float f6 = 1.f - s; f6 *= f6; f6 *= f6; f6 = 1.f - f6;
        const float f7 = sinf(f6 * PI_), f8 = sinf(s * PI_) * -(headX - .7f) * .75f;
        armRx -= f7 * 1.2f + f8;
        armRy += bodyY * 2.f;
        armRz += sinf(s * PI_) * -.4f;
    }
    if (p.sneak) { bodyX = .5f; armRx += .4f; armLx += .4f; legPz = 4.f; legPy = 9.f; headPy = 1.f; }
    armRz += cosf(p.age * .09f) * .05f + .05f; armLz -= cosf(p.age * .09f) * .05f + .05f;
    armRx += sinf(p.age * .067f) * .05f;       armLx -= sinf(p.age * .067f) * .05f;
    if (p.bow >= 0.f && !p.sit) {   // ModelBiped.aimedBow (1.8.9): os dois bracos apontam para a frente na direcao da cabeca
        const float hY = -p.headYaw / R2D;
        armRz = armLz = 0.f; armRy = -.1f + hY; armLy = .1f + hY + .4f; armRx = armLx = -PI_ / 2.f + headX;
        armRz += cosf(p.age * .09f) * .05f + .05f; armLz -= cosf(p.age * .09f) * .05f + .05f;
        armRx += sinf(p.age * .067f) * .05f; armLx -= sinf(p.age * .067f) * .05f;
    }

    const float armY = slim ? 21.5f : 22.f;
    if (p.sit && p.oar) for (int i = 0; i < 2; i++) {   // as maos SEGURAM o cabo do remo (IK): alvo = ponto do eixo do remo (mesma formula/pivo do ModelBoat em boat.inc), no referencial do corpo alinhado ao bote
        auto cl = [](float a, float b, float t) { return a + (b - a) * std::max(0.f, std::min(1.f, t)); };
        const float am = std::min(1.f, p.rowAmp[i]), rt = p.row[i];
        const float jx0 = cl(-1.0471976f, -.2617994f, .5f), jy0 = cl(-.7853982f, .7853982f, (sinf(1.f) + 1.f) / 2.f);
        const float jx = jx0 + (cl(-1.0471976f, -.2617994f, (sinf(-rt) + 1.f) / 2.f) - jx0) * am;
        float jy = jy0 + (cl(-.7853982f, .7853982f, (sinf(-rt + 1.f) + 1.f) / 2.f) - jy0) * am; if (i == 0) jy = PI_ - jy;   // braco 0 (direito) segue o remo 1
        float vx = 0.f, vy = -1.f, vz = 3.f;                    // ponto de pegada no eixo do remo (3 px do pivo rumo a pa): distancia ao ombro ~ comprimento do braco durante toda a remada
        { const float c = cosf(-jx), s = sinf(-jx), y2 = vy * c - vz * s, z2 = vy * s + vz * c; vy = y2; vz = z2; }   // Rx
        { const float c = cosf(-jy), s = sinf(-jy), x2 = vx * c + vz * s, z2 = -vx * s + vz * c; vx = x2; vz = z2; }   // Ry
        { const float c = cosf(.19634955f), s = sinf(.19634955f), x2 = vx * c - vy * s, y2 = vx * s + vy * c; vx = x2; vy = y2; }   // Rz (11,25 graus)
        const float mx = -3.f + vx, my = 5.f + vy, mz = (i ? 9.f : -9.f) + vz;   // referencial do modelo do bote (px)
        float tx = -mz, ty = my + 10.8f, tz = mx;                // -> referencial do jogador (pes na origem); 10,8 px = BOAT_SEAT_Y - .375
        tx /= .9375f; ty /= .9375f; tz /= .9375f;                // dentro do glScalef(.9375)
        const float cl_ = cosf(lean), sl_ = sinf(lean), dy = ty - 12.f;   // desfaz a inclinacao do tronco em torno do quadril
        ty = 12.f + dy * cl_ - tz * sl_; tz = dy * sl_ + tz * cl_;
        const float sx = i ? -5.f : 5.f, sy = slim ? 21.5f : 22.f;
        float dx = tx - sx, ddy = ty - sy, dz = tz;
        const float n = sqrtf(dx * dx + ddy * ddy + dz * dz); if (n < 1e-4f) continue; dx /= n; ddy /= n; dz /= n;
        (i ? armLx : armRx) = -asinf(std::max(-1.f, std::min(1.f, -dz)));
        (i ? armLz : armRz) = atan2f(dx, -ddy);
        (i ? armLy : armRy) = 0.f;
    }
    auto AR = [&](int slot, int layer, const std::vector<V>& m, float px, float py, float pz, float rx = 0, float ry = 0, float rz = 0) {   // peca de armadura com a mesma transformacao da parte do corpo
        int id = p.worn[slot]; if (!id || !armorOk(slot, id)) return;
        glBindTexture(GL_TEXTURE_2D, armorTexOf(id, layer)); part(m, px, py, pz, rx, ry, rz); glBindTexture(GL_TEXTURE_2D, skinTex);
    };
    glPushMatrix(); glTranslatef(0, 12 / 16.f, 0); glRotatef(-lean * R2D, 1, 0, 0); glTranslatef(0, -12 / 16.f, 0); glTranslatef(0, 24 / 16.f, 0); glRotatef(-bodyY * R2D, 0, 1, 0); glRotatef(-bodyX * R2D, 1, 0, 0); glTranslatef(0, -24 / 16.f, 0);
    part(pBody, 0, 0, 0); part(pJacket, 0, 0, 0); AR(1, 0, aBody1, 0, 0, 0); AR(2, 1, aBody2, 0, 0, 0);
    glPopMatrix();
    part(pRLeg, 1.9f, 24 - legPy, legPz, -legRx * R2D, -sitRy * R2D, sitRz * R2D); part(pRPants, 1.9f, 24 - legPy, legPz, -legRx * R2D, -sitRy * R2D, sitRz * R2D);
    AR(3, 0, aRLeg1, 1.9f, 24 - legPy, legPz, -legRx * R2D, -sitRy * R2D, sitRz * R2D); AR(2, 1, aRLeg2, 1.9f, 24 - legPy, legPz, -legRx * R2D, -sitRy * R2D, sitRz * R2D);
    part(pLLeg, -1.9f, 24 - legPy, legPz, -legLx * R2D, sitRy * R2D, -sitRz * R2D); part(pLPants, -1.9f, 24 - legPy, legPz, -legLx * R2D, sitRy * R2D, -sitRz * R2D);
    AR(3, 0, aLLeg1, -1.9f, 24 - legPy, legPz, -legLx * R2D, sitRy * R2D, -sitRz * R2D); AR(2, 1, aLLeg2, -1.9f, 24 - legPy, legPz, -legLx * R2D, sitRy * R2D, -sitRz * R2D);
    glPushMatrix(); glTranslatef(0, 12 / 16.f, 0); glRotatef(-lean * R2D, 1, 0, 0); glTranslatef(0, -12 / 16.f, 0);
    part(pLArm, -armLpx, armY, armLpz, -armLx * R2D, -armLy * R2D, armLz * R2D);
    part(pLSleeve, -armLpx, armY, armLpz, -armLx * R2D, -armLy * R2D, armLz * R2D);
    AR(1, 0, aLArm, -armLpx, armY, armLpz, -armLx * R2D, -armLy * R2D, armLz * R2D);
    if (p.held2) {   // item da segunda mao (mesma cadeia de transformacoes do braco esquerdo; espelho do item da direita)
        gEntHurt = 0.f; glPushMatrix(); glTranslatef(-armLpx / 16.f, armY / 16.f, armLpz / 16.f);
        glRotatef(armLz * R2D, 0, 0, 1); glRotatef(-armLy * R2D, 0, 1, 0); glRotatef(-armLx * R2D, 1, 0, 0);
        glRotatef(-90, 1, 0, 0); glTranslatef(-1 / 16.f, .125f, -.625f); drawHeldBody(p.held2); glPopMatrix();
        glBindTexture(GL_TEXTURE_2D, skinTex); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f); gEntHurt = p.hurt;
    }
    glPopMatrix();
    glPushMatrix(); glTranslatef(0, 12 / 16.f, 0); glRotatef(-lean * R2D, 1, 0, 0); glTranslatef(0, -12 / 16.f, 0); glTranslatef(0, (24 - headPy) / 16.f, 0); glRotatef(lean * R2D * .55f, 1, 0, 0); glRotatef(hYaw, 0, 1, 0); glRotatef(hPitch, 1, 0, 0);
    glPushMatrix(); glScalef(1 / 16.f, 1 / 16.f, 1 / 16.f); drawMesh(pHead); drawMesh(pHat);
    if (p.worn[0] && armorOk(0, p.worn[0])) { glBindTexture(GL_TEXTURE_2D, armorTexOf(p.worn[0], 0)); drawMesh(aHead); glBindTexture(GL_TEXTURE_2D, skinTex); } glPopMatrix(); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 12 / 16.f, 0); glRotatef(-lean * R2D, 1, 0, 0); glTranslatef(0, -12 / 16.f, 0); glTranslatef(-armRpx / 16.f, armY / 16.f, armRpz / 16.f);
    glRotatef(armRz * R2D, 0, 0, 1); glRotatef(-armRy * R2D, 0, 1, 0); glRotatef(-armRx * R2D, 1, 0, 0);
    glPushMatrix(); glScalef(1 / 16.f, 1 / 16.f, 1 / 16.f); drawMesh(pRArm); drawMesh(pRSleeve);
    if (p.worn[1] && armorOk(1, p.worn[1])) { glBindTexture(GL_TEXTURE_2D, armorTexOf(p.worn[1], 0)); drawMesh(aRArm); glBindTexture(GL_TEXTURE_2D, skinTex); }
    glPopMatrix();
    if (p.held) { gEntHurt = 0.f; glRotatef(-90, 1, 0, 0); glTranslatef(1 / 16.f, .125f, -.625f); drawHeldBody(bowPullItemT(p.held, p.bow >= 0.f && !p.sit ? (int)p.bow : -1)); glBindTexture(GL_TEXTURE_2D, skinTex); }
    glPopMatrix();
    if (p.elyWorn && elyTex && !eLWing.empty()) {   // LayerElytra: translate(0,0,0.125); as asas nao acompanham o recuo do agachar (ModelBiped.render so move o corpo)
        float fx, fy, fz, fpy; elytraAngles(p, fx, fy, fz, fpy);
        glPushMatrix(); if (p.sneak) glTranslatef(0, .2f, 0);
        gEntHurt = 0.f; glBindTexture(GL_TEXTURE_2D, elyTex);
        part(eLWing, -5.f, 24.f - fpy, 2.f, -fx * R2D, -fy * R2D, fz * R2D);
        part(eRWing, 5.f, 24.f - fpy, 2.f, -fx * R2D, fy * R2D, -fz * R2D);
        glBindTexture(GL_TEXTURE_2D, skinTex); glPopMatrix();
    }
    glPopMatrix(); glDisable(GL_ALPHA_TEST); gEntHurt = 0.f;
}
// Boneco do inventário (GuiInventory.drawEntityOnScreen do Java): pés em (51,75) do painel, escala 30, projeção ortográfica.
// Cabeça e corpo giram até o ponto exato do mouse: o ângulo é medido a partir dos olhos do boneco (plano virtual a 40 unidades),
// a cabeça faz o ângulo total e o corpo acompanha com metade dele.
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
static float gPreviewAura = 0.f;   // patchUI.1: >0 = espessura (unidades da GUI) do contorno rigido em volta da silhueta (stencil dilatado)
static float gPreviewDist = 40.f;   // distancia virtual do "olhar" (GUI); >41 liga o modo Bedrock do titulo
static GLuint entShadowTex();
// patchSombra.4: sombra do pack (misc/shadow.png) no chao sob os pes do boneco da GUI (elipse achatada, sem depender do giro)
static void uiFeetShadow(float fx, float fy, float S, float yawDeg = 0.f) {   // patchSkin.4: elipse do tamanho da pegada dos pes (0,5 x 0,25 bloco) girada com o corpo
    const GLuint t = entShadowTex(); const float ca = fabsf(cosf(yawDeg * .0174533f)), sa = fabsf(sinf(yawDeg * .0174533f)); const float rw = (.36f * ca + .22f * sa) * S, rh = std::max(.07f * S, rw * .32f);
    glDisable(GL_DEPTH_TEST); glDisable(GL_ALPHA_TEST); glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, t); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLoadIdentity(); glColor4f(1.f, 1.f, 1.f, .6f);
    glBegin(GL_QUADS); glTexCoord2f(0, 0); glVertex2f(fx - rw, fy - rh); glTexCoord2f(1, 0); glVertex2f(fx + rw, fy - rh); glTexCoord2f(1, 1); glVertex2f(fx + rw, fy + rh); glTexCoord2f(0, 1); glVertex2f(fx - rw, fy + rh); glEnd();
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
static void drawPlayerPreview(float ox, float oy, float gs, float mx, float my, float fxU, float fyU, float scU, float cx, float cy, float cw, float ch) {
    int vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    const float W = (float)vp[2], H = (float)vp[3];
    const float S = scU * gs, fx = ox + fxU * gs, fy = oy + fyU * gs, eyeY = fy - 1.62f * S;
    glEnable(GL_SCISSOR_TEST); glScissor(vp[0] + (int)(ox + cx * gs), vp[1] + (int)(H - (oy + (cy + ch) * gs)), (int)(cw * gs), (int)(ch * gs));   // patchPad.1: scissor e absoluto na janela — soma a origem do viewport (split screen)
    glDepthMask(GL_TRUE); glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, W, H, 0, -1000, 1000);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    const float dx = (mx - fx) / gs, dy = (eyeY - my) / gs;   // unidades da GUI; dy > 0 = mouse acima dos olhos
    float yawTot = std::max(-80.f, std::min(80.f, atan2f(dx, gPreviewDist) * 57.2957795f));
    float pit = std::max(-45.f, std::min(45.f, atan2f(dy, gPreviewDist) * 57.2957795f));
    float bodyK = .5f;   // fracao do giro que vai para o corpo (o resto fica so na cabeca)
    if (gPreviewDist > 41.f) {   // estilo Bedrock (titulo): corpo e cabeca seguem o cursor de longe, com suavizacao
        static float sY = 0.f, sP = 0.f; static Uint32 sT = 0; const Uint32 tn = SDL_GetTicks(); const float dt = sT ? std::min(.1f, (tn - sT) / 1000.f) : 0.f; sT = tn;
        const float k = 1.f - expf(-dt * 14.f); sY += (yawTot - sY) * k; sP += (pit - sP) * k; yawTot = sY; pit = sP; bodyK = .7f;
    }
    Pose p; p.headYaw = yawTot * (1.f - bodyK); p.headPitch = pit; p.held = (gHeldSel >= 0 && gHeldSel < 9) ? inv[gHeldSel].id : 0; wornFromInv(p);
    auto place = [&](float ddx, float ddy) {
        glLoadIdentity(); glTranslatef(ddx, ddy, 0); glTranslatef(fx, fy, 0); glScalef(S, -S, S);   // y da tela cresce para baixo
        glRotatef(-atanf(dy / 40.f) * 20.f, 1, 0, 0);  // o corpo inclina com o mouse (GuiInventory: -atan(mouseY/40)*20)
        glRotatef(180, 0, 1, 0);                       // de frente para quem olha
        glRotatef(yawTot * bodyK, 0, 1, 0);
    };
    if (gPreviewAura > 0.f) {   // aura: silhueta dilatada no stencil (varias copias deslocadas), preenchida de branco, e o boneco normal por cima
        const float t = gPreviewAura * gs;
        glClear(GL_STENCIL_BUFFER_BIT); glEnable(GL_STENCIL_TEST); glStencilFunc(GL_ALWAYS, 1, 1); glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        for (int ring = 1; ring <= 2; ring++) for (int k = 0; k < 16; k++) { const float a = k * 6.2831853f / 16.f, r = t * ring / 2.f; place(cosf(a) * r, sinf(a) * r); drawPlayer(p); }
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilFunc(GL_EQUAL, 1, 1); glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glLoadIdentity(); glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D); glDisable(GL_ALPHA_TEST); glColor4f(1.f, 1.f, 1.f, 1.f);
        glBegin(GL_QUADS); glVertex2f(0.f, 0.f); glVertex2f(W, 0.f); glVertex2f(W, H); glVertex2f(0.f, H); glEnd();
        glDisable(GL_STENCIL_TEST); glEnable(GL_TEXTURE_2D); glEnable(GL_DEPTH_TEST); glClear(GL_DEPTH_BUFFER_BIT);
    }
    uiFeetShadow(fx, fy, S, yawTot * bodyK); glMatrixMode(GL_MODELVIEW);
    place(0.f, 0.f);
    drawPlayer(p);
    glDisable(GL_SCISSOR_TEST);
}
// patchSkin.1: boneco 3D da tela de skins: pes em (fxU,fyU), escala scU (pixels por bloco, em unidades da GUI), gira yawDeg; sem item/armadura; recorta em (cx,cy,cw,ch)
static void drawPlayerPreviewSpin(float gs, float fxU, float fyU, float scU, float yawDeg, float cx, float cy, float cw, float ch) {
    int vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    const float W = (float)vp[2], H = (float)vp[3], S = scU * gs, fx = fxU * gs, fy = fyU * gs;
    glEnable(GL_SCISSOR_TEST); glScissor(vp[0] + (int)(cx * gs), vp[1] + (int)(H - (cy + ch) * gs), (int)(cw * gs), (int)(ch * gs));
    glDepthMask(GL_TRUE); glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, W, H, 0, -1000, 1000);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    uiFeetShadow(fx, fy, S, yawDeg);
    glTranslatef(fx, fy, 0); glScalef(S, -S, S);
    glRotatef(180, 0, 1, 0); glRotatef(yawDeg, 0, 1, 0);
    Pose p; p.age = SDL_GetTicks() / 50.f;   // respiracao leve parada
    drawPlayer(p);
    glDisable(GL_SCISSOR_TEST);
}
// ---- 1a pessoa: o punho SEGURA o item (como no Minecraft moderno): cabo dentro do punho, item de pe, braco visivel ----
// Ferramentas/armas tem sprite na diagonal (cabo embaixo-esquerda, cabeca em cima-direita): giram 45 graus para ficar de pe.
static bool heldDiagonal(int id) {
    if (isTool(id) || id == IT_BOW || (isPk(id) && PK[id - PK0].file.compare(0, 16, "item/bow_pulling") == 0)) return true;
    if (isPk(id)) { const std::string& f = PK[id - PK0].file;
        if (f.compare(0, 5, "item/") == 0) for (const char* k : {"_sword", "_axe", "_pickaxe", "_shovel", "_hoe"}) { const size_t l = strlen(k); if (f.size() > l && f.compare(f.size() - l, l, k) == 0) return true; } }
    return false;
}
// (a pose antiga com braco segurando o item foi removida: 1a pessoa agora segue o ItemInHandRenderer do Minecraft)
// balanco da mao ao girar a camera (LocalPlayer.xBob/yBob do Minecraft): os "bobs" seguem a mira com metade da diferenca por tick (20/s)
static float wrap180(float a);
static float gHandBob[4] = {0, 0, 0, 0};   // pitchBobO, pitchBob, yawBobO, yawBob
static void handBobTick(float yaw, float pitch) {
    gHandBob[0] = gHandBob[1]; gHandBob[2] = gHandBob[3];
    gHandBob[1] += (pitch - gHandBob[1]) * .5f;
    gHandBob[3] = wrap180(gHandBob[3] + wrap180(yaw - gHandBob[3]) * .5f);
}
static void handBobApply(float yaw, float pitch, float pt) {   // chamado antes de drawHand (ItemInHandRenderer.renderHandsWithItems)
    const float pb = gHandBob[0] + (gHandBob[1] - gHandBob[0]) * pt, yb = wrap180(gHandBob[2] + wrap180(gHandBob[3] - gHandBob[2]) * pt);
    glRotatef(-(pitch - pb) * .1f, 1, 0, 0);   // aqui pitch positivo = olhar p/ cima e yaw cresce p/ a esquerda (inverso do Java): sinais trocados
    glRotatef(-wrap180(yaw - yb) * .1f, 0, 1, 0);
}
// ---- luz dinamica: item emissor (tocha, lava, glowstone...) na mao ou no chao ilumina em volta ----
static int itemEmit(int id) {
    if (id <= 0) return 0;
    const int xb = xbItemBlock(id);
    if (!xb && !isBlk(id)) return 0;
    return lightEmit(xb ? xb : id);
}
static int itemEmitC(int id) {   // itemEmit com cache por id (evita busca em mapa a cada frame)
    static int ids[8] = {-1, -1, -1, -1, -1, -1, -1, -1}, vals[8] = {}, nx = 0; static size_t pk = (size_t)-1;
    if (pk != PK.size()) { pk = PK.size(); for (int& i : ids) i = -1; }
    for (int i = 0; i < 8; i++) if (ids[i] == id) return vals[i];
    const int v = itemEmit(id); ids[nx] = id; vals[nx] = v; nx = (nx + 1) & 7; return v;
}
static void dynLightUpdate(const double* eye, const float* dir, int selSlot, double ex, double ey, double ez) {
    for (auto& d : gDyn) d[0] = d[1] = d[2] = d[3] = 0.f;
#ifdef BL_RPI
    return;   // Raspberry: luz dinamica proibida (item na mao/no chao nao ilumina)
#endif
    int n = 0;
    const int lv = (gGm == 3) ? 0 : std::max(itemEmitC(inv[selSlot].id), itemEmitC(offh.id));
    if (lv > 0) { gDyn[n][0] = (float)eye[0] + dir[0] * .3f; gDyn[n][1] = (float)eye[1] - .25f + dir[1] * .3f; gDyn[n][2] = (float)eye[2] + dir[2] * .3f; gDyn[n][3] = (float)lv; n++; }
    static int bi[3] = {-1, -1, -1}, bid[3] = {0, 0, 0}, frame = 0; static size_t lastN = (size_t)-1;   // 3 itens emissores mais proximos da camera (no raio de 40 blocos)
    const int keep = std::min(3, DYN_MAX - n);
    if (DROPS.empty() || keep <= 0) { bi[0] = bi[1] = bi[2] = -1; lastN = DROPS.size(); }
    else if (DROPS.size() != lastN || (++frame & 7) == 0) {   // varredura completa so a cada 8 frames (ou quando a lista muda); nos outros reaproveita os indices
        lastN = DROPS.size(); frame = 0;
        float bd[3] = {1e9f, 1e9f, 1e9f}; for (int k = 0; k < 3; k++) { bi[k] = -1; }
        for (int i = 0; i < (int)DROPS.size(); i++) {
        if (!itemEmitC(DROPS[i].id)) continue;
        const float dx = (float)(DROPS[i].x - ex), dy = (float)(DROPS[i].y - ey), dz = (float)(DROPS[i].z - ez), q = dx * dx + dy * dy + dz * dz;
        if (q > 1600.f) continue;
        for (int k = 0; k < 3; k++) if (q < bd[k]) { for (int m = 2; m > k; m--) { bd[m] = bd[m - 1]; bi[m] = bi[m - 1]; } bd[k] = q; bi[k] = i; break; }
        }
        for (int k = 0; k < 3; k++) bid[k] = bi[k] >= 0 ? DROPS[bi[k]].id : 0;
    }
    for (int k = 0; k < 3 && n < DYN_MAX; k++) if (bi[k] >= 0 && bi[k] < (int)DROPS.size() && DROPS[bi[k]].id == bid[k]) { const DropEnt& d = DROPS[bi[k]]; gDyn[n][0] = (float)d.x; gDyn[n][1] = (float)d.y + .25f; gDyn[n][2] = (float)d.z; gDyn[n][3] = (float)itemEmitC(d.id); n++; }
}

// ---- tocha na mao: modelo 3D real do bloco (haste + chama) + animacao (balanco, chama pulsando, brilho aditivo) ----
static int torchHeldXb(int id) { auto g = gXbItem.find(id); return g != gXbItem.end() && (g->second == XB_TORCH || g->second == XB_RTORCH) ? g->second : 0; }
static float torchFlicker(float t) { const float k = .5f + .5f * (sinf(t * 17.f) * .5f + sinf(t * 29.f + 1.f) * .3f + sinf(t * 7.3f + 2.f) * .2f); return std::min(1.f, std::max(0.f, k)); }
static const int TORCH_SEQ[18] = {0, 1, 2, 3, 4, 1, 2, 1, 3, 4, 2, 0, 4, 3, 0, 2, 0, 3};   // sequencia de torch.png.mcmeta do pack (frametime 3 ticks = 150 ms, sem interpolacao)
static int torchAnimTile() { if (gTorchAnimN <= 0) return -1; return gTorchAnim[TORCH_SEQ[(SDL_GetTicks() / 150u) % 18u] % gTorchAnimN]; }
static const std::vector<V>& torchMesh(int xb, int tileOv) {   // malha do modelo (cache por bloco + tile do quadro)
    static std::map<std::pair<int, int>, std::vector<V>> cache; std::vector<V>& m = cache[{xb, tileOv}];
    if (m.empty()) for (const XQuad& q : gXM[xb - XB0]) {
        float u0, v0, u1, v1; tileUV(tileOv >= 0 ? tileOv : q.tile, u0, v0, u1, v1);
        for (int k = 0; k < 4; k++) { const float fu = std::min(std::max(q.u[k], 0.f), 1.f), fv = std::min(std::max(q.v[k], 0.f), 1.f);
            m.push_back({q.p[k][0], q.p[k][1], q.p[k][2], u0 + fu / ATL, v0 + fv / ATLR, 1.f, 1.f, 1.f, 1.f}); }
    }
    return m;
}
static void drawTorchHand(int xb) {   // 1a e 3a pessoa: quem chama posiciona; aqui so desenha o quadro atual da textura animada + brilho
    const std::vector<V>& m = torchMesh(xb, xb == XB_TORCH ? torchAnimTile() : -1);
    if (m.empty()) return;
    const float t = SDL_GetTicks() * .001f, fl = torchFlicker(t);
    glBindTexture(GL_TEXTURE_2D, atlasTex); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .02f);
    drawMesh(m);   // modelo base (sempre aceso, sem sombreamento)
    glBlendFunc(GL_ONE, GL_ONE);   // brilho da chama: segunda passada aditiva, pulsando, um pouco maior ao redor da ponta
    static std::vector<V> glow; glow = m; const float g = .06f + .16f * fl; for (V& v : glow) { v.r *= g; v.g *= g * .9f; v.b *= g * .7f; }
    glPushMatrix(); glTranslatef(.5f, .66f, .5f); const float sc = 1.f + .05f + .06f * fl; glScalef(sc, sc, sc); glTranslatef(-.5f, -.66f, -.5f); drawMesh(glow); glPopMatrix();
    glDisable(GL_ALPHA_TEST); glDisable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static void drawHeldBody(int id) {   // origem = punho (ja alinhado ao antebraco); tocha = modelo 3D real inclinado para frente/cima, resto = item normal
    const int tk = torchHeldXb(id);
    if (!tk) { drawHeld(id, false); return; }
    const float tt = SDL_GetTicks() * .001f;
    glPushMatrix(); glRotatef(32.f + sinf(tt * 1.9f) * 1.5f, 1, 0, 0);   // levanta a ponta (chama) acima do punho
    glScalef(.72f, .72f, .72f); glTranslatef(-.5f, -5.f / 16.f, -.5f);   // cabo seguro a ~5px da base
    drawTorchHand(tk); glPopMatrix();
}
static void drawHand(int held, float swing, float equip, int side = 1) {   // side: 1 = mao direita, -1 = esquerda (espelha como o Minecraft)
    const float sd = (float)side;   // 1a pessoa = ItemInHandRenderer do Minecraft
    glClear(GL_DEPTH_BUFFER_BIT); glEnable(GL_DEPTH_TEST);
    const float PI_ = 3.14159265f, PI2 = 6.2831853f;
    float sq = sqrtf(swing), f1 = sinf(sq * PI_), f5 = sinf(swing * swing * PI_);
    if (held) {   // item na mao: SEM braco (como no Minecraft)
        glPushMatrix();
        if (gEatOn && gEatT >= 0 && foodValue(held) > 0) {   // ItemRenderer.performEating (1.8.9): a comida sobe ate a boca e balanca
            const float fr = std::min(1.f, (SDL_GetTicks() - gEatMs) / 50.f), f = 32.f - (gEatT + fr) + 1.f, f1 = std::min(1.f, f / 32.f);
            float f2 = fabsf(cosf(f / 4.f * PI_) * .1f); if (f1 >= .8f) f2 = 0.f;
            glTranslatef(0, f2, 0);
            const float f3 = 1.f - powf(std::max(0.f, f1), 27.f);
            glTranslatef(f3 * .6f * sd, f3 * -.5f, 0); glRotatef(f3 * 90.f * sd, 0, 1, 0); glRotatef(f3 * 10.f, 1, 0, 0); glRotatef(f3 * 30.f * sd, 0, 0, 1);
        }
        // ItemInHandRenderer.renderArmWithItem (Minecraft 1.9+ / atual): translacao do golpe, posicao do braco, rotacao do golpe
        glTranslatef(-.4f * sd * sinf(sq * PI_), .2f * sinf(sq * PI2), -.2f * sinf(swing * PI_));
        glTranslatef(.56f * sd, -.52f + (1.f - equip) * -.6f, -.72f);
        glRotatef(sd * (45.f - f5 * 20.f), 0, 1, 0); glRotatef(sd * f1 * -20.f, 0, 0, 1); glRotatef(f1 * -80.f, 1, 0, 0); glRotatef(sd * -45.f, 0, 1, 0);
        if (gBowOn && gBowT >= 0 && (held == IT_BOW || (isPk(held) && PK[held - PK0].file.compare(0, 13, "item/bow_pull") == 0))) {   // arco sendo puxado (1.12: case BOW)
            glTranslatef(-.2785682f, .18344387f, .15731531f); glRotatef(-13.935f, 1, 0, 0); glRotatef(35.3f, 0, 1, 0); glRotatef(-9.785f, 0, 0, 1);
            float f12 = gBowT / 20.f; f12 = (f12 * f12 + f12 * 2.f) / 3.f; if (f12 > 1.f) f12 = 1.f;
            if (f12 > .1f) glTranslatef(0.f, sinf((gBowT - .1f) * 1.3f) * (f12 - .1f) * .004f, 0.f);   // tremor
            glTranslatef(0.f, 0.f, f12 * .04f); glScalef(1.f, 1.f, 1.f + f12 * .2f); glRotatef(-45.f, 0, 1, 0);
        }
        const int tkx = torchHeldXb(held);
        if (tkx) {   // tocha: balanca de leve na mao e inclina a chama para a frente
            const float tt = SDL_GetTicks() * .001f;
            glTranslatef(-.02f * sd, .06f, .02f); glRotatef(45.f * sd, 0, 1, 0); glRotatef(-18.f, 1, 0, 0);
            glRotatef(sinf(tt * 1.7f) * 1.6f, 0, 0, 1); glRotatef(sinf(tt * 2.3f) * 1.2f, 1, 0, 0);
            glScalef(.85f, .85f, .85f); glTranslatef(-.5f, -.2f, -.5f);
        } else if (isProp(held) && !propIsFlat(held) && propHeldKind(held) == 2) {   // arma 3D de mod
            propGunXf(held, true, sd);
        } else if (isBlockLike(held)) {
            // bloco: display firstperson_righthand = rot [0,45,0], escala .4
            glRotatef(45.f * sd, 0, 1, 0); glScalef(.4f, .4f, .4f);
            glTranslatef(-.5f, -.5f, -.5f);
        } else {
            // item plano/ferramenta (item/generated e handheld): rot [0,-90,25], trans [1.13,3.2,1.13], escala .68
            glTranslatef(1.13f * sd / 16, 3.2f / 16, 1.13f / 16);
            glRotatef(-90 * sd, 0, 1, 0); glRotatef(25 * sd, 0, 0, 1);
            glScalef(.68f, .68f, .68f);
            glTranslatef(-.5f, -.5f, -.5f);
        }
        if (tkx) drawTorchHand(tkx); else drawHeldRaw(held);
        glPopMatrix();
        return;
    }
    glBindTexture(GL_TEXTURE_2D, skinTex); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f);
    glPushMatrix();
    glTranslatef(-.3f * f1 + .64f, .4f * sinf(sq * PI2) - .6f - (1 - equip) * .6f, -.4f * sinf(swing * 3.14159f) - .72f);
    glRotatef(45, 0, 1, 0); glRotatef(f1 * 70, 0, 1, 0); glRotatef(f5 * -20, 0, 0, 1);
    glTranslatef(-1, 3.6f, 3.5f); glRotatef(120, 0, 0, 1); glRotatef(200, 1, 0, 0); glRotatef(-135, 0, 1, 0); glTranslatef(5.6f, 0, 0);
    glTranslatef(-5 / 16.f, (slim ? 2.5f : 2.f) / 16.f, 0); glScalef(-1 / 16.f, -1 / 16.f, 1 / 16.f);
    drawMesh(pRArm); drawMesh(pRSleeve);
    glPopMatrix(); glDisable(GL_ALPHA_TEST);
}

// contorno da caixa de selecao REAL acertada (coordenadas de mundo), expandida 0,002 como o drawSelectionBox do Java
static void wireBox(const AABB& bb) {
    const float e = .002f, x0 = (float)bb.x0 - e, y0 = (float)bb.y0 - e, z0 = (float)bb.z0 - e, x1 = (float)bb.x1 + e, y1 = (float)bb.y1 + e, z1 = (float)bb.z1 + e;
    const float c[8][3] = {{x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{x0,y0,z1},{x0,y1,z0},{x1,y1,z0},{x1,y1,z1},{x0,y1,z1}};
    static const int E[12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    glDisable(GL_TEXTURE_2D); glColor3f(0, 0, 0); glLineWidth(2);
    glBegin(GL_LINES);
    for (auto& e2 : E) for (int k = 0; k < 2; k++) glVertex3f(c[e2[k]][0], c[e2[k]][1], c[e2[k]][2]);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}
// patchStairs.1: contorno do CONJUNTO de caixas (escada = L inteiro, sem a linha interna do degrau). Voxeliza em 16^3 e desenha so arestas de quina (1,3 ou 2 diagonais de 4 celulas cheias).
static bool wireShape(int bx, int by, int bz) {
    AABB bb[SH_MAXB]; const int n = blockBoxes(bx, by, bz, true, bb);
    if (n < 2) return false;
    static bool F[18][18][18]; memset(F, 0, sizeof F);
    for (int i = 0; i < n; i++) {
        const int x0 = (int)lround((bb[i].x0 - bx) * 16), x1 = (int)lround((bb[i].x1 - bx) * 16), y0 = (int)lround((bb[i].y0 - by) * 16), y1 = (int)lround((bb[i].y1 - by) * 16), z0 = (int)lround((bb[i].z0 - bz) * 16), z1 = (int)lround((bb[i].z1 - bz) * 16);
        for (int x = std::max(0, x0); x < std::min(16, x1); x++) for (int y = std::max(0, y0); y < std::min(16, y1); y++) for (int z = std::max(0, z0); z < std::min(16, z1); z++) F[x + 1][y + 1][z + 1] = true;
    }
    auto f = [&](int x, int y, int z) { return F[x + 1][y + 1][z + 1] ? 1 : 0; };
    const float e = .002f, k = 1.f / 16.f;
    glDisable(GL_TEXTURE_2D); glColor3f(0, 0, 0); glLineWidth(2); glBegin(GL_LINES);
    for (int a = 0; a < 3; a++) {   // eixo da aresta; u,v = os outros dois
        const int u = (a + 1) % 3, v = (a + 2) % 3;
        for (int i = 0; i <= 16; i++) for (int j = 0; j <= 16; j++) for (int l = 0; l < 16; l++) {
            int c[4][3];   // as 4 celulas em volta da aresta que comeca no ponto (l no eixo a; i no u; j no v)
            for (int q = 0; q < 4; q++) { c[q][a] = l; c[q][u] = i - 1 + (q & 1); c[q][v] = j - 1 + (q >> 1); }
            const int m00 = f(c[0][0], c[0][1], c[0][2]), m10 = f(c[1][0], c[1][1], c[1][2]), m01 = f(c[2][0], c[2][1], c[2][2]), m11 = f(c[3][0], c[3][1], c[3][2]);
            const int sum = m00 + m10 + m01 + m11;
            const bool diag = sum == 2 && m00 == m11;
            if (!(sum == 1 || sum == 3 || diag)) continue;
            float p0[3], p1[3];
            p0[a] = l * k; p1[a] = (l + 1) * k; p0[u] = p1[u] = i * k; p0[v] = p1[v] = j * k;
            const double org[3] = {(double)bx, (double)by, (double)bz};
            for (int q = 0; q < 3; q++) {   // expande 0,002 nas pontas externas do bloco, como wireBox
                const float o0 = (p0[q] <= 0.f) ? -e : (p0[q] >= 1.f ? e : 0.f), o1 = (p1[q] <= 0.f) ? -e : (p1[q] >= 1.f ? e : 0.f);
                p0[q] = (float)(org[q] + p0[q]) + o0; p1[q] = (float)(org[q] + p1[q]) + o1;
            }
            glVertex3f(p0[0], p0[1], p0[2]); glVertex3f(p1[0], p1[1], p1[2]);
        }
    }
    glEnd(); glEnable(GL_TEXTURE_2D);
    return true;
}
static float wrap180(float a) { while (a > 180) a -= 360; while (a < -180) a += 360; return a; }


// EntityRenderer.setupViewBobbing do 1.8.9
static void applyViewBob(float dwm, float prevDwm, float cYaw, float pcYaw, float cPitch, float pcPitch, float pt) {
    if (!gViewBobOn) return;
    const float PI_ = 3.14159265f;
    float f = dwm - prevDwm, f1 = -(dwm + f * pt);
    float f2 = pcYaw + (cYaw - pcYaw) * pt, f3 = pcPitch + (cPitch - pcPitch) * pt;
    glTranslatef(sinf(f1 * PI_) * f2 * .5f, -fabsf(cosf(f1 * PI_) * f2), 0.f);
    glRotatef(sinf(f1 * PI_) * f2 * 3.f, 0, 0, 1);
    glRotatef(fabsf(cosf(f1 * PI_ - .2f) * f2) * 5.f, 1, 0, 0);
    glRotatef(f3, 1, 0, 0);
}

// patchRfl.1: sombra de tocha/lava nos pes. Ate 2 fontes de luz de bloco (nivel >= 12) a menos de 7 blocos projetam uma mancha escura macia no chao,
// afastada da fonte (mais comprida quanto mais perto da luz). Some ao ar livre de dia; so desenha quando ha luz de bloco proxima.
// patchSombra.1: sombra preta simples sob os pes (RenderManager.renderShadow do 1.8.9). So SEM shader (F7 desligado). Textura circular macia gerada no codigo.
static float gZoom = 1.f, gZoomTgt = .25f;   // patchZoom.1: multiplicador do FOV (1 = sem zoom; C = zoom)
static float lightNear(int x, int y, int z);   // definida em mining_sky.inc (patchSombra.2)
// patchSombra.2: sombra redonda igual ao Render.renderShadow do 1.12.2: textura 32x32 pixelada (GL_NEAREST, como textures/misc/shadow.png),
// alfa = (1 - dist_camera^2/256 - altura/2) * 0.5 * brilho da luz, so em cubos inteiros ate `size` abaixo dos pes (some ao pular alto), raio = shadowSize do renderer.
static GLuint entShadowTex() {
    static GLuint tex = 0;
    if (!tex) {
        std::vector<uint8_t> pk; int pw = 0, ph = 0;   // patchSombra.3: usa textures/misc/shadow.png do pack (como o Java) quando existir
        const bool fromPack = packRGBA("misc/shadow.png", pk, pw, ph) && pw >= 8 && pw == ph;
        static unsigned char px[32 * 32 * 4];
        for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) {
            const float u = x + .5f - 16.f, v = y + .5f - 16.f, r = sqrtf(u * u + v * v), a = r <= 14.2f ? 1.f : r <= 15.8f ? .5f : 0.f;   // disco pixelado, borda de 1 px mais clara
            unsigned char* q = px + (y * 32 + x) * 4; q[0] = q[1] = q[2] = 0; q[3] = (unsigned char)(a * 255.f);
        }
        glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (fromPack) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pw, ph, 0, GL_RGBA, GL_UNSIGNED_BYTE, pk.data());
        else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }
    return tex;
}
static void entBlobShadow(double ex, double ey, double ez, float size, double cx, double cy, double cz) {
    if (gShadowsOn) return;
    const double dx = ex - cx, dy = ey - cy, dz = ez - cz; const float f = (float)(1.0 - (dx * dx + dy * dy + dz * dz) / 256.0); if (f <= 0.f) return;
    const GLuint tex = entShadowTex();
    GLint prev = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
    glBindTexture(GL_TEXTURE_2D, tex); glEnable(GL_TEXTURE_2D); glDisable(GL_ALPHA_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-2.f, -2.f);
    const int x0 = (int)floor(ex - size), x1 = (int)floor(ex + size), z0 = (int)floor(ez - size), z1 = (int)floor(ez + size), yHi = (int)floor(ey), yLo = (int)floor(ey - size);
    glBegin(GL_QUADS);
    for (int bx = x0; bx <= x1; bx++) for (int bz = z0; bz <= z1; bz++) for (int by = yHi; by >= yLo; by--) {   // by = topo do cubo de baixo (blockpos.down() no Java)
        if (by < 1 || by >= CH) continue;
        const int b = getB(bx, by - 1, bz); if (b == AIR || (unsigned)b >= 65536u || gBlkColl[b] != SHAPE_CUBE) continue;
        const float br = lightNear(bx, by, bz); if (br < .05f) continue;   // Java: so desenha com luz > 3
        float a = (f - (float)(ey - by) * .5f) * .5f * br; if (a < 0.f) continue; if (a > 1.f) a = 1.f;
        const float X0 = std::max((float)bx, (float)(ex - size)), X1 = std::min((float)bx + 1.f, (float)(ex + size)), Z0 = std::max((float)bz, (float)(ez - size)), Z1 = std::min((float)bz + 1.f, (float)(ez + size));
        if (X1 <= X0 || Z1 <= Z0) continue;
        const float is = .5f / size, y = by + .015625f;
        glColor4f(1.f, 1.f, 1.f, a);
        glTexCoord2f((X0 - (float)ex) * is + .5f, (Z0 - (float)ez) * is + .5f); glVertex3f(X0, y, Z0);
        glTexCoord2f((X0 - (float)ex) * is + .5f, (Z1 - (float)ez) * is + .5f); glVertex3f(X0, y, Z1);
        glTexCoord2f((X1 - (float)ex) * is + .5f, (Z1 - (float)ez) * is + .5f); glVertex3f(X1, y, Z1);
        glTexCoord2f((X1 - (float)ex) * is + .5f, (Z0 - (float)ez) * is + .5f); glVertex3f(X1, y, Z0);
    }
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glColor4f(1.f, 1.f, 1.f, 1.f); glBindTexture(GL_TEXTURE_2D, (GLuint)prev);
}
// shadowSize dos renderers do 1.12.2, na ordem de MD[] (porco, galinha, vaca, ovelha, aldeao, porco-zumbi, zumbi, esqueleto, creeper, blaze, aranha, husk, aranha-da-caverna, vaca-cogumelo)
static float mobShadowSize(int t) { static const float S[14] = {.7f, .3f, .7f, .7f, .5f, .5f, .5f, .5f, .5f, .5f, 1.f, .5f, .7f, .7f}; return (unsigned)t < 14u ? S[t] : .5f; }
static void torchFootShadow(double px, double py, double pz, float sc = 1.f, float H = 1.8f) {
    if (!gShadowsOn) return;   // patchShadow.1: so com "Sombras e efeitos realistas" (F7) ligado
    const int bx = (int)floor(px), by = (int)floor(py), bz = (int)floor(pz);
    struct Ls { float x, y, z, d2; } best[2] = {{0, 0, 0, 1e9f}, {0, 0, 0, 1e9f}};
    for (int dx = -6; dx <= 6; dx++) for (int dz = -6; dz <= 6; dz++) for (int dy = -2; dy <= 4; dy++) {
        const int b = getB(bx + dx, by + dy, bz + dz); if (b == AIR || lightEmit(b) < 12) continue;
        const float lx = bx + dx + .5f, ly = by + dy + .5f, lz = bz + dz + .5f, ex = lx - (float)px, ez = lz - (float)pz, d2 = ex * ex + ez * ez + (ly - (float)py) * (ly - (float)py);
        if (d2 > 49.f) continue;
        if (d2 < best[0].d2) { best[1] = best[0]; best[0] = {lx, ly, lz, d2}; } else if (d2 < best[1].d2) best[1] = {lx, ly, lz, d2};
    }
    if (best[0].d2 > 1e8f) return;
    int gy = by; while (gy > 0 && !solid(getB(bx, gy - 1, bz))) gy--; if (by - gy > 2) return;   // so com o chao logo abaixo
    bool open = true; for (int yy = by + 1; yy < std::min(CH, by + 24); yy++) if (solid(getB(bx, yy, bz))) { open = false; break; }
    const float dayK = open ? gSkyScale : 0.f; const float vis = 1.f - .9f * dayK; if (vis < .08f) return;
    glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(-2.f, -2.f);
    for (auto& L : best) {   // patchShadow.1: projecao real do corpo (altura H) para um ponto de luz: a ponta cai a h*H/(Hl-H) do pe, afastada da luz, alargando como a perspectiva
        if (L.d2 > 1e8f) break;
        const float Hl = L.y - (float)gy; if (Hl < .2f) continue;
        float hx = (float)px - L.x, hz = (float)pz - L.z; float h = sqrtf(hx * hx + hz * hz); if (h < .05f) continue; hx /= h; hz /= h;
        const float prox = std::max(0.f, 1.f - sqrtf(L.d2) / 7.f), a = .6f * prox * vis;
        const float tip = Hl > H + .3f ? std::min(5.f, std::max(.3f, h * H / (Hl - H))) : 5.f, wb = .3f * sc, wt = wb * (Hl > H + .3f ? std::min(2.5f, Hl / (Hl - H)) : 2.5f), cy = (float)gy + .02f;
        const float qx = -hz, qz = hx; const int N = 8;
        glBegin(GL_QUADS);
        for (int k = 0; k < N; k++) {
            const float t0 = (float)k / N, t1 = (float)(k + 1) / N, d0 = t0 * tip, d1 = t1 * tip, w0 = wb + (wt - wb) * t0, w1 = wb + (wt - wb) * t1;
            const float a0 = a * (1.f - t0 * t0), a1 = a * (1.f - t1 * t1);
            const float c0x = (float)px + hx * d0, c0z = (float)pz + hz * d0, c1x = (float)px + hx * d1, c1z = (float)pz + hz * d1;
            for (int sd = -1; sd <= 1; sd += 2) {   // duas metades (esq/dir): centro opaco, borda transparente (penumbra)
                glColor4f(0.f, 0.f, 0.f, a0); glVertex3f(c0x, cy, c0z);
                glColor4f(0.f, 0.f, 0.f, 0.f); glVertex3f(c0x + qx * w0 * sd, cy, c0z + qz * w0 * sd);
                glColor4f(0.f, 0.f, 0.f, 0.f); glVertex3f(c1x + qx * w1 * sd, cy, c1z + qz * w1 * sd);
                glColor4f(0.f, 0.f, 0.f, a1); glVertex3f(c1x, cy, c1z);
            }
        }
        glEnd();
    }
    glDisable(GL_POLYGON_OFFSET_FILL); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
}

// patchEmber.1: brasas subindo no Nether (so com o shader do Nether ligado): 48 faiscas aditivas em cruz de 2 quadrados, posicao deterministica pelo tempo
static void netherEmbers(double px, double py, double pz, float t) {
    glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glDepthMask(GL_FALSE);
    glBegin(GL_QUADS);
    for (int i = 0; i < 48; i++) {
        auto hs = [&](float k) { const float v = sinf(i * 12.9898f + k * 78.233f) * 43758.5453f; return v - floorf(v); };
        const float ph = hs(1.f) + t * (.05f + .05f * hs(2.f)); const float f = ph - floorf(ph);
        const float x = (float)px + (hs(3.f) - .5f) * 26.f + sinf(t * .8f + i) * .7f, z = (float)pz + (hs(4.f) - .5f) * 26.f + cosf(t * .6f + i * 1.3f) * .7f, y = (float)py - 5.f + f * 20.f;
        const float a = sinf(f * 3.14159f) * .85f, s = .03f + .02f * hs(5.f);
        glColor4f(1.f, .3f + .35f * hs(6.f), .05f, a);
        glVertex3f(x - s, y - s, z); glVertex3f(x + s, y - s, z); glVertex3f(x + s, y + s, z); glVertex3f(x - s, y + s, z);
        glVertex3f(x, y - s, z - s); glVertex3f(x, y - s, z + s); glVertex3f(x, y + s, z + s); glVertex3f(x, y + s, z - s);
    }
    glEnd();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glColor4f(1.f, 1.f, 1.f, 1.f);
}
