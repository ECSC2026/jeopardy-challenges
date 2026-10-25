## Trezor Forensics
* **Category:** Hardware
* **Author:** hextree.io

While cleaning out the basement I found my old Trezor-based hardware wallet... Unfortunately I lost my PIN, and the backup seed is long gone. Can you recover my wallet?

Note: You can develop the attack using the firmware linked below. Use trezorctl to install it. To get the final secret installed, please visit the hextree.io desk. You can at any point come to us and we will install the correct target firmware & secret.

If you accidentally erase your wallet please come to us and we will re-configure it.

Notes:
- When glitching while USB is connected the target regulator can get very hot
- Bootloader mode: press down both buttons, and reset
- Faultier documentation: https://www.hextree.io/faultier
- If you erase (intentionally or accidentally) the board, come to us and we will fix it.
- DO NOT overwrite the bootloader. We can not recover from this.
- It's normal to get Hardfault messages during regular use as we removed some capacitors
