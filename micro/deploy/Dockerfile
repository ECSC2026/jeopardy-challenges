# $ crane digest debian:bookworm-20260824-slim
# sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171
FROM debian:bookworm-20260824-slim@sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171 AS build

# ADD can't check the hash for us, buildah only learned that in 1.35 and
# podman 4.x still errors out on it. sha256sum below does the job.
ADD --chmod=0755 \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.0/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    echo "4c97fd03a3b181996b1473f3a99b69a1efc6ecaf2b4ede061b6bd60a96b9325a  /usr/local/bin/repro-sources-list.sh" | sha256sum -c - && \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && \
    apt-get install -y --no-install-recommends build-essential python3 xz-utils

# tarball has the submodules in it, the git tag doesn't
ADD https://github.com/micropython/micropython/releases/download/v1.29.0/micropython-1.29.0.tar.xz \
    /tmp/micropython.tar.xz

# none of ffi/btree/ssl are used, leaves us with a libc-only binary
RUN echo "d925a7c664e79a2bdf3dfcb285ba5e2237041cc35a0bd4ee573b6c5711efeca0  /tmp/micropython.tar.xz" | sha256sum -c - && \
    mkdir -p /tmp/micropython && \
    tar -xJf /tmp/micropython.tar.xz -C /tmp/micropython --strip-components=1 && \
    make -C /tmp/micropython/mpy-cross -j"$(nproc)" && \
    make -C /tmp/micropython/ports/unix -j"$(nproc)" \
        MICROPY_PY_FFI=0 MICROPY_PY_BTREE=0 MICROPY_PY_SSL=0 && \
    strip /tmp/micropython/ports/unix/build-standard/micropython

FROM debian:bookworm-20260824-slim@sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171

COPY --from=build /tmp/micropython/ports/unix/build-standard/micropython /usr/local/bin/micropython

RUN groupadd --system challenge && \
    useradd --system --gid challenge --no-create-home challenge

COPY flag.txt /flag.txt
COPY server.py /srv/server.py
COPY www /srv/www

RUN chmod 0444 /flag.txt && \
    chmod -R a-w /srv

USER challenge
WORKDIR /srv

EXPOSE 1337
ENTRYPOINT ["micropython", "/srv/server.py"]
