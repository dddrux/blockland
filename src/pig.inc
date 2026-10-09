// ================= porco (sem IA) =================
// Modelo = ModelPig do Minecraft Java (ModelQuadruped, pernas de 6): cabeça 8x8x8 em (0,12,-6) + focinho 4x3x1 (UV 16,16),
// corpo 10x16x8 (UV 28,8) deitado (rotação X de 90°) em (0,11,2), 4 pernas 4x6x4 (UV 0,16) em (±3,18,7) e (±3,18,-5).
// Medidas em pixels do modelo Java (y para baixo, chão em y=24). Aqui y é para cima, então y_nosso = 24 - y_java e as caixas viram [-y1, -y0].
// A textura vem do resource pack ativo: entity/pig/pig_temperate.png (1.21.5+) ou entity/pig/pig.png (antigo). Layout 64x64 ou 64x32.
static GLuint pigTex = 0; static bool pigTried = false;
static std::vector<V> pgHead, pgBody, pgLeg;
struct PigEnt { double x, y, z; float yaw; };   // yaw em graus; 0 = olhando para -Z (como o jogador)
static std::vector<PigEnt> PIGS;
static float gPlayerYaw = 0.f;                  // atualizado a cada quadro (usado por /porco)

static void buildPigModel() {
    pgHead.clear(); pgBody.clear(); pgLeg.clear();
    boxM(pgHead, -4,4, -4,4, -8,0, 0,0, 8,8,8, 0);        // cabeça
    boxM(pgHead, -2,2, -3,0, -9,-8, 16,16, 4,3,1, 0);     // focinho (java y 0..3)
    boxM(pgBody, -5,5, -6,10, -7,1, 28,8, 10,16,8, 0);    // corpo (java y -10..6, z -7..1), girado -90° em X ao desenhar
    boxM(pgLeg, -2,2, -6,0, -2,2, 0,16, 4,6,4, 0);        // perna (java y 0..6)
    // boxM sombreia pelas faces locais; o corpo é girado, então reatribui pela direção final:
    // [0..3] frente local = barriga, [4..7] costas local = dorso, [8..15] laterais, [16..19] topo local = peito, [20..23] base local = traseira
    for (size_t i = 0; i < pgBody.size(); i++) {
        float s = i < 4 ? .55f : i < 8 ? 1.f : i < 16 ? .7f : .85f;
        pgBody[i].r = pgBody[i].g = pgBody[i].b = s;
    }
}
static void uploadPigTex(const uint8_t* px, int w, int h) {
    if (!pigTex) glGenTextures(1, &pigTex);
    glBindTexture(GL_TEXTURE_2D, pigTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
}
static void loadPig() {   // precisa de contexto GL e do pack já aberto; chamado na 1ª vez que um porco é desenhado
    pigTried = true;
    std::vector<uint8_t> px; int w = 0, h = 0; bool fromPack = false;
    for (const char* n : {"entity/pig/pig_temperate.png", "entity/pig/pig.png"}) if (packRGBA(n, px, w, h)) { fromPack = true; break; }
    if (fromPack && (w < 64 || (h * 2 != w && h != w))) fromPack = false;   // só 64x32 / 64x64 (e múltiplos)
    if (!fromPack) {   // sem pack: textura rosa simples no mesmo layout (64x64)
        w = h = 64; px.assign(64 * 64 * 4, 0);
        auto R = [&](int x, int y, int rw, int rh, int r, int g, int b) { for (int j = 0; j < rh; j++) for (int i = 0; i < rw; i++) { uint8_t* p = &px[((y + j) * 64 + x + i) * 4]; p[0] = r; p[1] = g; p[2] = b; p[3] = 255; } };
        R(0, 0, 32, 16, 240, 160, 160); R(28, 8, 36, 24, 240, 160, 160); R(0, 16, 16, 16, 240, 160, 160); R(16, 16, 8, 4, 220, 120, 130);
        R(10, 12, 1, 1, 30, 20, 20); R(13, 12, 1, 1, 30, 20, 20);
    }
    uploadPigTex(px.data(), w, h);
    buildPigModel();
    const float vs = (float)w / (float)h;   // boxM normaliza por 64 nos dois eixos; textura 64x32 precisa de v * 2
    for (auto* m : {&pgHead, &pgBody, &pgLeg}) for (V& v : *m) v.v *= vs;
    printf("Porco: textura %dx%d %s\n", w, h, fromPack ? "do resource pack" : "(padrão, sem pack)");
}
static void drawPig(const PigEnt& e) {   // origem = pés, olhando para -Z
    if (!pigTried) loadPig();
    glBindTexture(GL_TEXTURE_2D, pigTex); glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f);
    glPushMatrix(); glTranslatef((float)e.x, (float)e.y, (float)e.z); glRotatef(e.yaw, 0, 1, 0);
    part(pgBody, 0, 13, 2, -90.f);   // pivô java (0,11,2); rotateAngleX = +90° do Java = -90° aqui
    part(pgHead, 0, 12, -6);         // pivô java (0,12,-6)
    part(pgLeg, -3, 6, 7); part(pgLeg, 3, 6, 7); part(pgLeg, -3, 6, -5); part(pgLeg, 3, 6, -5);   // pivôs java y=18
    glPopMatrix(); glDisable(GL_ALPHA_TEST);
}
static void drawPigs() { for (const PigEnt& e : PIGS) drawPig(e); }
static void spawnPig(const Phys& P, float yaw, float side = 0.f) {   // 2 blocos à frente do jogador (side = deslocamento lateral), no chão, virado para ele
    const float r = yaw * .0174533f;
    const double x = P.x - sin(r) * 2.0 + cos(r) * side, z = P.z - cos(r) * 2.0 - sin(r) * side;
    double y = P.y;
    for (int by = (int)floor(P.y) + 3; by >= 0; by--) if (solid(getB((int)floor(x), by, (int)floor(z)))) { y = by + 1.0; break; }
    PIGS.push_back({x, y, z, yaw + 180.f});
}
