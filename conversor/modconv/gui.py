"""patchF6: mods que mudam/estendem a INTERFACE. O Forge usa GuiScreen/GuiContainer (+ Container/Slot no servidor, ligados por IGuiHandler) e eventos de overlay
(RenderGameOverlayEvent, GuiOpenEvent, GuiScreenEvent) para HUD. O Blockland nao carrega codigo de GUI, mas da para levar o que e dado:
1) texturas assets/<ns>/textures/gui/** -> mods/<mod>/gui/ (o jogo/resource pack pode usar como fundo de tela),
2) texturas que SUBSTITUEM a interface vanilla (assets/minecraft/textures/gui/**) -> gui_override/ (pack de interface),
3) gui.json: cada tela (classe, papel, textura citada, tamanho do PNG), blocos ligados (BlockX -> GuiX) e os ganchos de HUD."""
import os, io, re, json
import classinfo
def _png_size(b):
    return (int.from_bytes(b[16:20], 'big'), int.from_bytes(b[20:24], 'big')) if b[:8] == b'\x89PNG\r\n\x1a\n' else None
def extract(jar, outdir, nss, ctx=None, info=None):
    info = info or getattr(ctx, 'info', None) or classinfo.analyse(jar)
    screens = []; hud = []
    for c, v in sorted(info.items()):
        r = set(v['roles'])
        if r & {'gui', 'guicontainer'} and '$' not in c and (v['simple'].startswith('Gui') or 'gui' in c.lower()):
            screens.append(dict(classe=v['simple'], papel='container' if 'guicontainer' in r or 'GuiContainer' in v['sup'] else 'tela', texturas=v['paths'], ligado=re.sub(r'^Gui', 'Block', v['simple'])))
        if 'hud' in r: hud.append(dict(classe=v['simple'], usa=[o.rsplit('/', 1)[-1] for o in classinfo.USE if o in {x for x in jar_refs(jar, c)}]))
    n = 0; override = 0; seen = {}
    for ns in nss + ['minecraft']:
        for f in jar.find(r'^assets/%s/textures/gui/.+\.png$' % re.escape(ns)):
            if ns == 'minecraft': dst = os.path.join(outdir, 'gui_override', f.split('/textures/gui/', 1)[1]); override += 1
            else: dst = os.path.join(outdir, 'gui', ns, f.split('/textures/gui/', 1)[1]); n += 1
            os.makedirs(os.path.dirname(dst), exist_ok=True); b = jar.read(f); open(dst, 'wb').write(b); seen[f] = _png_size(b)
    for s in screens:   # textura de tela fora de textures/gui/ (ex.: Chisel: chisel:textures/chisel2Gui.png)
        for t in s['texturas']:
            m0 = re.match(r'(?:(\w+):)?(textures/.+\.png)$', t)
            if m0 and 'textures/gui/' not in t:
                for ns in ([m0.group(1)] if m0.group(1) else nss):
                    k = 'assets/%s/%s' % (ns, m0.group(2))
                    if jar.has(k) and k not in seen:
                        dst = os.path.join(outdir, 'gui', ns, m0.group(2)[len('textures/'):]); os.makedirs(os.path.dirname(dst), exist_ok=True)
                        b = jar.read(k); open(dst, 'wb').write(b); seen[k] = _png_size(b); n += 1
                    if k in seen: s['tamanho_png'] = seen[k]
    for s in screens:
        for t in s['texturas']:
            m = re.search(r'(?:(\w+):)?textures/gui/(.+)$', t)
            if m:
                for ns in ([m.group(1)] if m.group(1) else nss):
                    k = 'assets/%s/textures/gui/%s' % (ns, m.group(2))
                    if k in seen: s['tamanho_png'] = seen[k]
    if not (screens or hud or n or override): return 0
    json.dump(dict(telas=screens, hud=hud, texturas_gui=n, texturas_vanilla_substituidas=override), open(os.path.join(outdir, 'gui.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
    if ctx:
        ctx.stat('telas_gui', len(screens)); ctx.stat('ganchos_hud', len(hud)); ctx.stat('texturas_gui', n); ctx.stat('gui_vanilla_substituida', override)
        if override: ctx.gap('interface vanilla substituida (gui_override)', 'assets/minecraft/textures/gui', override)
        for h in hud: ctx.gap('HUD por evento do Forge (sem codigo no Blockland)', h['classe'])
    return len(screens) + len(hud)
def jar_refs(jar, cls):
    p = classinfo.parse(jar.read(cls + '.class')) if jar.has(cls + '.class') else None
    return p['refs'] if p else set()
