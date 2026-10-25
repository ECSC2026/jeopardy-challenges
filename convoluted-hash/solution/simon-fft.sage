from itertools import product
from ast import literal_eval
import sys

p = 10**9 + 7
n = 32

ciphertext = [403160923, 293253565, 33836461, 341933798, 578676274, 177411219, 287450491, 692942752, 592558930, 99586660, 621145624, 13093019, 80721975, 348693824, 477681186, 109423341, 907759570, 860054035, 857857159, 679379185, 88344655, 992937107, 843440688, 776295281, 44427169, 375765406, 518334641, 661523963, 301136230, 827376279, 635431929, 608662711]

exponent = 8193
assert len(ciphertext) == n

# Rough plan: Go into the FFT-space, so the convolution operation becomes pointswise exponentiation. Then, do pointwise root computation.
# This will give 3**32 roots. Many of them can be disregarded immediately, because they only exist in the 4th-degree field extension required for FFT.
# Filtering will leave 3**11 roots, as in CRT. This can be bruteforced.

# p has order four modulo 32, so GF(p^4) contains all 32nd roots of unity.  Evaluation at those roots diagonalizes cyclic convolution.
K = GF(p**4, name="a")
zeta = K.multiplicative_generator() ** ((K.order() - 1) // n)
assert zeta.multiplicative_order() == n

fourier = [
    sum(K(ciphertext[j]) * zeta**(k*j) for j in range(n))
    for k in range(n)
]

# Frobenius sends the Fourier coordinate k to p*k (mod n).  Selecting
# one exponent-th root for an entire Frobenius orbit therefore preserves the
# fact that the inverse transform has coefficients in GF(p).
orbits = []
seen = set()
for k in range(n):
    if k in seen:
        continue
    orbit = []
    current = k
    while current not in orbit:
        orbit.append(current)
        seen.add(current)
        current = (p * current) % n
    orbits.append(orbit)

inv_n = K(n)**(-1)

def prime_field_int(value):
    assert value**p == value
    return int(value.polynomial()[0])

def inverse_transform(values):
    return [
        inv_n * sum(values[k] * zeta**(-k*j) for k in range(n))
        for j in range(n)
    ]

# Store each orbits additive contribution to the plaintext.
choices = []
for orbit in orbits:
    representative = orbit[0]
    # A root must live in the same degree-d subfield as this orbit.  This
    # matters for the two degree-one coordinates: nth_root(..., all=True)
    # also returns two roots that only exist in the larger ambient field.
    roots = [
        root
        for root in fourier[representative].nth_root(exponent, all=True)
        if root ** (p**len(orbit)) == root
    ]
    orbit_choices = []
    for root in roots:
        spectrum = [K(0)] * n
        value = root
        for k in orbit:
            assert value**exponent == fourier[k]
            spectrum[k] = value
            value = value**p
        contribution = inverse_transform(spectrum)
        orbit_choices.append([prime_field_int(x) for x in contribution])
    choices.append(orbit_choices)

def hash_message(message):
    result = message[:]
    for _ in range(exponent - 1):
        result = [
            sum(result[i] * message[(k-i) % n] for i in range(n)) % p
            for k in range(n)
        ]
    return result

for selected in product(*choices):
    candidate = [sum(parts[j] for parts in selected) % p for j in range(n)]
    if all(32 <= byte < 127 for byte in candidate):
        assert hash_message(candidate) == ciphertext
        print(bytes(candidate).decode())
