
from Crypto.Cipher import AES
from Crypto.Hash import SHA512
from Crypto.Protocol import DH
from Crypto.Protocol.KDF import HKDF
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS
from Crypto.Util.Padding import pad, unpad

from typing import Callable

import anyio
import anyio.abc
import binascii
import enum
import hashlib
import hmac
import msgpack
import os
import secrets
import struct

import logging
logging.basicConfig(level = logging.INFO)

logger = logging.getLogger(__name__)


FLAG = os.environ["FLAG"]


LEVEL_COUNT = 999


type Value = dict[Value, Value] | list[Value] | int | float | bool | str | \
    bytes | None

type DictValue = dict[Value, Value]

type HandlerMap = dict[int, Callable[[RequestMessage], DictValue]]


SignaturePrivateKey = ECC.construct(
    curve = "p256",
    d = int(
        "584653863501378959197702124110487272167"
        "21512129507859585550361015315123086956"
    )
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


class ProtocolError(Exception):
    _code: ResultCode

    def __init__(self, code: ResultCode):
        self._code = code

    def code(self) -> ResultCode:
        return self._code


class RequestMessage:
    _data: dict[Value, Value]

    def __init__(self, data: dict[Value, Value]):
        self._data = data

    def value(self, key: int) -> Value:
        if key not in self._data:
            raise ProtocolError(ResultCode.InvalidRequest)

        return self._data[key]

    def integer(self, key: int) -> int:
        value = self.value(key)
        if not isinstance(value, int):
            raise ProtocolError(ResultCode.InvalidRequest)
        return value

    def enum[T: enum.IntEnum](self, key: int, cls: type[T]) -> T:
        value = self.integer(key)
        if value not in cls:
            raise ProtocolError(ResultCode.InvalidRequest)
        return cls(value)

    def bytes(self, key: int) -> bytes:
        value = self.value(key)
        if not isinstance(value, bytes):
            raise ProtocolError(ResultCode.InvalidRequest)
        return value

    def method_id(self) -> int:
        return self.integer(RequestField.MethodID)


class TCPClient:
    _stream: anyio.abc.SocketStream
    
    def __init__(self, stream: anyio.abc.SocketStream):
        self._stream = stream

    async def send(self, data: bytes) -> None:
        await self._stream.send(data)

    async def recv(self, num: int = 65536) -> bytes:
        return await self._stream.receive(num)


class PacketClient:
    _client: TCPClient
    _buffer: bytes

    def __init__(self, stream: anyio.abc.SocketStream):
        self._client = TCPClient(stream)
        self._buffer = b""

    async def send(self, data: bytes) -> None:
        data = struct.pack(">I", len(data)) + data
        await self._client.send(data)

    async def recv(self) -> bytes:
        while len(self._buffer) < 4:
            self._buffer += await self._client.recv()

        length = struct.unpack_from(">I", self._buffer)[0]
        while len(self._buffer) < 4 + length:
            self._buffer += await self._client.recv()

        data = self._buffer[4 : 4 + length]
        self._buffer = self._buffer[4 + length:]
        return data



class GameState:
    _colors: list[Color]

    def __init__(self):
        self._colors = []

    def reset(self) -> Color:
        self._colors = []
        return self._next_color()

    def solve_level(self, solution: bytes) -> tuple[Color, str | None]:
        data = struct.pack(f">{len(self._colors)}I", *self._colors)
        correct_solution = hashlib.sha512(data).digest()
        if solution != correct_solution:
            raise ProtocolError(ResultCode.InvalidRequest)

        color = self._next_color()
        flag = FLAG if len(self._colors) > LEVEL_COUNT else None
        return color, flag

    def _next_color(self) -> Color:
        colors = [Color.Red, Color.Green, Color.Blue, Color.Yellow]
        color = colors[secrets.randbits(2)]
        self._colors.append(color)
        return color


class GameClient:
    _client: PacketClient

    _encryption_key: bytes | None
    _authentication_key: bytes | None

    _insecure_handlers: HandlerMap
    _secure_handlers: HandlerMap

    _state: GameState | None

    def __init__(self, stream: anyio.abc.SocketStream):
        self._client = PacketClient(stream)

        self._encryption_key = None
        self._authentication_key = None

        self._insecure_handlers = {
            InsecureMethodID.KeyAgreement: self._handle_key_agreement,
            InsecureMethodID.Proxy: self._handle_proxy
        }

        self._secure_handlers = {
            SecureMethodID.ResetGame: self._handle_reset_game,
            SecureMethodID.SolveLevel: self._handle_solve_level
        }

        self._state = None

    async def run(self) -> None:
        while True:
            try:
                data = await self._client.recv()
            except anyio.EndOfStream:
                logger.info("Connection was closed")
                return
            
            logger.info(f"Received packet with {len(data)} bytes")
            response = self._process_data(data, self._insecure_handlers)
            logger.info(f"Sending response with {len(response)} bytes")
            await self._client.send(response)

    def _encode_error(self, code: ResultCode) -> bytes:
        return msgpack.packb({ResponseField.Result: code})

    def _process_data(self, data: bytes, handlers: HandlerMap) -> bytes:
        try:
            message = msgpack.unpackb(data, strict_map_key=False)
        except ValueError:
            logger.info("Message pack decoding failed")
            return self._encode_error(ResultCode.InvalidRequest)

        try:
            response = self._process(message, handlers)
            return msgpack.packb(response)
        except ProtocolError as e:
            logger.info("A protocol error occurred")
            return self._encode_error(e.code())
        except Exception:
            logger.exception("An exception occurred")
            return self._encode_error(ResultCode.InternalError)
    
    def _process(self, data: Value, handlers: HandlerMap) -> Value:
        if not isinstance(data, dict):
            raise ProtocolError(ResultCode.InvalidRequest)
        
        request = RequestMessage(data)
        method_id = request.method_id()
        if method_id not in handlers:
            raise ProtocolError(ResultCode.InvalidRequest)
        
        return handlers[method_id](request)

    def _handle_key_agreement(self, request: RequestMessage) -> DictValue:
        client_public_data = \
            request.bytes(KeyAgreementRequestField.ClientPublicKey)
        client_nonce = request.bytes(KeyAgreementRequestField.ClientNonce)

        try:
            client_public_key = ECC.import_key(
                client_public_data, curve_name="p256"
            )
        except ValueError:
            raise ProtocolError(ResultCode.InvalidRequest)

        if len(client_nonce) != 64:
            raise ProtocolError(ResultCode.InvalidRequest)
        
        server_private_key = ECC.generate(curve="p256")
        server_public_key = server_private_key.public_key()
        server_public_data = server_public_key.export_key(format="SEC1")

        server_nonce = secrets.token_bytes(32)

        payload = client_public_data + server_public_data + \
            client_nonce + server_nonce

        signer = DSS.new(SignaturePrivateKey, "fips-186-3")
        signature = signer.sign(SHA512.new(payload))

        shared_secret = DH.key_agreement(
            kdf = lambda data: HKDF(data, 96, server_nonce, SHA512),
            eph_priv = server_private_key,
            eph_pub = client_public_key
        )
        assert isinstance(shared_secret, bytes)

        self._encryption_key = shared_secret[:32]
        self._authentication_key = shared_secret[32:]

        return {
            ResponseField.Result: ResultCode.Success,
            KeyAgreementResponseField.ServerPublicKey: server_public_data,
            KeyAgreementResponseField.ServerNonce: server_nonce,
            KeyAgreementResponseField.Signature: signature
        }

    def _handle_proxy(self, request: RequestMessage) -> DictValue:
        if self._encryption_key is None or self._authentication_key is None:
            raise ProtocolError(ResultCode.InvalidRequest)

        payload = request.bytes(ProxyRequestField.Payload)
        if len(payload) < 80 or len(payload) % 16:
            raise ProtocolError(ResultCode.InvalidRequest)
        
        ciphertext = payload[:-80]
        iv = payload[-80:-64]
        mac = payload[-64:]

        expected_mac = hmac.digest(
            self._authentication_key, ciphertext + iv, "sha512"
        )
        if mac != expected_mac:
            raise ProtocolError(ResultCode.InvalidRequest)

        decryptor = AES.new(self._encryption_key, AES.MODE_CBC, iv=iv)
        try:
            plaintext = unpad(decryptor.decrypt(ciphertext), 16)
        except ValueError:
            raise ProtocolError(ResultCode.InvalidRequest)

        response = self._process_data(plaintext, self._secure_handlers)

        iv = secrets.token_bytes(16)
        encryptor = AES.new(self._encryption_key, AES.MODE_CBC, iv=iv)

        message = encryptor.encrypt(pad(response, 16)) + iv
        message += hmac.digest(self._authentication_key, message, "sha512")
        return {
            ResponseField.Result: ResultCode.Success,
            ProxyResponseField.Payload: message
        }

    def _handle_reset_game(self, request: RequestMessage) -> DictValue:
        logger.info("Received reset game request")
        self._state = GameState()
        return {
            ResponseField.Result: ResultCode.Success,
            ResetGameResponseField.Color: self._state.reset()
        }

    def _handle_solve_level(self, request: RequestMessage) -> DictValue:
        if self._state is None:
            raise ProtocolError(ResultCode.InvalidRequest)

        solution = request.bytes(SolveLevelRequestField.Hash)

        color, flag = self._state.solve_level(solution)

        response: DictValue = {
            ResponseField.Result: ResultCode.Success,
            SolveLevelResponseField.Color: color
        }
        if flag is not None:
            response[SolveLevelResponseField.Flag] = flag
        return response


async def handle(stream: anyio.abc.SocketStream) -> None:
    try:
        async with stream:
            client = GameClient(stream)
            await client.run()
    except Exception:
        logger.exception("An exception occurred")


async def main() -> None:
    listener = await anyio.create_tcp_listener(local_port=5000)
    async with listener:
        await listener.serve(handle)


anyio.run(main)
