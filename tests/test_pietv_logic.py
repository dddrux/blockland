"""Testa parse + _rebuild_verified com PyQt6/vlc simulados (sem GUI)."""
import sys, types, pathlib
ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

class _M(type):
    def __getattr__(c, n): return _D
    def __add__(c, o): return _D
    def __or__(c, o): return _D
class _D(metaclass=_M):
    def __init__(self, *a, **k): pass
    def __getattr__(self, n): return _D()
    def __call__(self, *a, **k): return _D()
    def __add__(self, o): return _D()
for name in ("PyQt6", "PyQt6.QtCore", "PyQt6.QtGui", "PyQt6.QtNetwork", "PyQt6.QtWidgets", "vlc"):
    m = types.ModuleType(name); m.__getattr__ = lambda n: _D; sys.modules[name] = m
import pie_tv as P

txt = (ROOT / "assets" / "pietv_extra.m3u").read_text("utf-8")
ch = P.parse_m3u(txt)
assert len(ch) == 15, len(ch)
assert P.APP_NAME == "Pie-TV" and P.ICON_PNG.exists() and P.EXTRA_M3U.exists()

pool = P.parse_m3u("#EXTM3U\n#EXTINF:-1,Canal A (720p)\nhttp://a/720.m3u8\n#EXTINF:-1,Canal A (1080p)\nhttp://a/1080.m3u8\n"
                   "#EXTINF:-1,Canal B\nhttp://b/x.m3u8\n#EXTINF:-1,Canal C\nhttp://c/x.m3u8\n")
class Fake:
    _rebuild_verified = P.MainWindow._rebuild_verified
    def __init__(s):
        s.pool = pool; s.good = {"http://a/720.m3u8", "http://a/1080.m3u8", "http://c/x.m3u8"}
        s.group_filter = P.KEY_ALL; s.channels = []; s._autoplayed = True; s.player = None
        s.settings = types.SimpleNamespace(last="")
        s.log = []
    def rebuild_chips(s): s.log.append("chips")
    def refilter(s, keep_scroll=False): s.log.append(("refilter", keep_scroll))
    count = types.SimpleNamespace(setText=lambda t: setattr(Fake, "txt", t))
f = Fake(); f._rebuild_verified()
names = [c.name for c in f.channels]
assert names == ["Canal A", "Canal C"], names          # B (não verificado) fica de fora
a = f.channels[0]
assert a.url == "http://a/1080.m3u8" and [x.url for x in a.alts] == ["http://a/720.m3u8"]  # melhor qualidade + espelho
assert ("refilter", True) in f.log and Fake.txt == "2 canais no ar"
print("logic OK")
