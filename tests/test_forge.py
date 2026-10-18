#!/usr/bin/env python3
"""patchMod.4: tradutor de acoes do Forge (forge.py): varredura de .class, receitas, drops e sons com um jar sintetico."""
import io, json, os, struct, subprocess, sys, tempfile, zipfile
from PIL import Image
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'conversor', 'modconv')
def u(s): b = s.encode(); return b'\x01' + struct.pack('>H', len(b)) + b
def cls(i): return b'\x07' + struct.pack('>H', i)
def nat(a, b): return b'\x0c' + struct.pack('>HH', a, b)
def ref(a, b): return b'\x0a' + struct.pack('>HH', a, b)
cp = [u('net/minecraft/block/Block'), cls(1), u('setHardness'), u('(F)V'), nat(3, 4), ref(2, 5), u('net/minecraft/tileentity/TileEntity'), cls(7), u('mymod/Foo'), cls(9), u('onBlockActivated'), nat(11, 4), ref(2, 12), u('net/minecraft/block/Block'), ref(2, 12)]
klass = b'\xca\xfe\xba\xbe' + struct.pack('>HHH', 0, 52, len(cp) + 1) + b''.join(cp) + struct.pack('>HHHH', 0x21, 10, 8, 0) + struct.pack('>HHH', 0, 0, 0)
def png(): b = io.BytesIO(); Image.new('RGBA', (16, 16), (1, 2, 3, 255)).save(b, 'PNG'); return b.getvalue()
t = tempfile.mkdtemp(); jar = os.path.join(t, 'f.jar')
with zipfile.ZipFile(jar, 'w') as z:
    P = lambda n, d: z.writestr('assets/fm/' + n, json.dumps(d))
    z.writestr('mymod/Foo.class', klass); z.writestr('assets/fm/textures/blocks/a.png', png())
    P('blockstates/a.json', {'variants': {'normal': {'model': 'fm:a'}}}); P('models/block/a.json', {'parent': 'block/cube_all', 'textures': {'all': 'fm:blocks/a'}})
    P('recipes/a.json', {'type': 'minecraft:crafting_shaped', 'pattern': ['ii', 'ii'], 'key': {'i': {'type': 'forge:ore_dict', 'ore': 'ingotCopper'}}, 'result': {'item': 'fm:a', 'count': 2}})
    P('recipes/b.json', {'type': 'minecraft:crafting_shapeless', 'ingredients': [{'item': 'fm:a'}, {'item': 'minecraft:dye', 'data': 4}], 'result': {'item': 'fm:b'}})
    P('recipes/c.json', {'type': 'minecraft:smelting', 'ingredient': {'item': 'fm:a'}, 'result': {'item': 'minecraft:iron_ingot'}, 'experience': 0.7})
    P('recipes/_constants.json', [{'x': 1}])
    P('loot_tables/blocks/a.json', {'pools': [{'rolls': 1, 'entries': [{'type': 'item', 'name': 'fm:gem', 'functions': [{'function': 'set_count', 'count': {'min': 1, 'max': 3}}], 'conditions': [{'condition': 'minecraft:survives_explosion'}]}]}]})
    P('sounds.json', {'fm.hum': {'sounds': ['fm:hum1', {'name': 'hum2'}]}})
out = os.path.join(t, 'o'); db = os.path.join(t, 'db')
r = subprocess.run([sys.executable, os.path.join(R, 'modconv.py'), jar, '--out', out, '--db', db], capture_output=True, text=True); assert r.returncode == 0, r.stderr
d = os.path.join(out, 'fm'); rd = lambda f: open(os.path.join(d, f), encoding='utf-8').read().splitlines()
fails = 0; n = 0
def chk(c, m):
    global fails, n; n += 1
    if not c: fails += 1; print('  FALHOU:', m)
rc = rd('receitas.txt')
chk('shaped fm:a 2 ii|ii i=ore:ingotCopper' in rc, 'shaped com ore dict: %r' % rc)
chk('shapeless fm:b 1 fm:a,minecraft:dye@4' in rc, 'shapeless com data')
chk(any(l.startswith('smelt fm:a minecraft:iron_ingot') for l in rc) and len(rc) == 3, 'smelting; arquivo _constants ignorado')
chk(rd('drops.txt') == ['a -> fm:gem 1-3 se survives_explosion'], 'drop com faixa e condicao: %r' % rd('drops.txt'))
chk(rd('sons.txt') == ['fm.hum fm:hum1', 'fm.hum fm:hum2'], 'sons')
A = json.load(open(os.path.join(d, 'forge_acoes.json')))
chk(A['net/minecraft/block/Block.setHardness']['status'] == 'aproximado', 'setHardness classificado')
chk(A['net/minecraft/block/Block.onBlockActivated']['status'] == 'aproximado', 'onBlockActivated classificado')
chk(A['net/minecraft/tileentity/TileEntity.<extends>']['status'] == 'sem equivalente', 'extends TileEntity = sem equivalente')
U = json.load(open(os.path.join(db, 'unknown.json')))
chk('mc/tileentity/TileEntity.<extends>' in U.get('Forge: sem equivalente', {}), 'lacuna acumulada no db')
print('test_forge: %d verificacoes, %d falhas' % (n, fails)); sys.exit(1 if fails else 0)
