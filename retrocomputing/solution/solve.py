#!/usr/bin/python3
import argparse
import pathlib

# Many people reverse ARM code, few people reverse _old_ ARM code.
#
# This is code generated for a DEC/Intel StrongARM SA-110.
# It implements some version of ARMv4 (before Thumb was a thing),
# with some support for the old ARMv3 26-bit mode.
# It turns out that Linux still supports switching to that mode.
#
# The image in the description is a hint that should reveal either
# that chip or one of its immediate successors (the SA-1100 and
# SA-1110). Mixing them up is not likely to be a problem.
#
# ARMv4 tricks, in order of occurrence:
#  - the usr26 syscall that switches to 26-bit mode, and the usr32
#    one that switches back.
#  - unaligned loads rotating the value (until ARMv6, I think).
#  - pc/pstate fusion: Naming pc/r15 in Rm (= reading bits 3:0)
#    fuses the pstate flags (NZCV) into the top of the register,
#    but naming it in Rn (= bits 19:16) strips them out.
#  - p-variant instructions (CMPP, etc.) write the ALU result
#    directly to the flags instead of setting the flags correctly.
#  - the S bit when doing maths with PC does funny things because it
#    again writes directly to the flags.
#  - shifts are encoded weirdly, where large enough shifts zero the
#    result rather than shifting modulo the word length, and ASR #32
#    is encoded as a shift by zero.
#  - an immediate operand with a nonzero rotate takes the carry from
#    bit 31 of the result, where a rotate of zero leaves it alone.

BYTES = 64
ROUNDS = 32

WORDS = BYTES // 4

M = 2 ** 32 - 1

def ror32(value: int, bits: int) -> int:
    return ((value >> bits) | (value << (32 - bits))) & M


def untransform_one(words: list[int], r: int, phi: int, sbox: list[int]):
    y, a = words[r & 15], words[(r + 1) & 15]

    t = ror32(sbox[(a & 0xff) >> 2], 8 * (a & 3))

    psi = phi ^ ((t >> 28) & 0xf)
    s = a ^ t
    if ((psi >> 3) ^ psi) & 1:
        s ^= t
    if psi & 2 and not psi & 4:
        s = (s - phi) & M

    n = a & 0xff
    sigma = t if n == 0 else (t << n) & M if n < 32 else 0
    alpha = M if (t >> 31) & 1 else 0
    f = s ^ sigma ^ alpha

    c = 0 if (a & 0x40) else (psi >> 1) & 1
    return (y - f - c) & M


def untransform(output: bytes, sbox: bytes):
    words = [int.from_bytes(output[i:i+4], "little") for i in range(0, len(output), 4)]
    s = [int.from_bytes(sbox[i:i+4], "little") for i in range(0, len(sbox), 4)]
    initial_nzcv = 8 # NZCV before the loop (from adds r4, r10, #0 with r10 = -ROUNDS)
    for r in range(ROUNDS - 1, -1, -1):
        phi = initial_nzcv if r == 0 else (words[(r - 1) & 15] >> 28) & 0xf
        words[r & 15] = untransform_one(words, r, phi, s)
    return b"".join(w.to_bytes(4, "little") for w in words)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", help="distributed binary", type=pathlib.Path)
    args = parser.parse_args()

    elf = args.binary.read_bytes()
    target = elf[0x688:0x688 + BYTES]
    sbox = elf[0x6e0:0x6e0 + 256]

    print(untransform(target, sbox).decode())
