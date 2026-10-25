# syntax=docker/dockerfile:1
FROM ubuntu:22.04@sha256:0d779ea97881505f5ef0039336ee85edba27519bdba968c284c86ee066a973c8 AS ubuntu

ADD --chmod=0755 --checksum=sha256:c125df9762b0c7233459087bb840c0e5dbfc4d9690ee227f1ed8994f4d51d2e0 \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.4/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

FROM ubuntu AS builder

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends gcc libc6-dev unzip

COPY public/uxn-lfb-src.zip /tmp/uxn-lfb-src.zip

RUN unzip /tmp/uxn-lfb-src.zip -d /tmp \
    && gcc -Wall -o /tmp/uxn-lfb/uxn-lfb /tmp/uxn-lfb/uxn-lfb.c

FROM ubuntu AS runtime

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends python3 socat

RUN useradd -s /usr/bin/nologin ctf

COPY --from=builder /tmp/uxn-lfb/uxn-lfb /uxn-lfb
COPY public/server.py /server.py
COPY deploy/flag.txt /flag

RUN chmod 0555 /uxn-lfb /server.py \
    && chmod 0444 /flag

USER ctf
EXPOSE 1337
CMD ["socat", "-T40", "TCP-LISTEN:1337,reuseaddr,fork", "EXEC:python3 /server.py,stderr"]
