#!/usr/bin/env python3
# patchWorld.1: converte um mundo Minecraft 1.12.2 (Anvil, com mods Forge) para um save do Blockland (saves/<nome>/: chunks16.dat BL18, chunks16n.dat, level.cfg, dim.txt).
# Uso: python3 mc2bl.py <pasta_do_mundo> <saves/NOME> [--pack zip_do_pack] [--relatorio rel.txt]
import sys, os, glob, struct, pickle, argparse, collections
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE); sys.path.insert(0, os.path.join(HERE, '..'))
from nbt import level, chunks
PK0 = 1016
ap = argparse.ArgumentParser(); ap.add_argument('mundo'); ap.add_argument('saida'); ap.add_argument('--pack'); ap.add_argument('--relatorio'); ap.add_argument('--so-regioes', type=int, default=0)
args = ap.parse_args()
if args.pack: os.environ['BL_PACK'] = args.pack
import mapping2 as M
def nib(a):
    a = np.frombuffer(a, dtype=np.uint8); o = np.empty(4096, np.uint8); o[0::2] = a & 15; o[1::2] = a >> 4; return o
root = level(args.mundo + '/level.dat'); D = root['Data']
reg = {int(e['V']): e['K'] for e in root.get('FML', {}).get('Registries', {}).get('minecraft:blocks', {}).get('ids', [])}
if not reg:   # mundo vanilla: sem registro FML -> tabela de ids do 1.12.2 (names.pkl do conversor antigo)
    reg = {int(k): v for k, v in pickle.load(open(os.path.join(HERE, '..', 'names.pkl'), 'rb')).items()}
pal = {}
def pid(n):
    if n not in pal: pal[n] = len(pal)
    return PK0 + pal[n]
lutB = np.zeros(4096 * 16, np.uint16); lutM = np.zeros(4096 * 16, np.uint8); rel = collections.Counter(); relmap = {}
def build_lut(used):
    for k in np.nonzero(used)[0]:
        i, d = int(k) >> 4, int(k) & 15
        nm = reg.get(i, 'minecraft:stone'); b, m = M.resolve(nm, d)
        if isinstance(b, str): b = pid(b)
        lutB[k] = b; lutM[k] = m & 255; relmap[(nm, d)] = (b, m)
XBF = (M.XB_FENCE, M.XB_PANE, M.XB_WALL)
def solidmask(B): return ((B >= 1) & (B <= 31) & (B != 9)) | (B >= PK0)
def conn(B, Mt):   # conexoes de cerca/painel/muro (bits N(-Z)=1 L(+X)=2 S(+Z)=4 O(-X)=8)
    for xb in XBF:
        sel = B == xb
        if not sel.any(): continue
        S_ = solidmask(B) | sel
        def sh(a, ax, s):
            o = np.zeros_like(a); sa = [slice(None)] * 3; so = [slice(None)] * 3
            if s > 0: so[ax] = slice(0, -1); sa[ax] = slice(1, None)
            else: so[ax] = slice(1, None); sa[ax] = slice(0, -1)
            o[tuple(so)] = a[tuple(sa)]; return o
        mask = (sh(S_, 1, -1) * 1 + sh(S_, 2, 1) * 2 + sh(S_, 1, 1) * 4 + sh(S_, 2, -1) * 8).astype(np.uint8)
        Mt[sel] = (Mt[sel] & 0xF0) | mask[sel]
DOORS = set(i for i, n in reg.items() if n.endswith('_door') or n == 'minecraft:wooden_door'); TALL = {i for i, n in reg.items() if n == 'minecraft:double_plant'}
def conv_chunk(lv):
    B = np.zeros((256, 16, 16), np.uint16); Mt = np.zeros((256, 16, 16), np.uint8)
    for s in lv.get('Sections', []):
        y0 = s['Y'] * 16
        if y0 < 0 or y0 >= 256: continue
        b = np.frombuffer(s['Blocks'], dtype=np.uint8).astype(np.int32)
        if 'Add' in s: b = b | (nib(s['Add']).astype(np.int32) << 8)
        d = nib(s['Data']).astype(np.int32); b = b.reshape(16, 16, 16); d = d.reshape(16, 16, 16)
        if TALL:   # planta alta: a metade de cima herda o tipo da de baixo
            dp = np.isin(b, list(TALL)) & ((d & 8) != 0)
            if dp.any(): dd = np.zeros_like(d); dd[1:] = d[:-1]; d = np.where(dp, (dd & 7) | 8, d)
        dr = np.isin(b, list(DOORS)) & ((d & 8) != 0)   # porta: a metade de cima herda facing/aberta da de baixo
        if dr.any(): dd = np.zeros_like(d); dd[1:] = d[:-1]; d = np.where(dr, (dd & 7) | 8, d)
        k = b * 16 + d; B[y0:y0 + 16] = lutB[k]; Mt[y0:y0 + 16] = lutM[k]
    conn(B, Mt); return B, Mt
def rle16(a):
    a = a.ravel(); ch = np.flatnonzero(np.diff(a)) + 1; st = np.concatenate(([0], ch)); en = np.concatenate((ch, [a.size])); out = []
    for s, e, v in zip(st.tolist(), en.tolist(), a[st].tolist()):
        n = e - s
        while n > 0: r = min(n, 65535); out.append((r, v)); n -= r
    return out
def write(path, chs):
    with open(path, 'wb') as f:
        f.write(b'BL18'); f.write(struct.pack('<i', len(pal)))
        for n, _ in sorted(pal.items(), key=lambda x: x[1]):
            e = n.encode(); f.write(struct.pack('<H', len(e)) + e)
        f.write(struct.pack('<i', len(chs)))
        for cx, cz, B, Mt in chs:
            f.write(struct.pack('<ii', cx, cz)); r = rle16(B); f.write(struct.pack('<i', len(r) * 2)); f.write(np.array([x for p in r for x in p], np.uint16).tobytes())
            m = rle16(Mt); buf = b''.join(struct.pack('<HB', n, v) for n, v in m); f.write(struct.pack('<i', len(buf)) + buf)
def load_dim(pat):
    files = sorted(glob.glob(args.mundo + pat))
    if args.so_regioes: files = files[:args.so_regioes]
    out = []
    for f in files:
        for c in chunks(f):
            lv = c['Level']; out.append((lv['xPos'], lv['zPos'], lv))
    return out
def survey(dimchunks):
    cnt = np.zeros(4096 * 16, np.int64)
    for _, _, lv in dimchunks:
        for s in lv.get('Sections', []):
            b = np.frombuffer(s['Blocks'], dtype=np.uint8).astype(np.int32)
            if 'Add' in s: b = b | (nib(s['Add']).astype(np.int32) << 8)
            cnt += np.bincount(b * 16 + nib(s['Data']).astype(np.int32), minlength=4096 * 16)[:4096 * 16]
    return cnt
os.makedirs(args.saida, exist_ok=True)
dims = [('/region/*.mca', 'chunks16.dat'), ('/DIM-1/region/*.mca', 'chunks16n.dat')]
tot = collections.Counter(); resultados = {}
for pat, fn in dims:
    dc = load_dim(pat)
    if not dc: continue
    cnt = survey(dc); build_lut(cnt > 0)
    for k in np.nonzero(cnt)[0]:
        i, d = int(k) >> 4, int(k) & 15; b, m = relmap[(reg.get(i, 'minecraft:stone'), d)]; nm0 = reg.get(i, '?%d' % i); tot[(nm0, 'AR' if (b == 0 and nm0 not in ('minecraft:air', 'minecraft:structure_void')) else ('PEDRA' if (b == 3 and nm0 not in ('minecraft:stone', 'minecraft:monster_egg')) else 'ok'))] += int(cnt[k])
    out = [(cx, cz) + conv_chunk(lv) for cx, cz, lv in dc]; write(os.path.join(args.saida, fn), out); print(fn, len(out), 'chunks, paleta', len(pal), file=sys.stderr)
    del dc, out
# level.cfg / dim.txt
P = D.get('Player', {}); pos = P.get('Pos', [D.get('SpawnX', 0) + .5, D.get('SpawnY', 64), D.get('SpawnZ', 0) + .5])
px, py, pz = float(pos[0]), float(pos[1]), float(pos[2])
with open(os.path.join(args.saida, 'level.cfg'), 'w', encoding='utf-8') as f:
    f.write('name=%s\nseed=%d\nmode=%d\nhardcore=%d\nplayed=0\npx=%.3f\npy=%.3f\npz=%.3f\ntime=%d\nweather=0\nwxleft=0\n' % (D.get('LevelName', 'Mundo convertido'), int(D.get('RandomSeed', 0)), 1 if D.get('GameType', 0) == 1 else 0, D.get('hardcore', 0), px, py, pz, int(D.get('DayTime', 1000))))
open(os.path.join(args.saida, 'dim.txt'), 'w').write('1' if int(P.get('Dimension', 0)) == -1 else '0')
rep = collections.defaultdict(lambda: [0, 0, 0])
for (n, st), c in tot.items(): rep[n][{'ok': 0, 'PEDRA': 1, 'AR': 2}[st]] += c
lines = ['# Relatorio: bloco | convertido | virou pedra (sem equivalente) | virou ar']
for n, v in sorted(rep.items(), key=lambda x: -sum(x[1])): lines.append('%s | %d | %d | %d' % (n, v[0], v[1], v[2]))
open(args.relatorio or os.path.join(args.saida, 'conversao.txt'), 'w', encoding='utf-8').write('\n'.join(lines))
print('pronto: paleta', len(pal), file=sys.stderr)
