// ---------- Cinzel (Chisel): botao direito num bloco do Chisel troca para a proxima variante do mesmo material (agachado: anterior) ----------
// Desgaste em Slot.d: ferro 250 usos, diamante 5000. Criativo nao gasta. Receita: lingote/diamante sobre graveto.
static int chiselMax(int id) { return id == gChiselDia ? 5000 : 250; }
static bool chiselUse(Slot& s, const float* eye, const float* dir, bool back) {
    if (!gChiselIron || (s.id != gChiselIron && s.id != gChiselDia)) return false;
    RayHit r; if (!rayCast(eye, dir, 5.f, r, false)) return true;
    const int b = getB(r.x, r.y, r.z);
    static const struct { int id; const char* mat; } VAN[] = {{STONE, "chisel/stone/"}, {COBBLE, "chisel/cobblestone/"}, {DIRT, "chisel/dirt/"}, {PLANKS, "chisel/planks-oak/"}, {BRICKS, "chisel/bricks/"},
        {SANDSTONE, "chisel/sandstoneyellow/"}, {OBSIDIAN, "chisel/obsidian/"}, {GLOWSTONE, "chisel/glowstone/"}, {BOOKSHELF, "chisel/bookshelf_oak/"}};   // vanilla entra no mesmo grupo (1o da lista)
    std::string mat; int van = 0;
    if (isPk(b)) { if (PK[b - PK0].file.rfind("chisel/", 0) != 0 || !PK[b - PK0].cube) return true; const std::string& f = PK[b - PK0].file; mat = f.substr(0, f.rfind('/') + 1); for (auto& v : VAN) if (mat == v.mat) van = v.id; }
    else { for (auto& v : VAN) if (v.id == b) { mat = v.mat; van = b; } if (mat.empty()) return true; }
    std::vector<int> grp; if (van) grp.push_back(van);   // variantes do material, na ordem do chisel.cfg (ids consecutivos)
    for (int i = (int)PK.size() - gChiselItems; i < (int)PK.size(); i++) if (PK[i].cube && PK[i].file.compare(0, mat.size(), mat) == 0) grp.push_back(PK0 + i);
    if (grp.size() < 2) return true;
    size_t k = 0; while (k < grp.size() && grp[k] != b) k++;
    const int nx = grp[(k + (back ? grp.size() - 1 : 1)) % grp.size()];
    setB(r.x, r.y, r.z, nx); netSendBlock(r.x, r.y, r.z, nx);
    if (!creative && ++s.d >= chiselMax(s.id)) s = Slot();
    return true;
}
