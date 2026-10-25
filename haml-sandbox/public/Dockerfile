# $ docker pull ruby:3.4-alpine
# sha256:c5a5064d190055633011c03aa800170cc36945ff3afb5f6c915329f92d6f1e00
FROM ruby:3.4-alpine@sha256:c5a5064d190055633011c03aa800170cc36945ff3afb5f6c915329f92d6f1e00

WORKDIR /app

# puma builds a native extension, so the toolchain has to be present for
# `bundle install` -- but only for that step. The lockfile is frozen: the gem
# set moves when Gemfile.lock is regenerated, not when the image is rebuilt.
COPY Gemfile Gemfile.lock ./
RUN apk add --no-cache --virtual .build-deps build-base \
  && bundle config set --local frozen true \
  && bundle install \
  && apk del .build-deps

COPY app.rb index.html ./
COPY flag.txt /flag.txt

# The challenge hands out code execution inside this container, so it must not
# run as root. The flag stays world readable: reading it *is* the objective.
RUN addgroup -S challenge \
  && adduser -S -G challenge -H -s /sbin/nologin challenge \
  && chmod 0444 /flag.txt \
  && chmod -R a-w /app

USER challenge
EXPOSE 31337
ENTRYPOINT ["ruby", "app.rb"]
