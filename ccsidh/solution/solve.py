import hashlib
from ast import literal_eval
from pathlib import Path

from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad
from sage.all import (
    BQFClassGroup,
    BinaryQF,
    ComplexField,
    EllipticCurve_from_j,
    ZZ,
    algdep,
    elliptic_j,
)


def recover_discriminant_and_class(j, discriminant_bound=None):
    """Recover a CM discriminant and quadratic form from an approximate j.

    The q-expansion

        j(q) = 1/q + 744 + 196884*q + O(q^2),

    gives an accurate tau for the enormous real root.  For smaller complex
    roots Sage recovers tau from the period lattice.  PARI's algdep, called
    through Sage, then uses LLL to recover the quadratic integer relation for
    tau, whose discriminant is the unknown CM discriminant.
    """

    j = CC(j)

    if abs(j) > 2**100:
        q = 1 / (j - 744)
        tau = q.log() / (2 * CC.pi() * CC.gen())
    else:
        E = EllipticCurve_from_j(j)
        tau = E.period_lattice().tau()

    tau_relation = algdep(tau, 2)
    if tau_relation.leading_coefficient() < 0:
        tau_relation = -tau_relation
    if tau_relation.degree() != 2:
        raise ArithmeticError(f"LLL did not recover a quadratic relation: {tau_relation}")

    recovered_D = ZZ(tau_relation.discriminant())
    if recovered_D >= 0 or recovered_D % 4 not in (0, 1):
        raise ArithmeticError(
            f"LLL returned an invalid imaginary quadratic discriminant: {recovered_D}"
        )
    if discriminant_bound is not None and abs(recovered_D) > discriminant_bound:
        raise ArithmeticError(
            f"Recovered discriminant {recovered_D} exceeds the bound "
            f"{discriminant_bound}"
        )

    return tau_relation, recovered_D


prec = 300
CC = ComplexField(prec)


def load_output(path):
    lines = Path(path).read_text().splitlines()

    prefixes = {
        "public j_invariant: ": "j_0",
        "Alice sends bob... ": "pk_A",
        "Bob sends Alice... ": "pk_B",
    }
    values = {}
    encrypted = None

    for line in lines:
        for prefix, name in prefixes.items():
            if line.startswith(prefix):
                values[name] = CC(line.removeprefix(prefix))
                break
        else:
            if line.startswith("("):
                encrypted = literal_eval(line)

    missing = set(prefixes.values()) - values.keys()
    if missing:
        raise ValueError(f"Missing values in {path}: {', '.join(sorted(missing))}")
    if not isinstance(encrypted, tuple) or len(encrypted) != 2:
        raise ValueError(f"Missing encrypted flag tuple in {path}")

    iv, ciphertext = encrypted
    return values["j_0"], values["pk_A"], values["pk_B"], iv, ciphertext


output_path = Path(__file__).with_name("output.txt")
j_0, pk_A, pk_B, iv, ct = load_output(output_path)

Q0, D = recover_discriminant_and_class(j_0)
QA, DA = recover_discriminant_and_class(pk_A)
assert D == DA
QB, DB = recover_discriminant_and_class(pk_B)
assert D == DB


def relation_to_form(relation):
    return BinaryQF([relation[2], relation[1], relation[0]])

def j_invariant_of_form(Q, D):
    A, B, _ = Q
    tau = (-CC(B) + CC.gen() * CC(-D).sqrt()) / (2 * CC(A))
    return CC(elliptic_j(tau, prec=prec))


Cl = BQFClassGroup(D)
class_0 = Cl(relation_to_form(Q0))
class_A = Cl(relation_to_form(QA))
class_B = Cl(relation_to_form(QB))

assert class_0.is_zero()
shared_class = class_A + class_B
shared_form = shared_class.form()
ss = j_invariant_of_form(shared_form, D)

print(ss.str(digits=20))
key = hashlib.sha1(ss.str(digits=20).encode('ascii')).digest()[:16]
cipher = AES.new(key, AES.MODE_CBC, bytes.fromhex(iv))
flag = unpad(cipher.decrypt(bytes.fromhex(ct)), AES.block_size)

print(f"discriminant: {D}")
print(f"Alice's class: {class_A.form()}")
print(f"Bob's class: {class_B.form()}")
print(f"shared class: {shared_form}")
print(f"shared j-invariant: {ss}")
print(flag.decode())

