"""Handler generico: qualquer mod Forge 1.12.2 com blockstates + modelos JSON (elements, parents de cubo). Tudo vira BLP1.
Para ensinar um mod novo: crie handlers/<mod>.py (como decocraft.py) OU acrescente regras em db/known.json."""
import io, json, os, re
from PIL import Image
from core import handler
import pack, vanilla
FACES = {'down': 2, 'up': 3, 'north': 4, 'south': 5, 'west': 0, 'east': 1}
def duv(face, f, t):   # UV padrao do MC (0..16) quando o elemento nao traz "uv"
    return {'up': (f[0], f[2], t[0], t[2]), 'down': (f[0], 16 - t[2], t[0], 16 - f[2]), 'north': (16 - t[0], 16 - t[1], 16 - f[0], 16 - f[1]),
            'south': (f[0], 16 - t[1], t[0], 16 - f[1]), 'west': (f[2], 16 - t[1], t[2], 16 - f[1]), 'east': (16 - t[2], 16 - t[1], 16 - f[2], 16 - f[1])}[face]
def cube(tex):
    return [dict(from_=[0, 0, 0], to=[16, 16, 16], faces={k: dict(texture=tex[k]) for k in FACES})]
VANILLA = {   # parents vanilla -> (texturas exigidas, elementos)
    'block/cube_all': lambda t: cube({k: t.get('all', '') for k in FACES}),
    'block/cube_column': lambda t: cube({'up': t.get('end', ''), 'down': t.get('end', ''), 'north': t.get('side', ''), 'south': t.get('side', ''), 'east': t.get('side', ''), 'west': t.get('side', '')}),
    'block/cube_bottom_top': lambda t: cube({'up': t.get('top', ''), 'down': t.get('bottom', ''), 'north': t.get('side', ''), 'south': t.get('side', ''), 'east': t.get('side', ''), 'west': t.get('side', '')}),
    'block/cube': lambda t: cube({k: t.get(k, '') for k in FACES}),
    'block/orientable': lambda t: cube({'up': t.get('top', ''), 'down': t.get('bottom', t.get('top', '')), 'north': t.get('front', ''), 'south': t.get('side', ''), 'east': t.get('side', ''), 'west': t.get('side', '')}),
    'block/cross': lambda t: [dict(from_=[0.8, 0, 8], to=[15.2, 16, 8], rotation=dict(origin=[8, 8, 8], axis='y', angle=45), faces={'north': dict(texture=t.get('cross', '')), 'south': dict(texture=t.get('cross', ''))}),
                              dict(from_=[8, 0, 0.8], to=[8, 16, 15.2], rotation=dict(origin=[8, 8, 8], axis='y', angle=45), faces={'east': dict(texture=t.get('cross', '')), 'west': dict(texture=t.get('cross', ''))})],
    'block/slab': lambda t: [dict(from_=[0, 0, 0], to=[16, 8, 16], faces={'up': dict(texture=t.get('top', ''), uv=[0, 0, 16, 16]), 'down': dict(texture=t.get('bottom', ''), uv=[0, 0, 16, 16]), **{k: dict(texture=t.get('side', ''), uv=[0, 8, 16, 16]) for k in ('north', 'south', 'east', 'west')}})],
    'block/slab_top': lambda t: [dict(from_=[0, 8, 0], to=[16, 16, 16], faces={'up': dict(texture=t.get('top', ''), uv=[0, 0, 16, 16]), 'down': dict(texture=t.get('bottom', ''), uv=[0, 0, 16, 16]), **{k: dict(texture=t.get('side', ''), uv=[0, 0, 16, 8]) for k in ('north', 'south', 'east', 'west')}})],
    'block/carpet': lambda t: [dict(from_=[0, 0, 0], to=[16, 1, 16], faces={k: dict(texture=t.get('wool', '')) for k in FACES})],
    'block/cube_top': lambda t: cube({'up': t.get('top', ''), 'down': t.get('side', ''), 'north': t.get('side', ''), 'south': t.get('side', ''), 'east': t.get('side', ''), 'west': t.get('side', '')}),
}
def split(ref, ns): return ref.split(':', 1) if ':' in ref else (ns, ref)
@handler
class Generic:
    name = 'generic'
    def detect(self, jar): return True
    def model(self, jar, ns, ref, ctx, depth=0):
        """-> (elements, textures{}) seguindo a cadeia de parents."""
        n, p = split(ref, ns)
        if n == 'minecraft' or p in VANILLA or ('block/' + p) in VANILLA:
            k = p if p in VANILLA else 'block/' + p.split('/')[-1]
            return (VANILLA.get(k), {}) if k in VANILLA else (None, {})
        f = 'assets/%s/models/%s.json' % (n, p)
        if not jar.has(f): f = 'assets/%s/models/%s.json' % (n, p.lower())   # patchMod.8: o Forge ignora maiusculas (StorageDrawers)
        if not jar.has(f) or depth > 8: return None, {}
        d = jar.json(f); els, tex = None, {}
        if d.get('parent'):
            pk = d['parent']; pn, pp = split(pk, n)
            if pp in VANILLA: els, tex = ('V', pp), {}
            else: els, tex = self.model(jar, n, pk, ctx, depth + 1)
            if els is None: ctx.gap('parent de modelo desconhecido', pk)
        tex = dict(tex); tex.update(d.get('textures', {}))
        if d.get('elements'): els = d['elements']
        return els, tex
    def extra(self, jar, ctx, ns, atlas, models, items, tid, lang, pt, used): pass   # gancho: handlers especificos acrescentam itens/modelos
    def flat_chain(self, jar, ns, d):   # patchMod.8: segue a cadeia de parents ate item/generated|handheld (IC2: ic2:item/default -> generated); junta as texturas
        tx = dict(d.get('textures', {})); cur = d; n = ns
        for _ in range(8):
            pn, pp = split(cur.get('parent') or '', n)
            if pp.replace('item/', '') in ('generated', 'handheld', 'handheld_rod', 'builtin/generated'): return True, tx
            f = 'assets/%s/models/%s.json' % (pn, pp)
            if pn == 'minecraft' or not jar.has(f): return False, tx
            cur = jar.json(f); n = pn
            for k, v in cur.get('textures', {}).items(): tx.setdefault(k, v)
        return False, tx
    def emit(self, els, tex, rot, slots, boxes):
        """Converte elementos JSON em caixas BLP1; rot=(x,y) graus da variante (gira em torno do centro do bloco)."""
        def res(t):
            for _ in range(8):
                if not t.startswith('#'): return t
                t = tex.get(t[1:], '')
            return ''
        qv = pack.qmul(pack.qaxis('y', -rot[1]), pack.qaxis('x', -rot[0])) if (rot[0] or rot[1]) else None
        for e in els:
            f = e.get('from', e.get('from_')); t = e['to']
            c = [(f[i] + t[i]) / 32 for i in range(3)]; h = [abs(t[i] - f[i]) / 32 for i in range(3)]; c = [c[0] - .5, c[1], c[2] - .5]
            q = (1, 0, 0, 0)
            r = e.get('rotation')
            if r:
                q = pack.qaxis(r['axis'], r['angle']); o = [r['origin'][0] / 16 - .5, r['origin'][1] / 16, r['origin'][2] / 16 - .5]
                rv = pack.qrot(q, [c[i] - o[i] for i in range(3)]); c = [o[i] + rv[i] for i in range(3)]
            fs = [0] * 6
            for fn, fd in (e.get('faces') or {}).items():
                if fn not in FACES: continue
                ref = res(fd.get('texture', ''))
                if not ref: continue
                if ref not in slots: slots.append(ref)
                u = fd.get('uv') or duv(fn, f, t)
                fs[FACES[fn]] = [slots.index(ref), u[0] / 16, u[1] / 16, u[2] / 16, u[3] / 16]
            if qv:
                rc = pack.qrot(qv, (c[0], c[1] - .5, c[2])); c = [rc[0], rc[1] + .5, rc[2]]; q = pack.qmul(qv, q)
            boxes.append([round(x, 5) for x in c + h + list(q)] + fs)
    def convert(self, jar, ctx, out_root):
        ns_list = [n for n in jar.namespaces() if n not in ('minecraft', 'forge')]; ns = ns_list[0] if ns_list else 'mod'
        mod = ns; out = os.path.join(out_root, mod); os.makedirs(out, exist_ok=True)
        atlas = pack.Atlas(); models = []; items = []; texid = {}
        lang = jar.lang(ns); pt = jar.lang(ns, 'pt_br')
        def tid(ref):
            if ref not in texid:
                n, p = split(ref, ns); f = 'assets/%s/textures/%s.png' % (n, p)
                if not jar.has(f): f = f.lower()
                if jar.has(f):
                    im = Image.open(io.BytesIO(jar.read(f))).convert('RGBA')
                    if im.height > im.width: im = im.crop((0, 0, im.width, im.width))   # animacao: 1o quadro
                    texid[ref] = atlas.add(vanilla.tint(p, im))
                else:
                    vim = vanilla.get(ref) if n in ('minecraft', ns) or ':' not in ref else None   # vanilla embutida no atlas (o jogo nao resolve ref por nome)
                    if vim is not None: texid[ref] = atlas.add(vanilla.tint(p, vim)); ctx.stat('vanilla_embutida', ctx.stats.get('vanilla_embutida', 0) + 1)
                    else: texid[ref] = atlas.ref(ref if ':' in ref else 'minecraft:' + ref); ctx.gap('textura so por nome (vanilla/ausente)', ref)
            return texid[ref]
        for bs in jar.find(r'^assets/%s/blockstates/.*\.json$' % re.escape(ns)):
            name = os.path.basename(bs)[:-5]
            try:
                d = jar.json(bs); vs = d.get('variants') or {}
                v = vs.get('normal') or vs.get('') or (next(iter(vs.values())) if vs else None)
                apps = []
                if v is None and d.get('multipart'):
                    for part in d['multipart']:
                        if part.get('when'): ctx.gap('multipart condicional ignorado', name); continue
                        ap = part.get('apply'); apps.append(ap[0] if isinstance(ap, list) else ap)
                    if not [x for x in apps if x and 'model' in x]:   # patchMod.8: so partes condicionais => usa a 1a (melhor que perder o bloco)
                        ap = d['multipart'][0].get('apply'); apps = [ap[0] if isinstance(ap, list) else ap]
                else: apps = [v[0] if isinstance(v, list) else v]
                apps = [x for x in apps if x and 'model' in x]
                if not apps: ctx.gap('blockstate sem modelo simples', name); continue
                slots = []; boxes = []
                for v in apps:
                    mref = v['model']; mref = mref if '/' in mref else (mref.replace(':', ':block/') if ':' in mref else 'block/' + mref)   # blockstate: modelo relativo a models/block
                    els, tex = self.model(jar, ns, mref, ctx)
                    if isinstance(els, tuple): els = VANILLA[els[1]](tex)
                    elif callable(els): els = els(tex)
                    if not els: ctx.gap('modelo sem elementos', name); continue
                    if any(fd.get('rotation') for e in els for fd in (e.get('faces') or {}).values()): ctx.gap('rotacao de UV de face ignorada', name)
                    self.emit(els, tex, (v.get('x', 0), v.get('y', 0)), slots, boxes)
                if not slots: ctx.gap('modelo sem textura resolvida', name); continue
                models.append(dict(boxes=boxes, bbox=pack.bbox(boxes)))
                key = 'tile.%s.name' % name; nm = lang.get(key) or lang.get('tile.%s.%s.name' % (ns, name)) or name
                items.append(dict(id='%s_%s' % (ns, name), name=pt.get(key, nm), en=nm, tab=mod, model=len(models) - 1, tex=[tid(s) for s in slots]))
            except Exception as ex: ctx.gap('erro ao converter blockstate', '%s: %s' % (name, str(ex)[:60]))
        # patchMod.7: itens (armas etc.): modelo 3D com elements OU icone plano (item/generated, handheld) viram props
        used = {i['id'] for i in items}; flat = ('generated', 'handheld', 'handheld_rod', 'builtin/generated')
        for mf in jar.find(r'^assets/%s/models/item/.+\.json$' % re.escape(ns)):   # patchMod.8: tambem subpastas (IC2, Aquaculture, cfm...)
            rel = mf.split('/models/item/', 1)[1][:-5]; name = os.path.basename(rel)
            try:
                d = jar.json(mf); isflat, tx0 = self.flat_chain(jar, ns, d)
                slots = []; boxes = []
                if isflat and tx0.get('layer0'):
                    if tx0.get('layer1'): ctx.gap('item com varias camadas (so a 1a)', name)
                    els = [dict(from_=[0, 0, 7.5], to=[16, 16, 8.5], faces={k: dict(texture='#layer0', uv=[0, 0, 16, 16]) for k in ('north', 'south')})]; tex = {'layer0': tx0['layer0']}
                else:
                    els, tex = self.model(jar, ns, 'item/' + rel, ctx)
                    if isinstance(els, tuple): els = VANILLA[els[1]](tex)
                    elif callable(els): els = els(tex)
                    if not els: ctx.gap('modelo de item sem elementos', name); continue
                self.emit(els, tex, (0, 0), slots, boxes)
                if not slots: ctx.gap('item sem textura resolvida', name); continue
                iid = '%s_%s' % (ns, rel.replace('/', '_'))
                if iid in used: continue   # o bloco de mesmo nome ja foi convertido (o item so o mostra no inventario)
                used.add(iid)
                models.append(dict(boxes=boxes, bbox=pack.bbox(boxes)))
                nm = lang.get('item.%s.name' % name) or lang.get('item.%s.%s.name' % (ns, name)) or name
                items.append(dict(id=iid, name=pt.get('item.%s.name' % name, nm), en=nm, tab=mod, model=len(models) - 1, tex=[tid(x) for x in slots]))
            except Exception as ex: ctx.gap('erro ao converter item', '%s: %s' % (name, str(ex)[:60]))
        # patchMod.9: itens SEM json de modelo (so textura em textures/item[s]/**): icone plano (upgrades do IronChest etc.)
        for tf in jar.find(r'^assets/%s/textures/items?/.+\.png$' % re.escape(ns)):
            rel = tf.split('/textures/', 1)[1][:-4]; ref = '%s:%s' % (ns, rel); base = os.path.basename(rel); iid = '%s_%s' % (ns, rel.split('/', 1)[1].replace('/', '_'))
            if ref in texid or iid in used: continue
            try:
                els = [dict(from_=[0, 0, 7.5], to=[16, 16, 8.5], faces={k: dict(texture='#layer0', uv=[0, 0, 16, 16]) for k in ('north', 'south')})]; slots = []; boxes = []
                self.emit(els, {'layer0': ref}, (0, 0), slots, boxes); used.add(iid)
                models.append(dict(boxes=boxes, bbox=pack.bbox(boxes))); nm = lang.get('item.%s.name' % base) or lang.get('item.%s.%s.name' % (ns, base)) or base
                items.append(dict(id=iid, name=pt.get('item.%s.name' % base, nm), en=nm, tab=mod, model=len(models) - 1, tex=[tid(x) for x in slots]))
            except Exception as ex: ctx.gap('erro ao converter textura de item', '%s: %s' % (rel, str(ex)[:60]))
        self.extra(jar, ctx, ns, atlas, models, items, tid, lang, pt, used)
        arm = {}
        for gf in jar.find(r'^assets/%s/guns/[^/]+\.json$' % re.escape(ns)):
            try: arm[os.path.basename(gf)[:-5]] = jar.json(gf)
            except Exception: ctx.gap('erro ao ler arma', os.path.basename(gf))
        if arm: json.dump(arm, open(os.path.join(out, 'armas.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1); ctx.stat('armas', len(arm))
        files, tex = atlas.write(out)
        pack.write_blp(os.path.join(out, 'props.json'), mod, mod, files, tex, models, items)
        pack.write_bin(os.path.join(out, 'props.bin'), files, tex, models, items)
        open(os.path.join(out, 'mod.txt'), 'w', encoding='utf-8').write('name=%s\nprops props.bin\n' % mod)
        for k, v in dict(blocos=len(items), caixas=sum(len(m['boxes']) for m in models), texturas=len(tex)).items(): ctx.stat(k, v)
        return mod
