
#pragma once

#include "client/socket.h"

#include <cstddef>
#include <cstdint>


#define PACKET_SIZE 4096


class Packet {
public:
    uint8_t data[PACKET_SIZE];
    size_t size;
};


class PacketTransport {
public:
    bool initialize(int timeout);
    bool connect(const char *ip, int port);
    bool send(const Packet *packet);
    bool receive(Packet *packet);

private:
    Socket socket;
};
