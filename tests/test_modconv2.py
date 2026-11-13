#!/usr/bin/env python3
"""patchMod.1: rotacao de variante (x/y), multipart incondicional, parents cross/slab/carpet no handler generico."""
import io, json, os, subprocess, sys, tempfile, zipfile
from PIL import Image
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'conversor', 'modconv')
def png():
    b = io.BytesIO(); Image.new('RGBA', (16, 16), (9, 9, 9, 255)).save(b, 'PNG'); return b.getvalue()
t = tempfile.mkdtemp(); jar = os.path.join(t, 't.jar')
with zipfile.ZipFile(jar, 'w') as z:
    P = lambda n, d: z.writestr('assets/t2/' + n, json.dumps(d))
    z.writestr('assets/t2/textures/blocks/a.png', png())
    T = {'parent': 'block/cube_all', 'textures': {'all': 't2:blocks/a'}}
    P('models/block/c.json', T)
    P('models/block/post.json', {'textures': {'a': 't2:blocks/a'}, 'elements': [{'from': [0, 0, 0], 'to': [4, 8, 4], 'faces': {'up': {'texture': '#a'}}}]})
    P('blockstates/rot.json', {'variants': {'normal': {'model': 't2:post', 'y': 90}}})
    P('blockstates/multi.json', {'multipart': [{'apply': {'model': 't2:post'}}, {'when': {'north': 'true'}, 'apply': {'model': 't2:c'}}, {'apply': {'model': 't2:c'}}]})
    P('models/block/plant.json', {'parent': 'block/cross', 'textures': {'cross': 't2:blocks/a'}})
    P('blockstates/plant.json', {'variants': {'normal': {'model': 't2:plant'}}})
    P('models/block/sl.json', {'parent': 'block/slab', 'textures': {'top': 't2:blocks/a', 'bottom': 't2:blocks/a', 'side': 't2:blocks/a'}})
    P('blockstates/sl.json', {'variants': {'normal': {'model': 't2:sl'}}})
out = os.path.join(t, 'o'); db = os.path.join(t, 'db')
r = subprocess.run([sys.executable, os.path.join(R, 'modconv.py'), jar, '--out', out, '--db', db], capture_output=True, text=True); assert r.returncode == 0, r.stderr
P = json.load(open(os.path.join(out, 't2', 'props.json'))); it = {i['id']: P['models'][i['model']]['boxes'] for i in P['items']}
fails = 0; n = 0
def chk(c, m):
    global fails, n; n += 1
    if not c: fails += 1; print('  FALHOU:', m)
b = it['t2_rot'][0]
chk(abs(b[0] - (-0.25)) < 1e-4 or abs(b[2] - (-0.25)) < 1e-4 or abs(b[0]) > 0.2, 'y=90 gira a caixa em torno do centro')
chk(abs(abs(b[6]) - 0.70711) < 1e-3 and abs(abs(b[8]) - 0.70711) < 1e-3, 'quaternion da variante (90 graus em Y)')
chk(len(it['t2_multi']) == 2, 'multipart: 1 incondicional + 1 sem when (o com when e ignorado)')
chk(len(it['t2_plant']) == 2 and len(it['t2_sl']) == 1 and it['t2_sl'][0][4] == .25, 'cross = 2 planos; slab = metade de baixo')
print('test_modconv2: %d verificacoes, %d falhas' % (n, fails)); sys.exit(1 if fails else 0)
