"""Leitor de .csmodel (CraftStudio, usado pelo Decocraft). Formato descoberto por engenharia reversa (ver doc/MODCONV.md).
Registro de no: i32 id | u8 len | nome | 13 floats LE (pos3, offset3, escala3, quat w,x,y,z) | 3 x u16 tamanho(px) | u8 1 | 12 x i32 UV (6 faces) + i32 0 | u16 0.
Ordem das 6 origens UV: frente, tras, esquerda, base, direita, topo (desdobramento padrao do CraftStudio)."""
import struct
REC = 52 + 6 + 1 + 52 + 2
def parse(b):
    png = b.find(b'\x89PNG'); end = b.find(b'IEND', png) + 8 if png >= 0 else -1
    if png < 0: raise ValueError('sem PNG')
    p = 7
    nodes = []
    while p < png - 4:
        # id i32 (root usa -65536 por causa do cabecalho), len, nome
        i = struct.unpack_from('<i', b, p)[0]; q = p + 4
        nl = b[q]; name = b[q + 1:q + 1 + nl].decode('latin1'); q += 1 + nl
        if q + REC > len(b): raise ValueError('registro truncado')
        f = struct.unpack_from('<13f', b, q); sz = struct.unpack_from('<3H', b, q + 52)
        uv = struct.unpack_from('<13i', b, q + 59)
        tail = struct.unpack_from('<H', b, q + REC - 2)[0]
        nodes.append(dict(id=i, name=name, pos=f[0:3], off=f[3:6], sc=f[6:9], q=f[9:13], size=sz, uv=uv[:12], tail=tail))
        p = q + REC
    if p != png - 4: raise ValueError('desalinhado %d != %d' % (p, png - 4))
    return nodes, b[png:end]

# ---------- hierarquia (patchProp.3) ----------
# O .csmodel e uma arvore: pos/quat/escala de um no sao RELATIVOS ao pai. O formato do vinculo pai/filho ainda nao foi confirmado
# (campo `id` i32 ou o u16 final de cada registro), entao testamos varias leituras e o conversor escolhe a que deixa os modelos
# mais centrados e com tamanho plausivel (score). A escolha vai para o RELATORIO.
def qmul(a, b):
    aw, ax, ay, az = a; bw, bx, by, bz = b
    return (aw*bw - ax*bx - ay*by - az*bz, aw*bx + ax*bw + ay*bz - az*by, aw*by - ax*bz + ay*bw + az*bx, aw*bz + ax*by - ay*bx + az*bw)
def qrot(q, v):
    w, x, y, z = q
    t = (2*(y*v[2]-z*v[1]), 2*(z*v[0]-x*v[2]), 2*(x*v[1]-y*v[0]))
    return (v[0]+w*t[0]+(y*t[2]-z*t[1]), v[1]+w*t[1]+(z*t[0]-x*t[2]), v[2]+w*t[2]+(x*t[1]-y*t[0]))
def _valid(par):
    n = len(par)
    for i, p in enumerate(par):
        if p is None or p < -1 or p >= n or p == i: return False
        seen = 0; j = p
        while j != -1:
            seen += 1
            if seen > n: return False
            j = par[j]
    return True
def par_flat(nodes): return [-1] * len(nodes)
def par_childcount(nodes):   # u16 final = numero de filhos; nos em pre-ordem
    par = [-2] * len(nodes); stack = []   # (indice, filhos restantes)
    for i, n in enumerate(nodes):
        while stack and stack[-1][1] == 0: stack.pop()
        par[i] = stack[-1][0] if stack else -1
        if stack: stack[-1][1] -= 1
        if n['tail']: stack.append([i, n['tail']])
    return par
def par_id_index(nodes):   # id i32 = indice do pai (negativo = raiz)
    return [(n['id'] if 0 <= n['id'] < len(nodes) else -1) for n in nodes]
def par_id_low(nodes): return [((n['id'] & 0xFFFF) if (n['id'] & 0xFFFF) < len(nodes) else -1) for n in nodes]
def par_id_high(nodes): return [(((n['id'] >> 16) & 0xFFFF) if ((n['id'] >> 16) & 0xFFFF) < len(nodes) else -1) for n in nodes]
def par_tail_index(nodes): return [(n['tail'] if n['tail'] < len(nodes) else -1) for n in nodes]
def par_tail_id(nodes):   # u16 final = id do pai (procura entre os ids)
    by = {n['id']: i for i, n in enumerate(nodes)}
    return [by.get(n['tail'], -1) if n['tail'] != 0xFFFF else -1 for n in nodes]
PARENTS = dict(flat=par_flat, childcount=par_childcount, id_index=par_id_index, id_low16=par_id_low, id_high16=par_id_high, tail_index=par_tail_index, tail_id=par_tail_id)
def compose(nodes, par, inherit_scale=True):
    """Devolve nos com pos/q/sc em coordenadas de MODELO (pais ja aplicados); `off` continua local ao no."""
    out = [None] * len(nodes)
    def world(i):
        if out[i] is not None: return out[i]
        n = nodes[i]; p = par[i]
        if p < 0: out[i] = dict(n)
        else:
            P = world(p); psc = P['sc'] if inherit_scale else (1, 1, 1)
            o = qrot(P['q'], tuple(n['pos'][k] * psc[k] for k in range(3)))
            d = dict(n); d['pos'] = tuple(P['pos'][k] + o[k] for k in range(3)); d['q'] = qmul(P['q'], n['q'])
            d['sc'] = tuple(P['sc'][k] * n['sc'][k] for k in range(3)) if inherit_scale else n['sc']
            out[i] = d
        return out[i]
    for i in range(len(nodes)): world(i)
    return out

def par_pair(nodes):
    """Decocraft: id i32 = (pai<<16)|proprio (16 bits baixos do id do pai). A raiz (id<0) TAMBEM e pai: tem pos/rotacao proprias em varios modelos."""
    by = {}
    for i, n in enumerate(nodes): by[n['id'] & 0xFFFF] = i
    out = []
    for i, n in enumerate(nodes):
        if n['id'] < 0: out.append(-1); continue
        p = by.get((n['id'] >> 16) & 0xFFFF, -1); out.append(p if p != i else -1)
    return out
def has_pair(nodes): return any(p >= 0 for p in par_pair(nodes))
def compose_pair(nodes):
    """Filho: pivo = pivo do pai + rot(q_pai, off_pai + pos_filho); q composta; escala NAO herdada (so o tamanho leva escala)."""
    par = par_pair(nodes); out = [None] * len(nodes)
    def world(i):
        if out[i] is not None: return out[i]
        n = nodes[i]; p = par[i]; d = dict(n)
        if p >= 0:
            P = world(p); o = qrot(P['q'], tuple(P['off'][k] + n['pos'][k] for k in range(3)))
            d['pos'] = tuple(P['pos'][k] + o[k] for k in range(3)); d['q'] = qmul(P['q'], n['q'])
        out[i] = d; return d
    for i in range(len(nodes)): world(i)
    return out
