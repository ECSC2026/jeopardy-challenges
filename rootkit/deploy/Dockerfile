# syntax=docker/dockerfile:1
FROM docker.io/ubuntu:jammy-20240125@sha256:bcc511d82482900604524a8e8d64bf4c53b2461868dac55f4d04d660e61983cb

ADD --chmod=0755 --checksum=sha256:4c97fd03a3b181996b1473f3a99b69a1efc6ecaf2b4ede061b6bd60a96b9325a \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.0/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && apt-get install -y \
        qemu-system \
        python3-minimal

COPY server.py /server.py
COPY run.sh /run.sh
COPY flag.txt /flag.txt
ADD rootfs.img.tar.gz /

EXPOSE 3000
# Start SSH server
CMD ["/run.sh"]
