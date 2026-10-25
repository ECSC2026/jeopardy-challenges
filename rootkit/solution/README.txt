This challenge is designed to be rather simple.

How it works:
The initrd contains a rogue kernel module (replaced squashfs.ko).
The module loads on startup and then relocates itself to some module memory
page while scraping any obvious information that the module loaded.
The module will then periodically perform work on the system workqueue:
- Negotiate a key from the "control server"
- Periodically fetch the encrypted flag from the server

The key is not cleared from memory, so player has to scan memory to find it.
Player can then connect to the control server as well to fetch the flag and
decrypt it.

./solve.sh ctf@host:port

Requirements:
    ssh
    scp
    wget
    python3
    sha256sum


