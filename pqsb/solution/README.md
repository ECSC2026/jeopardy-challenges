Run it with:

```
sage doit.sage <url>
```

## The Bug

`mldsa.py` is a from-scratch ML-DSA-87. 

The bug is in `expand_mask`. It fills a mask polynomial 
by slicing a SHAKE-256 stream into `GAMMA1_BITS = 20` bit fields:

```python
coeffs = [0] * N
bounds = range(0, N * GAMMA1_BITS, GAMMA1_BITS)
for pos, (lo, hi) in enumerate(itertools.pairwise(bounds)):
    ...
```

`bounds` has `N` elements, so `pairwise` yields `N - 1` slices:
coefficient 255 of every mask polynomial is 0.

## Recover the signing key

A signature is `z = y + c*s1`. 

So with `y[255] = 0` in every component, and because the top coefficient of a product mod `x^256 + 1` takes no wraparound term:

    z_j[255] = sum_i c[i] * s1_j[255 - i]

Holds over the integers, not just mod q. 

The right-hand side is bounded by `beta = tau * eta = 120`, so lifting `z_j[255]` to its centered representative gives an exact linear equation in the 256 coefficients of `s1_j`. 

However, we only hand out 199 signatures at most.
Therefore, the solvers need to use the *shortness* of `s1_j`: it has coefficients in `[-2, 2]`.
Using this, a solution can be recovered using LLL.
