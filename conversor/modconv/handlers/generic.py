"""Handler generico: qualquer mod Forge 1.12.2 com blockstates + modelos JSON (elements, parents de cubo). Tudo vira BLP1.
Para ensinar um mod novo: crie handlers/<mod>.py (como decocraft.py) OU acrescente regras em db/known.json."""
import io, json, os, re
from PIL import Image
from core import handler
import pack, vanilla, forgestate   # patchF1
def _fallback_name(jar):
    import modinfo
    try: m = modinfo.modid_of(jar)
    except Exception: m = ''
    return re.sub(r'[^a-z0-9_]', '', (m or os.path.basename(jar.path).lower().split('.jar')[0]).replace('-', '_')) or 'mod'
FACES = {'down': 2, 'up': 3, 'north': 4, 'south': 5, 'west': 0, 'east': 1}
def duv(face, f, t):   # UV padrao do MC (0..16) quando o elemento nao traz "uv"
    return {'up': (f[0], f[2], t[0], t[2]), 'down': (f[0], 16 - t[2], t[0], 16 - f[2]), 'north': (16 - t[0], 16 - t[1], 16 - f[0], 16 - f[1]),
            'south': (f[0], 16 - t[1], t[0], 16 - f[1]), 'west': (f[2], 16 - t[1], t[2], 16 - f[1]), 'east': (16 - t[2], 16 - t[1], 16 - f[2], 16 - f[1])}[face]
def cube(tex):
    return [dict(from_=[0, 0, 0], to=[16, 16, 16], faces={k: dict(texture=tex[k]) for k in FACES})]
VANILLA = {   # parents vanilla -> (texturas exigidas, elementos)
    'block/block': lambda t: [],   # patchF12: pai abstrato (so display/ao): o filho traz os elements
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
def _bx(f, t, tx):   # patchConv.8: caixa com a mesma textura em todas as faces (ou dict por face)
    return dict(from_=f, to=t, faces={k: dict(texture=(tx[k] if isinstance(tx, dict) else tx)) for k in FACES})
def _st(t, boxes):   # escadas: texturas bottom/top/side
    tx = lambda: {'up': t.get('top', ''), 'down': t.get('bottom', ''), **{k: t.get('side', '') for k in ('north', 'south', 'east', 'west')}}
    return [_bx(f, to, tx()) for f, to in boxes]
_S = ([0, 0, 0], [16, 8, 16])
def _gate(t, y, op):
    x = t.get('texture', ''); b = [([0, 5 + y, 7], [2, 16 + y, 9]), ([14, 5 + y, 7], [16, 16 + y, 9])]
    b += [([0, 6 + y, 9], [2, 9 + y, 15]), ([0, 12 + y, 9], [2, 15 + y, 15]), ([14, 6 + y, 9], [16, 9 + y, 15]), ([14, 12 + y, 9], [16, 15 + y, 15])] if op else [([2, 6 + y, 7], [14, 9 + y, 9]), ([2, 12 + y, 7], [14, 15 + y, 9])]
    return [_bx(f, to, x) for f, to in b]
VANILLA.update({
    'block/leaves': VANILLA['block/cube_all'], 'block/half_slab': VANILLA['block/slab'], 'block/upper_slab': VANILLA['block/slab_top'], 'block/tinted_cross': VANILLA['block/cross'],
    'block/stairs': lambda t: _st(t, [_S, ([8, 8, 0], [16, 16, 16])]),
    'block/inner_stairs': lambda t: _st(t, [_S, ([8, 8, 0], [16, 16, 16]), ([0, 8, 8], [8, 16, 16])]),
    'block/outer_stairs': lambda t: _st(t, [_S, ([8, 8, 8], [16, 16, 16])]),
    'block/fence_post': lambda t: [_bx([6, 0, 6], [10, 16, 10], t.get('texture', ''))],
    'block/fence_side': lambda t: [_bx([7, 6, 0], [9, 9, 6], t.get('texture', '')), _bx([7, 12, 0], [9, 15, 6], t.get('texture', ''))],
    'block/fence_inventory': lambda t: [_bx([6, 0, 6], [10, 16, 10], t.get('texture', '')), _bx([7, 6, 0], [9, 9, 16], t.get('texture', '')), _bx([7, 12, 0], [9, 15, 16], t.get('texture', ''))],
    'block/wall_post': lambda t: [_bx([4, 0, 4], [12, 16, 12], t.get('wall', t.get('texture', '')))],   # patchForge.2: muro vanilla 1.12
    'block/wall_side': lambda t: [_bx([5, 0, 0], [11, 13, 8], t.get('wall', t.get('texture', '')))],
    'block/wall_inventory': lambda t: [_bx([4, 0, 4], [12, 16, 12], t.get('wall', t.get('texture', ''))), _bx([5, 0, 0], [11, 13, 16], t.get('wall', t.get('texture', '')))],
    'block/fence_gate_closed': lambda t: _gate(t, 0, False), 'block/fence_gate_open': lambda t: _gate(t, 0, True),
    'block/wall_gate_closed': lambda t: _gate(t, -3, False), 'block/wall_gate_open': lambda t: _gate(t, -3, True),
    'block/door_bottom': lambda t: [_bx([0, 0, 0], [16, 16, 3], t.get('bottom', ''))], 'block/door_bottom_rh': lambda t: [_bx([0, 0, 0], [16, 16, 3], t.get('bottom', ''))],
    'block/door_top': lambda t: [_bx([0, 0, 0], [16, 16, 3], t.get('top', ''))], 'block/door_top_rh': lambda t: [_bx([0, 0, 0], [16, 16, 3], t.get('top', ''))],
    'block/grass': VANILLA['block/cube_bottom_top'],
    'block/grass_path': lambda t: [_bx([0, 0, 0], [16, 15, 16], {'up': t.get('top', ''), 'down': t.get('bottom', ''), **{k: t.get('side', '') for k in ('north', 'south', 'east', 'west')}})],
    'block/fire_floor': lambda t: [dict(from_=[0, .5, 0], to=[16, .5, 16], faces={'up': dict(texture=t.get('fire', '')), 'down': dict(texture=t.get('fire', ''))})],
    'block/fire_side': lambda t: [dict(from_=[0, 0, .5], to=[16, 16, .5], faces={'north': dict(texture=t.get('fire', '')), 'south': dict(texture=t.get('fire', ''))})],
    'block/torch': lambda t: [_bx([7, 0, 7], [9, 10, 9], t.get('torch', ''))],
    'block/crop': lambda t: [dict(from_=[4, -1, 0], to=[4, 15, 16], faces={'east': dict(texture=t.get('crop', '')), 'west': dict(texture=t.get('crop', ''))}), dict(from_=[12, -1, 0], to=[12, 15, 16], faces={'east': dict(texture=t.get('crop', '')), 'west': dict(texture=t.get('crop', ''))}),
                                dict(from_=[0, -1, 4], to=[16, 15, 4], faces={'north': dict(texture=t.get('crop', '')), 'south': dict(texture=t.get('crop', ''))}), dict(from_=[0, -1, 12], to=[16, 15, 12], faces={'north': dict(texture=t.get('crop', '')), 'south': dict(texture=t.get('crop', ''))})],
    'block/farmland': lambda t: [_bx([0, 0, 0], [16, 15, 16], {'up': t.get('top', ''), 'down': t.get('dirt', ''), **{k: t.get('dirt', '') for k in ('north', 'south', 'east', 'west')}})],
})
ALT114 = {'lantern': 'torch_on', 'soul_lantern': 'torch_on', 'chain': 'iron_bars', 'anvil': 'anvil_base', 'stripped_spruce_log': 'log_spruce', 'stripped_oak_log': 'log_oak', 'stripped_birch_log': 'log_birch',
          'stripped_jungle_log': 'log_jungle', 'stripped_acacia_log': 'log_acacia', 'stripped_dark_oak_log': 'log_big_oak', 'smooth_stone': 'stone_slab_top', 'blackstone': 'obsidian', 'deepslate': 'stone', 'copper_block': 'iron_block'}
_XJ = {}
def _xjar(jar, n, p):   # patchConv.10: procura assets/<n>/textures/<p>.png nos outros .jar ao lado e em mods/ mods_jar/
    import glob, zipfile
    if not _XJ:
        for d in {os.path.dirname(os.path.abspath(jar.path)), 'mods', 'mods_jar'}:
            for j in sorted(glob.glob(os.path.join(d, '*.jar'))):
                if os.path.abspath(j) == os.path.abspath(jar.path): continue
                try: z = zipfile.ZipFile(j); _XJ[j] = (z, {x.lower(): x for x in z.namelist()})
                except Exception: pass
    k = ('assets/%s/textures/%s.png' % (n, p)).lower()
    for z, N in _XJ.values():
        if k in N: return z.read(N[k])
    return None
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
        if not jar.has(f) and depth <= 8 and p.startswith('block/'):   # patchConv.9: modelo citado mas ausente do jar (BOP dead_planks): cubo/laje com a textura de mesmo nome
            b0 = p[6:]; sl = b0.startswith('half_slab_'); cand = [b0] if not sl else [b0[10:] + '_planks', b0[10:]]
            for c in cand:
                for d0 in ('blocks', 'block'):
                    if jar.has('assets/%s/textures/%s/%s.png' % (n, d0, c)):
                        t = '%s:%s/%s' % (n, d0, c)
                        return (VANILLA['block/slab'] if sl else VANILLA['block/cube_all']), ({'top': t, 'bottom': t, 'side': t} if sl else {'all': t})
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
        ns_list = [n for n in jar.namespaces() if n not in ('minecraft', 'forge')]; ns = ns_list[0] if ns_list else _fallback_name(jar)   # patchConv.7: mod sem assets nao cai mais em mods/mod (colidia)
        mod = ns; out = os.path.join(out_root, mod); os.makedirs(out, exist_ok=True)
        atlas = pack.Atlas(); models = []; items = []; texid = {}
        lang = jar.lang(ns); pt = jar.lang(ns, 'pt_br')
        def tid(ref):
            if ref not in texid:
                n, p = split(ref, ns); f = 'assets/%s/textures/%s.png' % (n, p)
                if not jar.has(f): f = f.lower()
                pim = vanilla.get_ns(n, p) if n != 'minecraft' else None   # patchConv.10: o resource pack do jogador pode trazer a textura do mod
                if not jar.has(f):   # patchConv.10: blocks<->block / items<->item (mods 1.13-style no 1.12) e textura de outro jar de mod
                    q = p.split('/'); q[0] = {'block': 'blocks', 'blocks': 'block', 'item': 'items', 'items': 'item'}.get(q[0], q[0]); g = 'assets/%s/textures/%s.png' % (n, '/'.join(q))
                    if jar.has(g): f = g
                    elif jar.has(g.lower()): f = g.lower()
                    else:
                        xb = _xjar(jar, n, p) or _xjar(jar, n, '/'.join(q))
                        if xb is not None: pim = pim or Image.open(io.BytesIO(xb)).convert('RGBA')
                if pim is not None:
                    if pim.height > pim.width: pim = pim.crop((0, 0, pim.width, pim.width))
                    texid[ref] = atlas.add(vanilla.tint(p, pim)); ctx.stat('textura_de_pack_ou_outro_jar', ctx.stats.get('textura_de_pack_ou_outro_jar', 0) + 1)
                elif jar.has(f):
                    im = Image.open(io.BytesIO(jar.read(f))).convert('RGBA')
                    if im.height > im.width: im = im.crop((0, 0, im.width, im.width))   # animacao: 1o quadro
                    texid[ref] = atlas.add(vanilla.tint(p, im))
                else:
                    vim = vanilla.get(ref) if n in ('minecraft', ns) or ':' not in ref else None   # vanilla embutida no atlas (o jogo nao resolve ref por nome)
                    if vim is not None: texid[ref] = atlas.add(vanilla.tint(p, vim)); ctx.stat('vanilla_embutida', ctx.stats.get('vanilla_embutida', 0) + 1)
                    elif n == 'minecraft' and p.split('/')[-1] in ALT114 and vanilla.get('blocks/' + ALT114[p.split('/')[-1]]) is not None:   # patchConv.11: textura 1.14+ -> equivalente do 1.12
                        texid[ref] = atlas.add(vanilla.tint(p, vanilla.get('blocks/' + ALT114[p.split('/')[-1]]))); ctx.stat('textura_114_trocada', ctx.stats.get('textura_114_trocada', 0) + 1)
                    else: texid[ref] = atlas.ref(ref if ':' in ref else 'minecraft:' + ref); ctx.gap('textura so por nome (vanilla/ausente)', ref)
            return texid[ref]
        def build(apps, name, slots, boxes):   # patchConv.3: apps -> caixas (reusado nas variantes por vizinhos)
            for v in apps:
                mref = v['model']; pth = mref.split(':', 1)[-1]; mref = mref if pth.startswith(('block/', 'item/', 'builtin/')) else (mref.replace(':', ':block/', 1) if ':' in mref else 'block/' + mref)   # patchF12: o Forge sempre resolve a partir de models/block/ (mossy/mossy -> block/mossy/mossy)   # blockstate: modelo relativo a models/block
                import objmodel   # patchF9
                if mref.lower().endswith('.obj'): els, tex = objmodel.boxes(jar, ns, mref.replace('block/', '', 1) if ':' not in mref else mref.replace(':block/', ':', 1), v, ctx, name)
                else: els, tex = self.model(jar, ns, mref, ctx)
                tex = dict(tex, **(v.get('textures') or {}))   # patchF1: textures da variante
                if isinstance(els, tuple): els = VANILLA[els[1]](tex)
                elif callable(els): els = els(tex)
                if not els: ctx.gap('modelo sem elementos', name); continue
                if any(fd.get('rotation') for e in els for fd in (e.get('faces') or {}).values()): ctx.gap('rotacao de UV de face ignorada', name)
                self.emit(els, tex, (v.get('x', 0), v.get('y', 0)), slots, boxes)
        opens = {}   # patchOpen.1: id base -> (slots, boxes) do estado open=true
        nbr = {}   # patchConv.3: id base -> (grupo, flags, {mascara: (slots, boxes)})
        for bs in jar.find(r'^assets/%s/blockstates/.*\.json$' % re.escape(ns)):
            name = bs.split('/blockstates/', 1)[1][:-5].replace('/', '_')   # patchF12: blockstates em subpastas (Chisel: glasspane_stained/magenta/framed) nao colidem mais
            try:
                if not jar.read(bs).strip(): ctx.gap('blockstate vazio no jar (ignorado)', name); continue   # patchConv.1: tree.json tem 0 bytes
                d = jar.json(bs); vs = d.get('variants') or {}
                v = forgestate.pick(vs, d)   # patchF1: formato Forge (forge_marker/propriedades)
                apps = []
                if v is None and d.get('multipart'): apps = forgestate.multipart(d)   # patchF13: partes condicionais pelo estado de repouso
                else: apps = [v[0] if isinstance(v, list) else v]
                apps = [x for x in apps if x and 'model' in x]
                apps = [y for x in apps for y in [x] + forgestate.submodels(x)]   # patchConv.1: partes "submodel" do Forge
                if not apps and isinstance(vs.get('inventory'), (dict, list)):   # patchConv.11: "normal" vazio (CookingForBlockheads: o mundo usa TESR): desenha o modelo estatico da variante inventory
                    iv = vs['inventory']; iv = iv[0] if isinstance(iv, list) and iv else iv
                    if isinstance(iv, dict) and 'model' in iv: apps = [y for x in [iv] for y in [x] + forgestate.submodels(x)]
                if not apps: ctx.gap('blockstate sem modelo simples', name); continue
                slots = []; boxes = []; build(apps, name, slots, boxes)
                if not slots: ctx.gap('modelo sem textura resolvida', name); continue
                models.append(dict(boxes=boxes, bbox=pack.bbox(boxes)))
                key = 'tile.%s.name' % name; nm = lang.get(key) or lang.get('tile.%s.%s.name' % (ns, name)) or name
                items.append(dict(id='%s_%s' % (ns, name), name=pt.get(key, nm), en=nm, tab=mod, model=len(models) - 1, tex=[tid(s) for s in slots]))
                try:   # patchConv.12: bloco "de metadado" (IC2 resource: type=copper_ore/tin_ore/..., 1 propriedade com modelos DIFERENTES): 1 item por valor (antes so o 1o existia)
                    gs = {k2: v2 for k2, v2 in vs.items() if forgestate.is_group(v2) and k2 not in ('defaults', 'inventory')}
                    if len(gs) == 1:
                        gk, gv = next(iter(gs.items())); dfl = forgestate._one(d.get('defaults') or vs.get('defaults'))
                        vals = {kk: forgestate._one(vv) for kk, vv in gv.items() if isinstance(forgestate._one(vv), dict) and 'model' in forgestate._one(vv)}
                        if 2 <= len(vals) <= 64 and len({vv['model'] for vv in vals.values()}) >= 2:
                            for kk, vv in vals.items():
                                if vv['model'] == apps[0].get('model') and vv.get('textures') == apps[0].get('textures'): continue
                                vid = '%s_%s_%s' % (ns, name, re.sub(r'[^a-z0-9_]', '_', kk.lower()))
                                if vid in {i['id'] for i in items}: continue
                                ap2 = forgestate.merge(dfl, vv) if isinstance(dfl, dict) else vv; s2 = []; b2 = []; build([y for x in [ap2] for y in [x] + forgestate.submodels(x)], name, s2, b2)
                                if not (b2 and s2): continue
                                models.append(dict(boxes=b2, bbox=pack.bbox(b2))); k3 = 'tile.%s.%s.name' % (name, kk); nm2 = lang.get(k3) or lang.get('tile.%s.%s.name' % (ns, kk)) or ('%s %s' % (nm, kk.replace('_', ' ')))
                                items.append(dict(id=vid, name=pt.get(k3, nm2), en=nm2, tab=mod, model=len(models) - 1, tex=[tid(x) for x in s2])); ctx.stat('variantes_de_metadado', ctx.stats.get('variantes_de_metadado', 0) + 1)
                except Exception as ex: ctx.gap('erro nas variantes de metadado', '%s: %s' % (name, str(ex)[:60]))
                ov = forgestate.open_variant(d)   # patchOpen.1
                if ov:
                    os2 = []; ob = []; build([ov] + forgestate.submodels(ov), name, os2, ob)
                    if ob and os2: opens['%s_%s' % (ns, name)] = (os2, ob)
                nv = forgestate.neighbor_variants(d)   # patchConv.3: mesa/cerca/vidraca ligam por vizinhos
                if nv:
                    vm = {}; seen = {}
                    for mk, ap in nv.items():
                        vs2 = []; vb = []; build([y for x in ap for y in [x]], name, vs2, vb)
                        if vb and vs2: sig = json.dumps([vb, vs2], sort_keys=True); vm[mk] = seen.setdefault(sig, (vs2, vb))
                    if len(vm) >= 2: nbr['%s_%s' % (ns, name)] = (name, vm)
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
            if not (lang.get('item.%s.name' % base) or lang.get('item.%s.%s.name' % (ns, base)) or jar.has_string(base)):   # patchConv.1: textura legada (itemkitchencabinet etc.) sem nome nem registro nao vira item
                ctx.gap('textura de item legada ignorada (sem lang/registro)', base); continue
            try:
                els = [dict(from_=[0, 0, 7.5], to=[16, 16, 8.5], faces={k: dict(texture='#layer0', uv=[0, 0, 16, 16]) for k in ('north', 'south')})]; slots = []; boxes = []
                self.emit(els, {'layer0': ref}, (0, 0), slots, boxes); used.add(iid)
                models.append(dict(boxes=boxes, bbox=pack.bbox(boxes))); nm = lang.get('item.%s.name' % base) or lang.get('item.%s.%s.name' % (ns, base)) or base
                items.append(dict(id=iid, name=pt.get('item.%s.name' % base, nm), en=nm, tab=mod, model=len(models) - 1, tex=[tid(x) for x in slots]))
            except Exception as ex: ctx.gap('erro ao converter textura de item', '%s: %s' % (rel, str(ex)[:60]))
        self.extra(jar, ctx, ns, atlas, models, items, tid, lang, pt, used)
        try:   # patchF2-F4: analise de classes do Forge -> colocadores, blocos tecnicos e baus
            import classinfo, placers, containers
            info = ctx.info = classinfo.analyse(jar); pl, _ = placers.apply(jar, ctx, ns, out, items, info); containers.apply(jar, ctx, ns, out, items, info, pl)
            import lights; lights.apply(jar, ctx, ns, out, items, info, pl)   # patchF14: luz emitida
            import modinfo; modinfo.apply(jar, ctx, ns, out, items)   # patchF7: perfil por mod
        except Exception as ex: ctx.gap('erro no analisador de classes do Forge', str(ex)[:60])
        arm = {}
        for gf in jar.find(r'^assets/%s/guns/[^/]+\.json$' % re.escape(ns)):
            try: arm[os.path.basename(gf)[:-5]] = jar.json(gf)
            except Exception: ctx.gap('erro ao ler arma', os.path.basename(gf))
        if arm: json.dump(arm, open(os.path.join(out, 'armas.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1); ctx.stat('armas', len(arm))
        conn = []; alive = {i['id'] for i in items}   # patchConv.3: itens-variante (aba "~" = fora do criativo) + conexoes.txt
        MAT = set('nether_brick andesite diorite granite oak spruce birch jungle acacia dark_oak darkoak white orange magenta light_blue lightblue yellow lime pink gray grey silver light_gray lightgray cyan purple blue brown green red black iron gold stone cobblestone brick glass wood stained'.split())
        for iid, (bname, vm) in nbr.items():
            if iid not in alive: continue
            grp = bname
            for mt in sorted(MAT, key=len, reverse=True):   # tira o sufixo de material: table_oak/table_dark_oak -> table (todas as madeiras ligam)
                if bname.endswith('_' + mt) and len(bname) > len(mt) + 1: grp = bname[:-len(mt) - 1]; break
            flags = 's' if re.search(r'fence|pane|wall|bars|gate|glass', bname) else ''; vid = {}; made = {}
            for mk, (vs2, vb) in sorted(vm.items()):
                sig = id(vb)
                if sig not in made:
                    models.append(dict(boxes=vb, bbox=pack.bbox(vb))); vi = '%s~%d' % (iid, mk); made[sig] = vi
                    items.append(dict(id=vi, name=next(i['name'] for i in items if i['id'] == iid), en=bname, tab='~', model=len(models) - 1, tex=[tid(x) for x in vs2]))
                vid[mk] = made[sig]
            conn.append('%s|%s|%s|%s' % (iid, grp, flags, ','.join(vid.get(m, '-') for m in range(16))))
        ab = []
        for iid, (os2, ob) in opens.items():
            if iid not in alive: continue
            models.append(dict(boxes=ob, bbox=pack.bbox(ob))); vi = iid + '~open'
            items.append(dict(id=vi, name=next(i['name'] for i in items if i['id'] == iid), en=iid, tab='~', model=len(models) - 1, tex=[tid(x) for x in os2])); ab.append('%s|%s' % (iid, vi))
        mn = []   # patchOre.2: minerios -> minerios.txt (item|quantidade por chunk|tamanho|y min|y max); valores por nome (o Forge poe isso em codigo)
        for it in items:
            iid = it['id']; low = iid.lower()
            if not re.search(r'(^|_)ore($|_)', low) or re.search(r'nether|end_|_end|iridium|dense|poor|drill|crusher|mining|processed|dust|chunk|stone_ore_', low): continue
            if re.search(r'uranium|platinum|silver|iridium|tungsten|titanium|cobalt|ardite|osmium', low): c = (2, 5, 0, 32)
            elif re.search(r'ruby|sapphire|peridot|amber|topaz|tanzanite|malachite|amethyst|opal|garnet|jade', low): c = (1, 4, 0, 24)
            elif re.search(r'copper|tin|lead|zinc|nickel|aluminum|aluminium|bauxite|mythril|sulfur|sulphur|salt|saltpeter', low): c = (8, 8, 0, 64)
            else: c = (4, 8, 0, 64)
            mn.append('%s|%d|%d|%d|%d' % ((iid,) + c))
        if mn: open(os.path.join(out, 'minerios.txt'), 'w', encoding='utf-8').write('\n'.join(mn) + '\n'); ctx.stat('minerios', len(mn))
        if ab: open(os.path.join(out, 'abertos.txt'), 'w', encoding='utf-8').write('\n'.join(ab) + '\n'); ctx.stat('abertos', len(ab))
        if conn: open(os.path.join(out, 'conexoes.txt'), 'w', encoding='utf-8').write('\n'.join(conn) + '\n'); ctx.stat('conexoes', len(conn))
        files, tex = atlas.write(out)
        pack.write_blp(os.path.join(out, 'props.json'), mod, mod, files, tex, models, items)
        pack.write_bin(os.path.join(out, 'props.bin'), files, tex, models, items)
        open(os.path.join(out, 'mod.txt'), 'w', encoding='utf-8').write('name=%s\nprops props.bin\n%s' % (mod, ('modid=%s\n' % ctx.modid) if getattr(ctx, 'modid', '') else ''))
        for k, v in dict(blocos=len(items), caixas=sum(len(m['boxes']) for m in models), texturas=len(tex)).items(): ctx.stat(k, v)
        return mod
