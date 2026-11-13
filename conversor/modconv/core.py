"""Nucleo do conversor de mods 1.12.2 -> Blockland. Cada mod novo vira um handler em handlers/ ou regras em db/ (ver doc/MODCONV.md)."""
import io, json, os, re, zipfile, time
HANDLERS = []
def handler(cls): HANDLERS.append(cls()); return cls
class Jar:
    def __init__(self, path): self.path = path; self.z = zipfile.ZipFile(path); self.names = self.z.namelist(); self.lower = {n.lower(): n for n in self.names}
    def has(self, n): return n in self.z.NameToInfo
    def read(self, n): return self.z.read(n)
    def text(self, n):
        b = self.z.read(n)
        try: return b.decode('utf-8')
        except UnicodeDecodeError: return b.decode('cp1252', 'replace')
    def json(self, n):
        t = self.text(n).lstrip('\ufeff'); t2 = re.sub(r',(\s*[}\]])', r'\1', t)
        try: return json.loads(t2)
        except ValueError:   # patchForge.2: o Minecraft le com Gson leniente (jsonlenient.py)
            try: from . import jsonlenient
            except ImportError: import jsonlenient
            return jsonlenient.loads(t)
    def has_string(self, tok):   # patchConv.1: o nome aparece em algum .class (item registrado de verdade)?
        if not hasattr(self, '_cls'): self._cls = b''.join(self.z.read(n) for n in self.names if n.endswith('.class'))
        return tok.encode() in self._cls
    def find(self, pat): r = re.compile(pat); return [n for n in self.names if r.search(n)]
    def namespaces(self): return sorted({n.split('/')[1] for n in self.names if n.startswith('assets/') and n.count('/') >= 2})
    def lang(self, ns, code='en_us'):
        out = {}
        for n in self.names:
            if n.lower() == 'assets/%s/lang/%s.lang' % (ns, code):
                for ln in self.text(n).splitlines():
                    if '=' in ln and not ln.startswith('#'): k, v = ln.split('=', 1); out[k.strip()] = v.strip()
        return out
class Ctx:
    """Contexto de uma conversao: pasta de saida, banco de conhecimento (db/) e relatorio de lacunas."""
    def __init__(self, out, db):
        self.out, self.db, self.gaps, self.stats = out, db, {}, {}
        self.known = _load(os.path.join(db, 'known.json'), {})
    def gap(self, kind, what, n=1): self.gaps.setdefault(kind, {}); self.gaps[kind][what] = self.gaps[kind].get(what, 0) + n
    def stat(self, k, v): self.stats[k] = v
def _load(p, d):
    try: return json.load(open(p, encoding='utf-8'))
    except Exception: return d
def pick(jar):
    for h in HANDLERS:
        if h.name != 'generic' and h.detect(jar): return h
    return next(h for h in HANDLERS if h.name == 'generic')
def convert(jar_path, out_root, db):
    jar = Jar(jar_path); h = pick(jar); os.makedirs(db, exist_ok=True)
    ctx = Ctx(None, db); t0 = time.time()
    mod = h.convert(jar, ctx, out_root)
    try:
        import forge; forge.translate(jar, ctx, os.path.join(out_root, mod), [n for n in jar.namespaces() if n not in ('minecraft', 'forge')] or [mod])   # patchMod.4: acoes do Forge
    except Exception as ex: ctx.gap('erro no tradutor do Forge', str(ex)[:60])
    try:
        import iniconfig; iniconfig.extract(jar, os.path.join(out_root, mod), [n for n in jar.namespaces() if n not in ('minecraft', 'forge')] or [mod], ctx)   # patchIni.1: receitas em .ini (IC2)
    except Exception as ex: ctx.gap('erro nas receitas .ini', str(ex)[:60])
    try:
        import armor; armor.extract(jar, os.path.join(out_root, mod), [n for n in jar.namespaces() if n not in ('minecraft', 'forge')] or [mod], ctx, getattr(ctx, 'info', None))   # patchMod.13: armaduras
    except Exception as ex: ctx.gap('erro no extrator de armaduras', str(ex)[:60])
    try:
        import gear; gear.extract(jar, os.path.join(out_root, mod), [n for n in jar.namespaces() if n not in ('minecraft', 'forge')] or [mod], ctx)   # patchF5: espadas e ferramentas
    except Exception as ex: ctx.gap('erro no extrator de espadas/ferramentas', str(ex)[:60])
    try:
        import gui; gui.extract(jar, os.path.join(out_root, mod), [n for n in jar.namespaces() if n not in ('minecraft', 'forge')] or [mod], ctx)   # patchF6: interface
    except Exception as ex: ctx.gap('erro no extrator de interface', str(ex)[:60])
    try:
        import forgemap; forgemap.extract(jar, os.path.join(out_root, mod), [n for n in jar.namespaces() if n not in ('minecraft', 'forge')] or [mod], ctx)   # patchF10: mapa do registro Forge
    except Exception as ex: ctx.gap('erro no mapa do Forge', str(ex)[:60])
    try:
        import bytecode; r = bytecode.extract(jar.path, os.path.join(out_root, mod), ctx); ctx.stat('classes_com_comportamento', len(r))   # patchConv.6: fatos lidos do bytecode
    except Exception as ex: ctx.gap('erro na leitura de bytecode', str(ex)[:60])
    try:
        import mecanicas; mecanicas.apply(jar, os.path.join(out_root, mod), [n for n in jar.namespaces() if n not in ('minecraft', 'forge')][:1] and [n for n in jar.namespaces() if n not in ('minecraft', 'forge')][0] or mod, ctx)   # patchMec.1: fatos do bytecode por item
    except Exception as ex: ctx.gap('erro nas mecanicas', str(ex)[:60])
    ctx.stat('handler', h.name); ctx.stat('segundos', round(time.time() - t0, 1))
    # banco de conhecimento: acumula lacunas e historico (e o que o proximo envio de mod aproveita)
    unk = _load(os.path.join(db, 'unknown.json'), {})
    for k, v in ctx.gaps.items():
        d = unk.setdefault(k, {})
        for w, n in v.items(): d[w] = d.get(w, 0) + n
    json.dump(unk, open(os.path.join(db, 'unknown.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1, sort_keys=True)
    hist = _load(os.path.join(db, 'history.json'), [])
    hist.append(dict(jar=os.path.basename(jar_path), handler=h.name, mod=mod, stats=ctx.stats, gaps={k: len(v) for k, v in ctx.gaps.items()}))
    json.dump(hist, open(os.path.join(db, 'history.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
    return mod, ctx
def report(path, mod, ctx):
    L = ['# Relatorio de conversao: %s' % mod, '', 'Handler: `%s`' % ctx.stats.get('handler'), '']
    L += ['- %s: %s' % (k, v) for k, v in sorted(ctx.stats.items()) if k != 'handler']
    for k, v in sorted(ctx.gaps.items()):
        L += ['', '## Lacuna: %s (%d tipos)' % (k, len(v))] + ['- `%s` x%d' % (w, n) for w, n in sorted(v.items(), key=lambda x: -x[1])[:40]]
    open(path, 'w', encoding='utf-8').write('\n'.join(L) + '\n')
