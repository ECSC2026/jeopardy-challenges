#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

org=ecsc-2026-codescan
team_slug="codescan-team-$(printf %s "${TEAMID:?}" | tr 'A-Z ' 'a-z-')"
if [[ ! $team_slug =~ ^codescan-team-[a-z0-9]+(-[a-z0-9]+)*$ ]]; then
    echo 'Invalid team ID.' >&2
    exit 1
fi

printf 'Codescan signup service\nTeam ID: %s\nRepository ID: %s\n' \
    "$TEAMID" "$team_slug"

claim_dir=/tmp/codescan-signup-used
if [[ -d $claim_dir ]]; then
    echo 'Signup has already been used for this instance. Recreate it to try again.'
    exit 0
fi

if ! token=$(printf %s "${GITHUB_SIGNUP_TOKEN_B64_REVERSED:?}" | rev | base64 -d); then
    echo 'Invalid signup token.' >&2
    echo 'Signup is unavailable. Contact the organizers.'
    exit 1
fi
if [[ -z $token ]]; then
    echo 'Invalid signup token.' >&2
    echo 'Signup is unavailable. Contact the organizers.'
    exit 1
fi

api_status() {
    local method=$1 path=$2 body=${3-}
    local args=(
        --silent --show-error --max-time 20
        --output /dev/null --write-out '%{http_code}'
        --request "$method"
        --header 'Accept: application/vnd.github+json'
        --header 'X-GitHub-Api-Version: 2026-03-10'
        --header "Authorization: Bearer $token"
    )
    if [[ -n $body ]]; then
        args+=(--header 'Content-Type: application/json' --data "$body")
    fi
    curl "${args[@]}" "https://api.github.com$path"
}

printf 'GitHub username (one submission per instance): '
if ! IFS= read -r -t 60 -n 40 username; then
    printf '\nNo username received. Connect again to try.\n'
    exit 1
fi
username=${username%$'\r'}
printf '\n'

if [[ ${#username} -gt 39 || ! $username =~ ^[A-Za-z0-9]+(-[A-Za-z0-9]+)*$ ]]; then
    echo 'Invalid GitHub username. Connect again to try.'
    exit 1
fi

if ! mkdir "$claim_dir" 2>/dev/null; then
    if [[ -d $claim_dir ]]; then
        echo 'Signup has already been used for this instance. Recreate it to try again.'
    else
        echo 'Signup is unavailable. Contact the organizers.'
    fi
    exit 1
fi

if ! status=$(api_status GET "/orgs/$org/teams/$team_slug"); then
    echo 'GitHub is unavailable. Recreate the instance to try again later.'
    exit 1
fi
if [[ $status != 200 ]]; then
    echo 'Team is not ready. Contact the organizers.'
    exit 1
fi

if ! status=$(api_status GET "/users/$username"); then
    echo 'GitHub is unavailable. Recreate the instance to try again later.'
    exit 1
fi
if [[ $status != 200 ]]; then
    echo 'That GitHub user was not found. Recreate the instance to try again.'
    exit 1
fi

if ! status=$(api_status PUT "/orgs/$org/teams/$team_slug/memberships/$username" '{"role":"member"}'); then
    echo 'The invitation failed. Recreate the instance to try again.'
    exit 1
fi
if [[ $status != 200 ]]; then
    echo "Membership request failed (HTTP $status) for $team_slug / @$username" >&2
    echo 'The invitation failed. Recreate the instance to try again.'
    exit 1
fi

echo "Access requested for @$username. Accept the GitHub organization invitation."
echo "Then clone https://github.com/$org/$team_slug."
