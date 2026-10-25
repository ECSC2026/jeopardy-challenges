# build stage: compile key generator
FROM debian:trixie-slim@sha256:020c0d20b9880058cbe785a9db107156c3c75c2ac944a6aa7ab59f2add76a7bd AS keygen
RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev libssl-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY keygen.c .
RUN gcc -O2 -Wall -Wno-deprecated-declarations -o keygen keygen.c -lcrypto

# run stage: admin bot
FROM node:26-trixie-slim@sha256:715e55e4b84e4bb0ff48e49b398a848f08e55daed8eb6a0ea1839ae53bc57583

ENV NODE_ENV=production \
    CERTS=/certs \
    PLAYWRIGHT_BROWSERS_PATH=/ms-playwright

# certutil (libnss3-tools) for the NSS trust store, libssl3 for the keygen binary
RUN apt-get update && apt-get install -y --no-install-recommends libnss3-tools libssl3 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY package.json package-lock.json ./

RUN PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD=1 npm ci --omit=dev
RUN npx playwright install --with-deps chromium \
 && rm -rf /var/lib/apt/lists/* /root/.npm

COPY --from=keygen /build/keygen /usr/local/bin/keygen
COPY bot.js bot.html entrypoint.sh ./
RUN chmod +x entrypoint.sh

EXPOSE 1337
CMD ["./entrypoint.sh"]
