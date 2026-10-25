# syntax=docker/dockerfile:1
FROM python:3.13-slim@sha256:69e18bd8d831d88e0ef70239dc7771ab7c28bc296ae78ac75cde71e60aa4434f

ADD --chmod=0755 --checksum=sha256:c125df9762b0c7233459087bb840c0e5dbfc4d9690ee227f1ed8994f4d51d2e0 \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.4/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends binutils

WORKDIR /solve
COPY solution/requirements-lock.txt .
RUN pip install --no-compile --no-cache-dir -r requirements-lock.txt

COPY solution/solve.py .
COPY public/libc.so.6 ./

ENTRYPOINT ["python3", "/solve/solve.py"]
