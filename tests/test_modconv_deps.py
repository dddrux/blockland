#!/usr/bin/env python3
"""patchConv.4: dependencias de mod (mcmod.info + @Mod dependencies): converte junto o que achar, marca `falta` o resto."""
import io, json, os, subprocess, sys, tempfile, zipfile
from PIL import Image
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'conversor', 'modconv')
def png():
    b = io.BytesIO(); Image.new('RGBA', (16, 16), (9, 9, 9, 255)).save(b, 'PNG'); return b.getvalue()
def mk(path, ns, info, extra=b''):
    with zipfile.ZipFile(path, 'w') as z:
        z.writestr('mcmod.info', json.dumps([info])); z.writestr('assets/%s/textures/blocks/a.png' % ns, png())
        z.writestr('assets/%s/models/block/c.json' % ns, json.dumps({'parent': 'block/cube_all', 'textures': {'all': '%s:blocks/a' % ns}}))
        z.writestr('assets/%s/blockstates/blk.json' % ns, json.dumps({'variants': {'normal': {'model': '%s:c' % ns}}}))
        if extra: z.writestr('x/Main.class', extra)
fails = n = 0
def chk(c, m):
    global fails, n
    n += 1
    if not c: fails += 1; print('  FALHOU:', m)
def run(*a): return subprocess.run([sys.executable, os.path.join(R, 'modconv.py')] + list(a), capture_output=True, text=True, cwd=t)
t = tempfile.mkdtemp(); os.makedirs(os.path.join(t, 'jars')); os.makedirs(os.path.join(t, 'dep'))
A = os.path.join(t, 'jars', 'a.jar'); mk(A, 'moda', {'modid': 'moda', 'name': 'A', 'version': '1'}, b'..dependencies\x00required-after:modb@[1,);after:jei;required-after:forge..')
mk(os.path.join(t, 'dep', 'b.jar'), 'modb', {'modid': 'modb', 'name': 'B', 'version': '1'})
db = os.path.join(t, 'db')
r = run(A, '--out', 'o1', '--db', db); chk(r.returncode == 0, r.stderr[-300:])
d = open(os.path.join(t, 'o1', 'moda', 'dependencias.txt')).read()
chk('modb required falta' in d, d); chk('jei optional falta' in d, d); chk('forge' not in d, 'forge e implicito'); chk('AVISO' in r.stdout, 'aviso de obrigatoria')
chk('modid=moda' in open(os.path.join(t, 'o1', 'moda', 'mod.txt')).read(), 'mod.txt com modid')
r = run(A, '--out', 'o2', '--db', db, '--deps-dir', os.path.join(t, 'dep')); chk(r.returncode == 0, r.stderr[-300:])
chk(os.path.isfile(os.path.join(t, 'o2', 'modb', 'props.bin')), 'dependencia convertida junto')
d = open(os.path.join(t, 'o2', 'moda', 'dependencias.txt')).read(); chk('modb required convertido junto' in d, d)
r = run(A, '--out', 'o3', '--db', db, '--deps-dir', os.path.join(t, 'dep'), '--no-deps'); chk(not os.path.exists(os.path.join(t, 'o3', 'modb')), '--no-deps nao converte')
print('%d verificacoes, %d falhas' % (n, fails)); sys.exit(1 if fails else 0)
