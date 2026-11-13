# Levantamento dos blocos usados num mundo 1.12.2 (Anvil): contagem por (id,meta) e nomes pelo registro do level.dat (FML), inclusive mods.
import sys, glob, pickle, os, numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from nbt import level, chunks
def nib(a):
    a = np.frombuffer(a, dtype=np.uint8); o = np.empty(4096, np.uint8); o[0::2] = a & 15; o[1::2] = a >> 4; return o
def registro(world):
    root = level(world + '/level.dat'); reg = root.get('FML', {}).get('Registries', {}).get('minecraft:blocks', {}).get('ids', [])
    return {int(e['V']): e['K'] for e in reg}
def survey(world):
    res = {}
    for dim, pat in (('o', '/region/*.mca'), ('n', '/DIM-1/region/*.mca'), ('e', '/DIM1/region/*.mca')):
        cnt = np.zeros(65536 * 16, np.int64); nch = 0
        for f in sorted(glob.glob(world + pat)):
            for c in chunks(f):
                lv = c['Level']; nch += 1
                for s in lv.get('Sections', []):
                    b = np.frombuffer(s['Blocks'], dtype=np.uint8).astype(np.int32)
                    if 'Add' in s: b = b | (nib(s['Add']).astype(np.int32) << 8)
                    cnt += np.bincount(b * 16 + nib(s['Data']).astype(np.int32), minlength=65536 * 16)[:65536 * 16]
        res[dim] = (cnt, nch); print(dim, nch, file=sys.stderr)
    return res
if __name__ == '__main__':
    w = sys.argv[1]; r = survey(w); reg = registro(w); pickle.dump({'survey': r, 'reg': reg}, open(sys.argv[2], 'wb'))
