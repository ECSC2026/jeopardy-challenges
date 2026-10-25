# $ crane digest debian:bookworm-20260824-slim
# sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171
FROM debian:bookworm-20260824-slim@sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171

ADD --chmod=0755 --checksum=sha256:4c97fd03a3b181996b1473f3a99b69a1efc6ecaf2b4ede061b6bd60a96b9325a \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.0/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && \
    apt-get install -y --no-install-recommends python3 python3-venv python3-venv

WORKDIR /solve

COPY requirements.txt /solve/requirements.txt
RUN python3 -m venv .venv && .venv/bin/python3 -m pip install -r requirements.txt

COPY solve.py /solve/solve.py
COPY expl.mc /solve/expl.mc

ENTRYPOINT [".venv/bin/python3", "/solve/solve.py"]
