"""Texturas vanilla (1.12.2) para o conversor: resolve referencias 'minecraft:blocks/x' / 'block/x' embutindo a imagem no atlas.
Fonte: assets/InventivetalentDev_minecraft-assets_1_12_2_textures.zip (ou --vanilla <zip>). Sem o zip, tudo continua por nome (como antes)."""
import os, io, glob, zipfile
from PIL import Image
_Z = []   # lista de (zip, {nome_minusculo: nome}); a 1a que tiver a textura vence
ALIAS = {'block': 'blocks', 'item': 'items'}   # nomes do 1.13+ -> pastas do 1.12
def _root(): return os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
def _has_tex(z): return any(n.lower().startswith('assets/minecraft/textures/blocks/') or n.lower().startswith('blocks/') for n in z.namelist())
def sources(path=None):
    """patchMod.18: fontes de textura, em ordem de prioridade: --vanilla (1+ zips, separados por ',') > resource packs de assets/resourcepacks (na ordem do pack.cfg; so os que
    trazem textures/blocks) > assets/*minecraft-assets*textures*.zip > ~/.minecraft/versions/1.12.2/1.12.2.jar. Nao precisa ser o vanilla: o pack do jogador serve."""
    r = _root(); L = []
    if path: L += [x for x in str(path).split(',') if x]
    rp = os.path.join(r, 'assets', 'resourcepacks'); cfg = os.path.join(rp, 'pack.cfg'); names = []
    if os.path.exists(cfg):
        for ln in open(cfg, encoding='utf-8', errors='ignore'):
            if ln.startswith('pack='): names.append(ln.split('=', 1)[1].strip())
    names += sorted(os.path.basename(f) for f in glob.glob(os.path.join(rp, '*.zip')) if os.path.basename(f) not in names)
    L += [os.path.join(rp, n) for n in names]
    L += sorted(glob.glob(os.path.join(r, 'assets', '*minecraft-assets*textures*.zip')))
    L.append(os.path.join(os.path.expanduser('~'), '.minecraft', 'versions', '1.12.2', '1.12.2.jar'))
    out = []
    for f in L:
        if not os.path.exists(f) or f in [o[0] for o in out]: continue
        try:
            z = zipfile.ZipFile(f)
            if _has_tex(z): out.append((f, z))
        except Exception: pass
    return out
def init(path=None):
    global _Z
    _Z = [(z, {n.lower(): n for n in z.namelist()}) for _, z in sources(path)]
    return bool(_Z)
def used(): return [os.path.basename(z.filename) for z, _ in _Z]
def get(ref):
    """ref: 'minecraft:blocks/dirt' | 'blocks/dirt' | 'block/dirt' -> Image RGBA (1o quadro se animada) ou None"""
    if not _Z and not init(): return None
    r = ref.split(':', 1)[1] if ':' in ref else ref
    parts = r.split('/'); parts[0] = ALIAS.get(parts[0], parts[0]); r = '/'.join(parts)
    for z, N in _Z:
        for base in ('', 'assets/minecraft/textures/'):
            n = N.get((base + r + '.png').lower())
            if n:
                im = Image.open(io.BytesIO(z.read(n))).convert('RGBA')
                if im.height > im.width: im = im.crop((0, 0, im.width, im.width))
                return im
    return None
def get_ns(n, p):
    """patchConv.10: textura de OUTRO namespace (mod) vinda do resource pack do jogador (assets/<n>/textures/<p>.png) ou None"""
    if not _Z and not init(): return None
    for z, N in _Z:
        k = N.get(('assets/%s/textures/%s.png' % (n, p)).lower())
        if k:
            im = Image.open(io.BytesIO(z.read(k))).convert('RGBA')
            return im.crop((0, 0, im.width, im.width)) if im.height > im.width else im
    return None
# ---- tintura de bioma (patchMod.11): grama/folha/vinha vem em cinza no pack; o jogo nao tinge props, entao multiplicamos aqui (bioma "planicie")
GRASS = (0x91, 0xBD, 0x59); FOLIAGE = (0x77, 0xAB, 0x2F)
SPECIAL = {'spruce': (0x61, 0x99, 0x61), 'birch': (0x80, 0xA7, 0x55), 'waterlily': (0x20, 0x80, 0x30), 'lily_pad': (0x20, 0x80, 0x30)}
def tint_color(name):
    """cor de tintura pelo nome da textura (ou None)"""
    n = name.lower().split('/')[-1]
    for k, c in SPECIAL.items():
        if k in n and ('leaves' in n or 'lily' in n or 'waterlily' in n): return c
    if 'leaves' in n or 'leaf' in n and 'overlay' not in n or 'vine' in n: return FOLIAGE
    if n in ('grass_top', 'grass_side_overlay', 'tallgrass', 'fern', 'double_plant_grass_top', 'double_plant_grass_bottom', 'double_plant_fern_top', 'double_plant_fern_bottom', 'reeds', 'grass_block_top', 'grass') or n.startswith(('tallgrass', 'grass_top')): return GRASS
    return None
def is_gray(im, tol=14):
    px = [p for p in im.getdata() if p[3] > 40]
    if not px: return False
    return sum(abs(p[0] - p[1]) + abs(p[1] - p[2]) for p in px) / len(px) < tol
def tint(name, im):
    """multiplica pela cor do bioma se a textura e (quase) cinza e o nome pede tintura; senao devolve igual"""
    c = tint_color(name)
    if c is None or not is_gray(im): return im
    r, g, b, a = im.split(); f = lambda ch, k: ch.point([min(255, v * k // 255) for v in range(256)])
    from PIL import Image
    return Image.merge('RGBA', (f(r, c[0]), f(g, c[1]), f(b, c[2]), a))
