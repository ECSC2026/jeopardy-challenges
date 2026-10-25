# Challenge layout

- `treckdemo-dbg.elf` the same build as the firmware the challenge server boots (cross-compiled with the original 2005ish Metrowerks CodeWarrior ColdFire toolchain), with DWARF debug info for local analysis in your favorite disassembler/debugger, converted from DWARF1 to DWARF4 for your sanity (the conversion is lossy so some tools might behave a bit buggy, but it's good enough imho).
- `TreckDemo5208.zip` original demo code, accessible at https://web.archive.org/web/20061230215344/http://www.treck.com:80/TreckDemo5208.zip. The bug is in `source/txcgi.c`.
- `deploy/` exactly what runs on our CTF-Platform, the haproxy stuff is just to work around some known bugs and to make the setup more consistent between our CTF-Platform and local container
