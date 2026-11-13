"""Formato canonico BLP1 (Blockland Props) + empacotador de atlas. Todo handler produz ISTO; o jogo so le BLP1.
Caixa: [cx,cy,cz, hx,hy,hz, qw,qx,qy,qz, f0..f5]  (blocos; x/z centrados no bloco, y=0 no chao; h = meio-tamanho)
Face fi (ordem do entFace do jogo): 0=-X 1=+X 2=-Y 3=+Y 4=-Z 5=+Z ; cada face = 0 (oculta) ou [slot,u0,v0,u1,v1] (UV normalizada 0..1)
Cantos de cada face (visto de fora, sentido TL,TR,BR,BL; uv do canto = (u0|u1, v0|v1)) em CORNERS."""
import hashlib, json, math, os
from PIL import Image
CORNERS = {
 0: [(-1,1,-1,0,0),(-1,1,1,1,0),(-1,-1,1,1,1),(-1,-1,-1,0,1)],
 1: [(1,1,1,0,0),(1,1,-1,1,0),(1,-1,-1,1,1),(1,-1,1,0,1)],
 2: [(-1,-1,1,0,0),(1,-1,1,1,0),(1,-1,-1,1,1),(-1,-1,-1,0,1)],
 3: [(-1,1,-1,0,0),(1,1,-1,1,0),(1,1,1,1,1),(-1,1,1,0,1)],
 4: [(1,1,-1,0,0),(-1,1,-1,1,0),(-1,-1,-1,1,1),(1,-1,-1,0,1)],
 5: [(-1,1,1,0,0),(1,1,1,1,0),(1,-1,1,1,1),(-1,-1,1,0,1)]}   # tuplas: sx,sy,sz (sinais x meio-tamanho), cu, cv (0=u0/v0, 1=u1/v1)
def qrot(q, v):
    w, x, y, z = q
    t = (2*(y*v[2]-z*v[1]), 2*(z*v[0]-x*v[2]), 2*(x*v[1]-y*v[0]))
    return (v[0]+w*t[0]+(y*t[2]-z*t[1]), v[1]+w*t[1]+(z*t[0]-x*t[2]), v[2]+w*t[2]+(x*t[1]-y*t[0]))
def qmul(a, b):
    aw, ax, ay, az = a; bw, bx, by, bz = b
    return (aw*bw-ax*bx-ay*by-az*bz, aw*bx+ax*bw+ay*bz-az*by, aw*by-ax*bz+ay*bw+az*bx, aw*bz+ax*by-ay*bx+az*bw)
def qaxis(axis, deg):
    a = math.radians(deg) / 2; s = math.sin(a); c = math.cos(a)
    return (c, s if axis == 'x' else 0, s if axis == 'y' else 0, s if axis == 'z' else 0)
def bbox(boxes):
    mn = [9e9]*3; mx = [-9e9]*3
    for b in boxes:
        q = b[6:10]
        for sx in (-1,1):
            for sy in (-1,1):
                for sz in (-1,1):
                    p = qrot(q, (sx*b[3], sy*b[4], sz*b[5]))
                    for i in range(3):
                        c = p[i] + b[i]; mn[i] = min(mn[i], c); mx[i] = max(mx[i], c)
    return [round(v, 4) for v in mn + mx] if boxes else [0]*6
class Atlas:
    """Dedup por conteudo + prateleiras em paginas (pag. W x ate H). tex(img)->id."""
    def __init__(self, W=4096, H=4096): self.W, self.H = W, H; self.imgs = []; self.by = {}; self.refs = {}
    def add(self, img):
        img = img.convert('RGBA'); h = hashlib.md5(img.tobytes() + repr(img.size).encode()).hexdigest()
        if h not in self.by: self.by[h] = len(self.imgs); self.imgs.append(img)
        return self.by[h]
    def ref(self, name):   # textura so por nome (ex.: vanilla "minecraft:blocks/stone"): o jogo resolve pelo resource pack
        if name not in self.refs: self.refs[name] = -1 - len(self.refs)
        return self.refs[name]
    def write(self, outdir, prefix='props_atlas'):
        order = sorted(range(len(self.imgs)), key=lambda i: (-self.imgs[i].height, -self.imgs[i].width))
        pages = []; pos = {}; cur = None
        def newpage(): pages.append(dict(img=Image.new('RGBA', (self.W, self.H)), x=0, y=0, rowh=0, maxy=0)); return pages[-1]
        for i in order:
            im = self.imgs[i]; w, h = im.size
            if w > self.W or h > self.H: raise ValueError('textura %dx%d maior que a pagina' % (w, h))
            if cur is None: cur = newpage()
            if cur['x'] + w > self.W: cur['y'] += cur['rowh']; cur['x'] = 0; cur['rowh'] = 0
            if cur['y'] + h > self.H: cur = newpage()
            cur['img'].paste(im, (cur['x'], cur['y'])); pos[i] = (len(pages)-1, cur['x'], cur['y'], w, h)
            cur['x'] += w; cur['rowh'] = max(cur['rowh'], h); cur['maxy'] = max(cur['maxy'], cur['y'] + h)
        files = []
        for n, p in enumerate(pages):
            hh = 1
            while hh < p['maxy']: hh *= 2
            f = '%s_%d.png' % (prefix, n); p['img'].crop((0, 0, self.W, hh)).save(os.path.join(outdir, f), optimize=True); files.append(dict(file=f, w=self.W, h=hh))
        tex = [dict(zip(('page','x','y','w','h'), pos[i])) for i in range(len(self.imgs))]
        for name, k in self.refs.items(): tex.append(dict(ref=name, id=k))
        return files, tex
def write_blp(path, mod, name, files, tex, models, items, extra=None):
    d = dict(format='BLP1', mod=mod, name=name, pages=files, textures=tex, models=models, items=items)
    if extra: d.update(extra)
    with open(path, 'w', encoding='utf-8') as f: json.dump(d, f, ensure_ascii=False, separators=(',', ':'))

def write_bin(path, files, tex, models, items):
    """props.bin (little-endian) lido por src/props.inc: 'BLPB',u32 versao=1; paginas; texturas; modelos; itens. Strings = u16 len + utf8."""
    import struct
    o = bytearray(b'BLPB'); P = lambda f, *a: o.extend(struct.pack('<' + f, *a))
    def S(s): e = s.encode('utf-8'); P('H', len(e)); o.extend(e)
    P('I', 1); P('I', len(files))
    for f in files: S(f['file']); P('II', f['w'], f['h'])
    P('I', len(tex))
    for t in tex:
        if 'ref' in t: P('b', 1); S(t['ref'])
        else: P('b', 0); P('iiiii', t['page'], t['x'], t['y'], t['w'], t['h'])
    P('I', len(models))
    for m in models:
        P('6f', *m['bbox']); P('I', len(m['boxes']))
        for b in m['boxes']:
            P('10f', *b[:10])
            for f in b[10:16]:
                if f: P('b4f', f[0], f[1], f[2], f[3], f[4]) if False else (P('b', f[0]), P('4f', f[1], f[2], f[3], f[4]))
                else: P('b', -1)
    P('I', len(items))
    for it in items:
        S(it['id']); S(it['name']); S(it['tab']); P('II', it['model'], it.get('light', 0)); P('B', 1 if it.get('nocoll') else 0); S(it.get('beh', ''))
        P('B', len(it['tex']))
        for t in it['tex']: P('i', t)
    open(path, 'wb').write(bytes(o))
