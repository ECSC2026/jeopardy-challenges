# syntax=docker/dockerfile:1
FROM ubuntu:26.04@sha256:53958ec7b67c2c9355df922dd08dbf0360611f8c3cdb656875e81873db9ffdba AS ubuntu

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

COPY uxn11-src.zip /tmp/uxn11-src.zip
RUN unzip /tmp/uxn11-src.zip -d /tmp \
    && cc /tmp/uxn11/etc/alt/uxncli11.c -lutil -o /tmp/uxn11/uxn11

FROM ubuntu AS runtime

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install \
    -y --no-install-recommends \
    openssh-server

RUN sed -i 's/^#Port 22$/Port 3000/' /etc/ssh/sshd_config \
    && sed -i 's/^#PermitEmptyPasswords no$/PermitEmptyPasswords yes/' /etc/ssh/sshd_config \
    && printf '\nAuthenticationMethods none\n' >> /etc/ssh/sshd_config
RUN useradd --system --no-create-home --shell /usr/sbin/nologin flag \
    && useradd --create-home --comment "CTF image user" -s /bin/bash ctf \
    && passwd -d ctf

COPY --from=builder /tmp/uxn11/uxn11 /usr/sbin/uxn11
COPY --chown=flag:root --chmod=0400 flag.txt /flag

RUN chown flag:root /usr/sbin/uxn11 \
    && chmod 4555 /usr/sbin/uxn11

WORKDIR /home/ctf
EXPOSE 3000

CMD ["/usr/sbin/sshd", "-D"]
