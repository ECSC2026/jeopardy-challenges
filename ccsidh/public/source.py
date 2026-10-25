from sage.all import (
    ComplexField,
    QQ,
    ZZ,
    QuadraticField,
    algdep,
    choice,
    elliptic_j,
    hilbert_class_polynomial,
    kronecker,
    next_prime,
    randint,
    random_prime,
)

prec = 300
CC = ComplexField(prec)

def ideal_to_j(I):
    "Associate a j-invariant to each ideal class"

    I = I.reduce_equiv()
    alpha, beta = list(I.basis())
    K = alpha.parent()

    NI = QQ(I.norm())
    if NI <= 0:
        raise ValueError("The ideal must be nonzero")

    alpha_bar = alpha.galois_conjugate()
    beta_bar = beta.galois_conjugate()

    # For Im(beta/alpha) > 0, we have
    #
    # alpha*beta_bar - alpha_bar*beta = -Norm(I)*sqrt(D).
    delta = alpha*beta_bar - alpha_bar*beta

    if delta == NI*sqrtD:
        # The basis has the opposite orientation.
        alpha, beta = beta, alpha
        alpha_bar, beta_bar = beta_bar, alpha_bar
    elif delta != -NI*sqrtD:
        raise ArithmeticError(
            "The ideal basis has the wrong discriminant scaling:\n"
            f"delta = {delta}\n"
            f"expected ±{NI*sqrtD}"
        )

    A = ZZ(alpha.norm() / NI)
    B = ZZ(-(alpha * beta.galois_conjugate()).trace() / NI)
    C = ZZ(beta.norm() / NI)
    
    tau = (-CC(B) + CC.gen() * CC(-D).sqrt()) / (2 * CC(A))

    if tau.imag() <= 0:
        raise ArithmeticError("Recovered point is not in the upper half-plane")

    return elliptic_j(tau, prec=prec)

def key_gen(D):
    bound = abs(ZZ(D))
    if bound <= 2:
        raise ValueError("The discriminant is too small to sample a split prime")

    while True:
        ell = random_prime(bound, lbound=2)
        if kronecker(D, ell) == 1:
            ideal = choice(K.primes_above(ell))
            return ideal, ideal_to_j(ideal)

def group_action(a, j, GA_data):
    """I'm just gonna cheat, its YOUR job to solve the challenge after all..."""
    for ideal, j_invariant in GA_data:
        if j_invariant == j:
            return ideal_to_j(a * ideal)
    else:
        raise ValueError("The j-invariant is not present in GA_data")

if __name__=="__main__":
    p = next_prime(randint(0, 2**100))
    while p % 4 != 3:
        p = next_prime(p)

    D = -p

    K = QuadraticField(D)
    w = K.gens()
    sqrtD = K(D).sqrt()

    ClK = K.maximal_order()

    GA_cache = []

    trivial_class = ClK.fractional_ideal(1)
    GA_cache.append([trivial_class, ideal_to_j(trivial_class)])

    public_j = GA_cache[0][1]
    print(f"public j_invariant: {public_j.str()}")

    sk_A, pk_A = key_gen(D)
    GA_cache.append([sk_A, pk_A])
    print(f"Alice sends bob... {pk_A.str()}")

    sk_B, pk_B = key_gen(D)
    GA_cache.append([sk_B, pk_B])
    print(f"Bob sends Alice... {pk_B.str()}")

    ss = group_action(sk_A, pk_B, GA_cache)
    ss_B = group_action(sk_B, pk_A, GA_cache)
    assert ss_B == ss

    FLAG = b"ECSC{???}"

    import hashlib
    import os
    from Crypto.Cipher import AES
    from Crypto.Util.Padding import pad, unpad

    def encrypt_flag(shared_secret):
        sha1 = hashlib.sha1()
        sha1.update(shared_secret.str(digits=20).encode('ascii'))
        key = sha1.digest()[:16]
        iv = os.urandom(16)
        cipher = AES.new(key, AES.MODE_CBC, iv)
        ciphertext = cipher.encrypt(pad(FLAG, 16))
        return iv.hex(), ciphertext.hex()
    
    print(encrypt_flag(ss))
