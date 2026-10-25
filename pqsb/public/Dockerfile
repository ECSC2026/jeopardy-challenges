FROM python:3.12-slim

COPY --from=ghcr.io/astral-sh/uv:0.8 /uv /bin/uv

RUN useradd --create-home --shell /usr/sbin/nologin app
WORKDIR /srv

COPY pyproject.toml uv.lock ./
RUN uv sync --locked

COPY app.py serve.py mldsa.py lru.py README.md entrypoint.sh ./
COPY frontend/ ./frontend/
COPY documents/ ./documents/

# The entrypoint rewrites the flag as the service user.
RUN chmod +x entrypoint.sh && chown app:app documents/flag.txt

USER app
EXPOSE 8080

ENV PATH="/srv/.venv/bin:$PATH"

CMD ["./entrypoint.sh"]
