FROM python:3.14.5-slim-bookworm@sha256:a9bee15510a364124aa24692899d269835683b883de42f7ebec8c293cf679ccb

WORKDIR /app

COPY index.html main.css main.js favicon.ico server.py ./

EXPOSE 8080

CMD ["python", "server.py"]
