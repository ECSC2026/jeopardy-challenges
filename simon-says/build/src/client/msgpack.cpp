
#include "client/msgpack.h"

#include <cstring>


MessagePackReader::MessagePackReader(const uint8_t *data, size_t size) {
    this->ptr = data;
    this->offset = 0;
    this->cap = size;
}

bool MessagePackReader::read(void *data, size_t size) {
    if (cap - offset < size) {
        return false;
    }

    memcpy(data, ptr + offset, size);
    offset += size;
    return true;
}

bool MessagePackReader::u32(uint32_t *value) {
    uint8_t type;
    if (!read(&type, 1)) {
        return false;
    }

    if (type <= 0x7F) {
        *value = type;
        return true;
    }

    *value = 0;
    if (type == 0xCC) return read(value, 1);
    else if (type == 0xCD) {
        uint8_t bytes[2];
        if (!read(bytes, 2)) {
            return false;
        }

        *value = (bytes[0] << 8) | bytes[1];
        return true;
    }
    else if (type == 0xCE) {
        uint8_t bytes[4];
        if (!read(bytes, 4)) {
            return false;
        }

        *value = (bytes[0] << 24) | (bytes[1] << 16) | 
            (bytes[2] << 8) | bytes[3];
        return true;
    }
    return false;
}

bool MessagePackReader::raw(void *data, size_t *size) {
    uint8_t type;
    if (!read(&type, 1)) {
        return false;
    }

    size_t received_size = 0;
    if (type == 0xC4) {
        if (!read(&received_size, 1)) {
            return false;
        }

        if (received_size > *size) {
            return false;
        }

        *size = received_size;
        return read(data, received_size);
    }
    return false;
}

bool MessagePackReader::string(char *data, size_t *size) {
    uint8_t type;
    if (!read(&type, 1)) {
        return false;
    }

    size_t received_size = 0;
    if (type >= 0xA0 && type <= 0xBF) {
        received_size = type - 0xA0;
    }
    else if (type == 0xD9) {
        if (!read(&received_size, 1)) {
            return false;
        }
    }
    else {
        return false;
    }

    if (received_size + 1 > *size) {
        return false;
    }

    if (!read(data, received_size)) {
        return false;
    }

    data[received_size] = 0;

    *size = received_size;
    return true;
}

bool MessagePackReader::map(size_t *count) {
    uint8_t type;
    if (!read(&type, 1)) {
        return false;
    }

    if (type >= 0x80 && type <= 0x8F) {
        *count = type - 0x80;
        return true;
    }

    return false;
}


MessagePackWriter::MessagePackWriter(uint8_t *data, size_t size) {
    this->ptr = data;
    this->offset = 0;
    this->cap = size;
}

size_t MessagePackWriter::size() {
    return offset;
}

bool MessagePackWriter::u32(uint32_t value) {
    if (value < 0x80) {
        return positive_fixint(value);
    }
    if (value < 0x100) {
        return uint8(value);
    }
    if (value < 0x10000) {
        return uint16(value);
    }
    return uint32(value);
}

bool MessagePackWriter::raw(const void *data, size_t size) {
    if (size < 0x100) {
        return bin8(data, size);
    }
    return false;
}

bool MessagePackWriter::map(size_t count) {
    if (count < 16) {
        return fixmap(count);
    }
    return false;
}

bool MessagePackWriter::positive_fixint(int value) {
    if (cap - offset < 1) {
        return false;
    }

    ptr[offset++] = value;
    return true;
}

bool MessagePackWriter::uint8(uint8_t value) {
    if (cap - offset < 2) {
        return false;
    }

    ptr[offset++] = 0xCC;
    ptr[offset++] = value;
    return true;
}

bool MessagePackWriter::uint16(uint16_t value) {
    if (cap - offset < 3) {
        return false;
    }

    ptr[offset++] = 0xCD;
    ptr[offset++] = value >> 8;
    ptr[offset++] = value & 0xFF;
    return true;
}

bool MessagePackWriter::uint32(uint32_t value) {
    if (cap - offset < 5) {
        return false;
    }

    ptr[offset++] = 0xCE;
    ptr[offset++] = value >> 24;
    ptr[offset++] = (value >> 16) & 0xFF;
    ptr[offset++] = (value >> 8) & 0xFF;
    ptr[offset++] = value & 0xFF;
    return true;
}

bool MessagePackWriter::bin8(const void *data, size_t size) {
    if (cap - offset < size + 2) {
        return false;
    }

    ptr[offset] = 0xC4;
    ptr[offset + 1] = size;
    memcpy(ptr + offset + 2, data, size);
    offset += size + 2;
    return true;
}

bool MessagePackWriter::fixmap(size_t count) {
    if (cap - offset < 1) {
        return false;
    }

    ptr[offset++] = 0x80 + count;
    return true;
}
