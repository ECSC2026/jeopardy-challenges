#!/usr/bin/env bash

set -euo pipefail

repository_id="${TEAMID,,}"
repository_id="${repository_id// /-}"

printf 'Team ID: %s\nRepository ID: codescan-team-%s\n' \
    "$TEAMID" "$repository_id"

if (( $# == 0 )); then
    printf 'Commit SHA: '
    IFS= read -r commit
elif (( $# == 1 )); then
    commit="$1"
else
    echo "Usage: $0 [commit SHA]" >&2
    exit 2
fi

if [[ ! "$commit" =~ ^[0-9a-f]{40}$ ]]; then
    echo "Expected a full, lowercase commit SHA." >&2
    exit 2
fi

workdir="$(mktemp -d)"
trap 'rm -rf -- "$workdir"' EXIT

GIT_TERMINAL_PROMPT=0 git -c advice.detachedHead=false \
    clone --quiet --revision="$commit" \
    "https://github.com/ecsc-2026-codescan/codescan-team-${repository_id}.git" \
    "$workdir"
cd "$workdir"
svelte-check
