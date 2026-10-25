
#include "client/packet.h"

#include <arpa/inet.h>

#include <cstdint>
#include <cstdlib>


bool PacketTransport::initialize(int timeout) {
    return socket.initialize(timeout);
}

bool PacketTransport::connect(const char *ip, int port) {
    return socket.connect(ip, port);
}

bool PacketTransport::send(const Packet *packet) {
    uint32_t size = htonl(packet->size);
    if (!socket.send(&size, 4)) return false;
    return socket.send(packet->data, packet->size);
}

bool PacketTransport::receive(Packet *packet) {
    uint32_t size;
    if (!socket.receive(&size, 4)) {
        return false;
    }

    size = ntohl(size);

    if (size > PACKET_SIZE) {
        return false;
    }

    if (!socket.receive(packet->data, size)) {
        return false;
    }

    packet->size = size;
    return true;
}
