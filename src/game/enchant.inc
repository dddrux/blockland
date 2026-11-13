// ---------- ENCANTAMENTOS (patchEnch.1): tabela completa do 1.12.2, guardada no nbt da pilha ("E:16=5,34=3"; id vanilla = nivel) ----------
// nbt ja e salvo (menu.inc "x <nbt>"), sincronizado (netSendNbt), nao empilha e acompanha drops. Efeitos: lotes seguintes (enchant_fx); brilho: render.
enum { EC_ARMOR = 1, EC_HEAD = 2, EC_FEET = 4, EC_SWORD = 8, EC_DIG = 16, EC_BOW = 32, EC_ROD = 64, EC_BREAK = 128, EC_AXE = 256, EC_ALL = 512 };
struct EnchType { int id; const char* key; const char* nm; int maxL, weight, cats; bool treasure, curse; };
static const EnchType EN_T[] = {
    { 0, "protection", "Proteção", 4, 10, EC_ARMOR, false, false}, { 1, "fire_protection", "Proteção Contra Fogo", 4, 5, EC_ARMOR, false, false},
    { 2, "feather_falling", "Peso-pena", 4, 5, EC_FEET, false, false}, { 3, "blast_protection", "Proteção Contra Explosões", 4, 2, EC_ARMOR, false, false},
    { 4, "projectile_protection", "Proteção Contra Projéteis", 4, 5, EC_ARMOR, false, false}, { 5, "respiration", "Respiração", 3, 2, EC_HEAD, false, false},
    { 6, "aqua_affinity", "Afinidade Aquática", 1, 2, EC_HEAD, false, false}, { 7, "thorns", "Espinhos", 3, 1, EC_ARMOR, false, false},
    { 8, "depth_strider", "Passos Profundos", 3, 2, EC_FEET, false, false}, { 9, "frost_walker", "Passos Gelados", 2, 2, EC_FEET, true, false},
    {10, "binding_curse", "Maldição de Vinculação", 1, 1, EC_ARMOR, true, true},
    {16, "sharpness", "Afiação", 5, 10, EC_SWORD | EC_AXE, false, false}, {17, "smite", "Julgamento", 5, 5, EC_SWORD | EC_AXE, false, false},
    {18, "bane_of_arthropods", "Ruína dos Artrópodes", 5, 5, EC_SWORD | EC_AXE, false, false}, {19, "knockback", "Repulsão", 2, 5, EC_SWORD, false, false},
    {20, "fire_aspect", "Aspecto Flamejante", 2, 2, EC_SWORD, false, false}, {21, "looting", "Pilhagem", 3, 2, EC_SWORD, false, false},
    {22, "sweeping", "Gume Varredor", 3, 2, EC_SWORD, false, false},
    {32, "efficiency", "Eficiência", 5, 10, EC_DIG, false, false}, {33, "silk_touch", "Toque Suave", 1, 1, EC_DIG, false, false},
    {34, "unbreaking", "Inquebrável", 3, 5, EC_BREAK, false, false}, {35, "fortune", "Fortuna", 3, 2, EC_DIG, false, false},
    {48, "power", "Poder", 5, 10, EC_BOW, false, false}, {49, "punch", "Impacto", 2, 2, EC_BOW, false, false},
    {50, "flame", "Chama", 1, 2, EC_BOW, false, false}, {51, "infinity", "Infinidade", 1, 1, EC_BOW, false, false},
    {61, "luck_of_the_sea", "Sorte do Mar", 3, 2, EC_ROD, false, false}, {62, "lure", "Isca", 3, 2, EC_ROD, false, false},
    {70, "mending", "Remendo", 1, 2, EC_BREAK, true, false}, {71, "vanishing_curse", "Maldição do Desaparecimento", 1, 1, EC_ALL, true, true},
};
static const int EN_N = (int)(sizeof EN_T / sizeof EN_T[0]);
static const EnchType* enchType(int id) { for (int i = 0; i < EN_N; i++) if (EN_T[i].id == id) return &EN_T[i]; return nullptr; }
static int enchByName(const std::string& q0) {   // chave em ingles ("sharpness"), nome pt-BR sem acento ("afiacao") ou id numerico; prefixo unico tambem vale; -1 = nao achou
    const std::string q = foldAscii(q0.c_str()); if (q.empty()) return -1;
    if (isdigit((unsigned char)q[0])) { const int v = atoi(q.c_str()); return enchType(v) ? v : -1; }
    int part = -1;
    for (int i = 0; i < EN_N; i++) { if (q == EN_T[i].key || q == foldAscii(EN_T[i].nm)) return EN_T[i].id; }
    for (int i = 0; i < EN_N; i++) { const std::string a = EN_T[i].key, b = foldAscii(EN_T[i].nm); if (a.compare(0, q.size(), q) == 0 || b.compare(0, q.size(), q) == 0) { if (part >= 0) return -2; part = EN_T[i].id; } }
    return part;
}
static std::string enchRoman(int n) { static const char* R[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X"}; return n >= 1 && n <= 10 ? R[n] : std::to_string(n); }
static bool enchIsNbt(const std::string& nbt) { return nbt.size() > 2 && nbt[0] == 'E' && nbt[1] == ':'; }
static std::vector<std::pair<int, int>> enchList(const std::string& nbt) {
    std::vector<std::pair<int, int>> v; if (!enchIsNbt(nbt)) return v;
    size_t p = 2; while (p < nbt.size()) { size_t q = nbt.find(',', p); if (q == std::string::npos) q = nbt.size();
        const std::string t = nbt.substr(p, q - p); const size_t e = t.find('='); if (e != std::string::npos) { const int id = atoi(t.c_str()), lv = atoi(t.c_str() + e + 1); if (lv > 0 && enchType(id)) v.push_back({id, lv}); } p = q + 1; }
    return v;
}
static std::string enchPack(const std::vector<std::pair<int, int>>& v) {
    if (v.empty()) return std::string(); std::string o = "E:"; for (size_t i = 0; i < v.size(); i++) { if (i) o += ","; o += std::to_string(v[i].first) + "=" + std::to_string(v[i].second); } return o;
}
// patchAnvil.2: trabalho anterior (RepairCost do 1.12) = sufixo "~wN" no nbt (N = vezes que o item passou pela bigorna, max 20); penalidade = 2^N - 1 niveis
static int enchWork(const std::string& nbt) { const size_t p = nbt.rfind("~w"); return p == std::string::npos ? 0 : std::max(0, std::min(20, atoi(nbt.c_str() + p + 2))); }
static std::string enchWithWork(std::string nbt, int w) { const size_t p = nbt.rfind("~w"); if (p != std::string::npos) nbt.resize(p); if (w > 0) nbt += "~w" + std::to_string(std::min(20, w)); return nbt; }
static int enchLv(const std::string& nbt, int id) { if (!enchIsNbt(nbt)) return 0; for (auto& pr : enchList(nbt)) if (pr.first == id) return pr.second; return 0; }
static int enchLv(const Slot& s, int id) { return s.id ? enchLv(s.nbt, id) : 0; }
static bool enchAny(const Slot& s) { return s.id && enchIsNbt(s.nbt); }
static int enchCatsOf(int id) {   // categorias do item (EC_*): decide quais encantamentos aceita
    if (id <= 0) return 0; int c = 0; const int as = armorSlotOf(id);
    if (as >= 0) { c |= EC_ARMOR | EC_BREAK; if (as == 0) c |= EC_HEAD; if (as == 3) c |= EC_FEET; }
    if (isSwordId(id)) c |= EC_SWORD | EC_BREAK;
    if (isDigToolId(id)) { c |= EC_DIG | EC_BREAK; if (id >= IT_WAXE && id <= IT_DAXE) c |= EC_AXE; }
    if (id == IT_BOW) c |= EC_BOW | EC_BREAK;
    if (id == shearsId() && id) c |= EC_DIG | EC_BREAK;
    if (isElytraItem(id)) c |= EC_BREAK;
    if (itemMaxDur(id) > 0) c |= EC_BREAK;
    return c;
}
static bool enchConflict(int a, int b) {
    if (a == b) return false;
    auto grp = [](int i) { if (i == 0 || i == 1 || i == 3 || i == 4) return 1; if (i >= 16 && i <= 18) return 2; if (i == 33 || i == 35) return 3; if (i == 8 || i == 9) return 4; if (i == 51 || i == 70) return 5; return 0; };
    return grp(a) && grp(a) == grp(b);
}
static bool enchCanApply(int itemId, int eid) {
    const EnchType* t = enchType(eid); if (!t) return false; const int c = enchCatsOf(itemId); if (!c) return false;
    if (t->cats & EC_ALL) return true;
    if (eid >= 16 && eid <= 18) return (c & (EC_SWORD | EC_AXE)) != 0;   // afiacao/julgamento/ruina: espada e machado
    if (eid == 34 || eid == 70) return (c & EC_BREAK) != 0;
    return (c & t->cats) != 0;
}
static bool enchAdd(Slot& s, int eid, int lv, bool force = false) {   // false = nao aplica (item errado / conflito); force ignora categoria e conflito (comando)
    const EnchType* t = enchType(eid); if (!t || !s.id || lv < 1) return false; if (!force && !enchCanApply(s.id, eid)) return false;
    if (!force) lv = std::min(lv, t->maxL); lv = std::min(lv, 255);
    std::vector<std::pair<int, int>> v = enchList(s.nbt); if (!s.nbt.empty() && !enchIsNbt(s.nbt)) return false;   // nbt de bau: nao mistura
    if (!force) for (auto& pr : v) if (enchConflict(pr.first, eid)) return false;
    bool f = false; for (auto& pr : v) if (pr.first == eid) { pr.second = lv; f = true; } if (!f) v.push_back({eid, lv});
    s.nbt = enchPack(v); return true;
}
static std::string enchTipNbt(const std::string& nbt) {   // linha do tooltip: "Afiação V, Inquebrável III" (ate 3 + "+N")
    const std::vector<std::pair<int, int>> v = enchList(nbt); std::string o;
    for (size_t i = 0; i < v.size() && i < 3; i++) { const EnchType* t = enchType(v[i].first); if (i) o += ", "; o += t->nm; if (t->maxL > 1 || v[i].second > 1) { o += " "; o += enchRoman(v[i].second); } }
    if (v.size() > 3) o += " +" + std::to_string(v.size() - 3); return o;
}
static Slot* gEnchHeld = nullptr;   // slot da mao (setado em tick.inc: sel e local do main)
// /enchant <nome|id|all|clear> [nivel]: encanta o item da mao (chat_debug.inc)
static std::string enchCommand(Slot& s, const std::vector<std::string>& w, bool& ok) {
    ok = false; if (!s.id) return "Segure um item na mão.";
    if (w.size() < 2) return "Uso: /enchant <nome|id|all|clear> [nível]   ex.: /enchant afiacao 5";
    const std::string a = foldAscii(w[1].c_str());
    if (a == "clear" || a == "limpar") { s.nbt.clear(); ok = true; return "Encantamentos removidos."; }
    if (a == "all" || a == "todos") { int n = 0; for (int i = 0; i < EN_N; i++) { if (EN_T[i].curse) continue; if (enchAdd(s, EN_T[i].id, EN_T[i].maxL)) n++; } ok = n > 0; return n ? std::to_string(n) + " encantamento(s) aplicado(s)." : "Esse item não aceita encantamentos."; }
    const int id = enchByName(w[1]); if (id == -2) return "Nome ambíguo: seja mais específico."; if (id < 0) return "Encantamento desconhecido.";
    const EnchType* t = enchType(id); const int lv = w.size() >= 3 ? std::max(1, atoi(w[2].c_str())) : t->maxL;
    if (!enchAdd(s, id, lv, true)) return "Não foi possível aplicar."; ok = true; return std::string(t->nm) + " " + enchRoman(lv) + " aplicado.";
}

// ---------- efeitos (patchEnch.3) ----------
static int enchHeldLv(int held, int eid) { return gEnchHeld && held && gEnchHeld->id == held ? enchLv(*gEnchHeld, eid) : 0; }   // encantamento do item da mao (so o jogador local)
static bool enchUnbreakSkip(const Slot& s, bool armorPiece) {   // Inquebrável: true = este ponto de desgaste nao conta (EnchantmentDurability.negateDamage)
    const int l = enchLv(s, 34); if (l <= 0) return false;
    if (armorPiece && (rand() % 100) < 60) return false;
    return rand() % (l + 1) > 0;
}
static int enchProtEpf(const char* why) {   // soma de nivel de protecao das 4 pecas (EnchantmentHelper.getEnchantmentModifierDamage): geral 1x, fogo/queda/explosao/projetil 2x/3x/2x/2x
    const std::string w = why ? why : ""; auto has = [&](const char* k) { return w.find(k) != std::string::npos; };
    const bool fire = has("fogo") || has("lava") || has("queimado") || has("Pegou"), fall = has("Caiu de uma altura"), blast = has("xplo"), proj = has("baleado") || has("flecha");
    int epf = 0;
    for (int k = 0; k < 4; k++) { if (!armor[k].id) continue;
        epf += enchLv(armor[k], 0); if (fire) epf += 2 * enchLv(armor[k], 1); if (fall) epf += 3 * enchLv(armor[k], 2); if (blast) epf += 2 * enchLv(armor[k], 3); if (proj) epf += 2 * enchLv(armor[k], 4); }
    return std::min(20, epf);
}
static int enchRespAir(int air) {   // patchEnch.3b: Respiracao (EntityLivingBase.decreaseAirSupply): com nivel N o ar so cai em 1 de cada N+1 ticks (aleatorio)
    const int l = enchLv(armor[0], 5); return (l > 0 && rand() % (l + 1) > 0) ? air : air - 1;
}
static float enchMineMul(bool headInWater, bool onGround) {   // patchEnch.3c: EntityPlayer.getDigSpeed 1.12: cabeca na agua sem Afinidade Aquatica /5; no ar /5
    float m = 1.f; if (headInWater && enchLv(armor[0], 6) <= 0) m *= .2f; if (!onGround) m *= .2f; return m;
}
static float enchDigSpeedMul(int held, float speed) {   // Eficiencia: +n^2+1 na velocidade quando a ferramenta ja e a certa (speed > 1)
    const int l = enchHeldLv(held, 32); return (l > 0 && speed > 1.f) ? speed + (float)(l * l + 1) : speed;
}
