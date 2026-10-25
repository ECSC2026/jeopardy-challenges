# uxn-lfb intended solution

## Challenge changes

The upstream program expects a Linux virtual terminal, input devices, and
`/dev/fb0`. The challenge patch replaces that hardware initialization with an
anonymous framebuffer so that the emulator can run in a container. It also
removes the console command-execution DEO and the two file-read DEOs. The VM
therefore cannot obtain the flag through its normal device interface; code
execution must come from corrupting the native process.

## Vulnerability

Uxn has sixteen 64 KiB RAM banks. The system expansion device copies memory
between locations described by an eleven-byte structure in VM RAM. For a copy,
`system_deo_expansion()` checks only that the source and destination bank bases
are below `BANKS_CAP`:

```c
if(bank < BANKS_CAP && dst_bank < BANKS_CAP)
    memmove(ram + dst_bank + dst_addr, ram + bank + addr, length);
```

It does not check `addr + length` or `dst_addr + length`. A descriptor starting
at bank 15, offset `0xf000`, can consequently cross the end of the 1 MiB `ram`
allocation. Copying toward VM RAM gives an out-of-bounds read; reversing the
copy gives an out-of-bounds write. Because the large `calloc()` is backed by an
anonymous mapping immediately before glibc's mappings in this runtime, the
overrun reaches libc and thread-local state without knowing the PIE base.

## Stage one: leak and loader

The first ROM copies `0x3000` bytes beginning at bank 15 offset `0xf000` into
ordinary VM RAM at `0x4000`, then writes those bytes to the console. The first
`0x1000` bytes are the end of valid VM RAM and the remaining bytes disclose the
adjacent mappings. The supplied libc and loader make the offsets within this
leak stable. The solver extracts:

- a pointer to libc's `_res`, used to calculate the libc base;
- the x86-64 thread pointer guard; and
- a loader pointer from which the real `_dl_fini` address is derived.

Stage one then installs a console vector which accepts exactly `0x4000` decoded
bytes into VM RAM. A raw `0xff` is interpreted as EOF by the emulator's signed
console-byte path, so `encode_stage()` chooses a one-byte XOR key that makes the
entire second stage free of `0xff` bytes.

## Stage two: redirect an exit handler

glibc stores exit-handler function pointers using the thread's pointer guard.
On x86-64 the relevant operation is equivalent to rotating
`pointer ^ guard`. One existing handler contains a mangled pointer to
`_dl_fini`. Instead of finding and overwriting the handler, the exploit changes
the pointer guard so that this already-stored value decodes to a libc gadget:

```text
new_guard = old_guard ^ _dl_fini ^ gadget
```

The second Uxn stage first copies `0x2800` bytes from the end of VM RAM into a
scratch area, preserving everything around the guard. It changes only the
guard in that snapshot and copies the prefix back across the end of the RAM
mapping. Finally it writes `0x80` to the system state port, causing the emulator
to return normally and invoke glibc's exit machinery.

## Reusing the console stack buffer

Before stage two exits, stage one changes the console vector once more. The
solver waits for that transition and sends one full 255-byte native console
read. `emu_run()` has already placed all 255 bytes in its `coninp` stack buffer
before it invokes the Uxn console vector for the first byte. That vector clears
itself, jumps to stage two, corrupts the guard, and requests termination while
the attacker-controlled buffer is still present in the reusable stack area.

The altered exit handler decodes to an `add rsp, 8; ret` gadget. At this point
the fixed native call layout places the tail of `coninp` at the next return
sequence, so the gadget enters the pwntools-generated ROP chain stored at
offset `0xe0` in the carrier. The chain calls `system("/bin/sh")`. The solver
then sends `cat /flag` through that shell and prints the returned flag.

## Running the solver

From the challenge directory:

```sh
docker build -f solution/Dockerfile -t uxn-lfb-solution .
docker run --rm uxn-lfb-solution HOST PORT
```

Pass `--ssl` after the port when connecting directly to the platform's TLS
endpoint.
