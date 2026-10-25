FROM alpine:3.23.3@sha256:25109184c71bdad752c8312a8623239686a9a2071e8825f20acb8f2198c3f659

RUN --mount=type=cache,target=/var/cache/apk,sharing=locked \
    --mount=type=cache,target=/root/.npm,sharing=locked \
    apk add --update-cache bash ca-certificates git nodejs npm socat \
    && npm install --global --ignore-scripts --no-audit --no-fund \
        svelte-check@4.7.6 \
        svelte@5.56.10 \
        typescript@5.9.2 \
    && git config --system credential.https://github.com.username x-access-token \
    && git config --system credential.https://github.com.helper \
        '!f() { if [ "$1" = get ] && [ -n "$GITHUB_TOKEN_B64_REVERSED" ]; then printf "password=%s\n" "$(printf %s "$GITHUB_TOKEN_B64_REVERSED" | rev | base64 -d)"; fi; }; f' \
    && adduser -D ctf

COPY --chmod=0555 run.sh /usr/local/bin/run.sh

USER ctf
WORKDIR /home/ctf

ENV FLAG="ECSC{fake_flag}"

EXPOSE 1337
ENTRYPOINT ["socat", "-T120", "TCP-LISTEN:1337,fork,reuseaddr", "EXEC:/usr/local/bin/run.sh,stderr"]
