#!/usr/bin/env sh
echo "$FLAG" > /flag

haproxy -f /home/ctf/haproxy.cfg -db > /home/ctf/haproxy.log 2>&1 &

qemu-system-m68k \
  -M mcf5208evb -nographic \
  -kernel /home/ctf/treckdemo.elf \
  -monitor none \
  -nic user,hostfwd=tcp:127.0.0.1:8080-:80 \
  -serial mon:stdio \
  -device loader,file=/flag,addr=0x80003000,force-raw=on 2>&1 | tee /home/ctf/qemu.log
