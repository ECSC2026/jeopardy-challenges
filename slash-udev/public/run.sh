#!/bin/sh
# Creates /udev containing symlinks to every file and directory in /dev
 
set -e
# It's ugly, but because /dev is mounted dynamically on startup this can't be baked into the image
chown ctf:ctf /dev/null
chown ctf:ctf /dev

/usr/sbin/sshd -D