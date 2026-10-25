FROM alpine:3.23.3@sha256:25109184c71bdad752c8312a8623239686a9a2071e8825f20acb8f2198c3f659

RUN --mount=type=cache,target=/var/cache/apk,sharing=locked \
    apk add --update-cache bash ca-certificates curl socat \
    && adduser -D ctf

COPY --chmod=0555 signup.sh /usr/local/bin/signup.sh

USER ctf
WORKDIR /home/ctf

EXPOSE 1338
ENTRYPOINT ["sh", "-ec", ": \"${TEAMID:?TEAMID is required}\"; : \"${GITHUB_SIGNUP_TOKEN_B64_REVERSED:?signup token is required}\"; exec socat -T120 TCP-LISTEN:1338,fork,reuseaddr EXEC:/usr/local/bin/signup.sh"]