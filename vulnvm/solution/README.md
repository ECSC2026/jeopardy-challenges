# vulnvm

## Exploit

Heap overflow in `toUpper`, expanding the size of `ß` to uppercase `ß`. Spray heap with arrays, create a hole, place string, overflow one heap array length, use corrupted array to overwrite another array to get AAW. Then scan the heap for libc pointer, leak libc and overwrite `strtol` to `system` in the main binary `got`. I had a better writeup somewhere, buts its gone, sorry :D Exploit is pretty self explaining though


