
#pragma once

#include <cstddef>
#include <cstdint>


class MessagePackReader {
public:
    MessagePackReader(const uint8_t *data, size_t size);

    bool u32(uint32_t *value);

    bool raw(void *data, size_t *size);
    bool string(char *data, size_t *size);

    bool map(size_t *count);

private:
    bool read(void *data, size_t size);

    const uint8_t *ptr;
    size_t offset;
    size_t cap;
};


class MessagePackWriter {
public:
    MessagePackWriter(uint8_t *data, size_t size);

    size_t size();

    bool u32(uint32_t value);

    bool raw(const void *data, size_t size);

    bool map(size_t count);

private:
    bool positive_fixint(int value);

    bool uint8(uint8_t value);
    bool uint16(uint16_t value);
    bool uint32(uint32_t value);

    bool bin8(const void *data, size_t size);

    bool fixmap(size_t count);

    uint8_t *ptr;
    size_t offset;
    size_t cap;
};
