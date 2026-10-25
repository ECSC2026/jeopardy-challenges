#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/objects.h>
#include <openssl/err.h>

#define NUM 32
#define WORD_BYTES 4

static int urandom = -1;

static uint32_t rand_word(void) {
    uint8_t b;
    if (read(urandom, &b, 1) != 1) { perror("read"); _exit(1); }
    return b;
}

static BIGNUM *gen_prime(BN_CTX *ctx) {
    uint8_t be[NUM * WORD_BYTES];
    BIGNUM *p = BN_new();
    for (;;) {
        memset(be, 0, sizeof(be));
        for (int i = 0; i < NUM; i++) {
            uint32_t v = rand_word();
            be[(NUM - 1 - i) * WORD_BYTES + 3] = (uint8_t)(v & 0xff);
            be[(NUM - 1 - i) * WORD_BYTES + 2] = (uint8_t)((v >> 8) & 0xff);
            be[(NUM - 1 - i) * WORD_BYTES + 1] = (uint8_t)((v >> 16) & 0xff);
            be[(NUM - 1 - i) * WORD_BYTES + 0] = (uint8_t)((v >> 24) & 0xff);
        }
        be[(NUM - 1) * WORD_BYTES + 3] |= 0x01;
        be[0 * WORD_BYTES + 3]         |= 0x80;
        BN_bin2bn(be, sizeof(be), p);
        if (BN_check_prime(p, ctx, NULL) == 1) return p;
    }
}

static void die(const char *msg) { fprintf(stderr, "%s\n", msg); ERR_print_errors_fp(stderr); exit(1); }

static void add_ext(X509 *cert, X509 *issuer, int nid, const char *value) {
    X509V3_CTX c;
    X509V3_set_ctx_nodb(&c);
    X509V3_set_ctx(&c, issuer, cert, NULL, NULL, 0);
    X509_EXTENSION *ex = X509V3_EXT_conf_nid(NULL, &c, nid, value);
    if (!ex) die("X509V3_EXT_conf_nid failed");
    X509_add_ext(cert, ex, -1);
    X509_EXTENSION_free(ex);
}

static EVP_PKEY *make_ca_key(void) {
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *e = BN_new(); BN_set_word(e, RSA_F4);
    BIGNUM *p, *q, *n = BN_new(), *phi = BN_new();
    BIGNUM *p1 = BN_new(), *q1 = BN_new(), *g = BN_new();
    BIGNUM *d = BN_new(), *dmp1 = BN_new(), *dmq1 = BN_new(), *iqmp = BN_new();

    for (;;) {
        p = gen_prime(ctx);
        q = gen_prime(ctx);
        if (BN_cmp(p, q) == 0) { BN_free(p); BN_free(q); continue; }
        BN_sub(p1, p, BN_value_one());
        BN_sub(q1, q, BN_value_one());
        BN_mul(phi, p1, q1, ctx);
        BN_gcd(g, e, phi, ctx);
        if (BN_is_one(g)) break;
        BN_free(p); BN_free(q);
    }
    BN_mul(n, p, q, ctx);
    BN_mod_inverse(d, e, phi, ctx);
    BN_mod(dmp1, d, p1, ctx);
    BN_mod(dmq1, d, q1, ctx);
    BN_mod_inverse(iqmp, q, p, ctx);

    RSA *rsa = RSA_new();
    RSA_set0_key(rsa, n, e, d);
    RSA_set0_factors(rsa, p, q);
    RSA_set0_crt_params(rsa, dmp1, dmq1, iqmp);

    EVP_PKEY *pkey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(pkey, rsa);

    fprintf(stderr, "[keygen] CA modulus is %d bits\n", BN_num_bits(n));
    BN_free(phi); BN_free(p1); BN_free(q1); BN_free(g);
    BN_CTX_free(ctx);
    return pkey;
}

static void set_validity(X509 *c, long secs) {
    X509_gmtime_adj(X509_getm_notBefore(c), 0);
    X509_gmtime_adj(X509_getm_notAfter(c), secs);
}

static void write_pem(const char *dir, const char *name, X509 *cert, EVP_PKEY *key) {
    char path[512];
    if (cert) {
        snprintf(path, sizeof(path), "%s/%s.crt", dir, name);
        FILE *f = fopen(path, "wb"); if (!f) die("open crt"); PEM_write_X509(f, cert); fclose(f);
    }
    if (key) {
        snprintf(path, sizeof(path), "%s/%s.key", dir, name);
        FILE *f = fopen(path, "wb"); if (!f) die("open key");
        PEM_write_PrivateKey(f, key, NULL, NULL, 0, NULL, NULL); fclose(f);
    }
}

int main(int argc, char **argv) {
    const char *outdir = (argc > 1) ? argv[1] : ".";
    urandom = open("/dev/urandom", O_RDONLY);
    if (urandom < 0) { perror("urandom"); return 1; }


    // Generate CA certificate
    EVP_PKEY *ca_key = make_ca_key();
    X509 *ca = X509_new();
    X509_set_version(ca, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(ca), 1);
    set_validity(ca, 3650L * 24 * 3600);
    X509_NAME *ca_name = X509_get_subject_name(ca);
    X509_NAME_add_entry_by_txt(ca_name, "CN", MBSTRING_ASC,
                               (unsigned char *)"Internal Root CA", -1, -1, 0);
    X509_set_issuer_name(ca, ca_name);
    X509_set_pubkey(ca, ca_key);
    add_ext(ca, ca, NID_basic_constraints, "critical,CA:TRUE");
    add_ext(ca, ca, NID_key_usage, "critical,keyCertSign,cRLSign");
    add_ext(ca, ca, NID_subject_key_identifier, "hash");
    if (!X509_sign(ca, ca_key, EVP_sha256())) die("sign CA");


    // Generate leaf certificate
    EVP_PKEY *leaf_key = EVP_EC_gen("P-256");
    if (!leaf_key) die("EVP_EC_gen");
    X509 *leaf = X509_new();
    X509_set_version(leaf, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(leaf), 0x1000);
    set_validity(leaf, 89L * 24 * 3600);
    X509_NAME *lname = X509_get_subject_name(leaf);
    X509_NAME_add_entry_by_txt(lname, "CN", MBSTRING_ASC,
                               (unsigned char *)"safehouse.internal", -1, -1, 0);
    X509_set_issuer_name(leaf, X509_get_subject_name(ca));
    X509_set_pubkey(leaf, leaf_key);
    add_ext(leaf, ca, NID_basic_constraints, "critical,CA:FALSE");
    add_ext(leaf, ca, NID_key_usage, "critical,digitalSignature");
    add_ext(leaf, ca, NID_ext_key_usage, "serverAuth");
    add_ext(leaf, ca, NID_subject_alt_name, "DNS:safehouse.internal");
    ASN1_OBJECT *obj = OBJ_txt2obj("1.3.6.1.4.1.11129.2.1.22", 1);
    if (!obj) die("OBJ_txt2obj failed");
    unsigned char v[2] = { 0x05, 0x00 };
    ASN1_OCTET_STRING *os = ASN1_OCTET_STRING_new();
    ASN1_OCTET_STRING_set(os, v, sizeof(v));
    X509_EXTENSION *ex = X509_EXTENSION_create_by_OBJ(NULL, obj, 0, os);
    if (!ex) die("X509_EXTENSION_create_by_OBJ failed");
    X509_add_ext(leaf, ex, -1);
    if (!X509_sign(leaf, ca_key, EVP_sha256())) die("sign leaf");

    write_pem(outdir, "ca", ca, ca_key);
    write_pem(outdir, "signer", leaf, leaf_key);
    fprintf(stderr, "[keygen] wrote ca.{crt,key} signer.{crt,key} to %s\n", outdir);

    X509_free(ca); X509_free(leaf);
    EVP_PKEY_free(ca_key); EVP_PKEY_free(leaf_key);
    close(urandom);
    return 0;
}
