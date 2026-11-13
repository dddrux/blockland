"""Handler do Decocraft (modid props): modelos CraftStudio (.csmodel dentro do .cspack) -> BLP1."""
import io, json, os, re, zipfile
from PIL import Image, ImageColor
from core import handler
import csmodel, pack
TABS = {'Kitchen': 'Cozinha', 'Patreon': 'Patreon', 'Toys': 'Brinquedos', 'Livingroom': 'Sala', 'Yard': 'Jardim', 'Holiday': 'Festas', 'Bedroom': 'Quarto',
        'Market': 'Mercado', 'Lighting': 'Iluminação', 'Science': 'Ciência', 'Misc': 'Diversos', 'Dungeon': 'Masmorra', 'Food': 'Comida', 'Tech': 'Tecnologia',
        'Architecture': 'Arquitetura', 'Bathroom': 'Banheiro', 'Office': 'Escritório', 'Art': 'Arte', 'Storage': 'Armazenamento', 'Sports': 'Esportes',
        'Religion': 'Religião', 'Signs': 'Placas', 'Trophies': 'Troféus', 'Plants': 'Plantas'}
FACE = (5, 4, 1, 2, 0, 3)   # ordem UV do CraftStudio [frente, tras, esquerda, base, direita, topo] -> face canonica
def boxes_of(nodes, tw, th, scale, clamped=None):
    out = []
    for n in nodes:
        w, h, d = n['size']
        if not (w or h or d): continue
        sc = n['sc']; half = (w * sc[0] * scale / 32, h * sc[1] * scale / 32, d * sc[2] * scale / 32)
        o = pack.qrot(n['q'], tuple(n['off']))   # o offset (pivo) NAO leva a escala do no (so o tamanho leva)
        c = [(n['pos'][i] + o[i]) * scale for i in range(3)]
        rects = ((w, h), (w, h), (d, h), (w, d), (d, h), (w, d))
        faces = [0] * 6
        for k in range(6):
            rw, rh = rects[k]
            if not rw or not rh: continue
            ux, uy = n['uv'][2 * k], n['uv'][2 * k + 1]
            r = [ux / tw, uy / th, (ux + rw) / tw, (uy + rh) / th]
            cl = [min(1.0, max(0.0, v)) for v in r]
            if cl != r and clamped is not None: clamped.append(1)
            faces[FACE[k]] = [0] + cl
        out.append([round(v, 5) for v in c + list(half) + list(n['q'])] + faces)
    return out
def model_score(nodes, scale):
    """Quanto menor, mais plausivel: centrado em x/z, apoiado perto de y=0, altura razoavel."""
    bx = boxes_of(nodes, 16, 16, scale)
    if not bx: return 0.0
    bb = pack.bbox(bx); cx = (bb[0] + bb[3]) / 2; cz = (bb[2] + bb[5]) / 2; h = bb[4] - bb[1]
    return abs(cx) + abs(cz) + 4 * max(0.0, -bb[1] - 0.05) + 2 * max(0.0, h - 4) + 0.5 * max(0.0, max(bb[3] - bb[0], bb[5] - bb[2]) - 4)
def pick_strategy(nodes_of, aids, ctx):
    tot = {}
    for name, fn in csmodel.PARENTS.items():
        for inh in (True, False):
            if name == 'flat' and inh: continue
            sc = 0.0; ok = True
            for aid, scale in aids.items():
                nodes = nodes_of(aid)[0]; par = fn(nodes)
                if not csmodel._valid(par): ok = False; break
                sc += model_score(csmodel.compose(nodes, par, inh), scale)
            if ok: tot[(name, inh)] = sc
    flat = tot[('flat', False)]; best = min(tot, key=lambda k: (tot[k], k[0] != 'flat'))
    if best != ('flat', False) and tot[best] > flat * 0.97: best = ('flat', False)   # so troca se melhorar de fato
    for k, v in sorted(tot.items(), key=lambda kv: kv[1]): ctx.stat('hierarquia %s%s' % (k[0], '' if k[1] else ' (sem escala do pai)'), round(v, 1))
    ctx.stat('hierarquia escolhida', '%s%s' % (best[0], '' if best[1] else ' (sem escala do pai)'))
    return best
NAMED = {'mc_black_wool': 0x1D1D21}
def color_of(v):
    v = str(v).strip().lower()
    if v in NAMED: return NAMED[v]
    try:
        if v.startswith('0x') or re.match(r'^[0-9a-f]{6}$', v): return int(v, 16)
        r, g, b = ImageColor.getrgb(v)[:3]; return (r << 16) | (g << 8) | b
    except Exception: return None
def tint(img, col):
    r, g, b = (col >> 16) & 255, (col >> 8) & 255, col & 255; px = img.convert('RGBA'); a = px.load()
    for y in range(px.height):
        for x in range(px.width):
            p = a[x, y]; a[x, y] = (p[0] * r // 255, p[1] * g // 255, p[2] * b // 255, p[3])
    return px
@handler
class Decocraft:
    name = 'decocraft'
    def detect(self, jar): return bool(jar.find(r'assets/props/models/Decopack/.*\.cspack$'))
    def convert(self, jar, ctx, out_root):
        out = os.path.join(out_root, 'decocraft'); os.makedirs(out, exist_ok=True)
        cs = zipfile.ZipFile(io.BytesIO(jar.read(jar.find(r'Decopack/.*\.cspack$')[0])))
        cat = json.loads(jar.read(jar.find(r'Decopack/.*\.json$')[0]).decode('utf-8'))
        en = jar.lang('props'); pt = jar.lang('props', 'pt_br'); en2pt = {v: pt[k] for k, v in en.items() if k in pt}
        texfiles = {os.path.basename(n).lower(): n for n in jar.names if n.startswith('assets/props/textures/models/') and n.endswith('.png')}
        clamp = []; atlas = pack.Atlas(); models = []; mindex = {}; items = []; gotov = 0; embed = 0; missing = 0
        parsed = {}
        def nodes_of(aid):
            if aid not in parsed: parsed[aid] = csmodel.parse(cs.read('Assets/Models/%d.csmodel' % aid))
            return parsed[aid]
        strat, inherit = pick_strategy(nodes_of, {e['craftstudioAssetID']: float(e.get('scale', 1)) for e in cat}, ctx)
        def model(d):   # d = item ja mesclado com a variante: a escala e o asset podem mudar na variante (antes o cache era so por asset)
            aid = d['craftstudioAssetID']; sc = float(d.get('scale', 1)); key = (aid, sc)
            if key not in mindex:
                nodes, png = nodes_of(aid)
                nodes = csmodel.compose_pair(nodes) if csmodel.has_pair(nodes) else csmodel.compose(nodes, csmodel.PARENTS[strat](nodes), inherit)
                im = Image.open(io.BytesIO(png)); tw, th = im.size
                bx = boxes_of(nodes, tw, th, sc, clamp)
                mindex[key] = len(models); models.append(dict(boxes=bx, bbox=pack.bbox(bx), emb=im.convert('RGBA')))
            return mindex[key]
        def item(key, e, base):
            nonlocal gotov, embed, missing
            d = dict(base or {}); d.update({k: v for k, v in e.items() if k != 'variants'})
            mi = model(d); m = models[mi]; img = m['emb']
            ov = d.get('textureOverride')
            if ov and ov.lower() in texfiles: img = Image.open(io.BytesIO(jar.read(texfiles[ov.lower()]))); gotov += 1
            else:
                embed += 1
                if ov: missing += 1
            if d.get('colorPrimary'):
                cc = color_of(d['colorPrimary'])
                if cc is None: ctx.gap('cor desconhecida', str(d['colorPrimary']))
                else: img = tint(img, cc)
            tid = atlas.add(img)
            nm = d['name']; tab = TABS.get(d.get('tab'), d.get('tab') or 'Diversos')
            it = dict(id='deco_%s' % key, name=en2pt.get(nm, nm), en=nm, tab=tab, model=mi, tex=[tid])
            if d.get('lightLevel'): it['light'] = d['lightLevel']
            if d.get('walkthrough'): it['nocoll'] = 1
            if d.get('tileType'):
                it['beh'] = d['tileType']; ctx.gap('comportamento (tileType)', d['tileType'])
                if (d.get('tileParams') or {}).get('linkedModel'): it['beh'] += ':' + str(d['tileParams']['linkedModel'])   # ex. Door:1199 = modelo da porta aberta (id do item deco_1199)
                if d.get('tileParams'): it['beh_params'] = d['tileParams']
            for k in ('colorSecondary', 'colorAccentPrimary', 'hasVariableRendering', 'faceCulling', 'sound', 'spiderweb'):
                if d.get(k) is not None: ctx.gap('campo ignorado', k)
            items.append(it)
        for e in cat:
            item(e['decocraftModelID'], e, None)
            for vk, ve in (e.get('variants') or {}).items(): item(vk, ve, e)
        for m in models: del m['emb']
        if clamp: ctx.gap('UV fora da textura (fixado nas bordas)', 'faces', len(clamp))
        files, tex = atlas.write(out)
        pack.write_blp(os.path.join(out, 'props.json'), 'decocraft', 'Decocraft', files, tex, models, items)
        pack.write_bin(os.path.join(out, 'props.bin'), files, tex, models, items)
        open(os.path.join(out, 'mod.txt'), 'w', encoding='utf-8').write('name=Decocraft\nprops props.bin\n')
        for k, v in dict(modelos=len(models), itens=len(items), caixas=sum(len(m['boxes']) for m in models), texturas=len(tex), paginas=len(files),
                         tex_override_ok=gotov, tex_embutida=embed, override_ausente_usou_embutida=missing).items(): ctx.stat(k, v)
        return 'decocraft'
