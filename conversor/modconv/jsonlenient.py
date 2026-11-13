"""patchForge.2: JSON do jeito do Minecraft/Forge 1.12.2. O jogo le blockstates, modelos e receitas com Gson em modo LENIENTE (JsonReader.setLenient(true)), entao mods "quebrados" carregam la e
o json.loads do Python recusava. Aceita: comentarios (//, /* */, #), nomes e valores sem aspas, aspas simples, ';' no lugar de ',', '=' ou '=>' no lugar de ':', virgula sobrando.
Um valor sem aspas vai ate um delimitador (/ \\ ; # = { } [ ] : , ou espaco) e pode conter aspas (BOP: { biomesoplenty:cherry_planks" } = {"biomesoplenty": 'cherry_planks"'})."""
_DELIM = set('/\\;#={}[]:,') | set(' \t\r\n\f')
class _P:
    def __init__(s, t): s.t = t; s.i = 0; s.n = len(t)
    def ws(s):
        while s.i < s.n:
            c = s.t[s.i]
            if c in ' \t\r\n\f\ufeff': s.i += 1
            elif c == '#' or s.t.startswith('//', s.i): j = s.t.find('\n', s.i); s.i = s.n if j < 0 else j + 1
            elif s.t.startswith('/*', s.i): j = s.t.find('*/', s.i + 2); s.i = s.n if j < 0 else j + 2
            else: break
    def peek(s):
        s.ws()
        if s.i >= s.n: raise ValueError('JSON terminou antes da hora')
        return s.t[s.i]
    def string(s, q):
        s.i += 1; out = []
        while s.i < s.n:
            c = s.t[s.i]; s.i += 1
            if c == q: return ''.join(out)
            if c == '\\' and s.i < s.n:
                e = s.t[s.i]; s.i += 1
                if e == 'u' and s.i + 4 <= s.n:
                    try: out.append(chr(int(s.t[s.i:s.i + 4], 16))); s.i += 4; continue
                    except ValueError: pass
                out.append({'n': '\n', 't': '\t', 'r': '\r', 'b': '\b', 'f': '\f'}.get(e, e))
            else: out.append(c)
        raise ValueError('string sem fim')
    def token(s):
        j = s.i
        while j < s.n and s.t[j] not in _DELIM: j += 1
        if j == s.i: raise ValueError('valor esperado na posicao %d' % s.i)
        w = s.t[s.i:j]; s.i = j; return w
    def name(s):
        c = s.peek(); return s.string(c) if c in '"\'' else s.token()
    def value(s):
        c = s.peek()
        if c == '{': return s.obj()
        if c == '[': return s.arr()
        if c in '"\'': return s.string(c)
        w = s.token()
        if w == 'true': return True
        if w == 'false': return False
        if w == 'null': return None
        try: return int(w)
        except ValueError: pass
        try: return float(w)
        except ValueError: return w
    def obj(s):
        s.i += 1; d = {}
        while True:
            c = s.peek()
            if c == '}': s.i += 1; return d
            if c in ',;': s.i += 1; continue
            k = s.name(); c = s.peek()
            if c == ':': s.i += 1
            elif c == '=': s.i += 1; s.i += 1 if s.t.startswith('>', s.i) else 0
            else: raise ValueError('":" esperado na posicao %d' % s.i)
            d[k] = s.value()
    def arr(s):
        s.i += 1; L = []
        while True:
            c = s.peek()
            if c == ']': s.i += 1; return L
            if c in ',;': s.i += 1; continue
            L.append(s.value())
def loads(t):
    t = t.lstrip('\ufeff')
    if t.startswith(")]}'"): t = t[4:]   # prefixo anti-XSSI que o Gson tambem aceita
    import re; t = re.sub(r'\{\s*([A-Za-z0-9_]+:[A-Za-z0-9_/.]+)"\s*\}', r'{"model": "\1"}', t)   # patchConv.11: BOP double_slab traz { modid:nome" } (faltam aspas e a chave)
    p = _P(t); v = p.value(); p.ws()
    return v
