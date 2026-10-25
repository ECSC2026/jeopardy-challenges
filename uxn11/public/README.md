# uxn11

This handout contains the source repository for the privileged Uxn command-line
emulator and the exact glibc runtime shipped by the challenge. The challenge
changes are left uncommitted in `uxn11-src.zip`, so they can be inspected with:

```sh
unzip uxn11-src.zip
cd uxn11
git diff
```

The same changes are also provided separately in `uxn11.patch`.

Build the emulator with:

```sh
make
```

The remote service accepts passwordless SSH connections using username `ctf`.
The emulator is installed at `/usr/sbin/uxn11` and accepts a ROM path as its
first argument.
