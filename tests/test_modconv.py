#!/usr/bin/env python3
"""patchProp.1: handler generico do conversor com um .jar sintetico (cubo, coluna, modelo com elements e rotacao, parent desconhecido)."""
import io, json, os, subprocess, sys, tempfile, zipfile, struct
from PIL import Image
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'conversor', 'modconv'); sys.path.insert(0, R)
def png(c, s=16):
    b = io.BytesIO(); Image.new('RGBA', (s, s), c).save(b, 'PNG'); return b.getvalue()
t = tempfile.mkdtemp(); jar = os.path.join(t, 'toy.jar')
with zipfile.ZipFile(jar, 'w') as z:
    P = lambda n, d: z.writestr('assets/toy/' + n, json.dumps(d))
    z.writestr('assets/toy/textures/blocks/ruby.png', png((200, 20, 60, 255))); z.writestr('assets/toy/textures/blocks/pole.png', png((90, 60, 20, 255), 32))
    P('blockstates/ruby.json', {'variants': {'normal': {'model': 'toy:ruby'}}}); P('models/block/ruby.json', {'parent': 'block/cube_all', 'textures': {'all': 'toy:blocks/ruby'}})
    P('blockstates/lamp.json', {'variants': {'': {'model': 'toy:lamp'}}})
    P('models/block/lamp.json', {'textures': {'p': 'toy:blocks/pole', 'g': 'minecraft:blocks/glass'}, 'elements': [
        {'from': [6, 0, 6], 'to': [10, 12, 10], 'faces': {'north': {'texture': '#p'}, 'up': {'texture': '#p', 'uv': [0, 0, 4, 4]}}},
        {'from': [4, 12, 4], 'to': [12, 14, 12], 'rotation': {'origin': [8, 12, 8], 'axis': 'y', 'angle': 45}, 'faces': {'south': {'texture': '#g'}}}]})
    P('blockstates/weird.json', {'variants': {'normal': {'model': 'toy:weird'}}}); P('models/block/weird.json', {'parent': 'toy:block/nao_existe', 'textures': {}})
    z.writestr('assets/toy/lang/en_us.lang', 'tile.ruby.name=Ruby Block\ntile.lamp.name=Lamp\n'); z.writestr('assets/toy/lang/pt_br.lang', 'tile.ruby.name=Bloco de Rubi\n')
out = os.path.join(t, 'o'); db = os.path.join(t, 'db')
r = subprocess.run([sys.executable, os.path.join(R, 'modconv.py'), jar, '--out', out, '--db', db], capture_output=True, text=True); print(r.stdout.strip()); assert r.returncode == 0, r.stderr
P = json.load(open(os.path.join(out, 'toy', 'props.json'))); fails = 0; n = 0
def chk(c, m):
    global fails, n; n += 1
    if not c: fails += 1; print('  FALHOU:', m)
it = {i['id']: i for i in P['items']}
chk(P['format'] == 'BLP1' and len(P['items']) == 2, 'dois blocos convertidos (o "weird" vira lacuna, nao quebra)')
chk(it['toy_ruby']['name'] == 'Bloco de Rubi' and it['toy_lamp']['en'] == 'Lamp', 'nomes: pt-BR e en')
rb = P['models'][it['toy_ruby']['model']]['boxes']; chk(len(rb) == 1 and all(f for f in rb[0][10:]) and rb[0][3:6] == [.5, .5, .5] and rb[0][0:3] == [0, .5, 0], 'cubo cheio centrado em x/z, y de 0 a 1')
lb = P['models'][it['toy_lamp']['model']]['boxes']; chk(len(lb) == 2 and abs(lb[1][6] - 0.92388) < 1e-4 and abs(lb[1][8] - 0.38268) < 1e-4, 'rotacao 45 graus em Y -> quaternio')
chk(lb[0][14] != 0 and lb[0][13] != 0 and lb[0][15] == 0 and lb[0][12] == 0, 'north(-Z)=face 4 e up=face 3 preenchidas; south(+Z)=5 e baixo=2 vazias')
chk(len(it['toy_lamp']['tex']) == 2 and (min(it['toy_lamp']['tex']) >= 0) == bool(__import__('vanilla').init()), 'vidro vanilla: embutido no atlas (com o zip vanilla) ou referencia por nome (id negativo, sem o zip)')
unk = json.load(open(os.path.join(db, 'unknown.json'))); chk('parent de modelo desconhecido' in unk, 'lacuna registrada no banco para o proximo mod')
b = open(os.path.join(out, 'toy', 'props.bin'), 'rb').read(); chk(b[:4] == b'BLPB' and struct.unpack('<I', b[4:8])[0] == 1, 'props.bin com cabecalho BLPB v1')
import vanilla as _v
from PIL import Image as _I
_g=_I.new('RGBA',(2,2),(140,140,140,255)); _t=_v.tint('blocks/leaves_oak',_g).getpixel((0,0)); chk(_t[1]>_t[0] and _t[1]>_t[2], 'folha cinza vira verde (tintura de bioma)')
_c=_I.new('RGBA',(2,2),(150,108,74,255)); chk(_v.tint('blocks/grass_top',_c).getpixel((0,0))==(150,108,74,255), 'textura colorida nao e tingida')
chk(_v.tint('blocks/stone',_g).getpixel((0,0))==(140,140,140,255), 'pedra cinza nao e tingida')
print('test_modconv: %d verificacoes, %d falhas' % (n, fails)); sys.exit(1 if fails else 0)
