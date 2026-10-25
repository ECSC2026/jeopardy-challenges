# ACME IoT gateway, config viewer.
# MicroPython unix port, so no os.path, no urllib, no str.translate.

import os
import socket

HOST = "0.0.0.0"
PORT = 1337
BASE = "www/"
INDEX = "index.html"
SERVER = "micro/1.0 (MicroPython 1.29.0)"

MAX_HEAD = 4096
MAX_NAME = 128
READ_CHUNK = 1024
SEND_CHUNK = 4096
TIMEOUT = 3

CONTENT_TYPES = {
    "html": "text/html; charset=utf-8",
    "css": "text/css; charset=utf-8",
    "js": "text/javascript; charset=utf-8",
    "json": "application/json",
    "txt": "text/plain; charset=utf-8",
    "cfg": "text/plain; charset=utf-8",
    "svg": "image/svg+xml",
    "png": "image/png",
    "jpg": "image/jpeg",
    "jpeg": "image/jpeg",
    "gif": "image/gif",
    "ico": "image/vnd.microsoft.icon",
}

STATUS_TEXT = {
    200: "OK",
    400: "Bad Request",
    403: "Forbidden",
    404: "Not Found",
    405: "Method Not Allowed",
    414: "URI Too Long",
}

# the windows uploader still sends backslashes, see [uploader] in device.cfg
_SEP = {ord("\\"): "/"}


def normpath(p):
    return "".join(_SEP.get(ord(c), c) for c in p)


def send_all(conn, data):
    view = memoryview(data)
    while len(view):
        sent = conn.send(view)  # may be short
        if not sent:
            raise OSError("connection closed while sending")
        view = view[sent:]


def unquote(raw):
    if b"%" not in raw:
        return raw
    out = bytearray()
    i = 0
    n = len(raw)
    while i < n:
        c = raw[i]
        if c == 0x25 and i + 2 < n:
            try:
                out.append(int(raw[i + 1:i + 3].decode(), 16))
                i += 3
                continue
            except ValueError:
                pass
        out.append(c)
        i += 1
    return bytes(out)


def resolve(target):
    for sep in (b"?", b"#"):
        if sep in target:
            target = target.split(sep, 1)[0]
    if not target.startswith(b"/"):
        return 400, None

    raw = unquote(target[1:])
    if len(raw) > MAX_NAME:
        return 414, None
    try:
        name = raw.decode("utf-8")
    except Exception:
        return 400, None
    if not name:
        name = INDEX

    # no separators, no way out of BASE
    if "/" in name or "\\" in name:
        return 403, None

    return 200, BASE + normpath(name)


def stat_file(path):
    try:
        st = os.stat(path)
    except OSError:
        return None
    if st[0] & 0xF000 != 0x8000:  # S_IFREG
        return None
    return st[6]


def content_type(path):
    name = path.split("/")[-1]
    if "." in name:
        return CONTENT_TYPES.get(name.split(".")[-1].lower(), "application/octet-stream")
    return "application/octet-stream"


def send_head(conn, status, ctype, length):
    head = "HTTP/1.1 %d %s\r\n" % (status, STATUS_TEXT.get(status, "Error"))
    head += "Server: %s\r\n" % SERVER
    head += "Content-Type: %s\r\n" % ctype
    head += "Content-Length: %d\r\n" % length
    head += "Connection: close\r\n\r\n"
    send_all(conn, head.encode())


def send_error(conn, status):
    body = ("%d %s\n" % (status, STATUS_TEXT.get(status, "Error"))).encode()
    send_head(conn, status, "text/plain; charset=utf-8", len(body))
    send_all(conn, body)


def send_file(conn, path, size, body):
    send_head(conn, 200, content_type(path), size)
    if not body:
        return
    # stop at Content-Length, /proc stats as 0 bytes but still reads back
    left = size
    with open(path, "rb") as f:
        while left > 0:
            chunk = f.read(SEND_CHUNK if left > SEND_CHUNK else left)
            if not chunk:
                break
            send_all(conn, chunk)
            left -= len(chunk)


def read_request_line(conn):
    buf = b""
    while b"\r\n" not in buf:
        if len(buf) > MAX_HEAD:
            return None
        chunk = conn.recv(READ_CHUNK)
        if not chunk:
            return None
        buf += chunk
    return buf.split(b"\r\n", 1)[0]


def handle(conn):
    line = read_request_line(conn)
    if line is None:
        return
    if len(line) > MAX_HEAD:
        send_error(conn, 414)
        return

    fields = line.split(b" ")
    if len(fields) != 3:
        send_error(conn, 400)
        return

    method, target = fields[0], fields[1]
    if method not in (b"GET", b"HEAD"):
        send_error(conn, 405)
        return

    status, path = resolve(target)
    if path is None:
        send_error(conn, status)
        return

    size = stat_file(path)
    if size is None:
        send_error(conn, 404)
        return

    send_file(conn, path, size, method == b"GET")


def main():
    addr = socket.getaddrinfo(HOST, PORT)[0][-1]
    server = socket.socket()
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(addr)
    server.listen(8)
    print("serving %s on %s:%d" % (BASE, HOST, PORT))

    while True:
        try:
            conn, _ = server.accept()
        except OSError:
            continue
        try:
            conn.settimeout(TIMEOUT)
            handle(conn)
        except OSError:
            pass  # hung up or timed out
        finally:
            conn.close()


main()
