// ================= ponte de resource packs do Minecraft =================
// Le packs no formato Java (assets/minecraft/textures/...) direto de um .zip ou de uma pasta,
// em assets/resourcepacks/. Texturas de outras resolucoes sao reduzidas para 16x16 (atlas) e
// animacoes (tira vertical) usam o primeiro quadro. Nomes antigos (pre-1.13) tambem sao aceitos.
#include <map>
#include <string>
struct McPack {
    bool ok = false, zip = false; std::string dir, prefix, name;
    std::vector<uint8_t> blob; struct E { uint32_t off, cs, us; int method; }; std::map<std::string, E> ents;
    static bool readAll(const std::string& p, std::vector<uint8_t>& d) {
        FILE* f = fopen(p.c_str(), "rb"); if (!f) return false;
        uint8_t tmp[65536]; size_t r; d.clear();
        while ((r = fread(tmp, 1, sizeof tmp, f)) > 0) d.insert(d.end(), tmp, tmp + r);
        fclose(f); return true;
    }
    static uint32_t u16(const uint8_t* p) { return p[0] | p[1] << 8; }
    static uint32_t u32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
    bool open(const std::string& path) {
        ok = false; ents.clear(); blob.clear(); prefix.clear(); name = path;
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec)) { zip = false; dir = path; if (dir.back() != '/') dir += '/'; ok = true; return true; }
        zip = true; if (!readAll(path, blob) || blob.size() < 22) return false;
        size_t e = blob.size() - 22; bool found = false;
        for (size_t i = 0; i <= std::min<size_t>(e, 70000); i++) if (u32(&blob[e - i]) == 0x06054b50) { e -= i; found = true; break; }
        if (!found) return false;
        uint32_t n = u16(&blob[e + 10]), cd = u32(&blob[e + 16]);
        for (uint32_t k = 0; k < n && cd + 46 <= blob.size(); k++) {
            const uint8_t* h = &blob[cd]; if (u32(h) != 0x02014b50) break;
            uint32_t nl = u16(h + 28), xl = u16(h + 30), cl = u16(h + 32);
            if (cd + 46 + nl > blob.size()) break;
            std::string nm((const char*)h + 46, nl);
            ents[nm] = {u32(h + 42), u32(h + 20), u32(h + 24), (int)u16(h + 10)};
            cd += 46 + nl + xl + cl;
        }
        for (auto& kv : ents) { size_t p = kv.first.find("pack.mcmeta"); if (p != std::string::npos && (p == 0 || kv.first[p - 1] == '/')) { prefix = kv.first.substr(0, p); break; } }
        ok = !ents.empty(); return ok;
    }
    bool get(const std::string& rel, std::vector<uint8_t>& out) const {
        if (!ok) return false;
        if (!zip) return readAll(dir + rel, out);
        auto it = ents.find(prefix + rel); if (it == ents.end()) return false;
        const E& e = it->second; if ((size_t)e.off + 30 > blob.size() || u32(&blob[e.off]) != 0x04034b50) return false;
        size_t st = (size_t)e.off + 30 + u16(&blob[e.off + 26]) + u16(&blob[e.off + 28]);
        if (st + e.cs > blob.size()) return false;
        if (e.method == 0) { out.assign(blob.begin() + st, blob.begin() + st + e.cs); return true; }
        if (e.method != 8) return false;
        std::vector<uint8_t> in; in.reserve(e.cs + 2); in.push_back(0x78); in.push_back(0x9c); in.insert(in.end(), blob.begin() + st, blob.begin() + st + e.cs);
        return inflateZ(in.data(), in.size(), out);   // zip = deflate "cru"; o cabecalho zlib falso e ignorado pelo inflateZ
    }
    // nomes (sem ".png") dos PNGs diretamente dentro de rel (ex.: "assets/minecraft/textures/block/"), em ordem alfabetica; sem subpastas
    std::vector<std::string> listPng(const std::string& rel) const {
        std::vector<std::string> r; if (!ok) return r;
        if (!zip) {
            std::error_code ec;
            for (auto& e : std::filesystem::directory_iterator(dir + rel, ec)) { std::string n = e.path().filename().string(); if (!e.is_directory(ec) && n.size() > 4 && n.compare(n.size() - 4, 4, ".png") == 0) r.push_back(n.substr(0, n.size() - 4)); }
            std::sort(r.begin(), r.end()); return r;
        }
        const std::string pre = prefix + rel;
        for (auto it = ents.lower_bound(pre); it != ents.end() && it->first.compare(0, pre.size(), pre) == 0; ++it) {
            const std::string n = it->first.substr(pre.size());
            if (n.size() > 4 && n.find('/') == std::string::npos && n.compare(n.size() - 4, 4, ".png") == 0) r.push_back(n.substr(0, n.size() - 4));
        }
        return r;   // std::map ja esta ordenado
    }
};
static McPack gPack;
// PNG da pasta assets/minecraft/textures/<rel> do pack ativo
static bool packRGBA(const std::string& rel, std::vector<uint8_t>& px, int& w, int& h) {
    std::vector<uint8_t> d; if (!gPack.get("assets/minecraft/textures/" + rel, d)) return false;
    return loadPNGMem(d, px, w, h);
}
// reduz/amplia (vizinho mais proximo) para tw x th; media ponderada pelo alfa ao reduzir
static void scaleRGBA(const std::vector<uint8_t>& s, int w, int h, uint8_t* o, int tw, int th) {
    for (int y = 0; y < th; y++) for (int x = 0; x < tw; x++) {
        int x0 = x * w / tw, x1 = std::max(x0 + 1, (x + 1) * w / tw), y0 = y * h / th, y1 = std::max(y0 + 1, (y + 1) * h / th);
        if (w <= tw && h <= th) { memcpy(o + (y * tw + x) * 4, &s[((size_t)y0 * w + x0) * 4], 4); continue; }
        float r = 0, g = 0, b = 0, a = 0; int c = 0;
        for (int j = y0; j < y1; j++) for (int i = x0; i < x1; i++) { const uint8_t* p = &s[((size_t)j * w + i) * 4]; float al = p[3]; r += p[0] * al; g += p[1] * al; b += p[2] * al; a += al; c++; }
        uint8_t* q = o + (y * tw + x) * 4;
        if (a > 0) { q[0] = (uint8_t)(r / a); q[1] = (uint8_t)(g / a); q[2] = (uint8_t)(b / a); q[3] = (uint8_t)(a / c); } else q[0] = q[1] = q[2] = q[3] = 0;
    }
}
// carrega "block/xxx.png" / "item/xxx.png" (relativo a assets/minecraft/textures/) como tile 16x16 (tira vertical de animacao: 1o quadro)
static bool packTileRel(const std::string& rel, uint8_t* out) {
    std::vector<uint8_t> px; int w, h;
    if (!packRGBA(rel, px, w, h) || w < 1 || h < 1) return false;
    int S = std::min(w, h);
    std::vector<uint8_t> f((size_t)S * S * 4); for (int y = 0; y < S; y++) memcpy(&f[(size_t)y * S * 4], &px[(size_t)y * w * 4], (size_t)S * 4);
    scaleRGBA(f, S, S, out, 16, 16); return true;
}
static std::string gLastPack;   // ultimo "kind/nome" carregado com sucesso por packTile16
static std::unordered_map<std::string, int> gPackId;   // "block/stone" / "item/apple" -> id do jogo (bloco/item real, nao item extra do pack)
static std::unordered_set<std::string> gPackUsed;   // "block/xxx" / "item/xxx" ja ligados a um bloco/item do jogo (nao viram item extra)
// "kind" = "block" ou "item"; names = alternativas separadas por '|' (nome moderno primeiro, antigo depois)
static bool packTile16(const char* kind, const char* names, uint8_t* out) {
    std::string all = names; size_t a = 0;
    while (a <= all.size()) {
        size_t b = all.find('|', a); if (b == std::string::npos) b = all.size();
        std::string n = all.substr(a, b - a); a = b + 1;
        for (const char* suf : {"", "s"}) {
            if (!packTileRel(std::string(kind) + suf + "/" + n + ".png", out)) continue;
            gPackUsed.insert(std::string(kind) + "/" + n); gLastPack = std::string(kind) + "/" + n; return true;
        }
    }
    return false;
}
