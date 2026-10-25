from itertools import product

p = 10**9 + 7
n = 32
exponent = 8193

ciphertext = [
    403160923, 293253565, 33836461, 341933798, 578676274,
    177411219, 287450491, 692942752, 592558930, 99586660,
    621145624, 13093019, 80721975, 348693824, 477681186,
    109423341, 907759570, 860054035, 857857159, 679379185,
    88344655, 992937107, 843440688, 776295281, 44427169,
    375765406, 518334641, 661523963, 301136230, 827376279,
    635431929, 608662711,
]

F = GF(p)
R.<x> = PolynomialRing(F)
modulus = x**n - 1
encrypted = R(ciphertext)

# Cyclic convolution is multiplication in F[x]/(x^32 - 1).  Factoring the modulus splits the hash into independent power maps over small fields.
factors = [factor for factor, multiplicity in modulus.factor()]
assert len(factors) == 13
assert all(multiplicity == 1 for factor, multiplicity in modulus.factor())

# For each factor of x^32 - 1, find every possible residue of the plaintext. This leaves 3**11 for bruteforce, all other roots are unique.
choices = []
for i, factor in enumerate(factors):
    degree = factor.degree()
    print(degree)
    if degree == 1:
        field = F
        alpha = -factor[0]
    else:
        field = F.extension(factor, names=("a%d" % i,))
        alpha = field.gen()

    encrypted_component = encrypted(alpha)
    roots = encrypted_component.nth_root(exponent, all=True)

    component_choices = []
    for root in roots:
        residue = R(root) if degree == 1 else R(root.polynomial())
        component_choices.append(residue)
    choices.append(component_choices)

# Rebuild complete flag via CRT -> 3**11 possibilities, easily feasible
for selected in product(*choices):
    candidate = crt(list(selected), factors) % modulus
    coefficients = [int(candidate[i]) for i in range(n)]
    if all(32 <= value < 127 for value in coefficients):
        assert power_mod(candidate, exponent, modulus) == encrypted
        print(bytes(coefficients).decode())
