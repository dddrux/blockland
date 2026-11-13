"""patchF4: mods com baus/armazenamento. O Forge guarda itens em TileEntity (IInventory/ISidedInventory/IItemHandler) aberta por IGuiHandler;
o Blockland so tem beh=Container (bau de 27 slots). Aqui: (1) classes com papel container/inventory ligadas ao bloco pelo nome de registro/classe;
(2) nomes tipicos (chest, crate, drawer, cabinet...) quando o jar tem TileEntity de inventario. Marca beh=Container; grava containers.txt (`item|por que|classe`)."""
import os, re
import classinfo
NAMES = re.compile(r'(^|_)(chest|crate|barrel|drawer|drawers|locker|cabinet|fridge|freezer|dresser|safe|trunk|cupboard|wardrobe|shulker|storage|bin|mailbox|jar|box)(_|$)')
NOT = re.compile(r'chestplate|armor|armour|helmet|leggings|boots|sword|upgrade|key|lock(?!er)|recipe|book')
def apply(jar, ctx, ns, out, items, info, placers=()):
    blk = {}
    for nm, cl in classinfo.by_name(info, [i['id'][len(ns) + 1:] for i in items if i['id'].startswith(ns + '_')], 'container').items():
        ok = [c for c in cl if classinfo.stores_items(info, c)]   # TileEntity so de maquina (liquidificador, banheira) NAO e bau
        if ok: blk.setdefault(nm, ok)
    p2b = {p['item']: p['bloco'] for p in placers}
    has_inv = any(set(v['roles']) & {'inventory', 'container'} for v in info.values())
    lines = []
    for it in items:
        if it.get('beh') or NOT.search(it['id']): continue
        short = it['id'][len(ns) + 1:] if it['id'].startswith(ns + '_') else it['id']; b = p2b.get(it['id'], short)
        why = None
        if b in blk: why = 'classe ' + blk[b][0].rsplit('/', 1)[-1]
        elif short in blk: why = 'classe ' + blk[short][0].rsplit('/', 1)[-1]
        elif has_inv and NAMES.search(short): why = 'nome (mod tem inventario)'
        if why: it['beh'] = 'Container'; lines.append('%s|%s' % (it['id'], why))
    if lines: open(os.path.join(out, 'containers.txt'), 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    ctx.stat('baus_container', len(lines))
    return lines
