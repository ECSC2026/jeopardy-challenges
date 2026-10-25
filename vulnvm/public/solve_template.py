#!/usr/bin/env python3
# solve.py — pwntools template for the VulnVM remote.
#
#
#   pip install pwntools
#   ./solve.py                                   # uses the defaults below
#   ./solve.py HOST=1.2.3.4 PORT=1337 SRC=solve.mc
from pwn import args, context, remote, log

HOST = args.HOST or "127.0.0.1"
PORT = int(args.PORT or 1337)
SRC = args.SRC or "solve.mc"  # your MonkeyC exploit source
context.log_level = args.LOGLEVEL or "info"


def main():
    src = open(SRC, "rb").read()
    log.info("uploading %d bytes of source (%s)", len(src), SRC)

    io = remote(HOST, PORT, ssl=True)
    io.recvuntil(b"stdin/stdout/stderr.\n", timeout=2)  # consume the banner

    # The protocol: decimal length, newline, then the bytes.
    io.sendline(str(len(src)).encode())
    io.send(src)

    # A compile error comes back as "error: compilation failed: ...".
    # On success the program runs; once it calls system("/bin/sh"), just type.
    io.interactive()


if __name__ == "__main__":
    main()
