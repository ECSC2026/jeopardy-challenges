#!/usr/bin/env python3
"""
Static solver for the ECSC 2026 Sponsor Portal (AES-GCM nonce reuse).

The vulnerability
-----------------
The server encrypts every session cookie with AES-GCM using one fixed,
process-local key, and *that same key is also used as the nonce*:

    cipher = AES.new(_AES_KEY, AES.MODE_GCM, _AES_KEY)   # key == nonce!

Since the nonce (and therefore the whole GCM keystream E_K(J0), E_K(J1), ...)
is identical for every cookie, registering two tiers gives us two
(plaintext, ciphertext, tag) triples under the same key/nonce:

    cookie = hex( C || T ),  plaintext = b"sponsortier=" + tier

Attack (single-block messages, empty AAD)
-----------------------------------------
Register two 4-character tiers, so the plaintexts "sponsortier=aaaa" /
"sponsortier=bbbb" are exactly 16 bytes — one full AES block, same length as
the target "sponsortier=flag" (registering "flag" itself is blocked).

For a 1-block ciphertext with empty AAD, GCM computes:

    T = S0  ^  C*H^2  ^  L*H        with S0 = E_K(J0), H = E_K(0^128),
                                    L = len-block = 0^64 || (128)

1. Keystream recovery:  KS = C1 ^ P1           (CTR mode, same nonce)
2. Tag-key recovery:    T1 ^ T2 = (C1 ^ C2) * H^2
                        =>  H^2 = (T1 ^ T2) / (C1 ^ C2)
                        =>  H   = sqrt(H^2) = (H^2)^(2^127)
                        (squaring is a bijection in GF(2^128) -> unique root)
3. Tag-mask recovery:   S0 = T1 ^ C1*H^2 ^ L*H
4. Forge "sponsortier=flag":  C* = P* ^ KS
                              T* = S0 ^ C*H^2 ^ L*H   (same length -> same L)
5. cookie = hex( C* || T* )  ->  GET /flag

All GF(2^128) arithmetic below follows the GCM bit convention
(multiplication per NIST SP 800-38D, reduction poly x^128 + x^7 + x^2 + x + 1).

Usage
-----
Just point it at the running challenge (it registers the helper tiers and
fetches the flag itself):

    python3 solve.py --url http://127.0.0.1:1337

Alternatively, supply two cookies you registered yourself:

    1. Register tier "aaaa" -> copy the `session` cookie (hex).
    2. Register tier "bbbb" -> copy the `session` cookie (hex).
    3. python3 solve.py --cookie1 <hex1> --cookie2 <hex2>

(order does not matter; both cookies must come from the same server run)
"""

import argparse
import sys

import requests

MASK128 = (1 << 128) - 1
R_POLY = 0xE1 << 120  # GCM reduction constant, x^7+x^2+x+1 reflected


# ----------------------------------------------------------------------------
# GF(2^128) arithmetic (GCM convention, blocks as big-endian integers)
# ----------------------------------------------------------------------------

def gmul(x: int, y: int) -> int:
    """Multiplication in GF(2^128) as used by GHASH."""
    z = 0
    v = y
    for i in range(127, -1, -1):
        if (x >> i) & 1:
            z ^= v
        if v & 1:
            v = (v >> 1) ^ R_POLY
        else:
            v >>= 1
    return z


# Multiplicative identity of the GCM field under this integer representation:
# the leftmost bit of a block is the coefficient of z^0, so polynomial "1"
# corresponds to the block 1000...0 (integer 1 << 127), NOT integer 1.
GCM_IDENTITY = 1 << 127


def gpow(x: int, e: int) -> int:
    """Exponentiation in GF(2^128) via square-and-multiply."""
    result = GCM_IDENTITY
    base = x
    while e:
        if e & 1:
            result = gmul(result, base)
        base = gmul(base, base)
        e >>= 1
    return result


def ginv(x: int) -> int:
    """Multiplicative inverse: x^(2^128 - 2)."""
    if x == 0:
        raise ZeroDivisionError("inverse of zero in GF(2^128)")
    return gpow(x, (1 << 128) - 2)


def gsqrt(x: int) -> int:
    """Square root in GF(2^128): the inverse of the Frobenius x -> x^2.

    Since x^(2^128) = x, we have sqrt(x) = x^(2^127).
    """
    r = x
    for _ in range(127):
        r = gmul(r, r)
    return r


# ----------------------------------------------------------------------------
# GCM helpers (single-block ciphertext, empty AAD)
# ----------------------------------------------------------------------------

def len_block(ct_len: int) -> int:
    """GHASH length block for empty AAD: 0^64 || bitlen(C) as 64-bit BE int."""
    return int.from_bytes((0).to_bytes(8, "big") + (ct_len * 8).to_bytes(8, "big"), "big")


def pad_block(data: bytes) -> int:
    """Zero-pad a partial block on the right, as GHASH does."""
    return int.from_bytes(data.ljust(16, b"\x00"), "big")


def ghash_1block(c: int, h: int, length: int) -> int:
    """GHASH of a single (possibly partial, pre-padded) ciphertext block
    with empty AAD:
    state = (0 ^ C)*H, then (state ^ L)*H  =>  C*H^2 ^ L*H
    """
    return gmul(gmul(c, h), h) ^ gmul(length, h)


def hexd(encoded: str) -> bytes:
    return bytes.fromhex(encoded)


# ----------------------------------------------------------------------------
# Challenge interaction
# ----------------------------------------------------------------------------

def register_tier(base_url: str, tier: str) -> str:
    """Register a sponsor tier and return the `session` cookie (hex)."""
    r = requests.post(f"{base_url.rstrip('/')}/register",
                      data={"sponsortier": tier}, allow_redirects=False)
    cookie = r.cookies.get("session")
    if not cookie:
        raise RuntimeError(f"registering tier {tier!r} failed: "
                           f"HTTP {r.status_code}, no session cookie set")
    return cookie


# ----------------------------------------------------------------------------
# Solve
# ----------------------------------------------------------------------------

def solve(cookie1: str, cookie2: str, tier1: str = "aaaa", tier2: str = "bbbb") -> str:
    p1 = f"sponsortier={tier1}".encode()
    p2 = f"sponsortier={tier2}".encode()
    target = b"sponsortier=flag"
    assert len(p1) == len(p2) == len(target) <= 16, (
        "tiers must yield plaintexts of the same length as 'sponsortier=flag' (<=16 bytes, 1 block)"
    )

    payload1, payload2 = hexd(cookie1), hexd(cookie2)
    c1, t1 = payload1[:-16], int.from_bytes(payload1[-16:], "big")
    c2, t2 = payload2[:-16], int.from_bytes(payload2[-16:], "big")
    assert len(c1) == len(c2) == len(p1), "unexpected ciphertext lengths"

    # GHASH zero-pads partial blocks -> work with padded blocks
    C1, C2 = pad_block(c1), pad_block(c2)
    P1 = pad_block(p1)
    Pstar = pad_block(target)
    L = len_block(len(c1))

    # 1. recover H:  (T1^T2) = (C1^C2) * H^2
    h_squared = gmul(t1 ^ t2, ginv(C1 ^ C2))
    h = gsqrt(h_squared)

    # 2. recover S0 = E_K(J0):  T1 = S0 ^ GHASH(C1)
    s0 = t1 ^ ghash_1block(C1, h, L)

    # sanity check against the second sample
    assert s0 ^ ghash_1block(C2, h, L) == t2, "recovered H/S0 failed verification"

    # 3. keystream & forge (keystream block covers the whole 16-byte block;
    #    we only need the first len(target) plaintext bytes of it)
    keystream = C1 ^ P1
    cstar = (target[i] ^ ((keystream >> (8 * (15 - i))) & 0xFF) for i in range(len(target)))
    cstar = bytes(cstar)
    Cstar = pad_block(cstar)
    tstar = (s0 ^ ghash_1block(Cstar, h, L)).to_bytes(16, "big")

    forged = cstar + tstar
    return forged.hex()


def main() -> None:
    ap = argparse.ArgumentParser(description="AES-GCM nonce-reuse forgery")
    ap.add_argument("--url", default="http://127.0.0.1:1337",
                    help="base URL of the challenge (default: %(default)s)")
    ap.add_argument("--tier1", default="aaaa", help="first helper tier name")
    ap.add_argument("--tier2", default="bbbb", help="second helper tier name")
    ap.add_argument("--cookie1", help="existing session cookie (hex) for tier1 "
                                      "(skips registration)")
    ap.add_argument("--cookie2", help="existing session cookie (hex) for tier2 "
                                      "(skips registration)")
    args = ap.parse_args()

    cookie1, cookie2 = args.cookie1, args.cookie2
    if not (cookie1 and cookie2):
        print(f"[*] registering helper tiers {args.tier1!r} / {args.tier2!r} "
              f"on {args.url}")
        cookie1 = register_tier(args.url, args.tier1)
        cookie2 = register_tier(args.url, args.tier2)
        print(f"[+] cookie1 ({args.tier1}): {cookie1}")
        print(f"[+] cookie2 ({args.tier2}): {cookie2}")

    forged = solve(cookie1, cookie2, tier1=args.tier1, tier2=args.tier2)
    print()
    print("[+] forged flag cookie:")
    print(f"    session={forged}")

    print()
    print(f"[+] use it:  curl -s --cookie 'session={forged}' {args.url.rstrip('/')}/flag")

    r = requests.get(f"{args.url.rstrip('/')}/flag", cookies={"session": forged})
    print(f"[+] /flag says: {r.text}")


if __name__ == "__main__":
    main()
