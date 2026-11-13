"""Handler do IronChest (patchMod.9): baus (TESR, sem modelo JSON) -> props Container com modelo montado do layout 64x64 da textura model/chest/*.png.
Os itens de upgrade (textures/items/**) vem do fallback do generic. Capacidade: o Container de prop tem 27 slots (limite do jogo, nao do modelo)."""
import io
from PIL import Image
from core import handler
import pack
from handlers.generic import Generic
PT = {'iron': 'Ferro', 'gold': 'Ouro', 'diamond': 'Diamante', 'copper': 'Cobre', 'silver': 'Prata', 'crystal': 'Cristal', 'obsidian': 'Obsidiana', 'dirt': 'Terra'}
def _faces(u, v, w, h, d):   # layout ModelBox 64x64: +X=desloc.0, -X=d+w, base, topo, -Z=costas, +Z=frente
    f = lambda a, b, c, e: [0, a / 64, b / 64, c / 64, e / 64]
    return [f(u + d + w, v + d, u + 2 * d + w, v + d + h), f(u, v + d, u + d, v + d + h), f(u + d + w, v, u + d + 2 * w, v + d), f(u + d, v, u + d + w, v + d),
            f(u + 2 * d + w, v + d, u + 2 * d + 2 * w, v + d + h), f(u + d, v + d, u + d + w, v + d + h)]
def _box(x0, y0, z0, x1, y1, z1, u, v):
    w, h, d = x1 - x0, y1 - y0, z1 - z0
    return [round(x, 5) for x in ((x0 + x1) / 32 - .5, (y0 + y1) / 32, (z0 + z1) / 32 - .5, w / 32, h / 32, d / 32, 1, 0, 0, 0)] + _faces(u, v, w, h, d)
@handler
class IronChest(Generic):
    name = 'ironchest'
    def detect(self, jar): return 'ironchest' in jar.namespaces()
    def extra(self, jar, ctx, ns, atlas, models, items, tid, lang, pt, used):
        for f in sorted(jar.find(r'^assets/ironchest/textures/model/chest/[a-z_]+_chest\.png$')):
            k = f.rsplit('/', 1)[1][:-10]; im = Image.open(io.BytesIO(jar.read(f))).convert('RGBA')
            if im.size != (64, 64): ctx.gap('textura de bau com tamanho inesperado', f); continue
            boxes = [_box(1, 0, 1, 15, 10, 15, 0, 19), _box(1, 10, 1, 15, 15, 15, 0, 0), _box(7, 8, 15, 9, 12, 16, 0, 0)]
            models.append(dict(boxes=boxes, bbox=pack.bbox(boxes)))
            items.append(dict(id='ironchest_%s_chest' % k, name='Baú de %s' % PT.get(k, k), en='%s Chest' % k.capitalize(), tab='ironchest', model=len(models) - 1, tex=[atlas.add(im)], beh='Container'))
