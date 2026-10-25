
FROM ubuntu:26.04@sha256:53958ec7b67c2c9355df922dd08dbf0360611f8c3cdb656875e81873db9ffdba AS builder

ADD --chmod=0755 --checksum=sha256:c125df9762b0c7233459087bb840c0e5dbfc4d9690ee227f1ed8994f4d51d2e0 \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.4/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && \
    apt-get install -y build-essential gyp ninja-build python3 wget zlib1g-dev

WORKDIR /build
RUN wget https://ftp.mozilla.org/pub/security/nss/releases/NSS_3_125_RTM/src/nss-3.125-with-nspr-4.39.tar.gz
RUN tar -xzf nss-3.125-with-nspr-4.39.tar.gz

WORKDIR /build/nss-3.125/nss
RUN ./build.sh --opt --static --enable-libpkix

WORKDIR /build
COPY client.c Makefile ./

RUN make

FROM scratch
COPY --from=builder /build/client /build/client_debug /
