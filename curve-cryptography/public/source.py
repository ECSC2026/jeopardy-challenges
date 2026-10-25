from addition_law import add
from sage.all import randint, GF

def scalar_mult(m, P):
    result = P
    for bit in bin(m)[3:]:
        result = add(result, result)
        if bit == "1":
            result = add(result, P)
    return normalize(result)

def normalize(pr_vec):
    assert not all(c == 0 for c in pr_vec)
    for idx, e in enumerate(pr_vec):
        if e != 0:
            break

    return [c/pr_vec[idx] for c in pr_vec]

if __name__=="__main__":
    p = 115792089237316195423570985008687907853269984665640564039457584007913129640233
    F = GF(p)
    G = [1, 102501992774029227351155115031847797614274516534395448030042501902357355317796, 81442358798769995389155212676266450730432498204531814621659521011285658626829, 9246190263189194919534163739311777360491039769766085394191224036693737930479, 28065049888694496239419841659306655403185082373087632429471260400778396294166]
    G = [F(c) for c in G]

    sk_A = randint(0, p)
    pk_A = scalar_mult(sk_A, G)
    print(f"Alice sends bob... {pk_A}")

    sk_B = randint(0, p)
    pk_B = scalar_mult(sk_B, G)
    print(f"Bob sends Alice... {pk_B}")

    ss = scalar_mult(sk_A, pk_B)
    assert ss == scalar_mult(sk_B, pk_A)

    FLAG = b"ECSC{???}"

    import hashlib
    import os
    from Crypto.Cipher import AES
    from Crypto.Util.Padding import pad, unpad

    def encrypt_flag(shared_secret: int):
        sha1 = hashlib.sha1()
        sha1.update(str(shared_secret).encode('ascii'))
        key = sha1.digest()[:16]
        iv = os.urandom(16)
        cipher = AES.new(key, AES.MODE_CBC, iv)
        ciphertext = cipher.encrypt(pad(FLAG, 16))
        return iv.hex(), ciphertext.hex()

    print(encrypt_flag(int("1".join(str(c) for c in ss))))
