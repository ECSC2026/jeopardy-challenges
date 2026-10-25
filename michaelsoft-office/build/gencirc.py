#!/usr/bin/env python3
"""
Vibe-coded script for generating a random flag verifier circuit. Everything below is LLM-ramblings.

flag_circuit_gen.py

Generates a randomized "flag verifier circuit": a sequence of byte-wise
operations whose only accepting input (of the correct length) is the flag.

ISA (every value is 8-bit, arithmetic is mod 256):

    ADDI  rd, imm        rd = (rd + imm) & 0xFF
    XORI  rd, imm        rd = rd ^ imm
    SHUF  rd, perm       rd = permute_bits(rd, perm)   # out_bit[i] = in_bit[perm[i]]
                         (arbitrary 8-bit reorder; shift / rotl / rotr are special cases)
    ADD   rd, rs         rd = (rd + rs) & 0xFF         (rd != rs)
    XOR   rd, rs         rd = rd ^ rs                  (rd != rs)
    MUL   rd, rs, rt     rd = ((rs & 0xF) * (rt & 0xF)) & 0xFF   # 4x4 -> 8, upper operand bits ignored

Why the accepting input is unique
---------------------------------
The "core" registers r0..r{n-1} are seeded with the input bytes and are only
ever transformed by invertible operations:

  * ADDI / XORI / SHUF are bijections on a single register.
  * ADD / XOR with rd != rs are shear maps, hence bijections on the whole
    core state (given the result you can recover the old rd because rs is
    unchanged).

A composition of bijections is a bijection F : (Z256)^n -> (Z256)^n. Checking
every core output against F(flag) therefore admits exactly one solution of the
correct length: the flag itself.

MUL is not invertible (it discards the high nibble of each operand and the
product is generally not one-to-one), so it never writes a core register. Each
MUL writes a fresh scratch register computed from two core registers; that
scratch result is also checked. Those checks are redundant for uniqueness but
add reversing surface for free, and the true flag always satisfies them.
"""

import argparse
import random
import sys


# ----------------------------------------------------------------------------
# Simulator
# ----------------------------------------------------------------------------

def permute_bits(x, perm):
    out = 0
    for i, src in enumerate(perm):
        out |= ((x >> src) & 1) << i
    return out & 0xFF


def run(program, regs):
    R = list(regs)
    for op in program:
        k = op[0]
        if k == 'ADDI':
            _, rd, imm = op
            R[rd] = (R[rd] + imm) & 0xFF
        elif k == 'XORI':
            _, rd, imm = op
            R[rd] = R[rd] ^ imm
        elif k == 'SHUF':
            _, rd, perm = op
            R[rd] = permute_bits(R[rd], perm)
        elif k == 'ADD':
            _, rd, rs = op
            R[rd] = (R[rd] + R[rs]) & 0xFF
        elif k == 'XOR':
            _, rd, rs = op
            R[rd] = R[rd] ^ R[rs]
        elif k == 'MUL':
            _, rd, rs, rt = op
            R[rd] = ((R[rs] & 0xF) * (R[rt] & 0xF)) & 0xFF
        else:
            raise ValueError("bad op: %r" % (op,))
    return R


# ----------------------------------------------------------------------------
# Generation
# ----------------------------------------------------------------------------

def generate(flag, n_ops=None, n_mul=None, seed=None):
    """flag: bytes. Returns a circuit dict (no plaintext flag inside)."""
    rng = random.Random(seed)
    n = len(flag)
    if n == 0:
        raise ValueError("empty flag")

    if n_ops is None:
        n_ops = max(8, n * 6)
    if n_mul is None:
        n_mul = max(1, n // 2)

    n_scratch = n_mul
    total_regs = n + n_scratch
    program = []

    core_unary = ['ADDI', 'XORI', 'SHUF']
    core_binary = ['ADD', 'XOR'] if n >= 2 else []

    for _ in range(n_ops):
        if core_binary and rng.random() < 0.45:
            kind = rng.choice(core_binary)
        else:
            kind = rng.choice(core_unary)

        rd = rng.randrange(n)
        if kind == 'ADDI':
            program.append(('ADDI', rd, rng.randrange(1, 256)))
        elif kind == 'XORI':
            program.append(('XORI', rd, rng.randrange(1, 256)))
        elif kind == 'SHUF':
            perm = list(range(8))
            rng.shuffle(perm)
            program.append(('SHUF', rd, perm))
        elif kind == 'ADD':
            rs = rng.randrange(n)
            while rs == rd:
                rs = rng.randrange(n)
            program.append(('ADD', rd, rs))
        elif kind == 'XOR':
            rs = rng.randrange(n)
            while rs == rd:
                rs = rng.randrange(n)
            program.append(('XOR', rd, rs))

    # Splice MUL ops in at random points, each into its own fresh scratch reg.
    scratch_next = n
    for _ in range(n_mul):
        rs = rng.randrange(n)
        rt = rng.randrange(n)
        sd = scratch_next
        scratch_next += 1
        pos = rng.randrange(len(program) + 1)
        program.insert(pos, ('MUL', sd, rs, rt))

    # Targets: simulate the finished program on the true flag.
    init = list(flag) + [0] * n_scratch
    final = run(program, init)
    checks = [(i, final[i]) for i in range(total_regs)]

    return {
        'n': n,
        'n_scratch': n_scratch,
        'program': program,
        'checks': checks,
    }


def verify(circuit, input_bytes):
    if len(input_bytes) != circuit['n']:
        return False
    init = list(input_bytes) + [0] * circuit['n_scratch']
    R = run(circuit['program'], init)
    return all(R[i] == v for (i, v) in circuit['checks'])


# ----------------------------------------------------------------------------
# Disassembly / emission
# ----------------------------------------------------------------------------

def disassemble(circuit):
    lines = []
    for i, op in enumerate(circuit['program']):
        k = op[0]
        if k in ('ADDI', 'XORI'):
            lines.append("%4d  %-4s r%d, 0x%02x" % (i, k, op[1], op[2]))
        elif k == 'SHUF':
            lines.append("%4d  SHUF r%d, %s" % (i, op[1], op[2]))
        elif k in ('ADD', 'XOR'):
            lines.append("%4d  %-4s r%d, r%d" % (i, k, op[1], op[2]))
        elif k == 'MUL':
            lines.append("%4d  MUL  r%d, r%d, r%d" % (i, op[1], op[2], op[3]))
    lines.append("CHECK " + ", ".join("r%d==0x%02x" % (r, v) for r, v in circuit['checks']))
    return "\n".join(lines)

def to_rust(circuit):
    lines = []
    for i, op in enumerate(circuit['program']):
        k = op[0]
        if k == 'ADDI':
            lines.append(f"i.add(AddImm(Mem({op[1]}), Mem({op[1]}), {op[2]}));")
        elif k == 'XORI':
            lines.append(f"i.add(XorImm(Mem({op[1]}), Mem({op[1]}), {op[2]}));")
        elif k == 'SHUF':
            lines.append(f"i.add(Swizzle(Mem({op[1]}), Mem({op[1]}), [{', '.join(f'Some({x})' for x in op[2])}]));")
        elif k == 'ADD':
            lines.append(f"i.add(Add(Mem({op[1]}), Mem({op[1]}), Mem({op[2]})));")
        elif k == 'XOR':
            lines.append(f"i.add(Xor(Mem({op[1]}), Mem({op[1]}), Mem({op[2]})));")
        elif k == 'MUL':
            lines.append(f"i.add(FourBitMultiply(Mem({op[1]}), Mem({op[2]}), Mem({op[3]})));")
        else:
            raise ValueError("bad op: %r" % (op,))
        
    lines.append(f"i.add(SetImm(Reg(A), 0));")
    for r, v in circuit['checks']:
        lines.append(f"i.add(XorImm(Reg(B), Mem({r}), {v}));")
        lines.append(f"i.add(Or(Reg(A), Reg(A), Reg(B)));")
    
    lines.append(f"i.add(SetImm(Reg(B), 0));")
    lines.append(f"i.add(Jeq(\"accept\", Reg(A), Reg(B)));")
    lines.append(f"i.add(Exit(\"Your identity as a human could not be verified.\", false));")
    lines.append(f"i.add(Label(\"accept\"));")
    lines.append(f"i.add(Exit(\"Thanks for proving your mortality. You may now proceed with the installation.\", true));")

    return "\n".join(lines)


_VERIFIER_TEMPLATE = '''#!/usr/bin/env python3
"""Standalone flag verifier. The flag is not stored here in plaintext."""
import sys

CIRCUIT = {circuit!r}


def permute_bits(x, perm):
    out = 0
    for i, src in enumerate(perm):
        out |= ((x >> src) & 1) << i
    return out & 0xFF


def run(program, regs):
    R = list(regs)
    for op in program:
        k = op[0]
        if k == 'ADDI':
            _, rd, imm = op; R[rd] = (R[rd] + imm) & 0xFF
        elif k == 'XORI':
            _, rd, imm = op; R[rd] = R[rd] ^ imm
        elif k == 'SHUF':
            _, rd, perm = op; R[rd] = permute_bits(R[rd], perm)
        elif k == 'ADD':
            _, rd, rs = op; R[rd] = (R[rd] + R[rs]) & 0xFF
        elif k == 'XOR':
            _, rd, rs = op; R[rd] = R[rd] ^ R[rs]
        elif k == 'MUL':
            _, rd, rs, rt = op; R[rd] = ((R[rs] & 0xF) * (R[rt] & 0xF)) & 0xFF
    return R


def verify(input_bytes):
    if len(input_bytes) != CIRCUIT['n']:
        return False
    init = list(input_bytes) + [0] * CIRCUIT['n_scratch']
    R = run(CIRCUIT['program'], init)
    return all(R[i] == v for (i, v) in CIRCUIT['checks'])


if __name__ == '__main__':
    if len(sys.argv) > 1:
        candidate = sys.argv[1].encode()
    else:
        candidate = sys.stdin.readline().rstrip("\\n").encode()
    print("PASS" if verify(candidate) else "FAIL")
'''


def emit_verifier(circuit):
    return _VERIFIER_TEMPLATE.format(circuit=circuit)


# ----------------------------------------------------------------------------
# CLI
# ----------------------------------------------------------------------------

def main(argv=None):
    ap = argparse.ArgumentParser(description="Generate a random flag verifier circuit.")
    ap.add_argument("flag", help="the flag string the circuit must accept")
    ap.add_argument("--out", default="verifier.py", help="output verifier file (default: verifier.py)")
    ap.add_argument("--ops", type=int, default=None, help="number of core operations")
    ap.add_argument("--mul", type=int, default=None, help="number of MUL operations")
    ap.add_argument("--seed", type=int, default=None, help="RNG seed for reproducibility")
    ap.add_argument("--disasm", action="store_true", help="print the generated program")
    args = ap.parse_args(argv)

    flag = args.flag.encode()
    circuit = generate(flag, n_ops=args.ops, n_mul=args.mul, seed=args.seed)

    # Self-tests.
    assert verify(circuit, flag), "generated circuit rejects the true flag"
    mutated = bytearray(flag)
    mutated[0] ^= 0x01
    assert not verify(circuit, bytes(mutated)), "circuit accepts a mutated flag"

    with open(args.out, "w") as f:
        f.write(emit_verifier(circuit))

    if args.disasm:
        print(to_rust(circuit))
        print()

    print("flag length      : %d" % circuit['n'])
    print("core ops          : %d" % sum(1 for op in circuit['program'] if op[0] != 'MUL'))
    print("MUL ops           : %d" % sum(1 for op in circuit['program'] if op[0] == 'MUL'))
    print("checks            : %d" % len(circuit['checks']))
    print("verifier written  : %s" % args.out)


if __name__ == '__main__':
    main()