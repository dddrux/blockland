// ---------- modulo jade (port de Jade 0.1.0 / Snownee, addon do Hwyla, MC 1.12.2) ----------
// J1: esqueleto/config/comando. J2: tooltip Hwyla do bloco/prop mirado (nome + mod). J3: baus/forno/cervejaria. J4: entidades (nome, vida, crescimento, amor, efeitos, sela/domado).
// Config: modulo_jade.cfg  (jade_on=1, jade_inv=1, jade_state=1, jade_mob=1, jade_potion=1, jade_horse=1, jade_growth=1, jade_frame=1)
namespace mod_jade {
    struct Opt { const char* key; const char* label; bool on; };
    // mesmas opcoes do en_us.lang do jar (option.*)
    static Opt OPT[] = {
        {"jade_on", "Jade", true}, {"jade_state", "Show states", true}, {"jade_inv", "Show inventory info", true},
        {"jade_horse", "Horse stat", true}, {"jade_potion", "Entity effects", true}, {"jade_frame", "Item frame", true},
        {"jade_llama", "Llama strength", true}, {"jade_growth", "Mob growth", true},
    };
    static const int NOPT = (int)(sizeof OPT / sizeof *OPT);
    static bool opt(const char* key) { for (Opt& o : OPT) if (!strcmp(o.key, key)) return o.on; return false; }
    static void load() { api::Cfg c("jade"); for (Opt& o : OPT) o.on = c.getInt(o.key, o.on ? 1 : 0) != 0; }
    static void save() { api::Cfg c("jade"); for (Opt& o : OPT) c.setInt(o.key, o.on ? 1 : 0); c.save(); }

    // ---- tooltip (estilo Hwyla: caixa escura + borda roxa = TTRenderBorder do jar) ----
    struct Tip { std::string name, mod; std::vector<std::pair<std::string, unsigned>> lines; };
    static std::string stackStr(const Slot& q) { const char* n = itemName(q.id); return std::string(n && *n ? n : "?") + (q.n > 1 ? " x" + std::to_string(q.n) : ""); }
    static std::string pct(int a, int b) { return std::to_string(b > 0 ? std::min(100, a * 100 / b) : 0) + "%"; }
    // ItemHandlerProvider: ate 5 pilhas distintas do bau (+ "e mais N"); BrewingStandProvider; forno (entrada/combustivel/resultado)
    static void extraBlock(Tip& t, int x, int y, int z, int, int) {
        if (opt("jade_inv")) {
            if (ChestInv* c = chestAt(x, y, z, false)) {
                int shown = 0, more = 0;
                for (const Slot& q : c->s) { if (!q.id || q.n <= 0) continue; if (shown < 5) { t.lines.push_back({stackStr(q), 0xDDDDDD}); shown++; } else more++; }
                if (!shown) t.lines.push_back({"(vazio)", 0x888888}); else if (more) t.lines.push_back({"... +" + std::to_string(more), 0x888888});
            }
            if (FurnData* f = furnAt(x, y, z, false)) {
                static const char* lb[3] = {"Entrada: ", "Combustivel: ", "Resultado: "};
                for (int i = 0; i < 3; i++) if (f->s[i].id && f->s[i].n > 0) t.lines.push_back({lb[i] + stackStr(f->s[i]), 0xDDDDDD});
                if (f->burn > 0) t.lines.push_back({"Queimando " + pct(f->burn, f->burnMax), 0xFFAA00});
                if (f->cook > 0) t.lines.push_back({"Progresso " + pct(f->cook, FURN_COOK), 0x55FF55});
            }
        }
        if (BrewData* b = brewAt(x, y, z, false)) {   // BrewingStandProvider
            if (b->brew > 0) t.lines.push_back({"Preparando " + pct(BREW_TIME - b->brew, BREW_TIME), 0x55FF55});
            t.lines.push_back({"Combustivel: " + std::to_string(b->fuel) + "/20", 0xFFAA00});
        }
    }
    static std::string niceMod(std::string s) { if (!s.empty()) s[0] = (char)toupper((unsigned char)s[0]); return s; }
    static std::string secs(int ticks) { return std::to_string((ticks + 19) / 20); }
    static std::string effStr(const char* nm, int amp, int ticks) { return std::string(nm) + " " + std::to_string(amp + 1) + " (" + std::to_string(ticks / 20 / 60) + ":" + (ticks / 20 % 60 < 10 ? "0" : "") + std::to_string(ticks / 20 % 60) + ")"; }
    // AgeableEntityProvider/BreedingProvider/PotionEffectsProvider + vida (Hwyla). Horse stat, Llama strength e Item frame: pendentes (MobEnt sem esses dados; sem item frame no jogo).
    static void extraMob(Tip& t, MobEnt& e) {
        t.lines.push_back({"Vida: " + std::to_string(std::max(0, e.hp)) + "/" + std::to_string(MD[e.type].hp), 0xFF5555});
        if (opt("jade_growth") && e.baby) t.lines.push_back({"Growing time: " + secs(24000 - e.babyAge) + "s", 0xDDDDDD});
        if (e.love > 0) t.lines.push_back({"Em modo amor", 0xFF55FF});
        else if (e.breedCd > 0 && !e.baby) t.lines.push_back({"Breeding cooldown: " + secs(e.breedCd) + "s", 0xDDDDDD});
        if (opt("jade_horse") && isHorse(e.type)) { if (e.tame) t.lines.push_back({"Domado", 0x55FF55}); if (e.saddled) t.lines.push_back({"Com sela", 0xDDDDDD}); }
        if (opt("jade_potion")) {
            if (e.potPoison > 0) t.lines.push_back({effStr("Veneno", e.potPoisonAmp, e.potPoison), 0x55AA00});
            if (e.potRegen > 0) t.lines.push_back({effStr("Regeneracao", 0, e.potRegen), 0xFF55FF});
            if (e.potSlow > 0) t.lines.push_back({effStr("Lentidao", e.potSlowAmp, e.potSlow), 0x8888FF});
            if (e.potSpeed > 0) t.lines.push_back({effStr("Velocidade", e.potSpeedAmp, e.potSpeed), 0x55FFFF});
        }
    }
    static bool aimMob(Tip& t, double blockT) {   // raio x caixa do mob (EntityLivingBase), mais perto que o bloco
        double o[3] = {gAimEye[0], gAimEye[1], gAimEye[2]}, d[3] = {gAimDir[0], gAimDir[1], gAimDir[2]};
        int best = -1; double bt = std::min((double)gBlockReach, blockT);
        for (size_t i = 0; i < MOBS.size(); i++) {
            const MobEnt& e = MOBS[i]; if (e.dead > 0) continue;
            const double hw = MD[e.type].w * .5 * (e.baby ? .5 : 1.), hh = MD[e.type].h * (e.baby ? .5 : 1.);
            const AABB b{e.x - hw, e.y, e.z - hw, e.x + hw, e.y + hh, e.z + hw}; double tt = 0; int f = 0;
            if (rayBox(o, d, b, bt, tt, f) && tt < bt) { bt = tt; best = (int)i; }
        }
        if (best < 0) return false;
        MobEnt& e = MOBS[best]; const char* nm = MD[e.type].nome;
        t.name = std::string(e.baby ? "Filhote - " : "") + niceMod(nm); t.mod = "Minecraft"; extraMob(t, e); return true;
    }
    static std::string modOfBlock(int id) { return id >= MOD0 && id < 256 ? "Mod" : "Minecraft"; }
    static bool aim(Tip& t) {   // acha o alvo mais proximo: prop (modelo de mod) ou bloco
        if (!gAimOk) return false;
        const float reach = gBlockReach;
        float tp = 1e9f; const int pi = gProps.empty() ? -1 : propPick(gAimEye, gAimDir, reach, &tp);
        RayHit r; const bool hb = rayCast(gAimEye, gAimDir, reach, r, false);
        if (aimMob(t, hb ? r.t : 1e9)) return true;   // mob mais perto que o bloco/prop
        if (pi >= 0 && (!hb || tp <= (float)r.t)) {
            int pk = -1; const PropItem* it = propItem(gProps[pi].id, &pk); if (!it) return false;
            t.name = it->name.empty() ? it->id : it->name;
            t.mod = pk >= 0 && pk < (int)gPropPacks.size() ? niceMod(gPropPacks[pk].mod.empty() ? gPropPacks[pk].modid : gPropPacks[pk].mod) : "Mod";
            return true;
        }
        if (!hb) return false;
        const int id = getB(r.x, r.y, r.z); if (id == AIR) return false;
        const char* nm = itemName(id); if (!nm || !*nm) return false;
        t.name = nm; t.mod = modOfBlock(id);
        extraBlock(t, r.x, r.y, r.z, id, getM(r.x, r.y, r.z));
        return true;
    }
    static void drawTip(int w, const Tip& t) {
        const float sc = 2.f, pad = 6.f, lh = 9.f * sc;
        float tw = std::max(api::hud::width(t.name, sc), api::hud::width(t.mod, sc * .85f));
        for (auto& l : t.lines) tw = std::max(tw, api::hud::width(l.first, sc));
        const float bw = tw + pad * 2, bh = pad * 2 + lh * (2 + (float)t.lines.size()), x = (w - bw) * .5f, y = 8.f;
        api::hud::rect(x, y, bw, bh, 0x100010, .82f);
        api::hud::rect(x, y, bw, 2, 0x5000FF, .9f); api::hud::rect(x, y + bh - 2, bw, 2, 0x28007F, .9f);   // borda roxa
        api::hud::rect(x, y, 2, bh, 0x5000FF, .9f); api::hud::rect(x + bw - 2, y, 2, bh, 0x28007F, .9f);
        float cy = y + pad;
        api::hud::text(x + pad, cy, sc, t.name, 0xFFFFFF); cy += lh;
        api::hud::text(x + pad, cy, sc * .85f, t.mod, 0x5555FF); cy += lh;
        for (auto& l : t.lines) { api::hud::text(x + pad, cy, sc, l.first, l.second); cy += lh; }
    }
    static void onInit() { load(); }
    static void onDrawHud(int w, int h) {
        if (!opt("jade_on") || gModHudBlocked || gScreen != SC_PLAY) return;
        Tip t; if (!aim(t)) return;
        api::hud::begin(w, h); drawTip(w, t); api::hud::end();
    }

    static bool cmdJade(const std::vector<std::string>& a, Phys&) {
        if (a.empty()) { OPT[0].on = !OPT[0].on; save(); api::chat::ok(std::string("Jade: ") + (OPT[0].on ? "ligado" : "desligado")); return true; }
        const std::string k = a[0];
        if (k == "lista") { for (const Opt& o : OPT) api::chat::say(std::string(o.on ? "[x] " : "[ ] ") + o.key + " - " + o.label); return true; }
        for (Opt& o : OPT) if (k == o.key || k == std::string(o.key).substr(5)) { o.on = a.size() > 1 ? (a[1] == "1" || a[1] == "on") : !o.on; save(); api::chat::ok(std::string(o.key) + (o.on ? " = 1" : " = 0")); return true; }
        return false;
    }
    static const api::Cmd CMDS[] = { {"jade", "/jade [lista|<opcao> [0|1]]", cmdJade} };
    static bool onCommand(const std::string& cmd, const std::vector<std::string>& args, Phys& P) { return api::dispatch(CMDS, sizeof CMDS / sizeof *CMDS, cmd, args, P); }
}
GAME_MODULE_REGISTER(jade, "jade", mod_jade::onInit, nullptr, nullptr, mod_jade::onDrawHud, mod_jade::onCommand, nullptr, nullptr);
