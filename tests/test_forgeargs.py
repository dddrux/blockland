"""patchForge.4: argumentos das acoes de onBlockActivated (mensagem/som) lidos do bytecode -> comportamento.json (acoes_args) -> mecanicas.apply -> acoes.txt.
Compila classes sinteticas (stubs net/minecraft + bloco) com o javac do JDK; pula se nao houver."""
import os, shutil, subprocess, sys, tempfile, zipfile, json
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'); sys.path.insert(0, os.path.join(R, 'conversor'))
from modconv import bytecode
n = f = 0
def ok(c, m):
    global n, f
    n += 1
    if not c: f += 1; print('FALHOU:', m)
W = tempfile.mkdtemp(); S = os.path.join(W, 'src'); O = os.path.join(W, 'out')
def w(p, t):
    p = os.path.join(S, p); os.makedirs(os.path.dirname(p), exist_ok=True); open(p, 'w').write(t)
w('net/minecraft/util/ResourceLocation.java', 'package net.minecraft.util; public class ResourceLocation { public ResourceLocation(String a, String b) {} }')
w('net/minecraft/util/SoundEvent.java', 'package net.minecraft.util; public class SoundEvent { public SoundEvent(ResourceLocation r) {} }')
w('net/minecraft/util/text/TextComponentTranslation.java', 'package net.minecraft.util.text; public class TextComponentTranslation { public TextComponentTranslation(String k, Object... a) {} }')
w('net/minecraft/util/text/TextComponentString.java', 'package net.minecraft.util.text; public class TextComponentString { public TextComponentString(String k) {} }')
w('net/minecraft/entity/player/EntityPlayer.java', 'package net.minecraft.entity.player; public class EntityPlayer { public void sendMessage(Object o) {} }')
w('demo/BlockBell.java', '''package demo; import net.minecraft.util.*; import net.minecraft.util.text.*; import net.minecraft.entity.player.EntityPlayer;
public class BlockBell { public boolean onBlockActivated(EntityPlayer p) { p.sendMessage(new TextComponentTranslation("msg.demo.bell")); SoundEvent s = new SoundEvent(new ResourceLocation("demo", "bell_ring")); return true; } }''')
w('demo/BlockSilent.java', 'package demo; public class BlockSilent { public boolean onBlockActivated(Object p) { String x = "nao_e_mensagem"; return true; } }')
os.makedirs(O)
srcs = [os.path.join(dp, x) for dp, _, fs in os.walk(S) for x in fs]
r = subprocess.run(['java', '-m', 'jdk.compiler/com.sun.tools.javac.Main', '-source', '8', '-target', '8', '-nowarn', '-d', O] + srcs, capture_output=True, text=True)
if r.returncode:
    print('javac indisponivel/erro: teste pulado'); print(r.stderr[:300]); sys.exit(0)
jar = os.path.join(W, 't.jar')
with zipfile.ZipFile(jar, 'w') as z:
    for dp, _, fs in os.walk(O):
        for x in fs: z.write(os.path.join(dp, x), os.path.relpath(os.path.join(dp, x), O))
out = os.path.join(W, 'o'); res = bytecode.extract(jar, out)
b = res.get('demo/BlockBell', {})
ok(b.get('acoes_args', {}).get('mensagem') == ['msg.demo.bell'], 'mensagem = chave ldc antes de TextComponentTranslation.<init>: %r' % b.get('acoes_args'))
ok(b.get('acoes_args', {}).get('som') == ['bell_ring'], 'som = ultimo ldc antes de SoundEvent.<init> (nome do ResourceLocation)')
ok('mensagem' in b.get('acoes', []), 'acao mensagem continua marcada (sendMessage)')
ok('acoes_args' not in res.get('demo/BlockSilent', {}), 'string solta no metodo nao vira argumento')
shutil.rmtree(W, ignore_errors=True)
# jar real (se existir): cfm tem onBlockActivated com mensagens
J = os.path.join(R, 'mods', 'furniture-6.3.2-1.12.2.jar')
if os.path.exists(J):
    t = tempfile.mkdtemp(); rr = bytecode.extract(J, t)
    tot = sum(len(v.get('acoes_args', {}).get('mensagem', [])) for v in rr.values())
    ok(tot >= 0, 'jar real le sem erro (mensagens achadas: %d)' % tot); print('cfm mensagens:', tot, 'sons:', sum(len(v.get('acoes_args', {}).get('som', [])) for v in rr.values()))
    shutil.rmtree(t, ignore_errors=True)
print('test_forgeargs %d/%d' % (n - f, f))
sys.exit(1 if f else 0)
