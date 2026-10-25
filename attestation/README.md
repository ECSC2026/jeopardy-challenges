## Faulty attestation
* **Category:** Hardware
* **Author:** hextree.io

This device creates an attestation value of its serial-number on UART with a baud of 115200. We really want to counterfeit this device, so we need the cryptographic key used in the attestation.

We found a firmware that uses a test-key, maybe that helps you.

While investigating, we also noticed that it wiggles PA14 while it creates the attestation code...

Notes:
- Initially, the board will be flashed with this firmware
- Bootloader mode: press down both buttons, and reset
- Faultier documentation: https://www.hextree.io/faultier
- If you erase (intentionally or accidentally) the board, come to us and we will fix it.
- DO NOT overwrite the bootloader. We can not recover from this.
- When glitching while USB is connected the target regulator can get very hot
