// ================= desenho do personagem =================
struct Pose { float hurt = 0; float headYaw = 0, headPitch = 0, limb = 0, amp = 0, atk = 0, age = 0; float bow = -1.f; int held = 0; bool sneak = false, sit = false, oar = false; float row[2] = {0, 0}, rowAmp[2] = {0, 0}; int worn[4] = {0, 0, 0, 0}; };   // worn = ids de armadura (capacete, peitoral, calcas, botas)
static void wornFromInv(Pose& p) { for (int i = 0; i < 4; i++) p.worn[i] = armor[i].id; }
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
static void drawPlayer(const Pose& p) {   // origem = pés, olhando para -Z
    const float PI_ = 3.14159265f, R2D = 57.2957795f;
    glBindTexture(GL_TEXTURE_2D, skinTex); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f);
    gEntHurt = p.hurt;   // flash vermelho (so o corpo/armadura; o item na mao volta ao normal)
    glPushMatrix(); glScalef(.9375f, .9375f, .9375f);
    if (p.sneak) glTranslatef(0, -.2f, 0);   // ModelBiped.render: translate(0, 0.2, 0) ao agachar (antes o corpo ficava flutuando)

    const float ph = p.limb * .6662f;
    float armRx = cosf(ph + PI_) * 2.f * p.amp * .5f, armLx = cosf(ph) * 2.f * p.amp * .5f;
    float legRx = cosf(ph) * 1.4f * p.amp,            legLx = cosf(ph + PI_) * 1.4f * p.amp;
    float armRy = 0, armLy = 0, armRz = 0, armLz = 0, bodyX = 0, bodyY = 0, lean = 0;   // lean = inclinação do tronco inteiro (corpo+braços+cabeça) em torno do QUADRIL (remar)
    float armRpx = -5, armLpx = 5, armRpz = 0, armLpz = 0;
    float legPy = 12, legPz = 0, headPy = 0;
    const float headX = -p.headPitch / R2D;   // Java: positivo = olhando para baixo

    // ModelBiped.isRiding (montado no bote): braços -36° e pernas para a frente, abertas 18° (Y) / 4,5° (Z)
    const float sitRy = p.sit ? PI_ / 10.f : 0.f, sitRz = p.sit ? .07853982f : 0.f;
    if (p.sit) { armRx -= PI_ / 5.f; armLx -= PI_ / 5.f; legRx = legLx = -1.4137167f; }
    if (p.held) armRx = armRx * .5f - PI_ / 10.f;
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
        glBindTexture(GL_TEXTURE_2D, armorTex[(id - IT_A0) / 4][layer]); part(m, px, py, pz, rx, ry, rz); glBindTexture(GL_TEXTURE_2D, skinTex);
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
    glPopMatrix();
    glPushMatrix(); glTranslatef(0, 12 / 16.f, 0); glRotatef(-lean * R2D, 1, 0, 0); glTranslatef(0, -12 / 16.f, 0); glTranslatef(0, (24 - headPy) / 16.f, 0); glRotatef(lean * R2D * .55f, 1, 0, 0); glRotatef(p.headYaw, 0, 1, 0); glRotatef(p.headPitch, 1, 0, 0);
    glPushMatrix(); glScalef(1 / 16.f, 1 / 16.f, 1 / 16.f); drawMesh(pHead); drawMesh(pHat);
    if (p.worn[0] && armorOk(0, p.worn[0])) { glBindTexture(GL_TEXTURE_2D, armorTex[(p.worn[0] - IT_A0) / 4][0]); drawMesh(aHead); glBindTexture(GL_TEXTURE_2D, skinTex); } glPopMatrix(); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 12 / 16.f, 0); glRotatef(-lean * R2D, 1, 0, 0); glTranslatef(0, -12 / 16.f, 0); glTranslatef(-armRpx / 16.f, armY / 16.f, armRpz / 16.f);
    glRotatef(armRz * R2D, 0, 0, 1); glRotatef(-armRy * R2D, 0, 1, 0); glRotatef(-armRx * R2D, 1, 0, 0);
    glPushMatrix(); glScalef(1 / 16.f, 1 / 16.f, 1 / 16.f); drawMesh(pRArm); drawMesh(pRSleeve);
    if (p.worn[1] && armorOk(1, p.worn[1])) { glBindTexture(GL_TEXTURE_2D, armorTex[(p.worn[1] - IT_A0) / 4][0]); drawMesh(aRArm); glBindTexture(GL_TEXTURE_2D, skinTex); }
    glPopMatrix();
    if (p.held) { gEntHurt = 0.f; glRotatef(-90, 1, 0, 0); glTranslatef(1 / 16.f, .125f, -.625f); drawHeld(bowPullItemT(p.held, p.bow >= 0.f && !p.sit ? (int)p.bow : -1), false); glBindTexture(GL_TEXTURE_2D, skinTex); }
    glPopMatrix();
    glPopMatrix(); glDisable(GL_ALPHA_TEST); gEntHurt = 0.f;
}
// Boneco do inventário (GuiInventory.drawEntityOnScreen do Java): pés em (51,75) do painel, escala 30, projeção ortográfica.
// Cabeça e corpo giram até o ponto exato do mouse: o ângulo é medido a partir dos olhos do boneco (plano virtual a 40 unidades),
// a cabeça faz o ângulo total e o corpo acompanha com metade dele.
static void drawPlayerPreview(float ox, float oy, float gs, float mx, float my, float fxU, float fyU, float scU, float cx, float cy, float cw, float ch) {
    int vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    const float W = (float)vp[2], H = (float)vp[3];
    const float S = scU * gs, fx = ox + fxU * gs, fy = oy + fyU * gs, eyeY = fy - 1.62f * S;
    glEnable(GL_SCISSOR_TEST); glScissor((int)(ox + cx * gs), (int)(H - (oy + (cy + ch) * gs)), (int)(cw * gs), (int)(ch * gs));
    glDepthMask(GL_TRUE); glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, W, H, 0, -1000, 1000);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    const float dx = (mx - fx) / gs, dy = (eyeY - my) / gs;   // unidades da GUI; dy > 0 = mouse acima dos olhos
    const float yawTot = std::max(-80.f, std::min(80.f, atan2f(dx, 40.f) * 57.2957795f));
    const float pit = std::max(-45.f, std::min(45.f, atan2f(dy, 40.f) * 57.2957795f));
    glTranslatef(fx, fy, 0); glScalef(S, -S, S);   // y da tela cresce para baixo
    glRotatef(-atanf(dy / 40.f) * 20.f, 1, 0, 0);  // o corpo inclina com o mouse (GuiInventory: -atan(mouseY/40)*20)
    glRotatef(180, 0, 1, 0);                       // de frente para quem olha
    glRotatef(yawTot * .5f, 0, 1, 0);
    Pose p; p.headYaw = yawTot * .5f; p.headPitch = pit; p.held = (gHeldSel >= 0 && gHeldSel < 9) ? inv[gHeldSel].id : 0; wornFromInv(p);
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
static void drawHand(int held, float swing, float equip) {   // 1a pessoa = ItemInHandRenderer do Minecraft
    glClear(GL_DEPTH_BUFFER_BIT); glEnable(GL_DEPTH_TEST);
    const float PI_ = 3.14159265f, PI2 = 6.2831853f;
    float sq = sqrtf(swing), f1 = sinf(sq * PI_), f5 = sinf(swing * swing * PI_);
    if (held) {   // item na mao: SEM braco (como no Minecraft)
        glPushMatrix();
        // ItemInHandRenderer.renderArmWithItem (Minecraft 1.9+ / atual): translacao do golpe, posicao do braco, rotacao do golpe
        glTranslatef(-.4f * sinf(sq * PI_), .2f * sinf(sq * PI2), -.2f * sinf(swing * PI_));
        glTranslatef(.56f, -.52f + (1.f - equip) * -.6f, -.72f);
        glRotatef(45.f - f5 * 20.f, 0, 1, 0); glRotatef(f1 * -20.f, 0, 0, 1); glRotatef(f1 * -80.f, 1, 0, 0); glRotatef(-45.f, 0, 1, 0);
        if (gBowOn && gBowT >= 0 && (held == IT_BOW || (isPk(held) && PK[held - PK0].file.compare(0, 13, "item/bow_pull") == 0))) {   // arco sendo puxado (1.12: case BOW)
            glTranslatef(-.2785682f, .18344387f, .15731531f); glRotatef(-13.935f, 1, 0, 0); glRotatef(35.3f, 0, 1, 0); glRotatef(-9.785f, 0, 0, 1);
            float f12 = gBowT / 20.f; f12 = (f12 * f12 + f12 * 2.f) / 3.f; if (f12 > 1.f) f12 = 1.f;
            if (f12 > .1f) glTranslatef(0.f, sinf((gBowT - .1f) * 1.3f) * (f12 - .1f) * .004f, 0.f);   // tremor
            glTranslatef(0.f, 0.f, f12 * .04f); glScalef(1.f, 1.f, 1.f + f12 * .2f); glRotatef(-45.f, 0, 1, 0);
        }
        if (isBlockLike(held)) {
            // bloco: display firstperson_righthand = rot [0,45,0], escala .4
            glRotatef(45.f, 0, 1, 0); glScalef(.4f, .4f, .4f);
            glTranslatef(-.5f, -.5f, -.5f);
        } else {
            // item plano/ferramenta (item/generated e handheld): rot [0,-90,25], trans [1.13,3.2,1.13], escala .68
            glTranslatef(1.13f / 16, 3.2f / 16, 1.13f / 16);
            glRotatef(-90, 0, 1, 0); glRotatef(25, 0, 0, 1);
            glScalef(.68f, .68f, .68f);
            glTranslatef(-.5f, -.5f, -.5f);
        }
        drawHeldRaw(held);
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
static float wrap180(float a) { while (a > 180) a -= 360; while (a < -180) a += 360; return a; }


// EntityRenderer.setupViewBobbing do 1.8.9
static void applyViewBob(float dwm, float prevDwm, float cYaw, float pcYaw, float cPitch, float pcPitch, float pt) {
    const float PI_ = 3.14159265f;
    float f = dwm - prevDwm, f1 = -(dwm + f * pt);
    float f2 = pcYaw + (cYaw - pcYaw) * pt, f3 = pcPitch + (cPitch - pcPitch) * pt;
    glTranslatef(sinf(f1 * PI_) * f2 * .5f, -fabsf(cosf(f1 * PI_) * f2), 0.f);
    glRotatef(sinf(f1 * PI_) * f2 * 3.f, 0, 0, 1);
    glRotatef(fabsf(cosf(f1 * PI_ - .2f) * f2) * 5.f, 1, 0, 0);
    glRotatef(f3, 1, 0, 0);
}
