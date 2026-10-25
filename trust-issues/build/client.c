
#include <nspr.h>
#include <nss/nss.h>
#include <nss/ssl.h>
#include <stdio.h>
#include <string.h>


#ifdef DEBUG
void failure(const char *message) {
    PRErrorCode error = PR_GetError();
    const char *name = PR_ErrorToName(error);
    const char *string = PR_ErrorToString(error, PR_LANGUAGE_I_DEFAULT);
    fprintf(stderr, "%s: %s (%s)\n", message, name, string);
}
#else
#define failure(...)
#endif


SECStatus auth_certificate(void *arg, PRFileDesc *fd, PRBool checkSig, PRBool isServer) {
    const char *url = SSL_RevealURL(fd);
    if (!url) {
        failure("SSL_RevealURL failed");
        return SECFailure;
    }

    CERTCertificate *cert = SSL_PeerCertificate(fd);
    if (!cert) {
        failure("SSL_PeerCertificate failed");
        return SECFailure;
    }

    CERTVerifyLog log = {0};
    log.arena = PORT_NewArena(512);
    if (!log.arena) {
        failure("PORT_NewArena failed");
        return SECFailure;
    }

    CERTValOutParam out_params[2];
    memset(out_params, 0, sizeof(out_params));

    out_params[0].type = cert_po_errorLog;
    out_params[0].value.pointer.log = &log;
    out_params[1].type = cert_po_end;

    if (CERT_PKIXVerifyCert(cert, certificateUsageSSLServer, NULL, out_params, NULL) != SECSuccess) {
        failure("CERT_PKIXVerifyCert");
    }

    if (log.count > 0) {
        return SECFailure;
    }

    if (CERT_VerifyCertName(cert, url) != SECSuccess) {
        return SECFailure;
    }

    return SECSuccess;
}


int main(int argc, const char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <ip> <port> <flag>\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *flag = argv[3];

    if (NSS_NoDB_Init(NULL) != SECSuccess) {
        failure("NSS_NoDB_Init failed");
        return 1;
    }

    PRFileDesc *tcp = PR_OpenTCPSocket(PR_AF_INET);
    if (!tcp) {
        failure("PR_OpenTCPSocket failed");
        return 1;
    }

    PRNetAddr addr;
    if (PR_StringToNetAddr(ip, &addr) != PR_SUCCESS) {
        failure("PR_StringToNetAddr failed");
        return 1;
    }

    addr.inet.port = PR_htons(port);

    if (PR_Connect(tcp, &addr, PR_SecondsToInterval(5)) != PR_SUCCESS) {
        failure("PR_Connect failed");
        return 1;
    }

    PRFileDesc *ssl = SSL_ImportFD(NULL, tcp);
    if (!ssl) {
        failure("SSL_ImportFD failed");
        return 1;
    }

    if (SSL_AuthCertificateHook(ssl, auth_certificate, NULL) != SECSuccess) {
        failure("SSL_AuthCertificateHook failed");
        return 1;
    }
    
    if (SSL_ResetHandshake(ssl, PR_FALSE) != SECSuccess) {
        failure("SSL_ResetHandshake failed");
        return 1;
    }

    if (SSL_SetURL(ssl, "admin.ecsc2026.ctf") != SECSuccess) {
        failure("SSL_SetURL failed");
        return 1;
    }

    if (SSL_ForceHandshakeWithTimeout(ssl, PR_SecondsToInterval(5)) != SECSuccess) {
        failure("SSL_ForceHandshake failed");
        return 1;
    }

    if (PR_Send(ssl, flag, strlen(flag), 0, PR_SecondsToInterval(5)) == -1) {
        failure("PR_Send failed");
        return 1;
    }

    return 0;
}
