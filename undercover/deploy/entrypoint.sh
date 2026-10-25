#!/usr/bin/env bash
set -euo pipefail
export HOME=/root
CERTS=${CERTS:-/certs}
mkdir -p "$CERTS"

# First boot only: generate this instance's own Root CA + leaf cert 
if [ ! -f "$CERTS/ca.crt" ]; then
  echo "[entrypoint] first boot: generating Internal Root CA ..."
  keygen "$CERTS"
fi

# Trust our Root CA in the admin browser
rm -rf "$HOME/.pki/nssdb"
mkdir -p "$HOME/.pki/nssdb"
certutil -d sql:"$HOME/.pki/nssdb" -N --empty-password < /dev/null
certutil -d sql:"$HOME/.pki/nssdb" -A -n internal-ca -t "C,," -i "$CERTS/ca.crt" < /dev/null
echo "[entrypoint] installed Internal Root CA into NSS trust store"

exec node bot.js
