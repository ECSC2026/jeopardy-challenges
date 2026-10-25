## What The Treck
* **Category:** Pwn
* **Author:** localo

The Treck TCP/IP Stack provides network connectivity for embedded and IoT devices like industrial controllers and medical devices. In 2005, they released a demo for the MCF5208EVB, an MCF5208 ColdFire microprocessor devboard by Freescale Semiconductor. Luckily, this board is supported by QEMU! So, after getting the ancient build chain to work, and with some minor patches, it is possible to enjoy this relic in modern times!

The bug is in `txcgi.c`. See `CHALLENGE.md` in the attachments for what's included and where to start.

A haproxy front-end sits between you and the emulated firmware. It's not part of the challenge itself, it just keeps the fragile 2005 HTTP stack from being knocked over by public CVEs. Where would be the fun in just using someone else's vulns, anyway?
