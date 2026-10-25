#!/bin/sh
 
set -e

KVM=""
if [ -w "/dev/kvm" ]; then
    KVM="-enable-kvm"
fi

python3 /server.py &

exec qemu-system-x86_64 -m 256 -smp 2 $KVM \
    -drive file=/rootfs.img,format=raw,index=0,media=disk,if=virtio,readonly=on \
    -nic user,model=virtio-net-pci,hostfwd=tcp::3000-:22 \
    -nographic \
    -monitor none \
    -serial stdio

