import secrets
import socketserver
from itertools import cycle, islice

try:
    with open("/flag.txt", "rb") as f:
        FLAG = f.read().strip()
except FileNotFoundError:
    FLAG = b"ecsc{flag}"

KEY = None

class MyUDPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        global KEY

        data, socket = self.request

        print("[server] data received", len(data), data, socket)

        if KEY is None and data.startswith(b"_key:"):
            _, key = data.split(b':', 1)
            KEY = key
            msg = b"_key:" + bytes(islice(cycle(KEY), 0, len(FLAG)))
            print("[server] echoing key back", msg)
            socket.sendto(msg, self.client_address)
        elif KEY is not None and data.startswith(b"flag:"):
            enc = b"".join( bytes([f ^ k]) for f, k in zip(FLAG, cycle(KEY)) )
            print("[server] sending encrypted flag", enc)
            socket.sendto(enc, self.client_address)


if __name__ == "__main__":
    HOST, PORT = "0.0.0.0", 13337
    with socketserver.UDPServer((HOST, PORT), MyUDPHandler) as server:
        server.serve_forever()

