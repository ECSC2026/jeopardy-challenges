#!/usr/bin/env sh
set -eu

# documents/flag.txt ships as a placeholder; the real flag arrives in FLAG.
if [ -n "${FLAG:-}" ]; then
    printf '%s\n' "$FLAG" >documents/flag.txt
fi
unset FLAG

exec python3 serve.py --host 0.0.0.0 --port 8080
