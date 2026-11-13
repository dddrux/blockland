"""patchF7: cada mod funciona do seu jeito. Le mcmod.info (modid, nome, versao, dependencias) e aplica o PERFIL do mod em db/mods/<modid>.json:
  beh: [[regex, "Container"|"Chair"|"Bed"|"Door"|"SwitchableLamp"]]  -> comportamento por nome de item
  hide: [regex]                -> tira do criativo (bloco tecnico)
  names: {id: "nome pt-BR"}    -> nomes
  notas: ["..."]               -> vao para o RELATORIO (como o mod realmente funciona)
Mods sem perfil usam so a analise automatica (classinfo/placers/containers). Perfil novo = um .json em db/mods/, sem mexer em codigo."""
import os, re, json
IGNORE = {'minecraft', 'forge', 'fml', 'mcp', 'minecraftforge', 'java', 'mc', 'sponge', 'spongeapi', 'spongeforge'}   # dependencias implicitas do Forge
def read_info(jar):
    for n in ('mcmod.info', 'META-INF/mods.toml'):
        if jar.has(n):
            try:
                t = jar.text(n).lstrip('\ufeff'); d = json.loads(re.sub(r',(\s*[}\]])', r'\1', t))
                d = d.get('modList', d) if isinstance(d, dict) else d; return d[0] if isinstance(d, list) and d else d
            except Exception: return {}
    return {}
def profile(db, modid):
    p = os.path.join(db, 'mods', modid + '.json')
    try: return json.load(open(p, encoding='utf-8'))
    except Exception: return {}
def apply(jar, ctx, ns, out, items):
    info = read_info(jar); mid = (info.get('modid') or ns).lower(); pf = profile(ctx.db, mid)
    ctx.stat('mod_id', mid); ctx.stat('mod_versao', info.get('version', '?')); ctx.stat('mod_nome', info.get('name', ns))
    dp = deps(jar, info); ctx.deps = dp; ctx.modid = mid
    if dp: ctx.stat('depende_de', ','.join('%s(%s)' % (k, v[:3]) for k, v in sorted(dp.items())))
    open(os.path.join(out, 'dependencias.txt'), 'w', encoding='utf-8').write(''.join('%s %s\n' % (k, v) for k, v in sorted(dp.items())))
    if not pf: ctx.gap('mod sem perfil em db/mods (so analise automatica)', mid); return pf
    hide = [re.compile(x) for x in pf.get('hide', [])]; rules = [(re.compile(r), b) for r, b in pf.get('beh', [])]; names = pf.get('names', {}); n = h = 0
    keep = []
    for it in items:
        s = it['id'][len(ns) + 1:] if it['id'].startswith(ns + '_') else it['id']
        if any(x.search(s) for x in hide): h += 1; continue
        if not it.get('beh'):
            for rx, b in rules:
                if rx.search(s): it['beh'] = b; n += 1; break
        if it['id'] in names: it['name'] = names[it['id']]
        keep.append(it)
    items[:] = keep; ctx.stat('perfil', mid); ctx.stat('perfil_beh', n); ctx.stat('perfil_ocultos', h)
    for t in pf.get('notas', []): ctx.gap('nota do perfil', t)
    return pf

def deps(jar, info):
    """patchConv.4: dependencias do mod -> {modid: 'required'|'optional'}. Fontes: mcmod.info (requiredMods/dependencies), mods.toml e
    o campo dependencies=\"required-after:x@[1,);after:y\" da anotacao @Mod (esta no .class, achada por regex nas strings)."""
    out = {}
    def add(tok, kind):
        m = re.match(r'^([A-Za-z0-9_\-\.]+)', str(tok).strip())
        nm = m.group(1).lower().rstrip('.-') if m else ''
        if nm and nm not in IGNORE: out.setdefault(nm, kind)
    for k, kind in (('requiredMods', 'required'), ('dependencies', 'required'), ('dependants', 'optional')):
        v = (info or {}).get(k)
        if isinstance(v, list):
            for x in v: add(x if not isinstance(x, dict) else x.get('modId', ''), kind) if k != 'dependants' else None
    try:
        t = jar.text('META-INF/mods.toml')
        for m in re.finditer(r'modId\s*=\s*"([^"]+)"[^\[]*?mandatory\s*=\s*(true|false)', t, re.S): add(m.group(1), 'required' if m.group(2) == 'true' else 'optional')
    except Exception: pass
    if not hasattr(jar, '_cls'): jar.has_string('x')
    for m in re.finditer(rb'(required-after|required-before|after|before):([A-Za-z0-9_\-\.]+)', jar._cls):
        add(m.group(2).decode('latin1'), 'required' if m.group(1).startswith(b'required') else 'optional')
    return out
def modid_of(jar):
    i = read_info(jar); return str((i or {}).get('modid') or '').lower()
