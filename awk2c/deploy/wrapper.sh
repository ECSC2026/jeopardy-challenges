#!/usr/bin/env bash

set -eu

tmp="$(mktemp -d)"
trap "rm -rf $tmp" EXIT

# Read awk code
sed '/^END$/Q' > "$tmp/exp.awk"
# Compile to C
./awk2c "$tmp/exp.awk" > "$tmp/exp.c"
# Compile to binary
gcc -std=gnu2x -O2 -Iruntime -Lruntime -o "$tmp/exp" "$tmp/exp.c" -lawk_runtime -lm
# Run binary
"$tmp/exp"
