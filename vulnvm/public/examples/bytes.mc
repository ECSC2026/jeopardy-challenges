// bytes.mc — ByteArray manipulation and binary number encoding/decoding.

var bytes = [0x48, 0x65, 0x6C, 0x6C, 0x6F]b;   // "Hello" in ASCII
println("Bytes:     " + bytes);
println("Size:      " + bytes.size());
println("First:     " + bytes[0]);

// Decode as a big-endian 16-bit number.
var packet = [0x01, 0x02, 0x03, 0x04]b;
var beValue = packet.decodeNumber(NUMBER_FORMAT_UINT16, {:offset => 0});
var leValue = packet.decodeNumber(NUMBER_FORMAT_UINT16,
                                  {:offset => 0, :endianness => ENDIAN_LITTLE});
println("Big-endian 16-bit:    " + beValue);
println("Little-endian 16-bit: " + leValue);

// Decode a 32-bit value.
var word = packet.decodeNumber(NUMBER_FORMAT_UINT32, {:offset => 0});
println("32-bit big-endian:    " + word);

// Encode a number back into a byte array.
var buffer = [0, 0, 0, 0]b;
buffer.encodeNumber(0xCAFE, NUMBER_FORMAT_UINT16, {:offset => 0});
buffer.encodeNumber(0xBABE, NUMBER_FORMAT_UINT16, {:offset => 2});
println("Encoded buffer:       " + buffer);
println("Read back [0..2):     " + buffer.decodeNumber(NUMBER_FORMAT_UINT16, {:offset => 0}));
println("Read back [2..4):     " + buffer.decodeNumber(NUMBER_FORMAT_UINT16, {:offset => 2}));

// Simple checksum over the bytes.
function checksum(ba) {
    var sum = 0;
    for (var i = 0; i < ba.size(); i += 1) {
        sum = (sum + ba[i]) % 256;
    }
    return sum;
}
println("Checksum of Hello:    " + checksum(bytes));

// Reverse and slice byte arrays.
println("Reversed:             " + packet.reverse());
println("Slice [1..3):         " + packet.slice(1, 3));
