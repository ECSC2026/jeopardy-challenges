FROM debian:trixie@sha256:ed49f2fa955a92dd859d7e2bdd101138b6db7ca855af572212d910d4e1445cc2

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      qemu-system-misc haproxy \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/sh ctf

WORKDIR /home/ctf

COPY treckdemo.elf .
COPY entrypoint.sh .
COPY haproxy.cfg .
RUN chmod +x entrypoint.sh

# Default flag for local testing; override with `-e FLAG=...` at runtime.
ENV FLAG="ECSC{public_flag}"

EXPOSE 1024

ENTRYPOINT ["./entrypoint.sh"]
