
from Crypto.Cipher import AES
from Crypto.Hash import SHA512
from Crypto.Protocol import DH
from Crypto.Protocol.KDF import HKDF
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS
from Crypto.Util.Padding import pad, unpad

import binascii
import enum
import hashlib
import hmac
import msgpack
import secrets
import ssl
import socket
import struct
import sys


SignaturePublicKey = ECC.construct(
    curve = "p256",
    point_x = 79816927782759614966183695680585780019416944383131264144698128183481085292197,
    point_y = 11328336846712708587769693750828162270808685653866558944416261431896196471925
)


def hash(string: str) -> int:
    return binascii.crc32(string.encode())


class RequestField(enum.IntEnum):
    MethodID = hash("RequestField.MethodID")


class ResponseField(enum.IntEnum):
    Result = hash("ResponseField.Result")


class ResultCode(enum.IntEnum):
    Success = hash("ResultCode.Success")
    InternalError = hash("ResultCode.InternalError")
    InvalidRequest = hash("ResultCode.InvalidRequest")


class InsecureMethodID(enum.IntEnum):
    KeyAgreement = hash("InsecureMethodID.KeyAgreement")
    Proxy = hash("InsecureMethodID.Proxy")


class SecureMethodID(enum.IntEnum):
    ResetGame = hash("SecureMethodID.ResetGame")
    SolveLevel = hash("SecureMethodID.SolveLevel")


class KeyAgreementRequestField(enum.IntEnum):
    ClientPublicKey = hash("KeyAgreementRequestField.ClientPublicKey")
    ClientNonce = hash("KeyAgreementRequestField.ClientNonce")


class KeyAgreementResponseField(enum.IntEnum):
    ServerPublicKey = hash("KeyAgreementResponseField.ServerPublicKey")
    ServerNonce = hash("KeyAgreementResponseField.ServerNonce")
    Signature = hash("KeyAgreementResponseField.Signature")


class ProxyRequestField(enum.IntEnum):
    Payload = hash("ProxyRequestField.Payload")


class ProxyResponseField(enum.IntEnum):
    Payload = hash("ProxyResponseField.Payload")


class ResetGameResponseField(enum.IntEnum):
    Color = hash("ResetGameResponseField.Color")


class SolveLevelRequestField(enum.IntEnum):
    Hash = hash("SolveLevelRequestField.Hash")


class SolveLevelResponseField(enum.IntEnum):
    Color = hash("SolveLevelResponseField.Color")
    Flag = hash("SolveLevelResponseField.Flag")


class Color(enum.IntEnum):
    Red = hash("Color.Red")
    Green = hash("Color.Green")
    Blue = hash("Color.Blue")
    Yellow = hash("Color.Yellow")


class Client:
    _socket: socket.socket

    _encryption_key: bytes
    _authentication_key: bytes

    def __init__(self, sock: socket.socket):
        self._socket = sock

        self._encryption_key = b""
        self._authentication_key = b""
    
    def solve(self) -> None:
        self.key_agreement()

        colors = [self.reset_game()]
        for i in range(1000):
            data = struct.pack(f">{len(colors)}I", *colors)
            hash = hashlib.sha512(data).digest()
            color, flag = self.solve_level(hash)

            if flag:
                print(flag)
                return

            colors.append(color)
    
    def reset_game(self) -> int:
        request = {
            RequestField.MethodID: SecureMethodID.ResetGame
        }
        response = self.send_secure_request(request)
        if response[ResponseField.Result] != ResultCode.Success:
            raise RuntimeError("Reset game request failed")
        return response[ResetGameResponseField.Color]

    def solve_level(self, hash: bytes) -> tuple[int, str | None]:
        request = {
            RequestField.MethodID: SecureMethodID.SolveLevel,
            SolveLevelRequestField.Hash: hash
        }
        response = self.send_secure_request(request)
        if response[ResponseField.Result] != ResultCode.Success:
            raise RuntimeError("Solve level request failed")
        return (
            response[SolveLevelResponseField.Color],
            response.get(SolveLevelResponseField.Flag)
        )

    def send_packet(self, data: bytes) -> None:
        self._socket.sendall(struct.pack(">I", len(data)) + data)
    
    def receive_packet(self) -> bytes:
        buffer = b""
        while len(buffer) < 4:
            buffer += self._socket.recv(8192)

        length = struct.unpack_from(">I", buffer)[0]
        buffer = buffer[4:]

        while len(buffer) < length:
            buffer += self._socket.recv(8192)
        
        return buffer

    def send_insecure_request(self, request):
        self.send_packet(msgpack.packb(request))

        response = self.receive_packet()
        return msgpack.unpackb(response, strict_map_key=False)

    def send_secure_request(self, request):
        data = msgpack.packb(request)

        iv = secrets.token_bytes(16)
        encryptor = AES.new(self._encryption_key, AES.MODE_CBC, iv=iv)

        message = encryptor.encrypt(pad(data, 16)) + iv
        message += hmac.digest(self._authentication_key, message, "sha512")

        response = self.proxy(message)

        ciphertext = response[:-80]
        iv = response[-80:-64]
        mac = response[-64:]

        expected_mac = hmac.digest(
            self._authentication_key, ciphertext + iv, "sha512"
        )
        if mac != expected_mac:
            raise ValueError("HMAC validation failed")

        decryptor = AES.new(self._encryption_key, AES.MODE_CBC, iv=iv)
        plaintext = unpad(decryptor.decrypt(ciphertext), 16)
        return msgpack.unpackb(plaintext, strict_map_key=False)

    def key_agreement(self) -> None:
        client_private_key = ECC.generate(curve="p256")
        client_public_key = client_private_key.public_key()
        client_public_data = client_public_key.export_key(format="SEC1")

        client_nonce = secrets.token_bytes(64)

        request = {
            RequestField.MethodID: InsecureMethodID.KeyAgreement,
            KeyAgreementRequestField.ClientPublicKey: client_public_data,
            KeyAgreementRequestField.ClientNonce: client_nonce
        }
        response = self.send_insecure_request(request)

        if response[ResponseField.Result] != ResultCode.Success:
            raise RuntimeError("Key agreement request failed")

        server_public_data = response[KeyAgreementResponseField.ServerPublicKey]
        server_nonce = response[KeyAgreementResponseField.ServerNonce]
        signature = response[KeyAgreementResponseField.Signature]

        server_public_key = ECC.import_key(
            server_public_data, curve_name="p256"
        )

        payload = client_public_data + server_public_data + \
            client_nonce + server_nonce

        verifier = DSS.new(SignaturePublicKey, "fips-186-3")
        verifier.verify(SHA512.new(payload), signature)

        shared_secret = DH.key_agreement(
            kdf = lambda data: HKDF(data, 96, server_nonce, SHA512),
            eph_priv = client_private_key,
            eph_pub = server_public_key
        )
        assert isinstance(shared_secret, bytes)
        
        self._encryption_key = shared_secret[:32]
        self._authentication_key = shared_secret[32:]

    def proxy(self, payload: bytes) -> bytes:
        request = {
            RequestField.MethodID: InsecureMethodID.Proxy,
            ProxyRequestField.Payload: payload
        }
        response = self.send_insecure_request(request)
        if response[ResponseField.Result] != ResultCode.Success:
            raise RuntimeError("Proxy request failed")
        return response[ProxyResponseField.Payload]


def main() -> None:
    if len(sys.argv) not in (3, 4):
        print(f"Usage: python3 {sys.argv[0]} <ip> <port> [--ssl]")
        return

    ip = sys.argv[1]
    port = int(sys.argv[2])

    s = socket.socket()
    s.connect((ip, port))
    if len(sys.argv) == 4:
        if sys.argv[3] != "--ssl":
            print(f"Usage: python3 {sys.argv[0]} <ip> <port> [--ssl]")
            return
        context = ssl.create_default_context()
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
        s = context.wrap_socket(s, server_hostname=ip)

    client = Client(s)
    client.solve()


if __name__ == "__main__":
    main()
