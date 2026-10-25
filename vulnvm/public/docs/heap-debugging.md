# Heap debugging built-ins

Two global built-in functions expose the allocator/object state to a running
MonkeyC program. They exist to make heap-corruption challenges tractable: you can
watch grooming, freeing and overflows land in real time, without a debugger.

## `dumpHeap([label])`

Prints the whole arena to stdout in physical (address) order — every allocator
block with its offset (from the heap base), absolute address, block size and
`USED`/`free` state, annotated with what lives there:

- **live objects** — `Type#id rc=N` plus a preview (a String's text, an
  Array/ByteArray/Dict's `count`/`cap`, a ByteArray's first bytes in hex, a
  Class/Instance/Module/Function's name);
- **owned buffers** — a block that backs an object is labelled with its owner,
  e.g. `bytes of ByteArray#74`, `items of Array#73`, `entries of Dict#38`,
  `code of Fn#24`;
- **forged / corrupted objects** — an object-sized `USED` block that is *not* a
  registered object but whose header looks like one (valid tag byte) is flagged
  `?Type UNREGISTERED (hdr.id=… rc=…) <<< forged/UAF/corrupt?`, printing the
  tell-tale fields (a huge `count`, an absurd id/refcount). This is exactly what
  a heap overflow that fabricates a fake object produces;
- everything else (VM stack, globals, intern/symbol tables) — `<runtime/buffer>`.

The optional `label` argument tags the banner so successive dumps are easy to
tell apart. The dump is **read-only and allocation-free** — calling it does not
perturb the heap it reports — and the block walk is bounded, so it survives (and
reveals) a corrupted heap instead of crashing.


## `heapOk()`

Returns a Bool: `true` when the allocator metadata is self-consistent (boundary
tags match, no un-coalesced adjacent free blocks, free lists agree with the
implicit block list), `false` otherwise. Cheap enough to sprinkle between steps
to pinpoint exactly when corruption lands — and to check that an overflow kept
the heap "valid enough" to keep running.

```monkeyc
println("before: " + heapOk());   // true
// ... grooming / overflow ...
println("after:  " + heapOk());
dumpHeap("post-overflow");
```

Both are implemented in `src/debug.c` (`heap_dump`) over the allocator's
`alloc_foreach_block` / `alloc_arena_base` and the object manager's
`objmgr_foreach`, and registered as natives in `vm_new` (`src/vm.c`).
