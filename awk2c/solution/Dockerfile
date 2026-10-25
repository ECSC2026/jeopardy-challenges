# syntax=docker/dockerfile:1
FROM python:3.13-slim@sha256:69e18bd8d831d88e0ef70239dc7771ab7c28bc296ae78ac75cde71e60aa4434f

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends binutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /chk
COPY . .
RUN pip install --no-compile --no-cache-dir -r requirements.txt

CMD ["python3", "solve.py"]
