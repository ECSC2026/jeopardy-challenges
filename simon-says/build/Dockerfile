FROM ubuntu:22.04@sha256:b8b6ee6aa931ecd9d0d952abc34dc0e5f7c6a30c6bb71b079fe399fde0329c02 AS builder

ADD --chmod=0755 --checksum=sha256:c125df9762b0c7233459087bb840c0e5dbfc4d9690ee227f1ed8994f4d51d2e0 \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.4/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && \
    apt-get install -y build-essential cmake git golang libbearssl-dev \
        libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev python3

WORKDIR /build
RUN git clone https://github.com/DeNA/DeClang

WORKDIR /build/DeClang
RUN git checkout 62389ff192c43e418ece322a4bcd7fc186d17f99

# This fixes compilation errors
COPY declang.patch .
RUN git apply declang.patch

WORKDIR /build/DeClang/script
RUN bash build.sh

WORKDIR /build
RUN mkdir .DeClang
RUN ln -s /build/DeClang/build .DeClang/compiler
COPY config.json .DeClang/config.json

COPY Makefile Makefile
COPY src src

RUN make main
RUN make main_debug

FROM scratch
COPY --from=builder /build/main /build/main_debug /
