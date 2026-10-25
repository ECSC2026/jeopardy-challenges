
#pragma once

#include "client/constants.h"
#include "client/packet.h"

#include <bearssl.h>


class GameClient {
public:
    bool connect(const char *ip, int port);

    bool reset_game(Color *color);
    bool solve_level(
        uint8_t *hash, Color *color, char *flag, size_t *flag_size
    );

private:
    bool key_agreement();
    bool key_agreement_request(
        const void *client_pubkey, size_t client_pubkey_size,
        const void *client_nonce, size_t client_nonce_size,
        void *server_pubkey, size_t *server_pubkey_size,
        void *server_nonce, size_t *server_nonce_size,
        void *signature, size_t *signature_size
    );

    bool proxy(const void *input, size_t insize, void *output, size_t *outsize);

    bool send_insecure_request();
    bool send_secure_request();

    Packet insecure_packet;
    Packet secure_packet;

    PacketTransport transport;

    br_hmac_drbg_context rngctx;
    br_hmac_key_context hmackey;

    uint8_t encryption_key[32];
    uint8_t authentication_key[64];
};
