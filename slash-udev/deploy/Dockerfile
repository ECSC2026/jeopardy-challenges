# syntax=docker/dockerfile:1
FROM ubuntu:jammy-20240125@sha256:bcc511d82482900604524a8e8d64bf4c53b2461868dac55f4d04d660e61983cb

ADD --chmod=0755 --checksum=sha256:4c97fd03a3b181996b1473f3a99b69a1efc6ecaf2b4ede061b6bd60a96b9325a \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.0/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && apt-get install -y \
    openssh-server \
    git \
    musl-dev \
    musl-tools \
    make \
    xz-utils \
    strace

#Ugly hack to be able to build static sudo
RUN printf '#!/bin/sh\nexec musl-gcc -static "$@"\n' > /usr/local/bin/musl-gcc-static && \
    chmod +x /usr/local/bin/musl-gcc-static

RUN git clone --depth=1 --branch v1.9.17p2 https://github.com/sudo-project/sudo/ && \
    cd sudo && \
    CC=musl-gcc-static ./configure \
        --disable-shared \
        --disable-shared-libutil \
        --without-pam \
        --without-selinux \
        --without-audit \
        --disable-log-server \
        --disable-log-client \
        --without-apparmor && \
    make && make install && cd .. && rm -rf sudo

RUN mkdir /var/run/sshd
#password for user login
RUN sed -i 's/#Port 22/Port 3000/' /etc/ssh/sshd_config \
    && sed -i 's/#PermitEmptyPasswords no/PermitEmptyPasswords yes/' /etc/ssh/sshd_config  \
    && printf "\n\nAuthenticationMethods none\n" >> /etc/ssh/sshd_config

COPY run.sh /run.sh
COPY flag.txt /flag.txt
RUN \
    useradd --create-home --comment "CTF image user" -s /bin/bash ctf && \
    usermod -aG sudo ctf && \
    echo "ctf ALL=(ctf) ALL" | tee /etc/sudoers.d/permissions && \
    passwd -d ctf && chmod 600 /flag.txt

WORKDIR /work

EXPOSE 3000
# Start SSH server
CMD ["/run.sh"]
