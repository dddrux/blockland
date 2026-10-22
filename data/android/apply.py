#!/usr/bin/env python3
"""Aplica data/android/ no projeto Gradle gerado (android/): (1) copia overlay/ por cima de android/app/src/main/ (ex.: overlay/AndroidManifest.xml
substitui o manifesto inteiro; overlay/res, overlay/java...); (2) aplica as linhas de manifest.cfg no manifesto. Idempotente."""
import os, re, shutil, sys
here = os.path.dirname(os.path.abspath(__file__)); root = os.path.abspath(os.path.join(here, '..', '..'))
main = os.path.join(root, 'android', 'app', 'src', 'main'); man = os.path.join(main, 'AndroidManifest.xml')
if not os.path.isdir(main): sys.exit('ERRO: %s nao existe (rode android/setup.sh antes)' % main)
ov = os.path.join(here, 'overlay')
if os.path.isdir(ov):
    for d, _, fs in os.walk(ov):
        for f in fs:
            src = os.path.join(d, f); dst = os.path.join(main, os.path.relpath(src, ov)); os.makedirs(os.path.dirname(dst), exist_ok=True); shutil.copy2(src, dst)
if not os.path.isfile(man): sys.exit('ERRO: manifesto nao encontrado em ' + man)
x = open(man, encoding='utf-8').read(); o = x
def set_attr(tag_re, attr, val):
    global x
    m = re.search(r'<%s\b[^>]*>' % tag_re, x)
    if not m: print('aviso: tag <%s> nao encontrada para %s' % (tag_re, attr)); return
    t = m.group(0); a = '%s="%s"' % (attr, val)
    nt = re.sub(r'%s\s*=\s*"[^"]*"' % re.escape(attr), a, t) if re.search(r'%s\s*=' % re.escape(attr), t) else t[:-2].rstrip() + ' ' + a + t[-2:] if t.endswith('/>') else t[:-1].rstrip() + ' ' + a + '>'
    x = x.replace(t, nt, 1)
def add_before_app(line, name):
    global x
    if name in x: return
    m = re.search(r'^[ \t]*<application\b', x, re.M)
    if not m: print('aviso: <application> nao encontrada'); return
    x = x[:m.start()] + '    ' + line + '\n' + x[m.start():]
cfg = os.path.join(here, 'manifest.cfg')
for ln in open(cfg, encoding='utf-8') if os.path.isfile(cfg) else []:
    ln = ln.split('#', 1)[0].strip()
    if not ln: continue
    k, _, rest = ln.partition(' '); rest = rest.strip()
    if k == 'permission': add_before_app('<uses-permission android:name="%s" />' % rest, '"%s"' % rest)
    elif k == 'feature':
        p = rest.split(); n = p[0]; req = ' android:required="false"' if 'required=false' in p[1:] else ''
        add_before_app('<uses-feature android:name="%s"%s />' % (n, req), '"%s"' % n)
    elif k in ('application', 'activity'):
        m = re.match(r'([\w:.\-]+)\s*=\s*"?([^"]*)"?$', rest)
        if m: set_attr(k, m.group(1), m.group(2))
        else: print('aviso: linha invalida:', ln)
    else: print('aviso: diretiva desconhecida:', ln)
if x != o: open(man, 'w', encoding='utf-8').write(x); print('manifesto atualizado (data/android)')
else: print('manifesto ja estava em dia (data/android)')
