# micro

## The service

`server.py` is a static file server for the MicroPython unix port. It serves
the files in `www/` and enforces exactly one rule:

```python
if "/" in name or "\\" in name:
    return 403, None
```

No separators, so the filename cannot leave `www/`. Right after that check the
request name is passed through `normpath`, which exists because the (fictional)
Windows uploader sends backslash-separated paths:

```python
_SEP = {ord("\\"): "/"}

def normpath(p):
    return "".join(_SEP.get(ord(c), c) for c in p)
```

Folding `\` to `/` after a check that rejects both looks harmless. It isn't.

## The bug

MicroPython stores `str` as UTF-8 bytes and its `bytes.decode("utf-8")` does
not validate the input, so an overlong encoding survives decoding unchanged.
`U+005C` (backslash) is normally the single byte `5C`, but it can also be
written as the two-byte overlong sequence `C1 9C`.

The two operations disagree about what that sequence is:

* `"\\" in name` is a **byte-level** substring search. It looks for the byte
  `5C`, which never appears in `C1 9C`, so the check passes.
* `ord(c)` **decodes** the character. It reads `C1 9C` and returns `92`, so
  `_SEP` matches and emits a real ASCII `/`.

The separator the check refused to accept is manufactured by the line that runs
immediately after it. Note that the overlong encoding of `/` itself
(`C0 AF`) does not work: `_SEP` only maps `92`, so that sequence is passed
through to `open()` unchanged and simply produces a 404. The backslash folding
is what repairs it.

CPython is not affected: its decoder rejects overlong forms, so `.decode()`
raises and the server answers 400.

## Solving it

`www/` is `/srv/www`, so two levels up is `/`:

```
GET /..%c1%9c..%c1%9cflag.txt HTTP/1.1
```

The percent-escapes decode to `..\..\flag.txt`, the check sees no separator,
`normpath` turns it into `../../flag.txt`, and the server opens
`www/../../flag.txt` = `/flag.txt`.

Send it with a raw socket. Every real HTTP client (curl included, unless you
pass `--path-as-is`) normalises `..` out of the path before it goes on the
wire.

Either engine works, podman needs no extra flags:

```
docker build -t micro-solve solution/
docker run --rm --network host micro-solve 127.0.0.1 1337
docker run --rm micro-solve https://<session>.challenge.ecsc2026.de/
```

```
podman build -t micro-solve solution/
podman run --rm --network host micro-solve 127.0.0.1 1337
podman run --rm micro-solve https://<session>.challenge.ecsc2026.de/
```

`C1 9C` is not the only encoding that works. MicroPython derives the codepoint
from the bits it is given, so the three- and four-byte overlong forms of the
same character (`E0 81 9C`, `F0 80 81 9C`) decode to 92 as well and are equally
valid payloads. A flag checker should not assume one particular form.

## Fix

Validate UTF-8 on the way in, or do the separator check after normalisation
rather than before it.
