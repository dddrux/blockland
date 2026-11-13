# "Chrome" falso: servidor HTTP/WebSocket com o minimo do DevTools Protocol (so para testar o cliente do jogo).
import os, sys, socket, threading, base64, hashlib, struct, json, re
if os.environ.get('FAKE_NEEDS_NOSANDBOX') and '--no-sandbox' not in sys.argv: sys.stderr.write('Running as root without --no-sandbox is not supported\n'); sys.exit(1)
port = [int(re.search(r'--remote-debugging-port=(\d+)', a).group(1)) for a in sys.argv if a.startswith('--remote-debugging-port=')][0]
import os; open('/tmp/fakecdp.pid', 'w').write(str(os.getpid()))
log = open('/tmp/fakecdp.log', 'a'); frames = [base64.b64encode(open(f'/tmp/jt/{n}.jpg', 'rb').read()).decode() for n in ('big', 'a420', 'a444')]
srv = socket.socket(); srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); srv.bind(('127.0.0.1', port)); srv.listen(5)
def send(c, t):
    b = t.encode(); n = len(b); h = b'\x81' + (bytes([n]) if n < 126 else b'\x7e' + struct.pack('>H', n) if n < 65536 else b'\x7f' + struct.pack('>Q', n)); c.sendall(h + b)
def recv_exact(c, n):
    d = b''
    while len(d) < n:
        x = c.recv(n - len(d))
        if not x: raise EOFError
        d += x
    return d
def handle(c):
    req = b''
    while b'\r\n\r\n' not in req: req += c.recv(4096)
    path = req.split(b' ')[1].decode()
    if path.startswith('/json/list'):
        body = json.dumps([{'id': 'ABC', 'type': 'page', 'url': 'about:blank', 'webSocketDebuggerUrl': f'ws://127.0.0.1:{port}/devtools/page/ABC'}], indent=2).encode()
        c.sendall(b'HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n' % len(body) + body); c.close(); return
    key = re.search(rb'Sec-WebSocket-Key: (.+)\r\n', req).group(1).strip(); acc = base64.b64encode(hashlib.sha1(key + b'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest())
    c.sendall(b'HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ' + acc + b'\r\n\r\n'); sid = 0
    try:
        while True:
            h = recv_exact(c, 2); n = h[1] & 0x7F
            if n == 126: n = struct.unpack('>H', recv_exact(c, 2))[0]
            elif n == 127: n = struct.unpack('>Q', recv_exact(c, 8))[0]
            m = recv_exact(c, 4); p = bytes(b ^ m[i & 3] for i, b in enumerate(recv_exact(c, n))); j = json.loads(p); log.write(j['method'] + ' ' + json.dumps(j.get('params')) + '\n'); log.flush()
            if j['method'] == 'Page.navigate': send(c, '{"method":"Page.frameNavigated","params":{"frame":{"id":"1","url":"https:\\/\\/example.com\\/redir?x=1\\u0026y=2"}}}')
            if j['method'] == 'Page.captureScreenshot': send(c, json.dumps({'id': j['id'], 'result': {'data': frames[1]}}))
            if os.environ.get('FAKE_NOSCREENCAST'): continue
            if j['method'] in ('Page.startScreencast', 'Page.screencastFrameAck') and sid < 12:
                sid += 1; send(c, json.dumps({'method': 'Page.screencastFrame', 'params': {'data': frames[sid % 3], 'metadata': {}, 'sessionId': sid}}))
    except Exception: pass
while True:
    c, _ = srv.accept(); threading.Thread(target=handle, args=(c,), daemon=True).start()
