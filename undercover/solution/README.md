# Solution

The flag lives at `https://safehouse.internal/flag`, reachable only from inside the
bot container and readable only by same-origin script.

1. **The Root CA key is factorable.** `../deploy/keygen.c` fills each base-2³² limb of
   the CA's RSA primes with a single random byte, so every limb is `< 256`. `p·q`
   has no inter-limb carries, so `n`'s base-2³² digits are exactly the coefficients
   of `f_p·f_q`. Factor that polynomial, evaluate each factor at 2³² and take the gcd
   with `n` to recover `p`, then rebuild the key.
   (see https://blog.trailofbits.com/2026/06/12/factoring-short-sleeve-rsa-keys-with-polynomials/)
2. **Signed Exchanges decouple an origin from who serves it.** Chrome attributes the
   content to the signed URL's origin, not the host that served it, and gives it real
   same-origin access. `safehouse.internal` is unreachable any other way, it resolves
   to 127.0.0.1 inside the bot. Chrome accepts an exchange whose ECDSA leaf carries the
   `CanSignHttpExchanges` extension (OID `1.3.6.1.4.1.11129.2.1.22`) and chains to a
   trusted root. The admin trusts the internal Root CA, so once you factor that CA
   you can mint your own leaf plus a CA-signed OCSP response.
3. **Forge and deliver.** `gen-certurl` + `gen-signedexchange` build the cert-chain CBOR
   and an SXG that signs your payload as `https://safehouse.internal/`. Host it over
   HTTPS and `POST` the URL to `/report`. The payload now runs as
   `https://safehouse.internal/`, so `fetch('/flag')` is same-origin.

> Chrome's SXG verifier rejects RSA (ECDSA/Ed25519 only). The weak key is the **CA**.
> The exchange itself is signed by an ECDSA leaf that chains to it.

> Chrome skips Certificate Transparency for certs chaining to a locally installed root,
> which is why a freshly minted leaf passes without SCTs. `gen-signedexchange` warns
> about the missing SCT list. Ignore it.

> [badkeys](https://github.com/badkeys/badkeys) names the bug class for free:
> `badkeys ca.crt` reports `rsapoly/completeftp`. It stops there. `rsapoly` is
> detection-only, `--keyrecover` recovers nothing, and step 1 still has to be written
> by hand.

## Setup

```bash
# WICG webpackage tools on PATH (~/go/bin):
go install github.com/WICG/webpackage/go/signedexchange/cmd/gen-signedexchange@latest
go install github.com/WICG/webpackage/go/signedexchange/cmd/gen-certurl@latest

pipenv install
```

`exploit.py` fetches the instance Root CA from `BOT_URL/ca.crt`. Override with `CA_CRT=path.pem`.

## Run

The bot has to reach the exploit host, so `DIST_HOST` needs an address the bot can
connect to. It does not need a publicly trusted certificate: the exploit host's TLS
cert is minted from the recovered CA, which the bot already trusts. Any public TCP
port works.

Against a deployed instance, ports 80 and 443 are taken, so remote-forward one from
a box with a public address:

```bash
ssh -N -R 0.0.0.0:8443:localhost:8443 myserver.example      # server needs GatewayPorts
BOT_URL=https://<session>-main-1337-undercover.challenge.ecsc2026.de \
DIST_HOST=myserver.example DIST_PORT=8443 pipenv run python exploit.py
```

Against the local compose instance, `BOT_URL=http://localhost:1337` and `DIST_HOST` is
your host's LAN address. `localhost` does not work, the bot resolves it inside its own
container.

Knobs: `CA_CRT`, `BOT_URL`, `SAFEHOUSE_ORIGIN`, `DIST_HOST`, `DIST_PORT`, `LISTEN_PORT`
(local bind port, if it differs from the forwarded `DIST_PORT`).
