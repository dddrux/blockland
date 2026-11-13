"""patchF9: modelos .obj do Forge (loader OBJ: "model": "mod:peca.obj" no blockstate). O Blockland so tem caixas, entao o OBJ vira UMA caixa (bounding box
dos vertices `v`), com a textura da variante ("textures") ou do .mtl (map_Kd). Aproximacao declarada no relatorio; melhor que perder o bloco."""
import re
def find(jar, ns, ref):
    n, p = ref.split(':', 1) if ':' in ref else (ns, ref)
    for c in ('assets/%s/models/block/%s' % (n, p), 'assets/%s/models/%s' % (n, p), 'assets/%s/models/block/%s' % (n, p.split('/')[-1]), 'assets/%s/models/item/%s' % (n, p)):
        for k in (c, c.lower()):
            if jar.has(k): return k
def bbox(text):
    vs = [tuple(map(float, m.groups())) for m in re.finditer(r'^v\s+(-?[\d.eE+-]+)\s+(-?[\d.eE+-]+)\s+(-?[\d.eE+-]+)', text, re.M)]
    if not vs: return None
    lo = [min(v[i] for v in vs) for i in range(3)]; hi = [max(v[i] for v in vs) for i in range(3)]
    big = max(abs(x) for x in lo + hi)
    k = 16.0 if big <= 2.0 else 1.0   # OBJ em unidades de bloco (0..1) ou em pixels (0..16)
    lo = [max(-8.0, min(24.0, lo[i] * k + (0 if big > 2.0 else 0))) for i in range(3)]; hi = [max(-8.0, min(24.0, hi[i] * k)) for i in range(3)]
    for i in range(3):
        if hi[i] - lo[i] < 0.5: hi[i] = lo[i] + 0.5
    return lo, hi
def boxes(jar, ns, ref, variant, ctx, name=''):
    """-> (elements, textures) ou (None, {})."""
    f = find(jar, ns, ref)
    if not f: ctx.gap('modelo OBJ nao encontrado', ref); return None, {}
    text = jar.text(f); bb = bbox(text)
    if not bb: ctx.gap('modelo OBJ sem vertices', ref); return None, {}
    tx = dict(variant.get('textures') or {}); tex = next((v for v in tx.values() if isinstance(v, str)), None)
    if not tex:
        m = re.search(r'^mtllib\s+(\S+)', text, re.M)
        mf = find(jar, ns, f.split('/models/', 1)[0].split('/')[1] + ':' + (m.group(1) if m else '')) if m else None
        if mf:
            mm = re.search(r'map_Kd\s+(\S+)', jar.text(mf))
            if mm: tex = mm.group(1).replace('.png', '')
    if not tex: ctx.gap('modelo OBJ sem textura', ref); return None, {}
    ctx.gap('modelo OBJ aproximado por caixa', name or ref)
    lo, hi = bb
    return [dict(from_=lo, to=hi, faces={k: dict(texture='#t', uv=[0, 0, 16, 16]) for k in ('down', 'up', 'north', 'south', 'west', 'east')})], {'t': tex}
