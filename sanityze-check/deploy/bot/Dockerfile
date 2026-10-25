FROM alpine:3.23.3@sha256:25109184c71bdad752c8312a8623239686a9a2071e8825f20acb8f2198c3f659

ENV PUPPETEER_SKIP_DOWNLOAD=true

WORKDIR /usr/app
COPY package.json .
RUN apk add --no-cache nodejs npm chromium su-exec tini && \
    npm install

COPY bot.js .
ENTRYPOINT ["/sbin/tini", "--"]
CMD ["su-exec", "guest", "node", "/usr/app/bot.js"]
