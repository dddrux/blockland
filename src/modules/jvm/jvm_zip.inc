// ---------- jvm: leitor de .jar (zip) usando inflateZ de core/png.inc ----------
namespace jv {
    struct Jar {
        std::vector<uint8_t> raw; struct Ent { std::string name; uint16_t method; uint32_t csz, usz, off; };
        std::vector<Ent> ents;
        static uint32_t le(const uint8_t* p, int n) { uint32_t v = 0; for (int i = n - 1; i >= 0; i--) v = v << 8 | p[i]; return v; }
        bool open(const std::string& path) {
            FILE* f = fopen(path.c_str(), "rb"); if (!f) return false;
            fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); raw.resize(n);
            size_t got = fread(raw.data(), 1, n, f); fclose(f); if ((long)got != n || n < 22) return false;
            long e = n - 22; while (e >= 0 && le(&raw[e], 4) != 0x06054b50u) e--;
            if (e < 0) return false;
            int cnt = le(&raw[e + 10], 2); uint32_t off = le(&raw[e + 16], 4);
            for (int i = 0; i < cnt && off + 46 <= raw.size(); i++) {
                const uint8_t* p = &raw[off]; if (le(p, 4) != 0x02014b50u) return false;
                int nl = le(p + 28, 2), xl = le(p + 30, 2), cl = le(p + 32, 2);
                if (off + 46 + nl > raw.size()) return false;
                Ent en; en.method = le(p + 10, 2); en.csz = le(p + 20, 4); en.usz = le(p + 24, 4); en.off = le(p + 42, 4);
                en.name.assign((const char*)p + 46, nl); ents.push_back(en); off += 46 + nl + xl + cl;
            }
            return true;
        }
        bool read(const Ent& en, std::vector<uint8_t>& out) {
            if ((uint64_t)en.off + 30 > raw.size()) return false;
            const uint8_t* p = &raw[en.off]; uint32_t d = en.off + 30 + le(p + 26, 2) + le(p + 28, 2);
            if ((uint64_t)d + en.csz > raw.size()) return false;
            out.clear();
            if (en.method == 0) { out.assign(raw.begin() + d, raw.begin() + d + en.csz); return true; }
            if (en.method != 8) return false;
            std::vector<uint8_t> in(2 + en.csz); memcpy(&in[2], &raw[d], en.csz);   // 2 bytes falsos de cabecalho zlib
            return inflateZ(in.data(), in.size(), out);
        }
        const Ent* find(const std::string& nm) const { for (auto& e : ents) if (e.name == nm) return &e; return nullptr; }
    };
}
