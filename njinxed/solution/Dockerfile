FROM alpine:3.21@sha256:48b0309ca019d89d40f670aa1bc06e426dc0931948452e8491e3d65087abc07d

RUN apk add --no-cache coreutils grep netcat-openbsd openssh-client curl

WORKDIR /chk
COPY --chmod=0755 solve.sh /chk/solve.sh

CMD ["/chk/solve.sh"]
