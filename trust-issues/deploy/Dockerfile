
FROM python:3.14-slim@sha256:e06cc1111ed84189e91866447f562b89faadbfbbb9937cd67e6bf4172cdb45df

ADD --chmod=0755 --checksum=sha256:c125df9762b0c7233459087bb840c0e5dbfc4d9690ee227f1ed8994f4d51d2e0 \
    https://raw.githubusercontent.com/reproducible-containers/repro-sources-list.sh/v0.1.4/repro-sources-list.sh \
    /usr/local/bin/repro-sources-list.sh

RUN \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    /usr/local/bin/repro-sources-list.sh && \
    apt-get update && \
    apt-get install -y libnss3

WORKDIR /app
RUN pip install Flask gunicorn
COPY . .

CMD ["gunicorn", "-b", "0.0.0.0:8000", "app:app"]
