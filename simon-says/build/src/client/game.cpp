
#include "client/game.h"
#include "client/msgpack.h"

#include "debug.h"

#include <bearssl.h>


const uint8_t SignaturePubkeyData[] = {
    0x04,
    0xB0, 0x76, 0xC7, 0xBA, 0x9B, 0x9C, 0x82, 0xA9, 0x09, 0xAD, 0x44, 0x9B,
    0x3F, 0xC3, 0x01, 0x4E, 0xB3, 0x45, 0x9D, 0xE9, 0x0A, 0xCD, 0x57, 0x1F,
    0x83, 0x7A, 0x1B, 0x64, 0x88, 0xD1, 0x6E, 0xA5,
    0x19, 0x0B, 0x9C, 0x87, 0x16, 0xBF, 0x59, 0x9C, 0x3C, 0xC2, 0x4C, 0xA6,
    0x05, 0x5A, 0x67, 0x2B, 0x81, 0xC2, 0xFF, 0x5C, 0xAF, 0xD1, 0x52, 0x2D,
    0x4F, 0x57, 0x7A, 0x46, 0x98, 0x3F, 0x5C, 0x75
};


bool GameClient::connect(const char *ip, int port) {
    br_prng_seeder seeder = br_prng_seeder_system(NULL);
    if (!seeder) {
        return false;
    }

    br_hmac_drbg_init(&rngctx, &br_sha256_vtable, NULL, 0);
    if (!seeder(&rngctx.vtable)) {
        return false;
    }

    if (!transport.initialize(5000)) {
        return false;
    }

    if (!transport.connect(ip, port)) {
        return false;
    }

    return key_agreement();
}

bool GameClient::reset_game(Color *color) {
    MessagePackWriter writer(secure_packet.data, sizeof(secure_packet.data));
    if (!writer.map(1)) return false;
    if (!writer.u32(RequestField_MethodID)) return false;
    if (!writer.u32(SecureMethodID_ResetGame)) return false;
    secure_packet.size = writer.size();

    if (!send_secure_request()) {
        DEBUG_PRINT("Reset game request failed internally");
        return false;
    }

    MessagePackReader reader(secure_packet.data, secure_packet.size);

    size_t count;
    if (!reader.map(&count)) {
        DEBUG_PRINT("Failed to read map size from reset game response");
        return false;
    }

    bool success = false;
    bool has_color = false;
    for (size_t i = 0; i < count; i++) {
        uint32_t type;
        if (!reader.u32(&type)) {
            DEBUG_PRINT("Failed to read map key from reset game response");
            return false;
        }

        if (type == ResponseField_Result) {
            uint32_t result;
            if (!reader.u32(&result)) {
                DEBUG_PRINT("Failed to read result from reset game response");
                return false;
            }

            if (result != ResultCode_Success) {
                DEBUG_PRINT("Reset game response contains error code");
                return false;
            }

            success = true;
        }
        else if (type == ResetGameResponseField_Color) {
            has_color = true;
            if (!reader.u32((uint32_t *)color)) {
                DEBUG_PRINT("Failed to read color from reset game response");
                return false;
            }
        }
    }
    return success && has_color;
}

bool GameClient::solve_level(
    uint8_t *hash, Color *color, char *flag, size_t *flag_size
) {
    MessagePackWriter writer(secure_packet.data, sizeof(secure_packet.data));
    if (!writer.map(2)) return false;
    if (!writer.u32(RequestField_MethodID)) return false;
    if (!writer.u32(SecureMethodID_SolveLevel)) return false;
    if (!writer.u32(SolveLevelRequestField_Hash)) return false;
    if (!writer.raw(hash, 64)) return false;
    secure_packet.size = writer.size();

    if (!send_secure_request()) {
        return false;
    }

    MessagePackReader reader(secure_packet.data, secure_packet.size);

    size_t count;
    if (!reader.map(&count)) {
        return false;
    }

    bool success = false;
    bool has_color = false;
    bool has_flag = false;
    for (size_t i = 0; i < count; i++) {
        uint32_t type;
        if (!reader.u32(&type)) {
            return false;
        }

        if (type == ResponseField_Result) {
            uint32_t result;
            if (!reader.u32(&result)) {
                return false;
            }

            if (result != ResultCode_Success) {
                return false;
            }

            success = true;
        }
        else if (type == SolveLevelResponseField_Color) {
            has_color = true;
            if (!reader.u32((uint32_t *)color)) {
                return false;
            }
        }
        else if (type == SolveLevelResponseField_Flag) {
            has_flag = true;
            if (!reader.string(flag, flag_size)) {
                return false;
            }
        }
    }

    if (!has_flag) {
        *flag_size = 0;
    }

    return success && has_color;
}

bool GameClient::key_agreement() {
    br_ec_private_key client_privkey;
    br_ec_public_key client_pubkey;
    br_ec_public_key server_pubkey;

    uint8_t client_privkey_buf[BR_EC_KBUF_PRIV_MAX_SIZE];
    uint8_t client_pubkey_buf[BR_EC_KBUF_PUB_MAX_SIZE];
    uint8_t server_pubkey_buf[BR_EC_KBUF_PUB_MAX_SIZE];

    size_t client_privkey_size;
    size_t client_pubkey_size;

    uint8_t client_nonce[64];
    uint8_t server_nonce[32];

    uint8_t signature[64];

    br_hmac_drbg_generate(&rngctx, client_nonce, sizeof(client_nonce));

    const br_ec_impl *impl = br_ec_get_default();

    client_privkey_size = br_ec_keygen(
        &rngctx.vtable, impl, &client_privkey,
        client_privkey_buf, BR_EC_secp256r1
    );
    if (!client_privkey_size) {
        return false;
    }

    client_pubkey_size = br_ec_compute_pub(
        impl, &client_pubkey, client_pubkey_buf, &client_privkey
    );
    if (!client_pubkey_size) {
        return false;
    }

    size_t server_pubkey_size = sizeof(server_pubkey_buf);
    size_t server_nonce_size = sizeof(server_nonce);
    size_t signature_size = sizeof(signature);
    if (!key_agreement_request(
        client_pubkey_buf, client_pubkey_size,
        client_nonce, sizeof(client_nonce),
        server_pubkey_buf, &server_pubkey_size,
        server_nonce, &server_nonce_size,
        signature, &signature_size
    )) {
        return false;
    }

    if (server_pubkey_size != 65 || server_pubkey_buf[0] != 4) {
        return false;
    }

    if (server_nonce_size != 32) {
        return false;
    }

    uint8_t hash[64];
    br_sha512_context sha512;
    br_sha512_init(&sha512);
    br_sha512_update(&sha512, client_pubkey_buf, client_pubkey_size);
    br_sha512_update(&sha512, server_pubkey_buf, server_pubkey_size);
    br_sha512_update(&sha512, client_nonce, sizeof(client_nonce));
    br_sha512_update(&sha512, server_nonce, sizeof(server_nonce));
    br_sha512_out(&sha512, hash);

    uint8_t signature_pubkey_buf[sizeof(SignaturePubkeyData)];
    memcpy(
        signature_pubkey_buf, SignaturePubkeyData, sizeof(SignaturePubkeyData)
    );

    br_ec_public_key signature_pubkey;
    signature_pubkey.curve = BR_EC_secp256r1;
    signature_pubkey.q = signature_pubkey_buf;
    signature_pubkey.qlen = sizeof(signature_pubkey_buf);

    br_ecdsa_vrfy verifier = br_ecdsa_vrfy_raw_get_default();
    if (!verifier(
        impl, hash, sizeof(hash), &signature_pubkey,
        signature, sizeof(signature)
    )) {
        return false;
    }

    server_pubkey.curve = BR_EC_secp256r1;
    server_pubkey.q = server_pubkey_buf;
    server_pubkey.qlen = server_pubkey_size;

    if (!impl->mul(
        server_pubkey_buf, server_pubkey_size,
        client_privkey.x, client_privkey.xlen,
        BR_EC_secp256r1
    )) {
        return false;
    }

    br_hkdf_context hkdf;
    br_hkdf_init(&hkdf, &br_sha512_vtable, server_nonce, server_nonce_size);
    br_hkdf_inject(&hkdf, server_pubkey_buf + 1, 32);
    br_hkdf_flip(&hkdf);
    br_hkdf_produce(&hkdf, NULL, 0, encryption_key, sizeof(encryption_key));
    br_hkdf_produce(
        &hkdf, NULL, 0, authentication_key, sizeof(authentication_key)
    );

    br_hmac_key_init(
        &hmackey, &br_sha512_vtable,
        authentication_key, sizeof(authentication_key)
    );
    return true;
}

bool GameClient::key_agreement_request(
    const void *client_pubkey, size_t client_pubkey_size,
    const void *client_nonce, size_t client_nonce_size,
    void *server_pubkey, size_t *server_pubkey_size,
    void *server_nonce, size_t *server_nonce_size,
    void *signature, size_t *signature_size
) {
    DEBUG_PRINT("Sending key agreement request...");
    MessagePackWriter writer(
        insecure_packet.data,
        sizeof(insecure_packet.data)
    );
    if (!writer.map(3)) return false;
    if (!writer.u32(RequestField_MethodID)) return false;
    if (!writer.u32(InsecureMethodID_KeyAgreement)) return false;
    if (!writer.u32(KeyAgreementRequestField_ClientPublicKey)) return false;
    if (!writer.raw(client_pubkey, client_pubkey_size)) return false;
    if (!writer.u32(KeyAgreementRequestField_ClientNonce)) return false;
    if (!writer.raw(client_nonce, client_nonce_size)) return false;
    insecure_packet.size = writer.size();

    if (!send_insecure_request()) {
        return false;
    }

    MessagePackReader reader(insecure_packet.data, insecure_packet.size);

    size_t count;
    if (!reader.map(&count)) {
        return false;
    }

    bool success = false;
    bool has_server_pubkey = false;
    bool has_server_nonce = false;
    bool has_signature = false;
    for (size_t i = 0; i < count; i++) {
        uint32_t type;
        if (!reader.u32(&type)) {
            return false;
        }

        if (type == ResponseField_Result) {
            uint32_t result;
            if (!reader.u32(&result)) {
                return false;
            }

            if (result != ResultCode_Success) {
                DEBUG_PRINT(
                    "Key agreement request failed with error 0x%08X", result
                );
                return false;
            }

            success = true;
        }
        else if (type == KeyAgreementResponseField_ServerPublicKey) {
            has_server_pubkey = true;
            if (!reader.raw(server_pubkey, server_pubkey_size)) {
                return false;
            }
        }
        else if (type == KeyAgreementResponseField_ServerNonce) {
            has_server_nonce = true;
            if (!reader.raw(server_nonce, server_nonce_size)) {
                return false;
            }
        }
        else if (type == KeyAgreementResponseField_Signature) {
            has_signature = true;
            if (!reader.raw(signature, signature_size)) {
                return false;
            }
        }
    }
    return success && has_server_pubkey && has_server_nonce && has_signature;
}

bool GameClient::proxy(
    const void *input, size_t insize, void *output, size_t *outsize
) {
    MessagePackWriter writer(
        insecure_packet.data,
        sizeof(insecure_packet.data)
    );
    if (!writer.map(2)) return false;
    if (!writer.u32(RequestField_MethodID)) return false;
    if (!writer.u32(InsecureMethodID_Proxy)) return false;
    if (!writer.u32(ProxyRequestField_Payload)) return false;
    if (!writer.raw(input, insize)) return false;
    insecure_packet.size = writer.size();

    if (!send_insecure_request()) {
        DEBUG_PRINT("Proxy request failed internally");
        return false;
    }

    MessagePackReader reader(insecure_packet.data, insecure_packet.size);

    size_t count;
    if (!reader.map(&count)) {
        DEBUG_PRINT("Failed to read map size from proxy response");
        return false;
    }

    DEBUG_PRINT("Proxy response contains %zi elements", count);

    bool success = false;
    bool has_payload = false;
    for (size_t i = 0; i < count; i++) {
        uint32_t type;
        if (!reader.u32(&type)) {
            DEBUG_PRINT("Failed to read map key from proxy response");
            return false;
        }

        if (type == ResponseField_Result) {
            uint32_t result;
            if (!reader.u32(&result)) {
                DEBUG_PRINT("Failed to read result from proxy response");
                return false;
            }

            if (result != ResultCode_Success) {
                DEBUG_PRINT("Proxy response contains an error code");
                return false;
            }

            success = true;
        }
        else if (type == ProxyResponseField_Payload) {
            has_payload = true;
            if (!reader.raw(output, outsize)) {
                DEBUG_PRINT("Failed to read payload from proxy response");
                return false;
            }

            DEBUG_PRINT("Proxy response payload contains %zi bytes", *outsize);
        }
        else {
            DEBUG_PRINT("Proxy response contains unknown field: 0x%08X", type);
        }
    }
    return success && has_payload;
}

bool GameClient::send_insecure_request() {
    if (!transport.send(&insecure_packet)) {
        DEBUG_PRINT("Failed to transmit RPC request");
        return false;
    }

    if (!transport.receive(&insecure_packet)) {
        DEBUG_PRINT("Receiving RPC response failed");
        return false;
    }

    DEBUG_PRINT("Received an RPC response");
    return true;
}

bool GameClient::send_secure_request() {
    size_t padding_size = 16 - secure_packet.size % 16;
    size_t padded_size = secure_packet.size + padding_size;
    size_t total_size = padded_size + 80;

    if (total_size > sizeof(secure_packet.data)) {
        return false;
    }

    memset(secure_packet.data + secure_packet.size, padding_size, padding_size);

    uint8_t *iv = secure_packet.data + padded_size;
    uint8_t *mac = iv + 16;

    br_hmac_drbg_generate(&rngctx, iv, 16);

    uint8_t iv_temp[16];
    memcpy(iv_temp, iv, 16);

    br_aes_ct_cbcenc_keys enc;
    br_aes_ct_cbcenc_init(&enc, encryption_key, sizeof(encryption_key));
    br_aes_ct_cbcenc_run(&enc, iv_temp, secure_packet.data, padded_size);

    br_hmac_context hmac;
    br_hmac_init(&hmac, &hmackey, 0);
    br_hmac_update(&hmac, secure_packet.data, padded_size + 16);
    br_hmac_out(&hmac, secure_packet.data + padded_size + 16);

    size_t response_size = sizeof(secure_packet.data);
    if (!proxy(
        secure_packet.data, total_size,
        secure_packet.data, &response_size)
    ) {
        return false;
    }

    if (response_size < 96) {
        DEBUG_PRINT("Secure response is too short");
        return false;
    }

    uint64_t expected_mac[64];

    br_hmac_init(&hmac, &hmackey, 0);
    br_hmac_update(&hmac, secure_packet.data, response_size - 64);
    br_hmac_out(&hmac, expected_mac);

    if (memcmp(expected_mac, secure_packet.data + response_size - 64, 64)) {
        DEBUG_PRINT("HMAC check failed");
        return false;
    }

    br_aes_ct_cbcdec_keys dec;
    br_aes_ct_cbcdec_init(&dec, encryption_key, sizeof(encryption_key));
    br_aes_ct_cbcdec_run(
        &dec, secure_packet.data + response_size - 80,
        secure_packet.data, response_size - 80
    );

    padding_size = secure_packet.data[response_size - 81];
    if (padding_size > 16) {
        DEBUG_PRINT("Padding is invalid");
        return false;
    }

    secure_packet.size = response_size - 80 - padding_size;
    return true;
}
