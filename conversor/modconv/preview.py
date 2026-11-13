#!/usr/bin/env python3
"""Folha de contato de um BLP1 (verificacao visual do conversor). Uso: preview.py <pasta_do_mod> <saida.png> [id_item ...]"""
import json, math, os, sys
from PIL import Image
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pack
def render(P, pages, item, S=150, yaw=35):
    m = P['models'][item['model']]; img = Image.new('RGBA', (S, S), (60, 70, 90, 255)); ys = math.radians(yaw)
    bb = m['bbox']; ext = max(bb[3] - bb[0], bb[4] - bb[1], bb[5] - bb[2], .5); k = S * .62 / ext
    ca, sa = math.cos(ys), math.sin(ys); tilt = .55
    def proj(p):   # rotaciona em Y e inclina: x' direita, y' para baixo
        x = p[0] * ca - p[2] * sa; z = p[0] * sa + p[2] * ca
        return (S / 2 + x * k, S * .85 - (p[1] * math.cos(tilt) - z * math.sin(tilt)) * k, z * math.cos(tilt) + p[1] * math.sin(tilt))
    quads = []
    for b in m['boxes']:
        q = b[6:10]
        for f in range(6):
            fd = b[10 + f]
            if not fd: continue
            pts = []
            for (sx, sy, sz, cu, cv) in pack.CORNERS[f]:
                v = pack.qrot(q, (sx * b[3], sy * b[4], sz * b[5])); pts.append(proj((v[0] + b[0], v[1] + b[1], v[2] + b[2])))
            ax, ay = pts[1][0] - pts[0][0], pts[1][1] - pts[0][1]; bx, by = pts[3][0] - pts[0][0], pts[3][1] - pts[0][1]
            if ax * by - ay * bx >= 0: continue   # costas
            quads.append((sum(p[2] for p in pts) / 4, pts, fd))
    t = P['textures'][item['tex'][0]]; pg = pages[t['page']]
    for _, pts, fd in sorted(quads, key=lambda x: -x[0]):
        _, u0, v0, u1, v1 = fd; tw, th = t['w'], t['h']
        x0, x1 = t['x'] + u0 * tw, t['x'] + u1 * tw; y0, y1 = t['y'] + v0 * th, t['y'] + v1 * th
        w = max(1, int(round(abs(x1 - x0)))); h = max(1, int(round(abs(y1 - y0))))
        tile = pg.crop((int(round(min(x0, x1))), int(round(min(y0, y1))), int(round(min(x0, x1))) + w, int(round(min(y0, y1))) + h)).convert('RGBA')
        if x1 < x0: tile = tile.transpose(Image.FLIP_LEFT_RIGHT)
        if y1 < y0: tile = tile.transpose(Image.FLIP_TOP_BOTTOM)
        # afim: (0,0)->TL (w,0)->TR (0,h)->BL ; PIL transform usa mapeamento inverso
        (X0, Y0), (X1, Y1), (X3, Y3) = [(p[0], p[1]) for p in (pts[0], pts[1], pts[3])]
        det = (X1 - X0) * (Y3 - Y0) - (Y1 - Y0) * (X3 - X0)
        if abs(det) < 1e-6: continue
        # coef inverso: (X,Y)->(u,v) em pixels da tile
        a = (Y3 - Y0) / det * w; b2 = -(X3 - X0) / det * w; c = -(a * X0 + b2 * Y0)
        d = -(Y1 - Y0) / det * h; e = (X1 - X0) / det * h; f_ = -(d * X0 + e * Y0)
        layer = tile.transform((S, S), Image.AFFINE, (a, b2, c, d, e, f_), Image.NEAREST)
        mask = Image.new('L', (S, S), 0); from PIL import ImageDraw; ImageDraw.Draw(mask).polygon([(p[0], p[1]) for p in pts], fill=255)
        alpha = Image.composite(layer.getchannel('A'), Image.new('L', (S, S), 0), mask); layer.putalpha(alpha); img.alpha_composite(layer)
    return img
if __name__ == '__main__':
    d = sys.argv[1]; P = json.load(open(os.path.join(d, 'props.json'))); pages = [Image.open(os.path.join(d, p['file'])).convert('RGBA') for p in P['pages']]
    ids = sys.argv[3:] or [i['id'] for i in P['items'][:12]]; by = {i['id']: i for i in P['items']}
    cols = min(len(ids), 4); rows = (len(ids) + cols - 1) // cols; sheet = Image.new('RGBA', (cols * 300, rows * 300))
    for n, i in enumerate(ids):
        for j, yaw in enumerate((35, 215)): sheet.alpha_composite(render(P, pages, by[i], 150, yaw), ((n % cols) * 300 + j * 150, (n // cols) * 300))
        print(i, by[i]['name'], by[i]['bbox'] if 'bbox' in by[i] else P['models'][by[i]['model']]['bbox'])
    sheet.convert('RGB').save(sys.argv[2])
