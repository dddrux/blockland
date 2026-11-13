# Gera JPEGs de teste (PIL) + o RGB que o PIL decodifica, para comparar com jpegDecode().
import os, random
from PIL import Image, ImageDraw
os.makedirs('/tmp/jt', exist_ok=True); random.seed(1)
def base(w, h):
    im = Image.new('RGB', (w, h)); px = im.load()
    for y in range(h):
        for x in range(w): px[x, y] = (x * 255 // w, y * 255 // h, (x + y) * 255 // (w + h))
    d = ImageDraw.Draw(im); d.rectangle([w // 4, h // 4, w // 2, h // 2], fill=(220, 30, 30)); d.ellipse([w // 2, h // 3, w - 10, h - 10], fill=(20, 200, 90)); return im
def noisy(w, h):
    im = base(w, h); px = im.load()
    for _ in range(w * h // 3): px[random.randrange(w), random.randrange(h)] = tuple(random.randrange(256) for _ in range(3))
    return im
specs = [('a420', base(480, 480), dict(quality=70, subsampling=2)), ('a444', base(203, 117), dict(quality=85, subsampling=0)), ('a422', base(331, 250), dict(quality=75, subsampling=1)),
         ('gray', base(100, 70).convert('L'), dict(quality=80)), ('big', noisy(900, 600), dict(quality=90, subsampling=0)), ('rst', base(320, 240), dict(quality=75, subsampling=2, restart_marker_blocks=5))]
for n, im, kw in specs:
    im.save(f'/tmp/jt/{n}.jpg', 'JPEG', **kw); r = Image.open(f'/tmp/jt/{n}.jpg').convert('RGB'); open(f'/tmp/jt/{n}.rgb', 'wb').write(r.tobytes()); open(f'/tmp/jt/{n}.txt', 'w').write(f'{r.width} {r.height}')
