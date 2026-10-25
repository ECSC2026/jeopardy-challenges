#!/usr/bin/env bash

set -eu

echo "$FLAG" > flag.txt
export -n FLAG

socat TCP-LISTEN:1337,fork,reuseaddr EXEC:./wrapper.sh,stderr
