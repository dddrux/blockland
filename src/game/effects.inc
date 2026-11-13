// ---------- EFEITOS DE STATUS (patchPot.1): sistema de efeitos do jogador (poções, 1.12.2). Usa survival.inc; chamado por tick.inc (fxTick) ----------
static void fxGive(int f, int dur, int amp) {   // instantaneos aplicam na hora; os demais mantem o mais forte/longo (PotionEffect.combine)
    if (f < 0 || f >= FX_N || gDead) return;   // patchPot.3: criativo tambem bebe (efeitos de tempo valem; cura/dano instantaneos nao)
    if (creative && (f == FX_HEAL || f == FX_HARM)) return;
    if (f == FX_HEAL) { gHP = std::min(20, gHP + (4 << std::min(amp, 3))); return; }
    if (f == FX_HARM) { survDamage(6 << std::min(amp, 3), "foi morto por magia", true); return; }
    if (gFxT[f] > 0 && (gFxA[f] > amp || (gFxA[f] == amp && gFxT[f] >= dur))) return;
    gFxT[f] = dur; gFxA[f] = amp;
}
static float fxMoveMul() { float m = 1.f; if (fxOn(FX_SPEED)) m *= 1.f + .2f * (gFxA[FX_SPEED] + 1); if (fxOn(FX_SLOW)) m *= std::max(0.f, 1.f - .15f * (gFxA[FX_SLOW] + 1)); return m; }
static double fxJump() { return .42 + (fxOn(FX_JUMP) ? .1 * (gFxA[FX_JUMP] + 1) : 0.); }
static int fxMelee(int dmg) { if (fxOn(FX_STR)) dmg += 3 * (gFxA[FX_STR] + 1); if (fxOn(FX_WEAK)) dmg -= 4 * (gFxA[FX_WEAK] + 1); return std::max(dmg, 0); }   // AttributeModifier 1.12.2: +3 / -4 por nivel
static void fxTick() {   // 1x por tick, logo depois de survTick
    gFxMoveMul = fxMoveMul();
    gFxNV = !fxOn(FX_NV) ? 0.f : gFxT[FX_NV] > 200 ? 1.f : .7f + sinf(gFxT[FX_NV] * 3.14159265f * .2f) * .3f;   // NightVision: pisca nos ultimos 10 s
    if (gDead) { fxClear(); return; }
    for (int i = 0; i < FX_N; i++) if (gFxT[i] > 0) --gFxT[i];
    if (creative) return;
    if (fxOn(FX_REGEN)) { const int iv = 50 >> std::min(gFxA[FX_REGEN], 5); if (iv > 0 && gFxT[FX_REGEN] % iv == 0 && gHP < 20) { ++gHP; gExh += .5f; } }
    if (fxOn(FX_POISON)) { const int iv = 25 >> std::min(gFxA[FX_POISON], 4); if (iv > 0 && gFxT[FX_POISON] % iv == 0 && gHP > 1) survDamage(1, "envenenado", true); }
    if (fxOn(FX_FIRE)) gFire = 0;
}
static bool potDrinkable(int id); static void potDrinkNow(Slot& s);   // definidos em potion_fx.inc
// HUD: lista de efeitos ativos no canto superior direito (nome + tempo)
static void fxDrawHud(int w, int h) {
    (void)h; int row = 0;
    for (int i = 0; i < FX_N; i++) if (gFxT[i] > 0) {
        char b[96]; const int s = gFxT[i] / 20; static const char* R[] = {"", " II", " III", " IV", " V"};
        snprintf(b, sizeof b, "%s%s %d:%02d", FX_NM[i], R[std::min(gFxA[i], 4)], s / 60, s % 60);
        static const unsigned FXC[FX_N] = {0x7CAFC6, 0x8AA3C0, 0x22FF4C, 0x6F6FFF, 0xB8BCD0, 0xE49A3A, 0x5C8CE0, 0xF82423, 0x9A2A28, 0x6FC24A, 0xFF7AD0, 0xD04040, 0x9AA09A, 0x66DD33};   // patchPot.3: cor do efeito (mesma familia da cor da pocao)
        uText((float)w - 130.f, 4.f + row * 10.f, b, FXC[i], true); row++;
    }
}
