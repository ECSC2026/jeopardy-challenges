# Solution

The challenge is simple XOR cipher with the hardcoded string `W3lc0m3_t0_ECSC2026!`. So to solve it, two ways are possible:

1. Reverse engineer the encryption key (e.g. `strings`)
2. Rename the `flag.txt.encrypted` file back to `flag.txt` and run the encrypter again => Decrypted

Challenge is build dynamic, not static. As i have quite some old glibc, it should run on (almost) all systems. Building it static will make the challenge much harder.
