#!/usr/bin/env python3
"""patchJVM.3: cobertura do runtime AOT. Le TODAS as chamadas (invoke*) e acessos a campo do jar e classifica: proprio (no jar), jdk coberto (native em jvmrt.h/jvmcoll.h), jdk faltando, minecraft/forge (precisa de shim), outro.
Saida: ranking do que falta implementar, por frequencia = fila de trabalho do shim. Uso: jvmcover.py <jar> [--out arquivo.txt] [--top 60]"""
import sys, os, re, zipfile, collections, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jvmlift as J
AOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src', 'modules', 'jvm', 'aot')
def natives():
    import subprocess, tempfile
    try:   # le a tabela REAL do runtime (as chaves do shim sao montadas em tempo de execucao)
        d = tempfile.mkdtemp(); src = os.path.join(d, 'd.cpp'); open(src, 'w').write('#include "jvmrt.h"\nint main(){rt::jdk();for(auto&k:rt::natives())puts(k.first.c_str());}\n')
        subprocess.check_call(['g++', '-std=c++17', '-w', '-I' + AOT, src, '-o', os.path.join(d, 'd')], timeout=120)
        r = set(subprocess.check_output([os.path.join(d, 'd')]).decode().split('\n')) - {''}
    except Exception: r = None
    if r is not None: return _alias(r)
    s = ''
    for f in ['jvmrt.h', 'jvmcoll.h'] + sorted(x for x in os.listdir(AOT) if x.startswith('mcshim') and x.endswith('.h')):
        p = os.path.join(AOT, f)
        if os.path.exists(p): s += open(p).read()
    r = set(re.findall(r'N_\("([^"]+)"', s))
    for pre in re.findall(r'(?:\[\]\(const char\* k, Nat f\) \{ N_\(\(std::string\("([^"]+)"\))', s): pass
    for m in re.finditer(r'auto (\w) = \[\]\(const char\* k, Nat f\) \{ N_\(\(std::string\("([^"]+)"\) \+ k\)', s):
        r |= {m.group(2) + k for k in re.findall(r'\b%s\("([^"]+)"' % m.group(1), s)}
    return _alias(r)
def _alias(r):
    AL = {'java/util/ArrayList.': ('java/util/List.', 'java/util/Collection.', 'java/lang/Iterable.', 'net/minecraft/util/NonNullList.'), 'java/util/ArrayList$Itr.': ('java/util/Iterator.',),
          'java/util/HashMap.': ('java/util/Map.',), 'java/util/HashSet.': ('java/util/Set.', 'java/util/Collection.'), 'java/util/HashMap$Node.': ('java/util/Map$Entry.',),
          'java/lang/Enum.': ('net/minecraft/util/EnumFacing.',), 'java/lang/Throwable.': ('java/lang/Exception.', 'java/lang/RuntimeException.')}
    for k in list(r):
        for pre, als in AL.items():
            if k.startswith(pre): r |= {a + k[len(pre):] for a in als}
    return r
def scan(jar):
    z = zipfile.ZipFile(jar); cls = {}; calls = collections.Counter(); fields = collections.Counter()
    for n in z.namelist():
        if not n.endswith('.class'): continue
        try: cf = J.parse(z.read(n))
        except Exception: continue
        cls[cf.name] = cf
    for cf in cls.values():
        cp = cf.cp; U = lambda k: cp[k][1]
        for m in cf.methods:
            if not m['code']: continue
            try: ins = J.decode(m['code'])
            except Exception: continue
            for pc, (op, nm, a, ln) in ins.items():
                if nm.startswith('invoke') and nm != 'invokedynamic' or nm in ('getstatic', 'putstatic', 'getfield', 'putfield'):
                    e = cp[a[0]]; nt = cp[e[2]]; k = '%s.%s%s' % (U(U(e[1])), U(nt[1]), U(nt[2]) if nm.startswith('invoke') else ' ' + U(nt[2]))
                    (calls if nm.startswith('invoke') else fields)[k] += 1
    return cls, calls, fields
def classify(k, cls, nat, supers):
    o = k.split('.')[0]
    if o in cls: return 'proprio'
    if k in nat: return 'jdk_ok'
    if o.startswith('[') and k.endswith('.clone()Ljava/lang/Object;'): return 'jdk_ok'   # patchJVM.12: array.clone nativo
    if o.startswith('net/minecraftforge/') or o.startswith('net/minecraftforge'): return 'forge'
    if o.startswith('net/minecraft/'): return 'minecraft'
    if o.startswith(('java/', 'javax/', 'sun/')): return 'jdk_falta'
    return 'outro'
def main():
    ap = argparse.ArgumentParser(); ap.add_argument('jar'); ap.add_argument('--out'); ap.add_argument('--top', type=int, default=60); a = ap.parse_args()
    cls, calls, fields = scan(a.jar); nat = natives(); cnt = collections.Counter(); per = collections.defaultdict(collections.Counter)
    for k, n in calls.items(): c = classify(k, cls, nat, None); cnt[c] += n; per[c][k] += n
    tot = sum(cnt.values()) or 1; out = ['# cobertura AOT: %s (%d classes, %d chamadas distintas, %d campos)' % (os.path.basename(a.jar), len(cls), len(calls), len(fields))]
    for c in ('proprio', 'jdk_ok', 'jdk_falta', 'minecraft', 'forge', 'outro'): out.append('%-10s %6d (%4.1f%%)  distintas=%d' % (c, cnt[c], 100.0 * cnt[c] / tot, len(per[c])))
    for c in ('jdk_falta', 'minecraft', 'forge', 'outro'):
        out.append('\n## faltando: %s (mais usadas)' % c); out += ['%5d %s' % (n, k) for k, n in per[c].most_common(a.top)]
    txt = '\n'.join(out) + '\n'
    if a.out: open(a.out, 'w').write(txt)
    print('\n'.join(out[:8]))
if __name__ == '__main__': main()
