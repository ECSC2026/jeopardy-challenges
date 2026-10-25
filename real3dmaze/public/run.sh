#!/bin/sh

# Qemu download and install instructions: https://www.qemu.org/download/ (Click on the tab of the operation system you are using)
# On Windows, WSL2 will also work but you may need to setup X11 forwarding.
qemu-system-i386 -drive format=raw,file=real3dmaze.img,if=ide -boot c -no-reboot -no-shutdown
