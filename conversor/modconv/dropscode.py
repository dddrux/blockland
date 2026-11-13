"""patchForge.3: drops por CODIGO. Nos mods de 1.12.2 quase nenhum bloco tem loot table (drops: 0): o que o bloco solta vem de
Block.getItemDropped / quantityDropped / damageDropped, em Java. Aqui o bytecode desses metodos vira linhas de drops.txt:
  <nome_do_bloco> -> <ns:item> <min>-<max> por codigo      (mesmo formato do loot table; o jogo ja le: src/pack/modrules.inc)
Passos: (1) registro: `ldc "nome"; new Classe; putstatic Dono.CAMPO` liga campo -> nome de registro (e a classe que criou o bloco/item);
(2) getItemDropped da classe (ou da superclasse, dentro do jar) e classificado pelos tokens do bytecode: item de campo, item do bloco, pilha do bloco, vanilla (Items.X), nenhum (null/AIR) ou
dependente de estado (so relatado); (3) quantityDropped constante (0 = nao solta).
Limites: item vanilla so resolve os campos de srgnames.VANILLA_ITEMS (ou todos, com o csv do MCP em db/mcp/); drop dependente de blockstate/NBT/aleatorio fica no relatorio, nao vira regra."""
import re, struct
try:
    from . import bytecode as B, srgnames
except ImportError:
    import bytecode as B, srgnames

_GET_DROP = ('func_180660_a', 'getItemDropped')
_QTY = ('func_149745_a', 'quantityDropped')
_OWN = ('func_180657_a', 'harvestBlock', 'removedByPlayer', 'getDrops')   # forte: a classe escreve o proprio drop (regra estatica estaria errada)
_OWN_WEAK = ('func_180663_b', 'breakBlock', 'func_176208_a', 'onBlockHarvested')   # fraco: bau solta o conteudo no breakBlock; o drop do bloco segue valendo, mas "nenhum" nao e confiavel
_NAME = re.compile(r'^[a-z0-9_]{2,64}$')

def _tokens(cp, d, at):
    """Code -> lista de tokens: gs:/ps:/gf:/pf:/vi:/sp:/st:/if: Dono.membro | ldc:'texto' | ci:inteiro | RET. Dono = nome simples da classe."""
    out = []
    for an, ap, al in at:
        if an != 'Code': continue
        ml = struct.unpack('>I', d[ap + 4:ap + 8])[0]; code = d[ap + 8:ap + 8 + ml]; i = 0
        while i < len(code):
            op = code[i]; ln = B._L[op]
            if op in (0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9):
                ix = struct.unpack('>H', code[i + 1:i + 3])[0]; e = cp[ix]
                if e and e[0] == 'r2':
                    nt = cp[e[1][1]]; ow = B._cls(cp, e[1][0])
                    out.append('%s:%s.%s' % ({0xb2: 'gs', 0xb3: 'ps', 0xb4: 'gf', 0xb5: 'pf', 0xb6: 'vi', 0xb7: 'sp', 0xb8: 'st', 0xb9: 'if'}[op], ow.split('/')[-1], B._u(cp, nt[1][0])))
            elif op in (0x12, 0x13):
                k = code[i + 1] if op == 0x12 else struct.unpack('>H', code[i + 1:i + 3])[0]; e = cp[k]
                if e and e[0] == 'r': out.append('ldc:' + B._u(cp, e[1]))
                elif e and e[0] == 'i': out.append('ci:%d' % e[1])
            elif 0x02 <= op <= 0x08: out.append('ci:%d' % (op - 3))
            elif op == 0x10: out.append('ci:%d' % struct.unpack('>b', code[i + 1:i + 2])[0])
            elif op == 0x11: out.append('ci:%d' % struct.unpack('>h', code[i + 1:i + 3])[0])
            elif op in (0xac, 0xb0, 0xb1): out.append('RET')
            if ln == -1:
                if op == 0xc4: ln = 5 if code[i + 1] == 0x84 else 3
                else:
                    b = (i + 4) & ~3
                    if op == 0xaa: lo, hi = struct.unpack('>ii', code[b + 4:b + 12]); ln = b + 12 + (hi - lo + 1) * 4 - i - 1
                    else: n = struct.unpack('>i', code[b + 4:b + 8])[0]; ln = b + 8 + n * 8 - i - 1
            i += 1 + ln
    return out

def parse(d):
    """bytes de .class -> dict(name, super, methods={nome: tokens}) ou None."""
    if d[:4] != b'\xca\xfe\xba\xbe': return None
    cp, p = B._pool(d); p += 6
    this = B._cls(cp, struct.unpack('>H', d[p - 4:p - 2])[0]); sup = B._cls(cp, struct.unpack('>H', d[p - 2:p])[0])
    ni = struct.unpack('>H', d[p:p + 2])[0]; p += 2 + ni * 2
    nf = struct.unpack('>H', d[p:p + 2])[0]; p += 2
    for _ in range(nf): _, p = B._attrs(d, p + 6, cp)
    nm_ = struct.unpack('>H', d[p:p + 2])[0]; p += 2; ms = {}
    for _ in range(nm_):
        nm = B._u(cp, struct.unpack('>H', d[p + 2:p + 4])[0]); at, p = B._attrs(d, p + 6, cp)
        t = _tokens(cp, d, at)
        if nm in ms: ms[nm] = ms[nm] + t   # sobrecargas: junta (so importa a presenca)
        else: ms[nm] = t
    return dict(name=this, super=sup, methods=ms)

def registry(classes):
    """{'Dono.CAMPO': (nome_de_registro, classe_que_criou)}: ultimo ldc que parece nome antes de cada putstatic + ultima classe instanciada (sp:X.<init>) no mesmo trecho."""
    reg = {}
    for c in classes.values():
        for nm, t in c['methods'].items():
            nm_ = None; ctor = None
            for x in t:
                if x.startswith('ldc:') and _NAME.match(x[4:]): nm_ = x[4:]
                elif x.startswith('sp:') and x.endswith('.<init>') and not x.startswith(('sp:Object.', 'sp:ItemBlock.', 'sp:ItemStack.')): ctor = x[3:-7]
                elif x.startswith('ps:'):
                    if nm_: reg.setdefault(x[3:], (nm_, ctor))
                    nm_ = None; ctor = None
    return reg

def _chain(classes, simple_to_full, cname):
    """classe + superclasses que estao no jar (nome completo)"""
    out = []; seen = set(); c = cname
    while c and c in classes and c not in seen: seen.add(c); out.append(c); c = classes[c]['super']
    return out

def classify(t, reg, ns):
    """tokens de getItemDropped -> (tipo, valor): item | nenhum | estado | vanilla? | desconhecido"""
    if not t or t == ['RET']: return 'nenhum', None
    if any(x.startswith(('if:IBlockState.func_177229_b', 'if:IBlockState.getValue', 'vi:Random.')) for x in t): return 'estado', None
    if any(x.startswith('gs:') and x.endswith('.field_190931_a') for x in t) and len([x for x in t if x == 'RET']) == 1: return 'nenhum', None
    g = [x for x in t if x.startswith('gs:')]
    if not g or sum(1 for x in t if x == 'RET') != 1: return 'desconhecido', None
    ow, _, fld = g[0][3:].partition('.')
    if ow == 'Items':
        v = srgnames.vanilla_item(fld) if fld.startswith('field_') else fld.lower()
        return ('item', 'minecraft:' + v) if v else ('vanilla?', fld)
    if ow == 'Blocks': return 'vanilla?', fld
    r = reg.get(ow + '.' + fld)
    if r: return 'item', '%s:%s' % (ns, r[0])
    return 'desconhecido', ow + '.' + fld

def scan(jar, ns):
    """-> (linhas de drops.txt, relatorio {tipo: [classe...]}). ns = namespace do mod."""
    classes = {}
    for n in jar.names:
        if n.endswith('.class'):
            try: c = parse(jar.read(n))
            except Exception: c = None
            if c: classes[c['name']] = c
    reg = registry(classes); simple = {}
    for full in classes: simple.setdefault(full.split('/')[-1], full)
    L = []; rep = {}; seen = set()
    for key, (name, ctor) in sorted(reg.items(), key=lambda kv: kv[1][0]):
        if not ctor or ctor not in simple: continue
        chain = _chain(classes, simple, simple[ctor])
        get = next((classes[c]['methods'][m] for c in chain for m in _GET_DROP if m in classes[c]['methods']), None)
        qty = next((classes[c]['methods'][m] for c in chain for m in _QTY if m in classes[c]['methods']), None)
        if get is None and qty is None: continue   # padrao do Minecraft: o bloco solta o proprio item
        if any(m in classes[c]['methods'] for c in chain for m in _OWN):   # a classe escreve o proprio drop (NBT, conteudo do bau): regra estatica estaria errada
            rep.setdefault('codigo proprio', []).append(name); continue
        q = 1
        if qty is not None:
            ci = [x for x in qty if x.startswith('ci:')]
            if len(qty) == len(ci) + 1 and qty[-1] == 'RET' and len(ci) == 1: q = int(ci[0][3:])   # constante
            elif qty and qty[-1] == 'RET' and not any(x.startswith(('vi:', 'sp:', 'st:', 'if:')) for x in qty) and len(ci) == 1: q = int(ci[0][3:])
        weak = any(m in classes[c]['methods'] for c in chain for m in _OWN_WEAK)
        if q <= 0 and weak: rep.setdefault('codigo proprio', []).append(name); continue
        if q <= 0:
            if (name, 0) not in seen: seen.add((name, 0)); rep.setdefault('nenhum', []).append(name)
            L.append('%s -> %s:%s 0-0 por codigo' % (name, ns, name)); continue
        if get is None: continue
        kind, val = classify(get, reg, ns)
        rep.setdefault(kind, []).append(name if kind not in ('desconhecido', 'vanilla?') else '%s(%s)' % (name, val))
        if kind == 'item': L.append('%s -> %s %d-%d por codigo' % (name, val, q, q))
        elif kind == 'nenhum' and weak: rep['nenhum'].remove(name); rep.setdefault('codigo proprio', []).append(name)
        elif kind == 'nenhum': L.append('%s -> %s:%s 0-0 por codigo' % (name, ns, name))
    return L, rep
