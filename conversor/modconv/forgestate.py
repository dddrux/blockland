"""patchF1: blockstates no formato do Forge (forge_marker 1). O Forge 1.12.2 aceita, alem do formato vanilla ("facing=north": {...}),
variantes POR PROPRIEDADE: "variants": {"facing": {"north": {"y": 270}}, "left": {"true": {"model": ...}}, "defaults": {...}, "inventory": {...}}.
O Forge combina uma entrada de cada propriedade (mescla; textures/submodel somam). O conversor antigo pegava so a 1a chave ("facing") e perdia o modelo."""
GROUP_KEYS = {'x', 'y', 'uvlock', 'weight', 'textures', 'submodel', 'custom', 'transform', 'gui3d', 'smooth_lighting', 'ambient', 'model', 'model_text', 'subnames', 'visible'}
def is_variant(v): return isinstance(v, list) or (isinstance(v, dict) and ('model' in v or (v and set(v) <= GROUP_KEYS and not all(isinstance(x, dict) for x in v.values()))))
def is_group(v): return isinstance(v, dict) and v and not is_variant(v) and all(isinstance(x, (dict, list)) for x in v.values())
def _one(v): return v[0] if isinstance(v, list) and v else v
def merge(a, b):
    r = dict(a)
    for k, x in b.items():
        if k == 'textures' and isinstance(x, dict): r['textures'] = dict(r.get('textures', {}), **x)
        elif k == 'submodel' and isinstance(x, (dict, str)):   # patchConv.1: submodel pode ser string (nome do modelo da parte)
            cur = r.get('submodel'); cur = dict(cur) if isinstance(cur, dict) else ({'_p0': {'model': cur}} if isinstance(cur, str) else {})
            if isinstance(x, str): x = {'_p%d' % len(cur): {'model': x}}
            r['submodel'] = dict(cur, **x)
        else: r[k] = x
    return r
def pick(vs, top=None):
    """+defaults: o Forge mescla vs['defaults'] (ex.: model=cube_all) em QUALQUER variante (Chisel)."""
    d = _one((top or {}).get('defaults') or (vs or {}).get('defaults')); v = _pick(vs, d)   # patchConv.1: defaults do topo entram na mescla dos grupos (cor/textura do armario)
    if isinstance(v, dict) and isinstance(d, dict): v = merge(d, v)
    elif v is None and isinstance(d, dict) and 'model' in d: v = d
    return v
REST_VALUES = {'false', '0', 'none', 'normal', 'default', 'north', 'down', 'bottom', 'lower', 'floor', 'x', 'off'}
def rest_score(key):
    """patchConv.1: quantas propriedades da chave "a=b,c=d" estao no valor de repouso."""
    n = 0
    for part in str(key).split(','):
        if '=' in part and part.split('=', 1)[1].strip().lower() in REST_VALUES: n += 1
    return n
def _pick(vs, d0=None):
    """variants -> UM variante {model,x,y,textures} (o 1o estado). None se nao houver."""
    if not vs: return None
    for k in ('normal', ''):
        if k in vs and is_variant(vs[k]): return _one(vs[k])
    groups = {k: v for k, v in vs.items() if is_group(v) and k not in ('defaults', 'inventory')}
    if not groups:
        best = None; bs = -1
        for k, v in vs.items():   # vanilla: estado de REPOUSO (patchConv.1: mais propriedades em valor padrao, ex. mesa back/forward/left/right=false); empate = 1a chave
            if k in ('defaults', 'inventory') or not is_variant(v): continue
            sc = rest_score(k)
            if sc > bs: best, bs = v, sc
        if best is not None: return _one(best)
        return _one(vs.get('inventory')) if is_variant(vs.get('inventory')) else None
    m = dict(_one(vs['defaults'])) if isinstance(_one(vs.get('defaults')), dict) else (dict(d0) if isinstance(d0, dict) else {})
    for k, g in groups.items():
        has_sub = any(isinstance(_one(v), dict) and 'submodel' in _one(v) for v in g.values())
        first = g.get('true') if (has_sub and str(k).lower() in ('north', 'south')) else None   # patchConv.1: vidraca/grade: reta no eixo norte-sul (sem vizinhos o Forge so desenha o poste)
        if first is None: first = next((v for kk, v in g.items() if str(kk).lower() in REST_VALUES), None)   # patchConv.1: valor de repouso (false/none) antes do 1o
        first = _one(first if first is not None else next(iter(g.values())))
        if isinstance(first, dict): m = merge(m, first)
    if 'model' not in m:   # submodel (partes) ou inventory como ultimo recurso
        sub = m.get('submodel')
        if isinstance(sub, dict) and sub:
            s = _one(next(iter(sub.values())))
            if isinstance(s, dict) and 'model' in s: m = merge(m, s)
        inv = _one(vs.get('inventory'))
        if 'model' not in m and isinstance(inv, dict) and 'model' in inv: m = merge(m, inv)
    return m if 'model' in m else None
def inventory(vs):
    v = _one((vs or {}).get('inventory')); return v if isinstance(v, dict) and 'model' in v else None
DEFAULTS = {'false', '0', 'none', 'normal', 'default', 'north'}   # estado "de repouso" suposto: boolean desligado, nivel 0, nenhum vizinho, norte
def when_ok(w):
    """patchF13: multipart `when` (blockstate 1.9+). True se o estado de repouso satisfaz a condicao (OR/AND, valores a|b)."""
    if not w: return True
    if 'OR' in w: return any(when_ok(x) for x in w['OR'])
    if 'AND' in w: return all(when_ok(x) for x in w['AND'])
    return all(bool(set(str(v).lower().split('|')) & DEFAULTS) for v in w.values())
def multipart(d):
    """-> lista de apply (1 por parte valida). Partes condicionais entram se o estado de repouso as satisfaz; sem nenhuma parte, devolve a 1a."""
    out = []
    for part in d.get('multipart') or []:
        if not when_ok(part.get('when')): continue
        ap = _one(part.get('apply'))
        if isinstance(ap, dict) and 'model' in ap: out.append(ap)
    if not out and d.get('multipart'):
        ap = _one(d['multipart'][0].get('apply')); out = [ap] if isinstance(ap, dict) and 'model' in ap else []
    return out

def submodels(a):
    """patchConv.1: partes extras (Forge "submodel") de uma variante -> lista de apply {model,x,y,textures}."""
    out = []; sub = a.get('submodel') if isinstance(a, dict) else None
    if not isinstance(sub, dict): return out
    base = {k: v for k, v in a.items() if k in ('textures', 'uvlock')}
    for s in sub.values():
        s = _one(s)
        if isinstance(s, str): s = {'model': s}
        if isinstance(s, dict) and 'model' in s: out.append(merge(base, s))
    return out

# ---------- patchConv.3: estados por vizinhos (mesa/cerca/vidraca ligando em tempo de jogo) ----------
NBR = (('north', 1), ('east', 2), ('south', 4), ('west', 8))   # mascara do jogo: N=1 (-Z) E=2 (+X) S=4 (+Z) W=8 (-X)
def _st(mask):
    return {k: ('true' if mask & b else 'false') for k, b in NBR}
def when_state(w, st):
    """multipart `when` avaliado num estado dado (vizinhos) + valor de repouso para as demais propriedades."""
    if not w: return True
    if 'OR' in w: return any(when_state(x, st) for x in w['OR'])
    if 'AND' in w: return all(when_state(x, st) for x in w['AND'])
    for k, v in w.items():
        cur = st.get(str(k).lower())
        if cur is None: cur = 'false' if not (set(str(v).lower().split('|')) & DEFAULTS) else sorted(set(str(v).lower().split('|')) & DEFAULTS)[0]
        if cur not in str(v).lower().split('|'): return False
    return True
def _props_of(key):
    out = {}
    for part in str(key).split(','):
        if '=' in part: a, b = part.split('=', 1); out[a.strip().lower()] = b.strip().lower()
    AL = {'forward': 'north', 'right': 'east', 'back': 'south', 'left': 'west'}   # mesa do cfm (perna na ponta em y0 aberta para +X = right): forward=N right=E back=S left=W
    if all(k in out for k in AL) and not any(k in out for k in AL.values()): out = {AL.get(k, k): v for k, v in out.items()}
    return out
def _uses_nbr(w):
    if not w: return False
    if 'OR' in w: return any(_uses_nbr(x) for x in w['OR'])
    if 'AND' in w: return any(_uses_nbr(x) for x in w['AND'])
    return any(str(k).lower() in dict(NBR) for k in w)
def neighbor_variants(d):
    """-> {mascara: [apply,...]} (16 entradas) se o blockstate depende de north/east/south/west; senao None.
    Tres formatos: multipart com `when`, variantes por chave composta (mesa cfm), grupos por propriedade (Chisel/Forge)."""
    nb = dict(NBR); out = {}
    if d.get('multipart') and any(_uses_nbr(p.get('when')) for p in d['multipart']):
        for m in range(16):
            ap = [_one(p.get('apply')) for p in d['multipart'] if when_state(p.get('when'), _st(m))]
            ap = [a for a in ap if isinstance(a, dict) and 'model' in a]
            if ap: out[m] = ap
        return out if len(out) >= 2 else None
    vs = d.get('variants') or {}
    keys = [k for k, v in vs.items() if is_variant(v) and any(p in nb for p in _props_of(k))]
    if len(keys) >= 3:   # chave composta: "east=false,north=true,..."
        dflt = _one(d.get('defaults') or vs.get('defaults'))
        for m in range(16):
            want = _st(m); best = None; bs = -1
            for k in keys:
                pr = _props_of(k)
                if any(pr.get(a) is not None and pr[a] != want[a] for a in want): continue
                if any(a not in pr for a in want): continue
                sc = rest_score(','.join('%s=%s' % (a, b) for a, b in pr.items() if a not in want))
                if sc > bs: best, bs = vs[k], sc
            if best is not None:
                v = _one(best); v = merge(dflt, v) if isinstance(dflt, dict) and isinstance(v, dict) else v
                if isinstance(v, dict) and 'model' in v: out[m] = [v] + submodels(v)
        return out if len(out) >= 4 else None
    groups = {k: g for k, g in vs.items() if str(k).lower() in nb and is_group(g)}
    if groups:   # grupos por propriedade: o Forge soma uma entrada de cada grupo
        top = _one(d.get('defaults') or vs.get('defaults')); top = dict(top) if isinstance(top, dict) else {}
        others = {k: g for k, g in vs.items() if k not in groups and k not in ('defaults', 'inventory') and is_group(g)}
        for m in range(16):
            cur = dict(top)
            for k, g in others.items():
                f = next((v for kk, v in g.items() if str(kk).lower() in REST_VALUES), None)
                f = _one(f if f is not None else next(iter(g.values())))
                if isinstance(f, dict): cur = merge(cur, f)
            for k, g in groups.items():
                f = _one(g.get(_st(m)[str(k).lower()]))
                if isinstance(f, dict): cur = merge(cur, f)
            ap = ([cur] if 'model' in cur else []) + submodels(cur)
            if ap: out[m] = ap
        return out if len(out) >= 2 else None
    return None

def open_variant(d):
    """patchOpen.1: variante `open=true` (portao, alcapao...) com as demais propriedades em repouso -> apply, ou None."""
    mp = d.get('multipart')
    if mp:   # portao do cfm: multipart com when {facing, hinge, opened, pole}; estado = o da 1a parte (repouso), com opened/open = true
        w0 = mp[0].get('when') or {}; flat = {}
        for x in (w0.get('AND') or [w0]):
            if isinstance(x, dict): flat.update({str(k).lower(): str(v).lower().split('|')[0] for k, v in x.items() if k not in ('AND', 'OR')})
        if not any(k in flat for k in ('open', 'opened')): return None
        flat['open'] = flat['opened'] = 'true'
        ap = [_one(p.get('apply')) for p in mp if when_state(p.get('when'), flat)]; ap = [x for x in ap if isinstance(x, dict) and 'model' in x]
        return ap[0] if ap else None
    vs = d.get('variants') or {}; dflt = _one(d.get('defaults') or vs.get('defaults')); dflt = dflt if isinstance(dflt, dict) else {}
    keys = [k for k, v in vs.items() if is_variant(v) and _props_of(k).get('open') == 'true']
    if keys:
        best = max(keys, key=lambda k: rest_score(','.join(p for p in str(k).split(',') if not p.strip().lower().startswith('open='))))
        v = _one(vs[best]); v = merge(dflt, v) if isinstance(v, dict) else v
        return v if isinstance(v, dict) and 'model' in v else None
    g = vs.get('open')
    if is_group(g) and 'true' in g:
        base = _pick({k: x for k, x in vs.items() if k != 'open'}, dflt) or dict(dflt); t = _one(g['true'])
        if isinstance(t, dict): m = merge(base, t); return m if 'model' in m else None
    return None
