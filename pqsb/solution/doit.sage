import base64
import json
import os
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "public"))

import mldsa  # noqa: E402

SCALE = 1 << 24


def b64url_decode(text):
    return base64.urlsafe_b64decode(text + "=" * (-len(text) % 4))


def b64url_encode(raw):
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode("ascii")


def http_raw(url, payload=None, session=None):
    data = json.dumps(payload, default=int).encode() if payload is not None else None
    req = urllib.request.Request(url, data=data, method="POST" if data else "GET")
    if data:
        req.add_header("Content-Type", "application/json")
    if session:
        req.add_header("Authorization", f"Bearer {session}")
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            return resp.status, dict(resp.headers), resp.read()
    except urllib.error.HTTPError as exc:
        return exc.code, dict(exc.headers), exc.read()


def http_json(url, payload=None, session=None):
    status, _, body = http_raw(url, payload, session)
    return status, json.loads(body)


class Bank:
    def __init__(self, url):
        self.url = url.rstrip("/")
        self.account = None

    def open(self):
        self.account = "hax" + os.urandom(6).hex()
        status, body = self.call("/api/balance")
        if status != 200:
            raise SystemExit(f"could not open a file: {body}")
        return body["balance"]

    def call(self, path, payload=None):
        return http_json(f"{self.url}{path}", payload, self.account)

    def buy(self, item):
        return http_raw(f"{self.url}/api/buy", {"item": item}, self.account)

    def public_key(self):
        status, body = self.call(f"/api/accounts/{self.account}/key")
        if status != 200:
            raise SystemExit(f"could not fetch the account key: {body}")
        key = body["keys"][0]
        return b64url_decode(key["pub"]), key["kid"]


def collect_notes(bank):
    notes = []
    while len(notes) < 256:
        status, body = bank.call("/api/withdraw", {"amount": 1})
        if status == 402:
            break
        if status != 200:
            raise SystemExit(f"withdraw failed: {body}")
        notes.append(body["note"])
        status, body = bank.call("/api/deposit", {"note": notes[-1]})
        if status != 200:
            raise SystemExit(f"deposit failed: {body}")
        if len(notes) % 32 == 0:
            print(f"  {len(notes)} notes", flush=True)
    print(f"  {len(notes)} notes")
    return notes


def centered(x):
    return x - mldsa.Q if x > mldsa.Q // 2 else x


def equations(notes):
    rows, values = [], [[] for _ in range(mldsa.L)]
    for note in notes:
        parsed = mldsa.Signature.unpack(b64url_decode(note.split(".")[2]))
        c_tilde, z = parsed.c_tilde, parsed.z
        c = [centered(x) for x in mldsa.sample_in_ball(c_tilde)]
        rows.append([c[mldsa.N - 1 - b] for b in range(mldsa.N)])
        for j in range(mldsa.L):
            values[j].append(centered(z[j][mldsa.N - 1] % mldsa.Q))
    return rows, values


def embedding(rows):
    m = len(rows)
    return [
        [1 if j == i else 0 for j in range(mldsa.N)]
        + [SCALE * rows[r][i] for r in range(m)]
        + [0]
        for i in range(mldsa.N)
    ]


def recover_s1(rows, values):
    m = len(rows)
    base = embedding(rows)
    s1 = []
    for j, vals in enumerate(values):
        target = [0] * mldsa.N + [-SCALE * v for v in vals] + [1]
        reduced = matrix(ZZ, [*base, target]).LLL()

        coeffs = None
        for row in reduced:
            if any(row[mldsa.N : mldsa.N + m]) or abs(row[mldsa.N + m]) != 1:
                continue
            sign = int(row[mldsa.N + m])
            candidate = [sign * int(c) for c in row[: mldsa.N]]
            if max(abs(c) for c in candidate) <= mldsa.ETA:
                coeffs = candidate
                break
        if coeffs is None:
            raise SystemExit(
                f"component {j}: reduction found no short solution -- "
                f"{m} equations may not be enough"
            )
        print(f"  s1[{j}] recovered")
        s1.append([c % mldsa.Q for c in coeffs])
    return s1


def random_mask():
    return [
        [
            ((int.from_bytes(os.urandom(3), "little") & 0xFFFFF) - (mldsa.GAMMA1 - 1))
            % mldsa.Q
            for _ in range(mldsa.N)
        ]
        for _ in range(mldsa.L)
    ]


def sign_with_s1(pk, s1, message) -> mldsa.Signature:
    vk = mldsa.Verifier.unpack(pk)
    A = vk.A
    mu = mldsa._h(vk.tr + mldsa.format_message(message), 64)
    s1_hat = mldsa.vec_ntt(s1)
    t1_hat = vk.t1_hat

    for _ in range(200):
        y = random_mask()
        w = mldsa.vec_intt(A @ mldsa.vec_ntt(y))
        w1 = [[mldsa.high_bits(c) for c in p] for p in w]

        c_tilde = mldsa._h(mu + mldsa.pack_w1(w1), mldsa.C_TILDE_BYTES)
        c_hat = mldsa.ntt(mldsa.sample_in_ball(c_tilde))

        z = mldsa.vec_add(
            y, mldsa.vec_intt([mldsa.poly_mul_ntt(c_hat, p) for p in s1_hat])
        )
        if mldsa.vec_norm_inf(z) >= mldsa.GAMMA1 - mldsa.BETA:
            continue

        az = A @ mldsa.vec_ntt(z)
        W = mldsa.vec_intt(
            [
                mldsa.poly_sub(a, mldsa.poly_mul_ntt(c_hat, t))
                for a, t in zip(az, t1_hat, strict=True)
            ]
        )

        hint, total, ok = [], 0, True
        for w_poly, w1_poly in zip(W, w1, strict=True):
            row = []
            for r, want in zip(w_poly, w1_poly, strict=True):
                if mldsa.use_hint(0, r) == want:
                    row.append(0)
                elif mldsa.use_hint(1, r) == want:
                    row.append(1)
                    total += 1
                else:
                    ok = False
                    break
            if not ok:
                break
            hint.append(row)
        if not ok or total > mldsa.OMEGA:
            continue

        return mldsa.Signature(c_tilde, z, hint)
    raise RuntimeError("signing did not succeed within the attempt limit")


def mint_note(pk, s1, kid, account, amount):
    header = {"alg": "ML-DSA-87", "typ": "JWT", "kid": kid}
    claims = {
        "acct": account,
        "amt": amount,
        "nonce": os.urandom(8).hex(),
        "iat": int(time.time()),
    }
    signing_input = "{}.{}".format(
        b64url_encode(json.dumps(header, separators=(",", ":")).encode()),
        b64url_encode(json.dumps(claims, separators=(",", ":"), default=int).encode()),
    )
    sig = sign_with_s1(pk, s1, signing_input.encode("ascii"))
    return f"{signing_input}.{b64url_encode(sig.pack())}"


def main():
    bank = Bank(sys.argv[1])
    print(f"opening a client file at {bank.url}")
    balance = bank.open()
    print(f"  serial {bank.account}, balance {balance}")
    pk, kid = bank.public_key()
    print("draining it one credit at a time")
    notes = collect_notes(bank)

    rows, values = equations(notes)
    leak = max(abs(v) for col in values for v in col)
    print(
        f"\n{len(notes)} notes -> {len(rows)} equations per component, "
        f"{mldsa.N} unknowns in [-{mldsa.ETA}, {mldsa.ETA}]"
    )
    print(
        f"largest leaked coefficient {leak} (beta = {mldsa.BETA}); "
        f"linear algebra would need {mldsa.N}\n"
    )

    s1 = recover_s1(rows, values)

    print("\nwriting myself a note for 100,000 credits")
    note = mint_note(pk, s1, kid, bank.account, 100_000)
    out = HERE / "forged_note.txt"
    out.write_text(note + "\n")
    print(f"wrote {out}")

    status, body = bank.call("/api/deposit", {"note": note})
    print(f"\nPOST /api/deposit -> {status}")
    if status != 200:
        raise SystemExit(json.dumps(body, indent=2))
    print(f"balance is now {body['balance']:,}")

    status, _, body = bank.buy("flag.txt")
    print(f"\nPOST /api/buy flag.txt -> {status}")
    if status != 200:
        raise SystemExit(json.dumps(json.loads(body), indent=2))
    sys.stdout.write(body.decode(errors="replace"))


if __name__ == "__main__":
    main()
