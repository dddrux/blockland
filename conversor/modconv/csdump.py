"""Diagnostico: python3 csdump.py <mod.jar> <assetID> -> mostra id, u16 final, pos e tamanho de cada no de um .csmodel (para ensinar a hierarquia)."""
import io, re, sys, zipfile
sys.path.insert(0, __file__.rsplit('/', 1)[0] if '/' in __file__ else '.')
import csmodel
jar = zipfile.ZipFile(sys.argv[1]); name = next(n for n in jar.namelist() if re.search(r'Decopack/.*\.cspack$', n))
cs = zipfile.ZipFile(io.BytesIO(jar.read(name))); nodes, _ = csmodel.parse(cs.read('Assets/Models/%s.csmodel' % sys.argv[2]))
for i, n in enumerate(nodes): print(i, 'id=%d' % n['id'], 'tail=%d' % n['tail'], n['name'], [round(v, 3) for v in n['pos']], n['size'], [round(v, 2) for v in n['q']])
