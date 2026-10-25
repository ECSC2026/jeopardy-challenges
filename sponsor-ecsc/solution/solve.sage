from Crypto.Cipher import AES
from Crypto.Util import strxor


BLOCK_SIZE = 16

def xor(a,b):
    return strxor.strxor(a,b)

#Converts a 128 bit string into a polynomial in GF(2^128)
#x has to be the unknown in the polynomial.
def strToPoly(s,x):
    if len(s) != BLOCK_SIZE:
        raise Exception("Need " + str(BLOCK_SIZE) + " bytes string")
    res = 0
    for i in reversed(range(BLOCK_SIZE)):
        res *= x^8
        temp = s[i]
        for j in range(8):
            res += x^j* ((temp >>(7-j))&1)
    return res

#Converts a polynomial in GF(2^128) into a 128 bitstring
def polyToStr(p):
    coefs = p.polynomial().coefficients(sparse=False)
    coefs += [0] * (BLOCK_SIZE*8 - len(coefs))
    res = bytearray(BLOCK_SIZE)
    for i in range(BLOCK_SIZE):
        for j in range(8):
            if coefs[8*i + j]:
                res[i] |= 1 << (7 - j)
    return bytes(res)


def get_tag(enc):
    _AES_KEY = b"yellow submarine"
    cipher = AES.new(_AES_KEY, AES.MODE_GCM, _AES_KEY)

    _, t1 = cipher.encrypt_and_digest(enc)
    return t1

def get_enc(enc):
    _AES_KEY = b"yellow submarine"
    cipher = AES.new(_AES_KEY, AES.MODE_GCM, _AES_KEY)

    c, _ = cipher.encrypt_and_digest(enc)
    return c

def get_H():
    _AES_KEY = b"yellow submarine"
    ecb_cipher = AES.new(_AES_KEY, AES.MODE_ECB)
    h_subkey = ecb_cipher.encrypt(b'\x00' * 16)
    print("Real H is: ", h_subkey.hex())

get_H()

pt1 = b"sponsortier=main"
pt2 = b"sponsortier=gold"

RES1 = bytes.fromhex("09101fd57583bb75d4f3d7d6bd94d84e0a37e351dec4dc55b69f8f114b72ff4c") # copy from "main" cookie
RES2 = bytes.fromhex("09101fd57583bb75d4f3d7d6b79add441d7937a8d310e7c65363e09d87c0220b") # copy from "gold" cookie

ct1 = RES1[:16] # get_enc(pt1)
ct2 = RES2[:16] # get_enc(p2)

assert(xor(ct1, ct2) == xor(pt1, pt2))

t1 = RES1[16:] # get_tag(pt1)
t2 = RES2[16:] # get_tag(pt2)

print("Diff-Tag")
print(xor(t1, t2).hex())

print("CT-Diff")
print(xor(pt1, pt2).hex())


# poly should be: ct_diff * H^2 + padding * H

F.<a> = GF(2)[]
F.<x> = GF(2^128, modulus=a^128 + a^7 + a^2 + a + 1)

R.<H> = PolynomialRing(F)


ct_diffp = strToPoly(xor(pt1, pt2), x)
t_diffp = strToPoly(xor(t1, t2), x)

roots = (ct_diffp * H**2 + t_diffp).roots()

assert(len(roots) == 1)
assert(roots[0][1] == 2)

root = roots[0][0]
H = root

print(H)
print("H = ", polyToStr(H).hex())

# H = strToPoly(bytes.fromhex("247c56131f0ffcc8b8aced0a74fa740f"), x)

to_forge = b"sponsortier=flag"
ctforge = xor(to_forge, xor(pt1, ct1))
# ctforge_c = get_enc(to_forge)
#assert ctforge == ctforge_c


def forge_tag(H, ct, x):
    padding = bytes.fromhex("00000000000000000000000000000080")
    poly = strToPoly(ct, x) * H**2 + strToPoly(padding, x) * H
    return poly

Ek0 = forge_tag(H, ct1, x) + strToPoly(t1, x)
Ek0p = forge_tag(H, ct2, x) + strToPoly(t2, x)

# assert (Ek0 == Ek0p)

final_tag = forge_tag(H, ctforge, x) + Ek0
print(polyToStr(final_tag))
print(get_tag(b"sponsortier=flag"))

print(pt1.hex())
print(ct1.hex())
print(t1.hex())
print(pt2.hex())
print(ct2.hex())
print(t2.hex())

print("==================")
# put this into your cookie.
print(ctforge.hex() + polyToStr(final_tag).hex())