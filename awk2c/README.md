## awk2c
* **Category:** Pwn
* **Author:** LevitatingLion

awk too slow? just compile it!

An awk to C compiler written in Rust, with runtime-support code in C. Send your awk script followed by END, it gets compiled and run, and you can interact with it. Note that default compiler flags in the Ubuntu container include all of the common mitigations.
