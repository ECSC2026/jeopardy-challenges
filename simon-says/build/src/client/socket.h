
#pragma once

#include <cstddef>


class Socket {
public:
    Socket();
    ~Socket();

    bool initialize(int timeout);
    bool connect(const char *ip, int port);
    bool send(const void *data, size_t size);
    bool receive(void *data, size_t size);

private:
    int fd;
};
