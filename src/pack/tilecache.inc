// patchFast.3: cache em disco dos tiles 16x16 JA decodificados (pack + Chisel): nas proximas aberturas pula ~4000 PNGs/inflate.
// Chave = tamanho+data do pack ativo e do chisel.cfg; mudou qualquer um -> cache descartado e refeito sozinho. Arquivo: cache/tiles.bin
#include <sys/stat.h>
static std::unordered_map<std::string, std::array<uint8_t, 1025>> gTc;   // nome -> 1024 bytes RGBA + 1 byte (ok)
static std::string gTcKey; static bool gTcDirty = false, gTcLoaded = false;
static std::string tcStat(const std::string& p) { struct stat st; if (stat(p.c_str(), &st) != 0) return p + ":0"; return p + ":" + std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime); }
static void tcLoad() {
    if (gTcLoaded) return; gTcLoaded = true; if (getenv("BLOCKLAND_NOTILECACHE")) return;
    FILE* f = fopen("cache/tiles.bin", "rb"); if (!f) return;
    uint32_t mg = 0, kl = 0, n = 0; bool good = fread(&mg, 4, 1, f) == 1 && mg == 0x54433031u && fread(&kl, 4, 1, f) == 1 && kl == gTcKey.size() && kl < 4096;
    if (good) { std::string k(kl, 0); good = fread(&k[0], 1, kl, f) == kl && k == gTcKey && fread(&n, 4, 1, f) == 1 && n < 200000; }
    for (uint32_t i = 0; good && i < n; i++) {
        uint16_t nl = 0; if (fread(&nl, 2, 1, f) != 1 || nl == 0 || nl > 400) break; std::string nm(nl, 0); std::array<uint8_t, 1025> e;
        if (fread(&nm[0], 1, nl, f) != nl || fread(e.data(), 1, 1025, f) != 1025) break; gTc[nm] = e; }
    fclose(f);
}
static const std::array<uint8_t, 1025>* tcGet(const std::string& k) { auto it = gTc.find(k); return it == gTc.end() ? nullptr : &it->second; }
static void tcPut(const std::string& k, bool ok, const uint8_t* d) { std::array<uint8_t, 1025> e; if (ok) memcpy(e.data(), d, 1024); else memset(e.data(), 0, 1024); e[1024] = ok ? 1 : 0; gTc[k] = e; gTcDirty = true; }
static void tcSave() {
    if (!gTcDirty || getenv("BLOCKLAND_NOTILECACHE")) return; gTcDirty = false; std::error_code ec; std::filesystem::create_directories("cache", ec);
    FILE* f = fopen("cache/tiles.bin.tmp", "wb"); if (!f) return;
    const uint32_t mg = 0x54433031u, kl = (uint32_t)gTcKey.size(), n = (uint32_t)gTc.size(); fwrite(&mg, 4, 1, f); fwrite(&kl, 4, 1, f); fwrite(gTcKey.data(), 1, kl, f); fwrite(&n, 4, 1, f);
    for (auto& kv : gTc) { const uint16_t nl = (uint16_t)kv.first.size(); fwrite(&nl, 2, 1, f); fwrite(kv.first.data(), 1, nl, f); fwrite(kv.second.data(), 1, 1025, f); }
    fclose(f); std::filesystem::rename("cache/tiles.bin.tmp", "cache/tiles.bin", ec);
}
