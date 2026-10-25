#!/usr/bin/env python3
import base64
import os
import subprocess
import tempfile


def main():
    print("ROM (base64): ", end="", flush=True)
    line = input().strip()
    try:
        rom = base64.b64decode(line, validate=True)
    except ValueError:
        print("invalid ROM")
        return
    if not 0 < len(rom) <= 0xFF00:
        print("invalid ROM")
        return

    with tempfile.TemporaryDirectory(dir="/tmp") as directory:
        path = os.path.join(directory, "challenge.rom")
        with open(path, "wb") as stream:
            stream.write(rom)
        try:
            subprocess.run(
                ["/uxn-lfb", path],
                cwd=directory,
                timeout=30,
                check=False,
            )
        except subprocess.TimeoutExpired:
            print("timeout")


if __name__ == "__main__":
    main()
