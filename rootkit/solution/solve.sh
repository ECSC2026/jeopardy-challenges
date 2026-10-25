#!/bin/sh

# For some reason ssh says it fails when connecting to remote. Why is that?
# (it works anyways)
set -xu

DST="$1"

# After figuring out how the implant works, we just need to scan memory for the
# key to decrypt the flag
# Some random tool that appears to do the job
if [ ! -f lemon.core.x86_64 ]; then
    wget https://github.com/eurecom-s3/lemon/releases/download/v2.0/lemon.core.x86_64
fi
echo "33ef9aab1e4105afd25e3ac6a74c754a7bd9fe3743714f03600434b92213e5c1  lemon.core.x86_64" | sha256sum -c -

ssh_opts="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"

scp -O $ssh_opts lemon.core.x86_64 "scp://$1//tmp/lemon"
echo root | ssh $ssh_opts "ssh://$1" su -c "'chmod +x /tmp/lemon && /tmp/lemon -d /dev/stdout'" > dump
ssh $ssh_opts "ssh://$1" sh -c "'echo flag: | nc -w 1 -u 10.0.2.2 13337'" > flag

python3 -c "$(cat <<EOF
dump = open("dump", "rb").read()
flag = open("flag", "rb").read()
print(flag)

off = -1
while (off := dump.find(b"_key:", off + 1)) >= 0:
    off += 5
    key = dump[off:off + len(flag)]
    print("".join( chr(f ^ k) for f, k in zip(flag, key) ))
EOF
)"

