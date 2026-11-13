#!/usr/bin/env python3
"""patchJVM.1: tradutor AOT bytecode JVM -> C++17 (sem fonte, sem decompilar estruturas): cada metodo vira uma funcao com pilha simulada (S[k]), locais (L[n]) e `goto` por label.
Fluxo: parse .class completo (pool, campos, metodos, Code, tabela de excecoes) -> desmonta TODOS os opcodes -> analise de fluxo (tipos da pilha em cada pc) -> C++ + grafo de chamadas.
Uso: jvmlift.py <jar|dir> --out <dir> [--prefix pkg/] [--calls chamadas.json]   (runtime: src/modules/jvm/aot/jvmrt.h)"""
import struct, sys, os, re, json, zipfile, argparse
N = ('nop aconst_null iconst_m1 iconst_0 iconst_1 iconst_2 iconst_3 iconst_4 iconst_5 lconst_0 lconst_1 fconst_0 fconst_1 fconst_2 dconst_0 dconst_1 bipush sipush ldc ldc_w ldc2_w '
     'iload lload fload dload aload').split()
N += ['%sload_%d' % (t, i) for t in 'ilfda' for i in range(4)]
N += 'iaload laload faload daload aaload baload caload saload istore lstore fstore dstore astore'.split()
N += ['%sstore_%d' % (t, i) for t in 'ilfda' for i in range(4)]
N += ('iastore lastore fastore dastore aastore bastore castore sastore pop pop2 dup dup_x1 dup_x2 dup2 dup2_x1 dup2_x2 swap '
      'iadd ladd fadd dadd isub lsub fsub dsub imul lmul fmul dmul idiv ldiv fdiv ddiv irem lrem frem drem ineg lneg fneg dneg ishl lshl ishr lshr iushr lushr iand land ior lor ixor lxor iinc '
      'i2l i2f i2d l2i l2f l2d f2i f2l f2d d2i d2l d2f i2b i2c i2s lcmp fcmpl fcmpg dcmpl dcmpg ifeq ifne iflt ifge ifgt ifle if_icmpeq if_icmpne if_icmplt if_icmpge if_icmpgt if_icmple if_acmpeq if_acmpne goto jsr ret '
      'tableswitch lookupswitch ireturn lreturn freturn dreturn areturn return getstatic putstatic getfield putfield invokevirtual invokespecial invokestatic invokeinterface invokedynamic new newarray anewarray arraylength athrow checkcast instanceof monitorenter monitorexit wide multianewarray ifnull ifnonnull goto_w jsr_w').split()
OP = {n: i for i, n in enumerate(N)}
class Cls: pass
def parse(d):
    p = [8]
    def u(f, n):
        v = struct.unpack_from(f, d, p[0]); p[0] += n; return v[0] if len(v) == 1 else v
    n = u('>H', 2); cp = [None] * n; i = 1
    while i < n:
        t = u('>B', 1)
        if t == 1: ln = u('>H', 2); cp[i] = ('U', d[p[0]:p[0] + ln].decode('utf-8', 'replace')); p[0] += ln
        elif t == 3: cp[i] = ('I', u('>i', 4))
        elif t == 4: cp[i] = ('F', u('>I', 4))
        elif t == 5: cp[i] = ('J', u('>q', 8)); i += 1
        elif t == 6: cp[i] = ('D', u('>Q', 8)); i += 1
        elif t == 7: cp[i] = ('C', u('>H', 2))
        elif t == 8: cp[i] = ('S', u('>H', 2))
        elif t in (9, 10, 11): cp[i] = ({9: 'FR', 10: 'MR', 11: 'IR'}[t], *u('>HH', 4))
        elif t == 12: cp[i] = ('NT', *u('>HH', 4))
        elif t == 15: cp[i] = ('MH', *u('>BH', 3))
        elif t == 16: cp[i] = ('MT', u('>H', 2))
        elif t == 17: cp[i] = ('CD', *u('>HH', 4))
        elif t == 18: cp[i] = ('ID', *u('>HH', 4))
        elif t in (19, 20): cp[i] = ('C', u('>H', 2))
        else: raise ValueError('tag %d' % t)
        i += 1
    c = Cls(); c.cp = cp; U = lambda k: cp[k][1]; CN = lambda k: U(cp[k][1]) if k else None
    p[0] += 2; c.name = CN(u('>H', 2)); c.sup = CN(u('>H', 2)); c.ifs = [CN(u('>H', 2)) for _ in range(u('>H', 2))]
    def attrs():
        r = {}
        for _ in range(u('>H', 2)):
            nm = U(u('>H', 2)); ln = u('>I', 4); r[nm] = (p[0], ln); p[0] += ln
        return r
    c.fields = []; c.methods = []
    for _ in range(u('>H', 2)): a, nm, ds = u('>HHH', 6); attrs(); c.fields.append((a, U(nm), U(ds)))
    for _ in range(u('>H', 2)):
        a, nm, ds = u('>HHH', 6); at = attrs(); m = dict(acc=a, name=U(nm), desc=U(ds), code=None)
        if 'Code' in at:
            o = at['Code'][0]; ms, ml, cl = struct.unpack_from('>HHI', d, o); code = d[o + 8:o + 8 + cl]; o += 8 + cl
            ne = struct.unpack_from('>H', d, o)[0]; o += 2; ex = []
            for k in range(ne):
                s, e, h, t = struct.unpack_from('>HHHH', d, o + 8 * k); ex.append((s, e, h, CN(t)))
            m.update(code=code, ms=ms, ml=ml, exc=ex)
        c.methods.append(m)
    ca = attrs(); c.bsm = []
    if 'BootstrapMethods' in ca:
        o = ca['BootstrapMethods'][0]; nb = struct.unpack_from('>H', d, o)[0]; o += 2
        for _ in range(nb):
            mh, na = struct.unpack_from('>HH', d, o); c.bsm.append((mh, struct.unpack_from('>%dH' % na, d, o + 4))); o += 4 + 2 * na
    return c
def sdesc(ds):
    """'(IJLfoo;[I)V' -> (['I','J','A','A'], 'V')  tipos de pilha: I J F D A"""
    a = []; i = 1; M = {'B': 'I', 'C': 'I', 'S': 'I', 'Z': 'I', 'I': 'I', 'J': 'J', 'F': 'F', 'D': 'D'}
    while ds[i] != ')':
        if ds[i] in M: a.append(M[ds[i]]); i += 1
        else:
            while ds[i] == '[': i += 1
            i = ds.index(';', i) + 1 if ds[i] == 'L' else i + 1; a.append('A')
    r = ds[i + 1]; return a, ('V' if r == 'V' else M.get(r, 'A'))
def fty(ds): return {'J': 'J', 'F': 'F', 'D': 'D'}.get(ds[0], 'A' if ds[0] in 'L[' else 'I')
def decode(code):
    ins = {}; i = 0; n = len(code)
    while i < n:
        op = code[i]; nm = N[op]; q = i + 1
        if op in (0x10,): a = (struct.unpack_from('>b', code, q)[0],); l = 1
        elif op == 0x11: a = (struct.unpack_from('>h', code, q)[0],); l = 2
        elif op in (0x12, 0x15, 0x16, 0x17, 0x18, 0x19, 0x36, 0x37, 0x38, 0x39, 0x3a, 0xa9, 0xbc): a = (code[q],); l = 1
        elif op == 0x84: a = (code[q], struct.unpack_from('>b', code, q + 1)[0]); l = 2
        elif 0x99 <= op <= 0xa8 or op in (0xc6, 0xc7): a = (i + struct.unpack_from('>h', code, q)[0],); l = 2
        elif op in (0xc8, 0xc9): a = (i + struct.unpack_from('>i', code, q)[0],); l = 4
        elif op in (0x13, 0x14, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xbb, 0xbd, 0xc0, 0xc1): a = (struct.unpack_from('>H', code, q)[0],); l = 2
        elif op in (0xb9, 0xba): a = (struct.unpack_from('>H', code, q)[0],); l = 4
        elif op == 0xc5: a = (struct.unpack_from('>H', code, q)[0], code[q + 2]); l = 3
        elif op == 0xc4:
            o2 = code[q]; nm = N[o2]; idx = struct.unpack_from('>H', code, q + 1)[0]; op = o2
            if o2 == 0x84: a = (idx, struct.unpack_from('>h', code, q + 3)[0]); l = 5
            else: a = (idx,); l = 3
        elif op in (0xaa, 0xab):
            b = (i + 4) & ~3
            if op == 0xaa:
                df, lo, hi = struct.unpack_from('>iii', code, b); t = struct.unpack_from('>%di' % (hi - lo + 1), code, b + 12)
                a = (i + df, lo, [i + x for x in t]); l = b + 12 + 4 * (hi - lo + 1) - q
            else:
                df, np = struct.unpack_from('>ii', code, b); t = struct.unpack_from('>%di' % (2 * np), code, b + 8)
                a = (i + df, [(t[2 * k], i + t[2 * k + 1]) for k in range(np)]); l = b + 8 + 8 * np - q
        else: a = (); l = 0
        ins[i] = (op, nm, a, l + 1 + (1 if code[i] == 0xc4 else 0)); i += ins[i][3]
    return ins
def opbase(nm):  # iload_2 -> (iload, 2)
    m = re.fullmatch(r'([ilfda](?:load|store))_(\d)', nm); return (m.group(1), int(m.group(2))) if m else (nm, None)
def cstr(s): return '"' + ''.join(chr(c) if 32 <= c < 127 and chr(c) not in '"\\?' else '\\%03o' % c for c in s.encode('utf-8')) + '"'
class Tabs:
    def __init__(s): s.C = {}; s.M = {}; s.F = {}
    def get(s, d, k): return d.setdefault(k, len(d))
BIN = {}
for t, m in (('i', 'I'), ('l', 'J'), ('f', 'F'), ('d', 'D')):
    for o in 'add sub mul div rem'.split(): BIN[t + o] = t
for o in 'shl shr ushr and or xor'.split(): BIN['i' + o] = 'i'; BIN['l' + o] = 'l'
TY = {'i': 'I', 'l': 'J', 'f': 'F', 'd': 'D'}; MB = {'I': 'i', 'J': 'j', 'F': 'f', 'D': 'd', 'A': 'a'}
CMP = {'ifeq': '==', 'ifne': '!=', 'iflt': '<', 'ifge': '>=', 'ifgt': '>', 'ifle': '<=', 'if_icmpeq': '==', 'if_icmpne': '!=', 'if_icmplt': '<', 'if_icmpge': '>=', 'if_icmpgt': '>', 'if_icmple': '<='}
CNV = {'i2l': ('J', '(int64_t)%s.i'), 'i2f': ('F', '(float)%s.i'), 'i2d': ('D', '(double)%s.i'), 'l2i': ('I', '(int32_t)%s.j'), 'l2f': ('F', '(float)%s.j'), 'l2d': ('D', '(double)%s.j'),
       'f2i': ('I', 'rt::f2i(%s.f)'), 'f2l': ('J', 'rt::f2l(%s.f)'), 'f2d': ('D', '(double)%s.f'), 'd2i': ('I', 'rt::d2i(%s.d)'), 'd2l': ('J', 'rt::d2l(%s.d)'), 'd2f': ('F', '(float)%s.d'),
       'i2b': ('I', '(int32_t)(int8_t)%s.i'), 'i2c': ('I', '(int32_t)(uint16_t)%s.i'), 'i2s': ('I', '(int32_t)(int16_t)%s.i')}
def words(st, n):
    idx = []; w = 0; i = len(st) - 1
    while w < n: idx.append(i); w += 2 if st[i] in 'JD' else 1; i -= 1
    return idx[::-1]
def step(cf, tb, pc, ins, st, calls):
    op, nm, a, ln = ins; st = list(st); L = []; succ = []; fall = True; cp = cf.cp; U = lambda k: cp[k][1]
    def top(): return len(st) - 1
    def push(t): st.append(t); return 'S[%d]' % (len(st) - 1)
    def pop(): v = 'S[%d]' % (len(st) - 1); st.pop(); return v
    def ref(k):
        e = cp[k]; o = U(U(e[1])); nt = cp[e[2]]; return o, U(nt[1]), U(nt[2])
    b, n = opbase(nm)
    if nm == 'nop': pass
    elif nm == 'aconst_null': L.append('%s.a = nullptr;' % push('A'))
    elif nm.startswith('iconst'): L.append('%s.i = %d;' % (push('I'), -1 if nm.endswith('m1') else int(nm[-1])))
    elif nm.startswith('lconst'): L.append('%s.j = %s;' % (push('J'), nm[-1]))
    elif nm.startswith('fconst'): L.append('%s.f = %s.0f;' % (push('F'), nm[-1]))
    elif nm.startswith('dconst'): L.append('%s.d = %s.0;' % (push('D'), nm[-1]))
    elif nm in ('bipush', 'sipush'): L.append('%s.i = %d;' % (push('I'), a[0]))
    elif nm in ('ldc', 'ldc_w', 'ldc2_w'):
        e = cp[a[0]]
        if e[0] == 'I': L.append('%s.i = %d;' % (push('I'), e[1]))
        elif e[0] == 'F': L.append('%s.f = rt::mkf(0x%08xu);' % (push('F'), e[1]))
        elif e[0] == 'J': L.append('%s.j = %dLL;' % (push('J'), e[1]) if e[1] != -2**63 else '%s.j = INT64_MIN;' % push('J'))
        elif e[0] == 'D': L.append('%s.d = rt::mkd(0x%016xULL);' % (push('D'), e[1]))
        elif e[0] == 'S': L.append('%s.a = rt::str(%s);' % (push('A'), cstr(U(e[1]))))
        elif e[0] == 'C': L.append('%s.a = rt::cls(CL[%d]);' % (push('A'), tb.get(tb.C, U(e[1]))))
        else: L.append('rt::unsupported("ldc %s");' % e[0]); push('A')
    elif b in ('iload', 'lload', 'fload', 'dload', 'aload'): L.append('%s = L[%d];' % (push(b[0].upper().replace('L', 'J') if b[0] != 'a' else 'A'), n if n is not None else a[0]))
    elif b in ('istore', 'lstore', 'fstore', 'dstore', 'astore'): L.append('L[%d] = %s;' % (n if n is not None else a[0], pop()))
    elif nm.endswith('aload') and nm[0] in 'ilfdabcs' and len(nm) == 6:
        i_ = pop(); ar = pop(); t = {'i': 'I', 'l': 'J', 'f': 'F', 'd': 'D', 'a': 'A', 'b': 'I', 'c': 'I', 's': 'I'}[nm[0]]; L.append('%s = rt::aget(%s.a, %s.i);' % (push(t), ar, i_))
    elif nm.endswith('astore') and nm[0] in 'ilfdabcs' and len(nm) == 7:
        v = pop(); i_ = pop(); ar = pop(); cv = {'b': '(int32_t)(int8_t)', 'c': '(int32_t)(uint16_t)', 's': '(int32_t)(int16_t)'}.get(nm[0])
        L.append('{ V t = %s; %s rt::aset(%s.a, %s.i, t); }' % (v, ('t.i = %s t.i;' % cv) if cv else '', ar, i_))
    elif nm == 'pop': pop()
    elif nm == 'pop2':
        for _ in words(st, 2): st.pop()
    elif nm in ('dup', 'dup_x1', 'dup_x2', 'dup2', 'dup2_x1', 'dup2_x2', 'swap'):
        n1, n2, dup = {'dup': (1, 0, 1), 'dup_x1': (1, 1, 1), 'dup_x2': (1, 2, 1), 'dup2': (2, 0, 1), 'dup2_x1': (2, 1, 1), 'dup2_x2': (2, 2, 1), 'swap': (1, 1, 0)}[nm]
        A = words(st, n1); base_n = len(st) - len(A); B = words(st[:base_n], n2) if n2 else []
        low = min(B + A) if (B + A) else len(st); newt = [st[i] for i in A] + [st[i] for i in B] + ([st[i] for i in A] if dup else [])
        order = [('A', i) for i in A] + [('B', i) for i in B] + ([('A', i) for i in A] if dup else [])
        # ordem final (base->topo): A, B, A  (dup_x1: [B,A] -> [A,B,A]); para swap: A, B
        tmp = ' '.join('V t%d = S[%d];' % (k, i) for k, (_, i) in enumerate(order))
        L.append('{ %s %s }' % (tmp, ' '.join('S[%d] = t%d;' % (low + k, k) for k in range(len(order)))))
        del st[low:]; st.extend(newt)
    elif nm in BIN:
        t = BIN[nm]; y = pop(); x = pop(); m = MB[TY[t]]; m2 = 'i' if nm[1:] in ('shl', 'shr', 'ushr') else m
        L.append('%s.%s = rt::%s(%s.%s, %s.%s);' % (push(TY[t]), m, nm, x, m, y, m2))
    elif nm in ('ineg', 'lneg', 'fneg', 'dneg'): x = pop(); m = MB[TY[nm[0]]]; L.append('%s.%s = rt::%s(%s.%s);' % (push(TY[nm[0]]), m, nm, x, m))
    elif nm == 'iinc': L.append('L[%d].i = (int32_t)((uint32_t)L[%d].i + (uint32_t)%d);' % (a[0], a[0], a[1]))
    elif nm in CNV: t, f = CNV[nm]; x = pop(); L.append('%s.%s = %s;' % (push(t), MB[t], f % x))
    elif nm == 'lcmp': y = pop(); x = pop(); L.append('%s.i = rt::lcmp(%s.j, %s.j);' % (push('I'), x, y))
    elif nm in ('fcmpl', 'fcmpg', 'dcmpl', 'dcmpg'):
        y = pop(); x = pop(); m = nm[0]; L.append('%s.i = rt::cmp(%s.%s, %s.%s, %d);' % (push('I'), x, m, y, m, -1 if nm.endswith('l') else 1))
    elif nm in CMP:
        if nm.startswith('if_'): y = pop(); x = pop(); L.append('if (%s.i %s %s.i) goto L%d;' % (x, CMP[nm], y, a[0]))
        else: x = pop(); L.append('if (%s.i %s 0) goto L%d;' % (x, CMP[nm], a[0]))
        succ.append(a[0])
    elif nm in ('if_acmpeq', 'if_acmpne'): y = pop(); x = pop(); L.append('if (%s.a %s %s.a) goto L%d;' % (x, '==' if nm.endswith('eq') else '!=', y, a[0])); succ.append(a[0])
    elif nm in ('ifnull', 'ifnonnull'): x = pop(); L.append('if (%s.a %s nullptr) goto L%d;' % (x, '==' if nm == 'ifnull' else '!=', a[0])); succ.append(a[0])
    elif nm in ('goto', 'goto_w'): L.append('goto L%d;' % a[0]); succ.append(a[0]); fall = False
    elif nm == 'tableswitch':
        x = pop(); L.append('switch (%s.i) { %s default: goto L%d; }' % (x, ' '.join('case %d: goto L%d;' % (a[1] + k, t) for k, t in enumerate(a[2])), a[0])); succ += [a[0]] + a[2]; fall = False
    elif nm == 'lookupswitch':
        x = pop(); L.append('switch (%s.i) { %s default: goto L%d; }' % (x, ' '.join('case %d: goto L%d;' % kv for kv in a[1]), a[0])); succ += [a[0]] + [t for _, t in a[1]]; fall = False
    elif nm in ('ireturn', 'lreturn', 'freturn', 'dreturn', 'areturn'): L.append('return %s;' % pop()); fall = False
    elif nm == 'return': L.append('return V();'); fall = False
    elif nm in ('getstatic', 'putstatic', 'getfield', 'putfield'):
        o, fn, fd = ref(a[0]); k = tb.get(tb.F, (o, fn, fd)); t = fty(fd)
        if nm == 'getstatic': L.append('%s = rt::getstatic(FD[%d]);' % (push(t), k))
        elif nm == 'putstatic': L.append('rt::putstatic(FD[%d], %s);' % (k, pop()))
        elif nm == 'getfield': x = pop(); L.append('%s = rt::getfield(%s.a, FD[%d]);' % (push(t), x, k))
        else: v = pop(); x = pop(); L.append('rt::putfield(%s.a, FD[%d], %s);' % (x, k, v))
    elif nm in ('invokevirtual', 'invokespecial', 'invokestatic', 'invokeinterface', 'invokedynamic'):
        ex_ = None
        if nm == 'invokedynamic':
            e = cp[a[0]]; nt = cp[e[2]]; o, mn, md = '<indy>', U(nt[1]), U(nt[2]); mh, bargs = cf.bsm[e[1]]
            bm = cp[cp[mh][2]]; bn = U(cp[bm[2]][1])
            if bn in ('metafactory', 'altMetafactory') and len(bargs) >= 3:
                ih = cp[bargs[1]]; ir = cp[ih[2]]; int_ = cp[ir[2]]; ex_ = (ih[1], U(U(ir[1])), U(int_[1]), U(int_[2]), U(cp[bargs[2]][1]))
        else: o, mn, md = ref(a[0])
        at, rt_ = sdesc(md); k = tb.get(tb.M, (o, mn, md) + ((ex_,) if ex_ else ())); na = len(at) + (0 if nm in ('invokestatic', 'invokedynamic') else 1)
        args = [pop() for _ in range(na)][::-1]; calls.append('%s.%s%s' % (o, mn, md))
        fn = {'invokevirtual': 'invokevirtual', 'invokeinterface': 'invokevirtual', 'invokespecial': 'invokespecial', 'invokestatic': 'invokestatic', 'invokedynamic': 'invokedynamic'}[nm]
        res = 'rt::%s(MT[%d], ar, %d)' % (fn, k, na)
        if rt_ == 'V': L.append('{ V ar[%d] = { %s }; %s; }' % (max(na, 1), ', '.join(args) or 'V()', res))
        else: tgt = push(rt_); L.append('{ V ar[%d] = { %s }; %s = %s; }' % (max(na, 1), ', '.join(args) or 'V()', tgt, res))
    elif nm == 'new': L.append('%s.a = rt::newobj(CL[%d]);' % (push('A'), tb.get(tb.C, U(cp[a[0]][1]))))
    elif nm == 'newarray': x = pop(); L.append('%s.a = rt::newarray(%s.i, %d);' % (push('A'), x, a[0]))
    elif nm == 'anewarray': x = pop(); L.append('%s.a = rt::newarray(%s.i, 0);' % (push('A'), x))
    elif nm == 'multianewarray':
        dims = [pop() for _ in range(a[1])][::-1]; L.append('{ int32_t dm[%d] = { %s }; %s.a = rt::multianew(dm, %d); }' % (a[1], ', '.join('%s.i' % d for d in dims), push('A'), a[1]))
    elif nm == 'arraylength': x = pop(); L.append('%s.i = rt::alen(%s.a);' % (push('I'), x))
    elif nm == 'athrow': L.append('rt::athrow(%s.a);' % pop()); fall = False
    elif nm == 'checkcast': L.append('rt::checkcast(S[%d].a, CL[%d]);' % (top(), tb.get(tb.C, U(cp[a[0]][1]))))
    elif nm == 'instanceof': x = pop(); L.append('%s.i = rt::instanceOf(%s.a, CL[%d]);' % (push('I'), x, tb.get(tb.C, U(cp[a[0]][1]))))
    elif nm in ('monitorenter', 'monitorexit'): pop()
    else: L.append('rt::unsupported("%s");' % nm); fall = False
    return L, st, succ, fall
def lift_method(cf, m, tb, calls):
    code = m['code']; ins = decode(code); ex = m['exc']; st_in = {0: []}; work = [0]
    for s, e, h, t in ex: st_in[h] = ['A']; work.append(h)
    ok = True; targets = {h for _, _, h, _ in ex}
    while work:
        pc = work.pop()
        while pc in ins:
            st = st_in[pc]; L, st2, succ, fall = step(cf, tb, pc, ins[pc], st, [])
            for s in succ:
                if s not in st_in: st_in[s] = st2; work.append(s)
            targets.update(succ)
            if not fall: break
            pc += ins[pc][3]
            if pc in st_in: break
            st_in[pc] = st2
    out = []; ss = ' V S[%d]; V L[%d];' % (m['ms'] + 2, m['ml'] + 1); at, _ = sdesc(m['desc']); slot = 0 if m['acc'] & 8 else 1; pro = []
    if not m['acc'] & 8: pro.append('L[0] = in[0];')
    ai = 0 if m['acc'] & 8 else 1
    for t in at: pro.append('L[%d] = in[%d];' % (slot, ai)); slot += 2 if t in 'JD' else 1; ai += 1
    out.append('%s int32_t pc_ = 0; int32_t ent_ = -1; (void)pc_;' % ss); out += pro
    out.append('for (;;) { try {')
    if ex: out.append('if (ent_ >= 0) switch (ent_) { %s default: break; }' % ' '.join('case %d: goto L%d;' % (h, h) for h in sorted({h for _, _, h, _ in ex})))
    for pc in sorted(ins):
        if pc not in st_in: continue
        if pc in targets or pc == 0: out.append('L%d:;' % pc)
        L, _, _, _ = step(cf, tb, pc, ins[pc], st_in[pc], calls)
        if any(s <= pc < e for s, e, _, _ in ex): out.append('pc_ = %d;' % pc)
        out += L
    out.append('} catch (rt::Thrown& t_) {')
    for s, e, h, t in ex: out.append('if (pc_ >= %d && pc_ < %d && %s) { S[0].a = t_.ex; ent_ = %d; continue; }' % (s, e, ('rt::instanceOfName(t_.ex, %s)' % cstr(t)) if t else 'true', h))
    out.append('throw; } }'); return out
def mang(s): return re.sub(r'[^A-Za-z0-9]', '_', s)
def lift_class(d, calls):
    cf = parse(d); tb = Tabs(); body = []; defs = []; cg = {}
    for i, m in enumerate(cf.methods):
        if m['code'] is None: continue
        fn = 'm%d_%s' % (i, mang(m['name'])); c = []
        try: lines = lift_method(cf, m, tb, c)
        except Exception as e: body.append('// FALHOU %s%s: %s' % (m['name'], m['desc'], e)); continue
        body.append('static V %s(const V* in) {' % fn); body += lines; body.append('}')
        defs.append('{%s, %s, %d, &%s}' % (cstr(m['name']), cstr(m['desc']), m['acc'], fn)); cg['%s%s' % (m['name'], m['desc'])] = sorted(set(c))
    hd = ['// gerado por conversor/modconv/jvmlift.py (patchJVM.1) a partir de %s' % cf.name, '#include "jvmrt.h"', 'namespace {', 'using rt::V;']
    hd.append('const char* CL[] = {%s};' % ', '.join(cstr(k) for k, _ in sorted(tb.C.items(), key=lambda x: x[1])))
    hd.append('const rt::MRef MT[] = {%s};' % ', '.join(('{%s,%s,%s}' % tuple(cstr(x) for x in k[:3])) if len(k) == 3 else ('{%s,%s,%s,%d,%s,%s,%s,%s}' % (cstr(k[0]), cstr(k[1]), cstr(k[2]), k[3][0], cstr(k[3][1]), cstr(k[3][2]), cstr(k[3][3]), cstr(k[3][4]))) for k, _ in sorted(tb.M.items(), key=lambda x: x[1])))
    hd.append('const rt::FRef FD[] = {%s};' % ', '.join('{%s,%s,%s}' % tuple(cstr(x) for x in k) for k, _ in sorted(tb.F.items(), key=lambda x: x[1])))
    for dd in ('CL', 'MT', 'FD'): pass
    tl = ['const rt::MDef MD[] = {%s};' % ', '.join(defs), 'rt::Reg R_(%s, %s, {%s}, MD, %d);' % (cstr(cf.name), cstr(cf.sup or ''), ', '.join(cstr(x) for x in cf.ifs), len(defs)), '}']
    return cf.name, '\n'.join(hd + body + tl) + '\n', cg
def main():
    ap = argparse.ArgumentParser(); ap.add_argument('src'); ap.add_argument('--out', required=True); ap.add_argument('--prefix', default=''); ap.add_argument('--calls', default=None); a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True); allcg = {}; n = 0; bad = 0
    items = []
    if os.path.isdir(a.src):
        for r, _, fs in os.walk(a.src):
            for f in fs:
                if f.endswith('.class'): items.append(open(os.path.join(r, f), 'rb').read())
    else:
        z = zipfile.ZipFile(a.src); items = [z.read(x) for x in z.namelist() if x.endswith('.class')]
    for d in items:
        try: nm, src, cg = lift_class(d, [])
        except Exception as e: bad += 1; print('ERRO', e); continue
        if not nm.startswith(a.prefix): continue
        open(os.path.join(a.out, mang(nm) + '.cpp'), 'w').write(src); allcg[nm] = cg; n += 1
    if a.calls: json.dump(allcg, open(a.calls, 'w'), indent=1, sort_keys=True)
    print('jvmlift: %d classes, %d erros' % (n, bad))
if __name__ == '__main__': main()
