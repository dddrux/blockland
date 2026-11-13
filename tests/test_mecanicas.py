"""patchMec.1/2: fatos do bytecode (dureza, metodos, acoes de onBlockActivated) e pareamento closed/open. Usa o jar do cfm em mods/ (pula se faltar)."""
import os, sys, subprocess, tempfile, shutil
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'); jar = os.path.join(R, 'mods', 'furniture-6.3.2-1.12.2.jar')
if not os.path.exists(jar): print('pulado: sem jar do cfm'); sys.exit(0)
t = tempfile.mkdtemp(); ok = bad = 0
def chk(c, m):
    global ok, bad
    if c: ok += 1
    else: bad += 1; print('FALHOU:', m)
try:
    subprocess.run([sys.executable, os.path.join(R, 'conversor/modconv/modconv.py'), jar, '--out', t], capture_output=True, timeout=250)
    L = [l.strip().split('|') for l in open(os.path.join(t, 'cfm', 'mecanicas.txt'), encoding='utf-8')]
    d = {v[0]: v for v in L}
    chk(len(L) > 500, 'mecanicas.txt com >500 itens (%d)' % len(L))
    chk(d.get('cfm_bar_stool', ['', '', '', '', '', '', ''])[1] == '0.5', 'dureza da banqueta = 0.5')
    chk('onBlockActivated' in d['cfm_bar_stool'][4], 'metodo onBlockActivated na banqueta')
    chk(sum(1 for v in L if len(v) > 6 and 'sentar' in v[6]) > 50, 'acao sentar em >50 itens')
    chk(any(len(v) > 6 and 'gui' in v[6] for v in L), 'acao gui existe')
    ab = open(os.path.join(t, 'cfm', 'abertos.txt'), encoding='utf-8').read()
    chk('cfm_curtains_closed|cfm_curtains_open' in ab, 'cortina closed->open pareada')
    chk('cfm_lamp_off|cfm_lamp_on' not in ab, 'lampada (beh proprio) nao entra em abertos')
finally: shutil.rmtree(t, ignore_errors=True)
print('%d verificacoes, %d falhas' % (ok + bad, bad)); sys.exit(1 if bad else 0)
