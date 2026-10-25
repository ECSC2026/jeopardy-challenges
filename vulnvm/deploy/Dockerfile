
# ---- build stage ------------------------------------------------------------
FROM debian:bookworm-slim AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends gcc make libc6-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . /src
# Fail early with an actionable message if built with the wrong context (deploy/
# instead of the repo root), rather than the cryptic "No rule to make target 'all'".
RUN make all \
 && gcc -O2 -Wall -Wextra -o build/wrapper wrapper.c

# ---- runtime stage ----------------------------------------------------------
FROM debian:bookworm-slim AS runtime
RUN useradd --create-home --uid 1000 ctf
WORKDIR /challenge
COPY --from=build /src/build/vulnvm  /challenge/vulnvm
COPY --from=build /src/build/vulnvmc  /challenge/vulnvmc
COPY --from=build /src/build/wrapper /challenge/wrapper

COPY flag.txt /flag.txt
# The prize. Replace at deploy time (e.g. --build-arg or a mounted secret).
RUN chmod 644 /flag.txt

USER ctf
EXPOSE 1337
ENV VULNVM_PORT=1337 \
    VULNVM_BIN=/challenge/vulnvm \
    VULNVM_TIMEOUT=120 \
    VULNVM_MAXSIZE=262144
CMD ["/challenge/wrapper"]
