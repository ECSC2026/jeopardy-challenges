#!/usr/bin/env python3
import argparse

from waitress import serve

from app import app

KB = 1024
MAX_BODY = 64 * KB
CHANNEL_TIMEOUT = 20
CONNECTION_LIMIT = 200


def main():
    ap = argparse.ArgumentParser(description="PQ Swiss Bank")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8080)
    args = ap.parse_args()

    serve(
        app,
        host=args.host,
        port=args.port,
        threads=1,
        channel_timeout=CHANNEL_TIMEOUT,
        connection_limit=CONNECTION_LIMIT,
        max_request_body_size=MAX_BODY,
        ident=None,
    )


if __name__ == "__main__":
    main()
