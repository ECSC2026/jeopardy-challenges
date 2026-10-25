import itertools
import os
from enum import Enum
from hashlib import shake_128, shake_256


class Algorithm(str, Enum):
    ML_DSA_87 = "ML-DSA-87"


Poly = list[int]
Vec = list[Poly]

Q = 8380417
N = 256
D = 13

K = 8
L = 7
ETA = 2
TAU = 60
BETA = TAU * ETA
GAMMA1 = 1 << 19
GAMMA1_BITS = 20
GAMMA1_MASK = (1 << GAMMA1_BITS) - 1
GAMMA2 = (Q - 1) // 32
OMEGA = 75
C_TILDE_BYTES = 64

PK_BYTES = 32 + 32 * K * 10
SK_BYTES = 32 + 32 + 64 + 32 * L * 3 + 32 * K * 3 + 32 * K * 13
SIG_BYTES = C_TILDE_BYTES + L * 32 * 20 + OMEGA + K


def _h(data: bytes, length: int) -> bytes:
    return shake_256(data).digest(length)


def _bitrev8(i: int) -> int:
    return int(format(i & 0xFF, "08b")[::-1], 2)


ZETAS = [pow(1753, _bitrev8(i), Q) for i in range(N)]
_N_INV = pow(N, -1, Q)


def ntt(f: Poly) -> Poly:
    a = list(f)
    k, length = 0, 128
    while length > 0:
        start = 0
        while start < N:
            k += 1
            z = ZETAS[k]
            for j in range(start, start + length):
                t = z * a[j + length] % Q
                a[j + length] = (a[j] - t) % Q
                a[j] = (a[j] + t) % Q
            start += 2 * length
        length >>= 1
    return a


def intt(f: Poly) -> Poly:
    a = list(f)
    length, k = 1, N
    while length < N:
        start = 0
        while start < N:
            k -= 1
            z = -ZETAS[k] % Q
            for j in range(start, start + length):
                t = a[j]
                a[j] = (t + a[j + length]) % Q
                a[j + length] = z * (t - a[j + length]) % Q
            start += 2 * length
        length <<= 1
    return [x * _N_INV % Q for x in a]


def poly_mul_ntt(a: Poly, b: Poly) -> Poly:
    return [x * y % Q for x, y in zip(a, b, strict=True)]


def poly_add(a: Poly, b: Poly) -> Poly:
    return [(x + y) % Q for x, y in zip(a, b, strict=True)]


def poly_sub(a: Poly, b: Poly) -> Poly:
    return [(x - y) % Q for x, y in zip(a, b, strict=True)]


def vec_add(u: Vec, v: Vec) -> Vec:
    return [poly_add(a, b) for a, b in zip(u, v, strict=True)]


def vec_sub(u: Vec, v: Vec) -> Vec:
    return [poly_sub(a, b) for a, b in zip(u, v, strict=True)]


def vec_ntt(v: Vec) -> Vec:
    return [ntt(p) for p in v]


def vec_intt(v: Vec) -> Vec:
    return [intt(p) for p in v]


def matrix_vector_ntt(A: list[Vec], v: Vec) -> Vec:
    out = []
    for row in A:
        acc = [0] * N
        for a, b in zip(row, v, strict=True):
            acc = [(x + y * z) % Q for x, y, z in zip(acc, a, b, strict=True)]
        out.append(acc)
    return out


def centered(r: int, m: int) -> int:
    r %= m
    return r - m if r > (m >> 1) else r


def power2round(r: int) -> tuple[int, int]:
    r %= Q
    r0 = centered(r, 1 << D)
    return (r - r0) >> D, r0


def decompose(r: int) -> tuple[int, int]:
    alpha = 2 * GAMMA2
    r %= Q
    r0 = centered(r, alpha)
    if r - r0 == Q - 1:
        return 0, r0 - 1
    return (r - r0) // alpha, r0


def high_bits(r: int) -> int:
    return decompose(r)[0]


def low_bits(r: int) -> int:
    return decompose(r)[1]


def make_hint(z: int, r: int) -> int:
    return int(high_bits(r) != high_bits(r + z))


def use_hint(h: int, r: int) -> int:
    m = (Q - 1) // (2 * GAMMA2)
    r1, r0 = decompose(r)
    if h:
        return (r1 + 1) % m if r0 > 0 else (r1 - 1) % m
    return r1


def norm_inf(poly: Poly) -> int:
    return max(abs(centered(c, Q)) for c in poly)


def vec_norm_inf(v: Vec) -> int:
    return max(norm_inf(p) for p in v)


class XOF:
    def __init__(self, shake, seed: bytes, initial: int = 1024) -> None:
        self._shake, self._seed = shake, seed
        self._buf = shake(seed).digest(initial)
        self._pos = 0

    def read(self, count: int) -> bytes:
        while self._pos + count > len(self._buf):
            self._buf = self._shake(self._seed).digest(len(self._buf) * 2)
        out = self._buf[self._pos : self._pos + count]
        self._pos += count
        return out


class Matrix:
    def __init__(self, rho: bytes) -> None:
        self.rho = rho
        self.rows: list[list[Poly]] = []
        for i in range(K):
            row = []
            for j in range(L):
                xof = XOF(shake_128, rho + bytes([j, i]))
                coeffs = []
                while len(coeffs) < N:
                    c = int.from_bytes(xof.read(3), "little") & 0x7FFFFF
                    if c < Q:
                        coeffs.append(c)
                row.append(coeffs)
            self.rows.append(row)

    def __matmul__(self, v_hat: Vec) -> Vec:
        return matrix_vector_ntt(self.rows, v_hat)


def expand_mask(rho_prime: bytes, kappa: int) -> Vec:
    def one(nonce: int) -> Poly:
        buf = _h(rho_prime + nonce.to_bytes(2, "little"), N * GAMMA1_BITS // 8)
        acc = int.from_bytes(buf, "little")
        coeffs = [0] * N
        bounds = range(0, N * GAMMA1_BITS, GAMMA1_BITS)
        for pos, (lo, hi) in enumerate(itertools.pairwise(bounds)):
            mask = (1 << (hi - lo)) - 1
            coeffs[pos] = (GAMMA1 - ((acc >> lo) & mask)) % Q
        return coeffs

    return [one(kappa + j) for j in range(L)]


def sample_in_ball(seed: bytes) -> Poly:
    xof = XOF(shake_256, seed)
    signs = int.from_bytes(xof.read(8), "little")
    coeffs = [0] * N
    for i in range(N - TAU, N):
        while True:
            j = xof.read(1)[0]
            if j <= i:
                break
        coeffs[i] = coeffs[j]
        coeffs[j] = (1 - 2 * (signs & 1)) % Q
        signs >>= 1
    return coeffs


def _pack(coeffs: Poly, nbits: int, nbytes: int) -> bytes:
    acc = 0
    for c in reversed(coeffs):
        acc = (acc << nbits) | c
    return acc.to_bytes(nbytes, "little")


def _unpack(buf: bytes, nbits: int) -> Poly:
    acc = int.from_bytes(buf, "little")
    mask = (1 << nbits) - 1
    return [(acc >> (nbits * i)) & mask for i in range(N)]


def pack_w1(w1: Vec) -> bytes:
    return b"".join(_pack(p, 4, 128) for p in w1)


class Signature:
    def __init__(self, c_tilde: bytes, z: Vec, h: Vec) -> None:
        self.c_tilde = c_tilde
        self.z = z
        self.h = h

    @classmethod
    def unpack(cls, sig: bytes) -> "Signature":
        if len(sig) != SIG_BYTES:
            raise ValueError("bad signature length")
        zbuf = sig[C_TILDE_BYTES : C_TILDE_BYTES + L * 640]
        z = [
            [GAMMA1 - c for c in _unpack(zbuf[i * 640 : (i + 1) * 640], 20)]
            for i in range(L)
        ]

        hbuf = sig[C_TILDE_BYTES + L * 640 :]
        offsets = [0, *list(hbuf[OMEGA:])]
        if offsets[-1] > OMEGA or any(a > b for a, b in itertools.pairwise(offsets)):
            raise ValueError("malformed hint")
        if any(hbuf[offsets[-1] : OMEGA]):
            raise ValueError("malformed hint padding")
        h = []
        for i in range(K):
            poly = [0] * N
            for pos in hbuf[offsets[i] : offsets[i + 1]]:
                poly[pos] = 1
            h.append(poly)

        return cls(sig[:C_TILDE_BYTES], z, h)

    def pack(self) -> bytes:
        z = b"".join(
            _pack([(GAMMA1 - centered(c, Q)) % Q for c in p], 20, 640) for p in self.z
        )

        packed, offsets = [], []
        for poly in self.h:
            packed.extend(i for i, c in enumerate(poly) if c)
            offsets.append(len(packed))
        if offsets[-1] > OMEGA:
            raise ValueError("too many hints")
        packed.extend([0] * (OMEGA - offsets[-1]))

        return self.c_tilde + z + bytes(packed + offsets)


def format_message(m: bytes, ctx: bytes = b"") -> bytes:
    if len(ctx) > 255:
        raise ValueError("context too long")
    return bytes([0, len(ctx)]) + ctx + m


class VerificationError(Exception):
    pass


class Verifier:
    def __init__(self, rho: bytes, t1: Vec, A: "Matrix | None" = None) -> None:
        self.rho = rho
        self.t1 = t1
        self.A = Matrix(rho) if A is None else A
        self.t1_hat = [ntt([(c << D) % Q for c in poly]) for poly in t1]
        self._pk = rho + b"".join(_pack(p, 10, 320) for p in t1)
        self.tr = _h(self._pk, 64)

    @classmethod
    def unpack(cls, pk: bytes) -> "Verifier":
        if len(pk) != PK_BYTES:
            raise ValueError("bad public key length")
        rho, rest = pk[:32], pk[32:]
        return cls(rho, [_unpack(rest[i * 320 : (i + 1) * 320], 10) for i in range(K)])

    def pack(self) -> bytes:
        return self._pk

    def verify(self, sig: "Signature", m: bytes, ctx: bytes = b"") -> None:
        if sum(sum(p) for p in sig.h) > OMEGA:
            raise VerificationError("hint has more than omega set bits")
        if vec_norm_inf(sig.z) >= GAMMA1 - BETA:
            raise VerificationError("z is out of range")

        mu = _h(self.tr + format_message(m, ctx), 64)
        c_hat = ntt(sample_in_ball(sig.c_tilde))
        az = self.A @ vec_ntt([[c % Q for c in p] for p in sig.z])
        diff = vec_intt(
            [
                poly_sub(a, poly_mul_ntt(c_hat, t))
                for a, t in zip(az, self.t1_hat, strict=True)
            ]
        )
        w1 = [
            [use_hint(hh, c) for hh, c in zip(ph, pd, strict=True)]
            for ph, pd in zip(sig.h, diff, strict=True)
        ]
        if sig.c_tilde != _h(mu + pack_w1(w1), C_TILDE_BYTES):
            raise VerificationError("recomputed challenge does not match")


class Signer:
    def __init__(self, rho: bytes, key: bytes, s1: Vec, s2: Vec) -> None:
        A = Matrix(rho)
        self.A = A
        self.rho = rho
        self.key = key
        self.s1 = s1
        self.s2 = s2

        t = vec_add(vec_intt(A @ vec_ntt(s1)), s2)
        t1, t0 = [], []
        for poly in t:
            hi, lo = zip(*(power2round(c) for c in poly), strict=True)
            t1.append(list(hi))
            t0.append([c % Q for c in lo])
        self.t0 = t0

        self._pk = Verifier(rho, t1, A)
        self.tr = self._pk.tr

        self.s1_hat = vec_ntt(s1)
        self.s2_hat = vec_ntt(s2)
        self.t0_hat = vec_ntt(t0)

    @classmethod
    def unpack(cls, sk: bytes) -> "Signer":
        if len(sk) != SK_BYTES:
            raise ValueError("bad secret key length")
        rho, key, tr, rest = sk[:32], sk[32:64], sk[64:128], sk[128:]
        s1 = [
            [(ETA - c) % Q for c in _unpack(rest[i * 96 : (i + 1) * 96], 3)]
            for i in range(L)
        ]
        rest = rest[96 * L :]
        s2 = [
            [(ETA - c) % Q for c in _unpack(rest[i * 96 : (i + 1) * 96], 3)]
            for i in range(K)
        ]
        rest = rest[96 * K :]
        t0 = [
            [
                ((1 << (D - 1)) - c) % Q
                for c in _unpack(rest[i * 416 : (i + 1) * 416], 13)
            ]
            for i in range(K)
        ]

        signer = cls(rho, key, s1, s2)
        if tr != signer.tr or t0 != signer.t0:
            raise ValueError("malformed secret key")
        return signer

    @staticmethod
    def _expand_s(rho_prime: bytes) -> tuple[Vec, Vec]:

        def one(nonce: int) -> Poly:
            xof = XOF(shake_256, rho_prime + nonce.to_bytes(2, "little"))
            coeffs = []
            while len(coeffs) < N:
                byte = xof.read(1)[0]
                for half in (byte & 0xF, byte >> 4):
                    if half < 15 and len(coeffs) < N:
                        coeffs.append((ETA - (half % 5)) % Q)
            return coeffs

        return [one(i) for i in range(L)], [one(L + i) for i in range(K)]

    @classmethod
    def generate(cls) -> "Signer":
        return cls.from_seed(os.urandom(32))

    @classmethod
    def from_seed(cls, zeta: bytes) -> "Signer":
        seed = _h(zeta + bytes([K]) + bytes([L]), 128)
        rho, rho_prime, key = seed[:32], seed[32:96], seed[96:]
        return cls(rho, key, *cls._expand_s(rho_prime))

    def pack(self) -> bytes:
        return (
            self.rho
            + self.key
            + self.tr
            + b"".join(
                _pack([(ETA - centered(c, Q)) for c in p], 3, 96) for p in self.s1
            )
            + b"".join(
                _pack([(ETA - centered(c, Q)) for c in p], 3, 96) for p in self.s2
            )
            + b"".join(
                _pack([((1 << (D - 1)) - centered(c, Q)) for c in p], 13, 416)
                for p in self.t0
            )
        )

    def pk(self) -> Verifier:
        return self._pk

    def sign(self, m: bytes, ctx: bytes = b"") -> "Signature":
        mu = _h(self.tr + format_message(m, ctx), 64)
        rho_prime = _h(self.key + os.urandom(32) + mu, 64)
        kappa = 0

        while True:
            y = expand_mask(rho_prime, kappa)
            kappa += L
            w = vec_intt(self.A @ vec_ntt(y))
            w1 = [[high_bits(c) for c in poly] for poly in w]

            c_tilde = _h(mu + pack_w1(w1), C_TILDE_BYTES)
            c_hat = ntt(sample_in_ball(c_tilde))

            cs1 = vec_intt([poly_mul_ntt(c_hat, p) for p in self.s1_hat])
            z = vec_add(y, cs1)
            if vec_norm_inf(z) >= GAMMA1 - BETA:
                continue

            cs2 = vec_intt([poly_mul_ntt(c_hat, p) for p in self.s2_hat])
            w_minus_cs2 = vec_sub(w, cs2)
            if (
                max(max(abs(low_bits(c)) for c in p) for p in w_minus_cs2)
                >= GAMMA2 - BETA
            ):
                continue

            ct0 = vec_intt([poly_mul_ntt(c_hat, p) for p in self.t0_hat])
            if vec_norm_inf(ct0) >= GAMMA2:
                continue

            target = vec_add(w_minus_cs2, ct0)
            h = [
                [make_hint(-centered(a, Q), b) for a, b in zip(pa, pb, strict=True)]
                for pa, pb in zip(ct0, target, strict=True)
            ]
            if sum(sum(p) for p in h) > OMEGA:
                continue

            return Signature(c_tilde, z, h)
