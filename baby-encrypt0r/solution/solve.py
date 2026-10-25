#!/usr/bin/env python3
"""Solution script: undo the repeating-key XOR applied by encrypt.c.

XOR is its own inverse, so decrypting is the exact same operation as
encrypting -- we just walk every *.encrypted file in the current
directory and XOR it against the key again.

The key is not hidden in any way: `strings encrypt` gives it up.
"""

import os
import sys

KEY = b"W3lc0m3_t0_ECSC2026!"
SUFFIX = ".encrypted"


def xor(data, key):
    return bytes(b ^ key[i % len(key)] for i, b in enumerate(data))


def main():
    force = "--force" in sys.argv

    targets = sorted(
        f for f in os.listdir(".")
        if f.endswith(SUFFIX) and len(f) > len(SUFFIX) and os.path.isfile(f)
    )

    if not targets:
        print("no %s files found in the current directory" % SUFFIX)
        return 1

    for name in targets:
        out_name = name[: -len(SUFFIX)]

        if os.path.exists(out_name) and not force:
            print("[-] %s already exists, skipping (use --force to overwrite)" % out_name)
            continue

        with open(name, "rb") as fh:
            data = fh.read()

        with open(out_name, "wb") as fh:
            fh.write(xor(data, KEY))

        print("[+] %s -> %s (%d bytes)" % (name, out_name, len(data)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
