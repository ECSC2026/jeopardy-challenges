import sys

p = 10**9+7

def enc(a, b):
    n = len(a)
    res = [0] * n
    for i in range(n):
        for j in range(n):
            res[(i + j) % n] = (res[(i + j) % n] + a[i] * b[j]) % p
    return res

def securehash(a):
    res = a
    for _ in range(8192):
        res = enc(res, a)
    return res

assert len(sys.argv) == 2 and len(sys.argv[1]) == 32

flag = [ord(c) for c in sys.argv[1]]
print(securehash(flag))
