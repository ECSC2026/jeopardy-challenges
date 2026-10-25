## VulnVM
* **Category:** Pwn
* **Author:** 0x4d5a

Lets pwn a simple, MonkeyC insprired VM. You can upload your script, which will get compiled and executed on the server.

Please note the examples to understand the language. For local debugging, use `vulnvmc` to compile your source to bytecode, then use `vulnvm` to execute it. We also introduced a heap dump function via `dumpHeap()` so you don't need to dive too deep into the re-using allocator.

Hint: The intended solution uses a vulnerability in one of the build-in methods.

> **Note:** during ECSC, the deployment used an outdated image, which caused it to differ slightly from a local Docker build.
