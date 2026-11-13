import http.server, threading, sys, time, tempfile, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import pietv_health as H

class Hd(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def _send(self, body, ctype="text/plain", code=200):
        self.send_response(code); self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body))); self.end_headers(); self.wfile.write(body)
    def do_GET(self):
        p = self.path
        if p == "/good.m3u8": self._send(b"#EXTM3U\n#EXTINF:4,\nseg1.ts\n", "application/vnd.apple.mpegurl")
        elif p == "/master.m3u8": self._send(b"#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1\nsub/v.m3u8\n")
        elif p == "/sub/v.m3u8": self._send(b"#EXTM3U\n#EXTINF:4,\nv1.ts\n")
        elif p in ("/seg1.ts", "/sub/v1.ts"): self._send(b"\x47" + b"\0" * 900, "video/mp2t")
        elif p == "/deadseg.m3u8": self._send(b"#EXTM3U\n#EXTINF:4,\nnope.ts\n")
        elif p == "/html": self._send(b"<html>erro</html>", "text/html")
        elif p == "/ts": self._send(b"\x47" + b"\0" * 2000, "video/mp2t")
        else: self._send(b"nf", code=404)

srv = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Hd)
threading.Thread(target=srv.serve_forever, daemon=True).start()
B = f"http://127.0.0.1:{srv.server_port}"
cases = {"/good.m3u8": True, "/master.m3u8": True, "/deadseg.m3u8": False, "/html": False,
         "/ts": True, "/missing.m3u8": False}
fails = 0
for path, want in cases.items():
    got = H.check_url(B + path)
    print(("OK  " if got == want else "FAIL"), path, got)
    fails += got != want
assert H.check_url("udp://224.0.0.4:49152") is False
assert H.check_url("http://127.0.0.1:1/x.m3u8") is False

d = pathlib.Path(tempfile.mkdtemp()) / "h.json"
c = H.HealthCache(d)
res = {}
items = [(B + p, "", "") for p in cases]
H.run_checks(items, c, lambda u, ok: res.__setitem__(u, ok))
assert len(res) == len(cases)
c2 = H.HealthCache(d)                       # cache persiste
assert all(c2.status(u) in ("ok", "bad") for u, _, _ in items)
res2 = {}
H.run_checks(items, c2, lambda u, ok: res2.__setitem__(u, ok))
assert res2 == {}, "não deve rechecar itens frescos"
c2.d[B + "/good.m3u8"][1] -= H.OK_TTL + 5
assert c2.status(B + "/good.m3u8") == "stale"
print("cache OK;", "falhas:", fails)
sys.exit(1 if fails else 0)
