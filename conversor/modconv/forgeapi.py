"""patchForge.1: ENGENHARIA REVERSA do Forge 1.12.2 a partir do proprio jar (forge-1.12.2-*-universal.jar).
Gera db/forge_api.json.gz = catalogo de TUDO que o Forge expoe: classes, supers, interfaces, metodos, campos, anotacoes (@Mod, @SubscribeEvent, @Cancelable, @HasResult...), eventos e,
pelo deobfuscation_data-1.12.2.lzma do jar, todas as classes e nomes SRG (func_/field_) do Minecraft. O conversor usa o catalogo para classificar cada chamada de um mod com precisao
(chamada existe no Forge? e evento? cancelavel? e do Minecraft? de outra versao?). Uso: python3 forgeapi.py <forge-universal.jar> [saida.json.gz]"""
import gzip, json, lzma, os, re, struct, sys, zipfile
DB = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'db', 'forge_api.json.gz')
def _ann_val(d, p, cp):
    t = chr(d[p]); p += 1
    if t in 'BCDFIJSZ': v = cp[struct.unpack('>H', d[p:p + 2])[0]]; return v, p + 2
    if t == 's': return cp[struct.unpack('>H', d[p:p + 2])[0]], p + 2
    if t == 'e': return cp[struct.unpack('>H', d[p + 2:p + 4])[0]], p + 4
    if t == 'c': return cp[struct.unpack('>H', d[p:p + 2])[0]], p + 2
    if t == '@': return _ann(d, p, cp)
    if t == '[':
        n = struct.unpack('>H', d[p:p + 2])[0]; p += 2; L = []
        for _ in range(n): v, p = _ann_val(d, p, cp); L.append(v)
        return L, p
    raise ValueError('anotacao ' + t)
def _ann(d, p, cp):
    ty = cp[struct.unpack('>H', d[p:p + 2])[0]]; n = struct.unpack('>H', d[p + 2:p + 4])[0]; p += 4; el = {}
    for _ in range(n):
        k = cp[struct.unpack('>H', d[p:p + 2])[0]]; v, p = _ann_val(d, p + 2, cp); el[k] = v
    return (ty, el), p
def _attrs(d, p, cp, want='RuntimeVisibleAnnotations'):
    """le a lista de atributos em p -> (anotacoes, novo p). Anotacao = (tipo 'Lx/y;', {elem: valor})."""
    out = []; n = struct.unpack('>H', d[p:p + 2])[0]; p += 2
    for _ in range(n):
        nm = cp[struct.unpack('>H', d[p:p + 2])[0]]; ln = struct.unpack('>I', d[p + 2:p + 6])[0]; q = p + 6
        if nm == want:
            k = struct.unpack('>H', d[q:q + 2])[0]; q += 2
            for _ in range(k): a, q = _ann(d, q, cp); out.append(a)
        p += 6 + ln
    return out, p
def parse_class(d):
    """.class -> dict(name, super, ifaces, flags, fields[(nome,desc,flags)], methods[(nome,desc,flags,anots)], anots) ou None. Parser completo (nao so a tabela de constantes)."""
    try:
        if d[:4] != b'\xca\xfe\xba\xbe': return None
        n = struct.unpack('>H', d[8:10])[0]; p = 10; cp = [None] * n; cls = {}; i = 1
        while i < n:
            t = d[p]; p += 1
            if t == 1: ln = struct.unpack('>H', d[p + 0:p + 2])[0]; cp[i] = d[p + 2:p + 2 + ln].decode('utf-8', 'replace'); p += 2 + ln
            elif t == 3: cp[i] = struct.unpack('>i', d[p:p + 4])[0]; p += 4
            elif t == 4: p += 4
            elif t in (5, 6): p += 8; i += 1
            elif t in (7, 8, 16, 19, 20): cls[i] = struct.unpack('>H', d[p:p + 2])[0]; p += 2
            elif t in (9, 10, 11, 12, 17, 18): p += 4
            elif t == 15: p += 3
            else: return None
            i += 1
        cn = lambda k: cp[cls[k]] if k in cls else ''
        fl = struct.unpack('>H', d[p:p + 2])[0]; name = cn(struct.unpack('>H', d[p + 2:p + 4])[0]); sup = cn(struct.unpack('>H', d[p + 4:p + 6])[0])
        ni = struct.unpack('>H', d[p + 6:p + 8])[0]; p += 8; ifs = [cn(struct.unpack('>H', d[p + 2 * j:p + 2 * j + 2])[0]) for j in range(ni)]; p += 2 * ni
        mem = []
        for _ in range(2):   # campos, metodos
            k = struct.unpack('>H', d[p:p + 2])[0]; p += 2; L = []
            for _ in range(k):
                f, a, b = struct.unpack('>HHH', d[p:p + 6]); an, p = _attrs(d, p + 6, cp); L.append((cp[a], cp[b], f, an))
            mem.append(L)
        an, p = _attrs(d, p, cp)
        return dict(name=name, super=sup, ifaces=ifs, flags=fl, fields=[x[:3] for x in mem[0]], methods=mem[1], anots=an)
    except Exception: return None
def short(t): return t[1:-1].rsplit('/', 1)[-1] if t and t[0] == 'L' else t   # 'Lnet/x/Mod;' -> 'Mod'
def build(forge_jar):
    z = zipfile.ZipFile(forge_jar); cl = {}; obf = {}; mc_cls = set(); mc_names = set()
    if 'deobfuscation_data-1.12.2.lzma' in z.namelist():   # notch -> SRG: o jar do Forge referencia o Minecraft por nomes ofuscados (super 'cgb')
        for ln in lzma.decompress(z.read('deobfuscation_data-1.12.2.lzma')).decode('utf-8', 'replace').split('\n'):
            t = ln.split(' ')
            if ln.startswith('CL:') and len(t) >= 3: mc_cls.add(t[2]); obf[t[1]] = t[2]
            elif ln.startswith('FD:') and len(t) >= 3: mc_names.add(t[2].rsplit('/', 1)[-1])
            elif ln.startswith('MD:') and len(t) >= 5: mc_names.add(t[3].rsplit('/', 1)[-1])
    ob = lambda k: obf.get(k, k)
    for nm in z.namelist():
        if not nm.endswith('.class'): continue
        c = parse_class(z.read(nm))
        if not c: continue
        ms = sorted({m[0] for m in c['methods'] if m[0] not in ('<clinit>',)}); an = sorted({short(a[0]) for a in c['anots']})
        e = dict(s=ob(c['super']), f=c['flags'], m=ms, fd=sorted(f[0] for f in c['fields']))
        if c['ifaces']: e['i'] = [ob(x) for x in c['ifaces']]
        if an: e['a'] = an
        sm = sorted({m[0] for m in c['methods'] if any(short(a[0]) == 'SubscribeEvent' for a in m[3])})
        if sm: e['sub'] = sm
        cl[c['name']] = e
    ver = re.search(r'forge-([\d._-]+?)-universal', os.path.basename(forge_jar))
    return dict(v=1, forge=ver.group(1).replace('_', '.').replace('1.12.2-', '') if ver else '?', classes=cl, mc_classes=sorted(mc_cls), mc_names=sorted(mc_names))
def save(cat, out=DB):
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with gzip.open(out, 'wt', encoding='utf-8', compresslevel=9) as f: json.dump(cat, f, separators=(',', ':'), sort_keys=True)
class API:
    """consulta ao catalogo. API().has('net/minecraftforge/oredict/OreDictionary', 'registerOre')"""
    def __init__(self, path=DB):
        self.ok = os.path.exists(path); c = {}
        if self.ok:
            with gzip.open(path, 'rt', encoding='utf-8') as f: c = json.load(f)
        self.c = c.get('classes', {}); self.forge = c.get('forge', '?'); self.mc_cls = set(c.get('mc_classes', [])); self.mc_names = set(c.get('mc_names', []))
    def chain(self, k):
        """k + supers + interfaces dentro do catalogo (ordem de busca de membro)."""
        seen = []; st = [k]
        while st:
            x = st.pop(0)
            if x in seen or x not in self.c: continue
            seen.append(x); e = self.c[x]; st += [e['s']] + e.get('i', [])
        return seen
    def is_forge(self, k): return k in self.c
    def has(self, owner, name):
        """True/False se o membro existe (no proprio owner ou herdado) | None se owner nao e do Forge."""
        if owner not in self.c: return None
        van = False
        for x in self.chain(owner):
            e = self.c[x]
            if name in e['m'] or name in e['fd']: return True
            van = van or any(y not in self.c and y != 'java/lang/Object' for y in [e['s']] + e.get('i', []))
        return 'mc' if van else False   # 'mc' = pode vir de superclasse do Minecraft (fora do catalogo do Forge)
    def is_event(self, k): return 'net/minecraftforge/fml/common/eventhandler/Event' in self.chain(k)
    def event(self, k):
        """-> dict(cancelavel, resultado, pacote) para classe de evento do Forge ou None."""
        if not self.is_event(k): return None
        an = {a for x in self.chain(k) for a in self.c[x].get('a', [])}
        return dict(cancelavel='Cancelable' in an, resultado='HasResult' in an, bus=k.split('/')[2] if k.startswith('net/minecraftforge/') else '?')
    def is_mc(self, k): return k in self.mc_cls
    def mc_known(self, name): return name in self.mc_names   # nome SRG (func_/field_) existe no Minecraft 1.12.2?
_API = None
def api():
    global _API
    if _API is None: _API = API()
    return _API
if __name__ == '__main__':
    if len(sys.argv) < 2: sys.exit(__doc__)
    cat = build(sys.argv[1]); save(cat, sys.argv[2] if len(sys.argv) > 2 else DB)
    print('forge %s: %d classes, %d classes MC, %d nomes SRG -> %s' % (cat['forge'], len(cat['classes']), len(cat['mc_classes']), len(cat['mc_names']), sys.argv[2] if len(sys.argv) > 2 else DB))
