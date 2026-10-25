#!/usr/bin/env python3
"""
Generate a passphrase and wrap a bcachefs master key with it, reproducing
exactly the bytes bcachefs itself would store in bch_sb_field_crypt.key
(the struct bch_encrypted_key on-disk field).

This mirrors, byte for byte:
  - c_src/crypto.c:    derive_passphrase()       (scrypt KDF)
  - c_src/crypto.c:    bch_crypt_update_passphrase()  (default KDF params)
  - fs/data/checksum.c / include/crypto/chacha.h:  bch2_chacha20() (ChaCha20 wrap)
  - fs/bcachefs_format.h: struct bch_encrypted_key, __bch2_sb_key_nonce()

in bcachefs-tools (see fs/, c_src/crypto.c in this repo). A superblock
patched with the output of wrap_master_key() will unlock with the printed
passphrase using a completely stock, unmodified bcachefs binary / kernel
module -- no code changes needed on the bcachefs side.

Requires:
  - PyNaCl        (scrypt KDF -- pip install pynacl)
  - libsodium.so  (ChaCha20 -- same library bcachefs-tools links against)

The scrypt salt bcachefs uses is a fixed, non-random literal ("bcache\\0"),
not a per-filesystem random salt -- see c_src/crypto.c derive_passphrase().
That's mirrored here, not a bug in this script.
"""

import argparse
import ctypes
import ctypes.util
import dataclasses
import secrets
import struct
import sys
import uuid

import nacl.bindings

# ---- constants mirrored from fs/bcachefs_format.h / c_src/crypto.c ----

BCH_KEY_MAGIC = b"bch**key"    # __le64 magic 'b','c','h','*','*','k','e','y' in LE byte order
SCRYPT_SALT = b"bcache\x00"    # c_src/crypto.c: const unsigned char salt[] = "bcache";
SCRYPT_LOG_N = 14              # bch_crypt_update_passphrase(): ilog2(16384)
SCRYPT_LOG_R = 3               #                                ilog2(8)
SCRYPT_LOG_P = 4               #                                ilog2(16)

KEY_BYTES = 32                 # struct bch_key { __le64 key[4]; }
WRAPPED_KEY_BYTES = 8 + KEY_BYTES  # struct bch_encrypted_key { magic; key; }

# ---- struct bch_sb header, for locating the internal UUID from raw bytes ----
# fs/bcachefs_format.h: #define BCH_SB_SECTOR 8  -- the primary superblock
# starts at byte offset 8*512 within the filesystem (read_super_and_backups()
# in fs/sb/io.c tries this offset first). This assumes `path` points at the
# start of the bcachefs filesystem/partition itself, not a whole-disk image
# with a partition table in front of it.
BCH_SB_BYTE_OFFSET = 8 * 512

# struct bch_sb is `__packed`, so these offsets are exact, no compiler
# padding to account for:
#   struct bch_csum csum;        // 16 bytes: 2x __le64 (lo, hi)
#   __le16 version;               //  2
#   __le16 version_min;           //  2
#   __le16 pad[2];                //  4
#   __uuid_t magic;               // 16  -- filesystem-type magic, BCHFS_MAGIC
#   __uuid_t uuid;                // 16  -- INTERNAL uuid (what we want)
#   __uuid_t user_uuid;           // 16  -- EXTERNAL uuid (what blkid/lsblk show)
#   __u8 label[32];
#   __le64 offset;
#   __le64 seq;
_BCH_SB_HEADER_FMT = "<QQHHHH16s16s16s32sQQ"
_BCH_SB_HEADER_SIZE = struct.calcsize(_BCH_SB_HEADER_FMT)

# Byte offset of struct bch_sb.start[]/_data[] (the variable-length field
# list) relative to the start of struct bch_sb itself -- i.e. sizeof(struct
# bch_sb) minus its trailing incomplete arrays. Confirmed against bindgen's
# generated offsets (target/release/build/.../out/bcachefs.rs): csum(16) +
# version/version_min/pad(8) + magic/uuid/user_uuid(48) + label(32) +
# offset/seq(16) + block_size/dev_idx/nr_devices/u64s(8) + time_base_lo/hi +
# time_precision(16) + flags[7](56) + write_time(8) + features[2](16) +
# compat[2](16) + struct bch_sb_layout (512) = 752.
_BCH_SB_FIELDS_OFFSET = 752

# struct bch_sb_field { __u64 _data[0]; __le32 u64s; __le32 type; }; -- an
# 8-byte header prefixing every field. u64s is the *whole* field's size
# (header included) in units of 8 bytes.
_BCH_SB_FIELD_HDR_FMT = "<II"
_BCH_SB_FIELD_HDR_SIZE = struct.calcsize(_BCH_SB_FIELD_HDR_FMT)

BCH_SB_FIELD_crypt = 2  # enum bch_sb_field_type, fs/bcachefs_format.h BCH_SB_FIELDS()

# struct bch_sb_field_crypt { struct bch_sb_field field; __le64 flags;
# __le64 kdf_flags; struct bch_encrypted_key key; }; -- key is WRAPPED_KEY_BYTES
# (40) bytes and starts after the 8-byte field header + flags(8) + kdf_flags(8).
_CRYPT_KEY_OFFSET_IN_FIELD = _BCH_SB_FIELD_HDR_SIZE + 8 + 8

# ---- struct bch_sb.csum, for re-checksumming a copy after patching it ----
# fs/bcachefs_format.h: struct bch_sb { struct bch_csum csum; __le16 version;
# ...; __le32 u64s; ...; __le64 flags[7]; ... }. u64s lives right after
# dev_idx/nr_devices (offset 120 -> block_size(2)+dev_idx(1)+nr_devices(1) =
# 124), flags[0] starts at offset 144 (see _BCH_SB_FIELDS_OFFSET's derivation
# above: 16+8+48+32+16+8 = 128... plus time_base_lo/hi/time_precision(16) =
# 144). BCH_SB_CSUM_TYPE is flags[0] bits [2,8) (LE64_BITMASK, bcachefs_format.h).
_BCH_SB_U64S_OFFSET = 124
_BCH_SB_FLAGS0_OFFSET = 144
BCH_CSUM_crc32c_nonzero = 1  # enum bch_csum_type, fs/bcachefs_format.h BCH_CSUM_TYPES()


def _make_crc32c_table() -> list[int]:
    poly = 0x82F63B78  # reflected Castagnoli polynomial (0x1EDC6F41 bit-reversed)
    table = []
    for i in range(256):
        crc = i
        for _ in range(8):
            crc = (crc >> 1) ^ poly if crc & 1 else crc >> 1
        table.append(crc)
    return table


_CRC32C_TABLE = _make_crc32c_table()


def crc32c_nonzero(data: bytes) -> int:
    """Mirror BCH_CSUM_crc32c_nonzero in fs/data/checksum.c:
    bch2_checksum_init() seeds state->seed = U32_MAX; bch2_checksum_update()
    folds in each byte via crc32c(seed, data, len) (the kernel's crc32c() is
    a bare running CRC register -- it does *not* apply its own init/final
    complement, that's left to the caller); bch2_checksum_final() then does
    state->seed ^ U32_MAX. Net effect: ordinary CRC-32C (Castagnoli) with the
    textbook init=0xFFFFFFFF / final-XOR=0xFFFFFFFF -- not the "raw"
    uncomplemented crc32c that BCH_CSUM_crc32c (no _nonzero) uses."""
    crc = 0xFFFFFFFF
    for b in data:
        crc = _CRC32C_TABLE[(crc ^ b) & 0xFF] ^ (crc >> 8)
    return (crc ^ 0xFFFFFFFF) & 0xFFFFFFFF


def recompute_sb_checksum(path: str, sb_base: int) -> int:
    """Recompute and rewrite struct bch_sb.csum for one on-disk superblock
    copy, mirroring csum_vstruct(_c, _type, _nonce, sb) (fs/data/checksum.h):
    the checksum covers every byte from right after the csum field itself
    (offset 16) up to vstruct_end(sb) == _BCH_SB_FIELDS_OFFSET + u64s*8 --
    i.e. the whole fixed header plus every bch_sb_field, checksum field
    excluded. Any write into that range (such as write-key patching
    bch_sb_field_crypt.key) invalidates this and must be followed by a call
    here, or every copy fails bch2_read_super()'s checksum check (see
    fs/sb/io.c:851-861) and the filesystem won't mount."""
    with open(path, "rb") as f:
        f.seek(sb_base + _BCH_SB_FLAGS0_OFFSET)
        flags0 = struct.unpack("<Q", f.read(8))[0]
        csum_type = (flags0 >> 2) & 0x3F  # LE64_BITMASK(BCH_SB_CSUM_TYPE, ..., 2, 8)
        if csum_type != BCH_CSUM_crc32c_nonzero:
            raise ValueError(
                f"sb @ {sb_base} in {path} uses BCH_SB_CSUM_TYPE {csum_type}, "
                "but this script only knows how to recompute crc32c_nonzero (1)"
            )

        f.seek(sb_base + _BCH_SB_U64S_OFFSET)
        u64s = struct.unpack("<I", f.read(4))[0]

        range_start = sb_base + 16  # skip struct bch_csum csum itself
        range_end = sb_base + _BCH_SB_FIELDS_OFFSET + u64s * 8
        f.seek(range_start)
        data = f.read(range_end - range_start)
        if len(data) != range_end - range_start:
            raise ValueError(f"short read computing checksum for sb @ {sb_base} in {path}")

    csum_val = crc32c_nonzero(data)
    with open(path, "r+b") as f:
        f.seek(sb_base)
        f.write(struct.pack("<QQ", csum_val, 0))  # bch_csum { lo; hi=0 }
    return csum_val

# ---- struct bch_sb_layout, for finding *every* on-disk superblock copy ----
# fs/bcachefs_format.h: #define BCH_SB_LAYOUT_SECTOR 7 -- immediately before
# the primary superblock. A freshly formatted filesystem has multiple
# replicated copies (primary + backups spread across the device); every
# copy is a byte-for-byte copy of the same fields (see write_one_super() in
# fs/sb/io.c, which writes the same in-core sb to each sb_offset[i]). A
# `write-key` that only patches the primary leaves the backups holding the
# old key -- bch2_read_super()'s copy-selection logic can then still pick
# a backup and mount with the *old* passphrase, which looks like "the key
# went to the wrong place" from the outside.
#
#   struct bch_sb_layout {
#       __uuid_t magic;                 // 16
#       __u8     layout_type;           //  1
#       __u8     sb_max_size_bits;      //  1
#       __u8     nr_superblocks;        //  1
#       __u8     pad[5];                //  5
#       __le64   sb_offset[61];         // 488  (sector numbers, not bytes)
#   } __packed __aligned(8);            // = 512 bytes total
BCH_SB_LAYOUT_SECTOR = 7
_BCH_SB_LAYOUT_NR_SB_OFFSET = 18
_BCH_SB_LAYOUT_SB_OFFSET_ARRAY_OFFSET = 24
_BCH_SB_LAYOUT_MAX_SUPERBLOCKS = 61


def find_sb_copy_offsets(path: str) -> list[int]:
    """Return the absolute byte offset of every on-disk superblock copy
    (primary + backups), read out of struct bch_sb_layout."""
    layout_byte_offset = BCH_SB_LAYOUT_SECTOR * 512
    with open(path, "rb") as f:
        f.seek(layout_byte_offset)
        layout = f.read(512)
    if len(layout) != 512:
        raise ValueError(f"{path} is too short to contain a bch_sb_layout")

    nr_superblocks = layout[_BCH_SB_LAYOUT_NR_SB_OFFSET]
    if not (0 < nr_superblocks <= _BCH_SB_LAYOUT_MAX_SUPERBLOCKS):
        raise ValueError(f"bogus nr_superblocks ({nr_superblocks}) in {path}'s bch_sb_layout")

    sb_offset_sectors = struct.unpack_from(
        f"<{_BCH_SB_LAYOUT_MAX_SUPERBLOCKS}Q", layout, _BCH_SB_LAYOUT_SB_OFFSET_ARRAY_OFFSET
    )[:nr_superblocks]
    return [sector * 512 for sector in sb_offset_sectors]

# fs/bcachefs_format.h: BCHFS_MAGIC = UUID_INIT(0xc68573f6, 0x66ce, 0x90a9, ...)
# UUID_INIT's byte layout is the same as a standard hyphenated UUID string's
# .bytes, so this is just that UUID's raw bytes.
BCHFS_MAGIC = uuid.UUID("c68573f6-66ce-90a9-d96a-60cf803df7ef").bytes
# Legacy bcache magic -- fs/sb/io.c accepts either when validating a superblock.
BCACHE_MAGIC = uuid.UUID("c68573f6-4e1a-45ca-8265-f57f48ba6d81").bytes


@dataclasses.dataclass
class BchSbHeader:
    version: int
    version_min: int
    magic: bytes
    uuid: bytes         # internal uuid -- struct bch_sb.uuid
    user_uuid: bytes    # external uuid -- struct bch_sb.user_uuid
    label: str
    offset: int
    seq: int


def read_bch_sb_header(path: str) -> BchSbHeader:
    """Read and parse the fixed-size header of the primary bcachefs
    superblock out of a raw device/partition/disk image."""
    with open(path, "rb") as f:
        f.seek(BCH_SB_BYTE_OFFSET)
        raw = f.read(_BCH_SB_HEADER_SIZE)
    if len(raw) != _BCH_SB_HEADER_SIZE:
        raise ValueError(
            f"{path} is too short to contain a bcachefs superblock "
            f"(need {BCH_SB_BYTE_OFFSET + _BCH_SB_HEADER_SIZE} bytes)"
        )

    (csum_lo, csum_hi, version, version_min, _pad0, _pad1,
     magic, sb_uuid, user_uuid, label, offset, seq) = struct.unpack(_BCH_SB_HEADER_FMT, raw)

    if magic not in (BCHFS_MAGIC, BCACHE_MAGIC):
        raise ValueError(
            f"no bcachefs superblock found at byte offset {BCH_SB_BYTE_OFFSET} in {path} "
            f"(magic was {magic.hex()}, expected {BCHFS_MAGIC.hex()}). "
            "Wrong file, wrong offset (whole-disk image with a partition table in "
            "front?), or not a bcachefs filesystem."
        )

    label_str = label.split(b"\x00", 1)[0].decode("utf-8", "replace")
    return BchSbHeader(version, version_min, magic, sb_uuid, user_uuid, label_str, offset, seq)


def find_internal_uuid(path: str) -> uuid.UUID:
    """The function this feature is really about: given a bcachefs disk
    image/device, return its internal UUID (struct bch_sb.uuid) -- the value
    sb_key_nonce()/wrap_master_key() need, equivalent to reading the
    "Internal UUID:" line out of `bcachefs show-super`."""
    return uuid.UUID(bytes=read_bch_sb_header(path).uuid)


def find_crypt_key_offset(path: str, sb_base: int = BCH_SB_BYTE_OFFSET) -> int:
    """Walk one superblock copy's field list (fs/util/vstructs.h's
    vstruct_for_each, done by hand on raw bytes), starting at byte offset
    `sb_base` (defaults to the primary superblock), looking for
    BCH_SB_FIELD_crypt. Returns the absolute file offset of that copy's
    bch_sb_field_crypt.key (the wrapped master key, WRAPPED_KEY_BYTES long).

    Field physical disk order does *not* match the enum's numeric order (and
    thus not the order `bcachefs show-super`'s "Sections:" line prints them
    in, which is driven by the enum, not by vstruct_for_each) -- this always
    walks the real on-disk bytes rather than assuming a position."""
    fields_start = sb_base + _BCH_SB_FIELDS_OFFSET
    with open(path, "rb") as f:
        f.seek(fields_start)
        offset = fields_start
        while True:
            hdr = f.read(_BCH_SB_FIELD_HDR_SIZE)
            if len(hdr) < _BCH_SB_FIELD_HDR_SIZE:
                break
            u64s, field_type = struct.unpack(_BCH_SB_FIELD_HDR_FMT, hdr)
            if u64s == 0:
                break
            field_bytes = u64s * 8
            if field_type == BCH_SB_FIELD_crypt:
                return offset + _CRYPT_KEY_OFFSET_IN_FIELD
            offset += field_bytes
            f.seek(offset)
    raise ValueError(
        f"no BCH_SB_FIELD_crypt found at superblock copy @ byte {sb_base} in {path} "
        "-- filesystem wasn't formatted with --encrypted"
    )


def find_all_crypt_key_offsets(path: str) -> list[int]:
    """find_crypt_key_offset(), but for every on-disk superblock copy
    (primary + backups). A filesystem is only fully re-keyed if the wrapped
    key is written to *all* of these -- see find_sb_copy_offsets()."""
    return [find_crypt_key_offset(path, sb_base) for sb_base in find_sb_copy_offsets(path)]


def cmd_show_key(args):
    key_offsets = find_all_crypt_key_offsets(args.image)
    seen = set()
    with open(args.image, "rb") as f:
        for key_offset in key_offsets:
            f.seek(key_offset)
            wrapped = f.read(WRAPPED_KEY_BYTES)
            if len(wrapped) != WRAPPED_KEY_BYTES:
                sys.exit(f"short read at offset {key_offset} in {args.image}")
            print(f"wrapped key: {wrapped.hex()}  (bch_sb_field_crypt.key, offset {key_offset})")
            seen.add(wrapped)

    if len(seen) > 1:
        print(f"warning: {len(seen)} superblock copies disagree on the wrapped key -- "
              "a previous write-key likely only patched some of them", file=sys.stderr)


def cmd_write_key(args):
    try:
        wrapped = bytes.fromhex(args.wrapped_key)
    except ValueError:
        sys.exit("--wrapped-key must be hex")
    if len(wrapped) != WRAPPED_KEY_BYTES:
        sys.exit(f"--wrapped-key must decode to {WRAPPED_KEY_BYTES} bytes, got {len(wrapped)}")

    sb_bases = find_sb_copy_offsets(args.image)

    with open(args.image, "r+b") as f:
        for sb_base in sb_bases:
            key_offset = find_crypt_key_offset(args.image, sb_base)
            f.seek(key_offset)
            f.write(wrapped)
            print(f"wrote {len(wrapped)}-byte wrapped key to {args.image} at offset {key_offset}")

    # bch_sb_field_crypt.key lives inside the byte range struct bch_sb.csum
    # covers (csum_vstruct(), fs/data/checksum.h) -- patching it without
    # redoing that checksum leaves every copy failing bch2_read_super()'s
    # validation (fs/sb/io.c), which is exactly the "checksum error,
    # type crc32c_nonzero" this script exists to avoid producing.
    try:
        for sb_base in sb_bases:
            csum_val = recompute_sb_checksum(args.image, sb_base)
            print(f"recomputed checksum for sb @ {sb_base}: {csum_val:08x} (crc32c_nonzero)")
    except ValueError as e:
        sys.exit(str(e))

# Compact built-in wordlist for passphrase generation. ~7 bits/word -- for
# serious use, prefer a real diceware list (e.g. EFF's) or pass --passphrase
# yourself. This is a convenience default, not a security claim.
WORDLIST = """
able acid aged also area army away baby back ball band bank base bath bead
beam bean bear beat been beer bell belt bend bent best bike bird bite black
blue boat body bold bolt bone book boot born boss both bowl bulk burn bush
busy cage cake call calm camp card care case cash cast cave cell chat chef
chip city clay clip club coal coat code coin cold come cook cool cope copy
core cost crew crop dark dawn deal debt deck deep deny desk dial dice diet
dirt disk dock dose down draw drop drug drum dust duty each earn easy edge
epic even ever evil exam exit face fact fade fair fall farm fast fear feed
feel file film find fine fire fish flat flow foam fold folk food foot ford
form fort four free frog fuel full fund gaze gear gene gift girl give goal
gold golf good gray grid grow gulf half hall hand hang hard harm hate have
head heat help herb here hero hide high hill hint hire hold hole holy home
hope host huge hunt hurt icon idea inch iron item join joke jury just keen
keep kick kind king knee knew lack lake lamp land lane last late lawn lazy
lead leaf lean left lens less life lift like line link list live load loan
lock logo long look lord lose loss loud love luck lung made mail main make
mall many mark mask mass matt maze meal mean meat meet menu mild mile milk
mind mine mint miss mode mood moon more most move much must myth name navy
near neat neck need nest news next nice nine node none nose note oath obey
odds once only onto open oral over pace pack page pain pair palm park part
pass past path peak pick pile pine pink pipe plan play plot plug plus poem
poet poll pond pool poor port pose post pour pray prep pull pure push race
rack rail rain rank rare rate read real rear rely rent rest rice rich ride
ring rise risk road roar rock role roll roof room root rope rose rule rush
safe sail salt same sand save scan seal seat seed seek seem seen self sell
send sent shed ship shoe shop shot show shut sick side sign silk sing sink
site size skin slot snap snow soap soft soil sold sole some song soul soup
spin spot star stay stem step stop such suit sure swim take tale talk tank
tape task team tell tend tent term test text than that them then they thin
this thus tide tidy tile time tiny tone tool tour town toys tree trip true
tube tune turn twin type unit upon urge used user vast very vice view vote
wage wait wake walk wall want warm wash wave weak wear well went were what
when whom wide wife wild will wind wine wing wire wise wish with wolf wood
wool word work yard yarn year your zeal zero zone zoom
""".split()


def generate_passphrase(num_words: int) -> str:
    words = [secrets.choice(WORDLIST) for _ in range(num_words)]
    return "-".join(words)


def passphrase_entropy_bits(num_words: int) -> float:
    import math
    return num_words * math.log2(len(WORDLIST))


def derive_passphrase_key(passphrase: bytes, log_n=SCRYPT_LOG_N, log_r=SCRYPT_LOG_R, log_p=SCRYPT_LOG_P) -> bytes:
    """Mirror derive_passphrase() in c_src/crypto.c (BCH_KDF_SCRYPT path).

    log_n/r/p default to the values bch_crypt_update_passphrase() picks for
    a freshly-formatted filesystem. If you're targeting an *existing*
    encrypted superblock, read its actual BCH_KDF_SCRYPT_N/R/P bitmask
    fields out of bch_sb_field_crypt.kdf_flags instead (bch_crypt_update_
    passphrase() reuses whatever params were already there rather than
    these defaults once a filesystem has ever had a passphrase set) and
    pass them via --scrypt-log-{n,r,p}.
    """
    return nacl.bindings.crypto_pwhash_scryptsalsa208sha256_ll(
        passphrase,
        SCRYPT_SALT,
        n=1 << log_n,
        r=1 << log_r,
        p=1 << log_p,
        dklen=KEY_BYTES,
    )


class ChaCha20:
    """ctypes binding to libsodium's crypto_stream_chacha20_xor_ic.

    This is the exact primitive fs/data/checksum.c's userspace chacha20_crypt()
    (include/crypto/chacha.h) calls into -- using it directly here (rather than
    a from-scratch ChaCha20 implementation) means this script matches
    bcachefs's on-disk format bit for bit instead of just "compatible in
    spirit".
    """

    def __init__(self):
        libname = ctypes.util.find_library("sodium")
        if not libname:
            sys.exit("libsodium not found (apt/pacman install libsodium)")
        self._lib = ctypes.CDLL(libname)
        self._lib.crypto_stream_chacha20_xor_ic.restype = ctypes.c_int
        self._lib.crypto_stream_chacha20_xor_ic.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_ulonglong,
            ctypes.c_char_p,
            ctypes.c_uint64,
            ctypes.c_char_p,
        ]

    def crypt(self, key: bytes, nonce8: bytes, data: bytes, ic: int = 0) -> bytes:
        assert len(key) == 32, "chacha20 key must be 32 bytes"
        assert len(nonce8) == 8, "chacha20 nonce must be 8 bytes"
        out = ctypes.create_string_buffer(len(data))
        ret = self._lib.crypto_stream_chacha20_xor_ic(
            out, data, len(data), nonce8, ic, key
        )
        if ret != 0:
            raise RuntimeError("crypto_stream_chacha20_xor_ic failed")
        return out.raw


def sb_key_nonce(sb_uuid: bytes) -> bytes:
    """Mirror __bch2_sb_key_nonce(): struct nonce there is
    { d[0]=0, d[1]=0, d[2..3]=first 8 bytes of sb->uuid }. d[0]/d[1] feed the
    ChaCha20 block *counter* (state words 12-13), which is 0 here -- that's
    libsodium's separate `ic` parameter (crypt()'s default ic=0), not part
    of the 8-byte nonce. So the nonce libsodium wants is just those first 8
    bytes of the superblock's *internal* uuid field (struct bch_sb.uuid --
    NOT user_uuid, which is what blkid/lsblk show you). Get it with:

        bcachefs show-super <device>   # look at "Internal UUID:"
    """
    if len(sb_uuid) != 16:
        raise ValueError("sb_uuid must be 16 bytes (a full UUID)")
    return sb_uuid[:8]


def wrap_master_key(master_key: bytes, passphrase: str, sb_uuid: bytes,
                     log_n=SCRYPT_LOG_N, log_r=SCRYPT_LOG_R, log_p=SCRYPT_LOG_P) -> bytes:
    """Produce the on-disk bytes for bch_sb_field_crypt.key
    (struct bch_encrypted_key: __le64 magic + struct bch_key key)."""
    if len(master_key) != KEY_BYTES:
        raise ValueError(f"master key must be {KEY_BYTES} bytes, got {len(master_key)}")

    passphrase_key = derive_passphrase_key(passphrase.encode(), log_n, log_r, log_p)
    # print(passphrase_key.hex())
    plaintext = BCH_KEY_MAGIC + master_key
    nonce = sb_key_nonce(sb_uuid)

    return ChaCha20().crypt(passphrase_key, nonce, plaintext)


def unwrap_master_key(wrapped: bytes, passphrase: str, sb_uuid: bytes,
                       log_n=SCRYPT_LOG_N, log_r=SCRYPT_LOG_R, log_p=SCRYPT_LOG_P) -> bytes:
    """Inverse of wrap_master_key(); also checks the magic, like
    bch2_key_is_encrypted() does, to confirm the passphrase was correct."""
    if len(wrapped) != WRAPPED_KEY_BYTES:
        raise ValueError(f"wrapped key must be {WRAPPED_KEY_BYTES} bytes")

    passphrase_key = derive_passphrase_key(passphrase.encode(), log_n, log_r, log_p)
    nonce = sb_key_nonce(sb_uuid)
    cleartext = ChaCha20().crypt(passphrase_key, nonce, wrapped)

    if cleartext[:8] != BCH_KEY_MAGIC:
        raise ValueError("incorrect passphrase (magic mismatch after unwrap)")
    return cleartext[8:]


def parse_uuid_arg(s: str, argname: str) -> bytes:
    try:
        return uuid.UUID(s).bytes
    except ValueError:
        pass
    try:
        b = bytes.fromhex(s)
    except ValueError:
        sys.exit(f"{argname} must be a UUID or 32 hex chars")
    if len(b) != 16:
        sys.exit(f"{argname} must be 16 bytes")
    return b


def resolve_sb_uuid(args) -> bytes:
    """--sb-uuid and --image are mutually exclusive; --image reads the
    internal uuid straight off the disk image instead of making you run
    `bcachefs show-super`/copy-paste it first."""
    if args.image:
        try:
            hdr = read_bch_sb_header(args.image)
        except (OSError, ValueError) as e:
            sys.exit(str(e))
        print(f"read internal uuid {uuid.UUID(bytes=hdr.uuid)} from {args.image}", file=sys.stderr)
        return hdr.uuid
    return parse_uuid_arg(args.sb_uuid, "--sb-uuid")


def cmd_show_uuid(args):
    try:
        hdr = read_bch_sb_header(args.image)
    except (OSError, ValueError) as e:
        sys.exit(str(e))

    print(f"internal uuid: {uuid.UUID(bytes=hdr.uuid)}   (use this for `wrap --sb-uuid`)")
    print(f"external uuid: {uuid.UUID(bytes=hdr.user_uuid)}   (what blkid/lsblk show you)")
    print(f"label:         {hdr.label or '(none)'}")
    print(f"version:       {hdr.version} (min {hdr.version_min})")
    print(f"seq:           {hdr.seq}")


def cmd_wrap(args):
    try:
        master_key = bytes.fromhex(args.master_key)
    except ValueError:
        sys.exit("--master-key must be hex")
    if len(master_key) != KEY_BYTES:
        sys.exit(f"--master-key must decode to {KEY_BYTES} bytes, got {len(master_key)}")

    sb_uuid = resolve_sb_uuid(args)

    if args.passphrase:
        passphrase = args.passphrase
        print("using supplied passphrase", file=sys.stderr)
    else:
        passphrase = generate_passphrase(args.words)
        bits = passphrase_entropy_bits(args.words)
        print(f"generated passphrase (~{bits:.0f} bits from wordlist; "
              f"use --words to raise this, or --passphrase to supply your own)",
              file=sys.stderr)

    kdf_params = (args.scrypt_log_n, args.scrypt_log_r, args.scrypt_log_p)
    wrapped = wrap_master_key(master_key, passphrase, sb_uuid, *kdf_params)

    # self-check: unwrap what we just wrapped and confirm it round-trips,
    # so a broken environment (e.g. mismatched libsodium) fails loudly here
    # instead of silently producing a superblock nobody can unlock.
    recovered = unwrap_master_key(wrapped, passphrase, sb_uuid, *kdf_params)
    assert recovered == master_key, "internal error: roundtrip mismatch"

    print(f"passphrase:  {passphrase}")
    print(f"master key:  {master_key.hex()}")
    print(f"sb uuid:     {uuid.UUID(bytes=sb_uuid)}")
    print(f"wrapped key: {wrapped.hex()}  (write this into bch_sb_field_crypt.key)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p_wrap = sub.add_parser("wrap", help="wrap a master key under a passphrase")
    sb_group = p_wrap.add_mutually_exclusive_group(required=True)
    sb_group.add_argument("--sb-uuid",
                           help="superblock's internal UUID (bcachefs show-super -> 'Internal UUID:'), "
                                "as a standard UUID string or 32 hex chars")
    sb_group.add_argument("--image",
                           help="path to the disk image/device/partition -- read the internal UUID "
                                "from its superblock automatically instead of passing --sb-uuid")
    p_wrap.add_argument("--master-key", required=True,
                         help="master key as 64 hex chars (32 bytes)")
    p_wrap.add_argument("--passphrase", help="use this passphrase instead of generating one")
    p_wrap.add_argument("--words", type=int, default=8,
                         help="number of words for a generated passphrase (default: 8)")
    p_wrap.add_argument("--scrypt-log-n", type=int, default=SCRYPT_LOG_N,
                         help=f"BCH_KDF_SCRYPT_N from the target superblock's kdf_flags (default: {SCRYPT_LOG_N}, "
                              "the value used for a freshly-formatted filesystem)")
    p_wrap.add_argument("--scrypt-log-r", type=int, default=SCRYPT_LOG_R,
                         help=f"BCH_KDF_SCRYPT_R from the target superblock's kdf_flags (default: {SCRYPT_LOG_R})")
    p_wrap.add_argument("--scrypt-log-p", type=int, default=SCRYPT_LOG_P,
                         help=f"BCH_KDF_SCRYPT_P from the target superblock's kdf_flags (default: {SCRYPT_LOG_P})")
    p_wrap.set_defaults(func=cmd_wrap)

    p_show = sub.add_parser("show-uuid", help="read the internal/external UUID out of a disk image's superblock")
    p_show.add_argument("image", help="path to the disk image/device/partition")
    p_show.set_defaults(func=cmd_show_uuid)

    p_readkey = sub.add_parser("show-key", help="print the wrapped (encrypted) master key out of a disk image's superblock")
    p_readkey.add_argument("image", help="path to the disk image/device/partition")
    p_readkey.set_defaults(func=cmd_show_key)

    p_write = sub.add_parser("write-key", help="overwrite bch_sb_field_crypt.key in a disk image with a wrapped key")
    p_write.add_argument("--image", required=True, help="path to the disk image/device/partition")
    p_write.add_argument("--wrapped-key", required=True,
                          help=f"wrapped key as {WRAPPED_KEY_BYTES * 2} hex chars ({WRAPPED_KEY_BYTES} bytes) -- "
                               "the 'wrapped key' output of the `wrap` subcommand")
    p_write.set_defaults(func=cmd_write_key)

    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
