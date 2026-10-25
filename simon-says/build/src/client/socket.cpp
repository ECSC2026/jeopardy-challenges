
#include "client/socket.h"

#include "debug.h"

#include <stdexcept>

#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <unistd.h>


Socket::Socket() {
    fd = -1;
}

Socket::~Socket() {
    if (fd != -1) {
        close(fd);
    }
}

bool Socket::initialize(int timeout) {
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
        return false;
    }
    
    DEBUG_PRINT("Setting socket timeout to %i milliseconds", timeout);
    timeval tv = {timeout / 1000, (timeout % 1000) * 1000};

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
        return false;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
        return false;
    }

    return true;
}

bool Socket::connect(const char *ip, int port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    if (inet_aton(ip, &addr.sin_addr) == 0) {
        DEBUG_PRINT("inet_aton() failed");
        return false;
    }
    addr.sin_port = htons(port);

    if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
        DEBUG_PRINT("connect() failed: %s", strerror(errno));
        return false;
    }
    return true;
}

bool Socket::send(const void *data, size_t size) {
    DEBUG_PRINT("Sending %zi bytes through socket", size);

    size_t offset = 0;
    while (offset < size) {
        ssize_t result = ::send(
            fd, (const uint8_t *)data + offset, size - offset, 0
        );
        if (result <= 0) {
            DEBUG_PRINT("send() failed: %s", strerror(errno));
            return false;
        }

        offset += result;
    }
    return true;
}

bool Socket::receive(void *data, size_t size) {
    size_t offset = 0;
    while (offset < size) {
        ssize_t result = ::recv(fd, (uint8_t *)data + offset, size - offset, 0);
        if (result <= 0) {
            if (result < 0) {
                DEBUG_PRINT("recv() failed: %s", strerror(errno));
            }
            else {
                DEBUG_PRINT("Connection was closed");
            }
            return false;
        }

        offset += result;
    }
    return true;
}
