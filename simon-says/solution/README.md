
## Simon Says
The challenge implements the game Simon Says. The game is written in C++ with SDL. It is compiled with [DeClang](https://github.com/DeNA/DeClang) for obfuscation.

The goal is to reach level 1000, which is impossible by playing the game normally, because it would take longer than the duration of the CTF. Instead, the player should reverse engineer and reimplement the communication protocol that is used by the game and reach level 1000 by quickly sending the correct packets to the server. The server transmits the flag to the client after level 1000 is reached.

The player may either reimplement the protocol from scratch, or reuse parts of the game binary (e.g. by emulating functions with unicorn) if this is easier for the player. Which approach is easier depends on the experience of the player.

## Protocol
The game communicates with the server via TCP. A custom remote procedure call (RPC) protocol is used. The client sends a request, and the server sends a response.

Every packet starts with a 32-bit big-endian integer that indicates the payload size, followed by the payload itself. The payload contains either a [request](#request-message) or a [response](#response-message), serialized with MessagePack.

To increase the difficulty of man-in-the-middle attacks, the protocol uses a custom Diffie-Hellman handshake and encryption. The game-specific methods can only be accessed with an encrypted request, after completing the handshake.

## Request Message
| Field | Description |
| --- | --- |
| `0x075BD0EA` | [Method id](#method-list) |
| ... | Arguments |

The remaining fields depend on the method id.

## Response Message
| Field | Description |
| --- | --- |
| `0x8834B03A` | [Result code](#result-code) |
| ... | Response fields |

The result code either indicates success or an error code. If an error code is returned, no other fields are returned in the response. Otherwise, the other response fields depend on the method that was invoked by the client.

### Result Code
| Code | Description |
| --- | --- |
| `0xF5BE76C5` | Success |
| `0x4B80B6B4` | Internal server error |
| `0x3122BFA7` | Invalid parameter in request |

## Method List
The following methods are called unencrypted:

| Method ID | Description |
| --- | --- |
| `0xE9DC0847` | [Key agreement](#key-agreement) |
| `0x3BA9BEDC` | [Encrypted request](#encrypted-request) |

The following methods can only be called within an [encrypted request](#encrypted-request):

| Method ID | Description |
| --- | --- |
| `0x997DDAA3` | [Reset game](#reset-game) |
| `0x704A32EB` | [Solve level](#solve-level) |

### Key Agreement
This method performs a Diffie-Hellman handshake. A shared secret is generated from which the encryption key for further requests is derived. In addition, the server sends a signature to the client that allows the client to verify the authenticity of the server.

The request contains the following fields:

| Field | Description |
| --- | --- |
| `0xF4F4F477` | Client public key |
| `0x6CB648F8` | Client nonce |

On success, the server transmits the following response:

| Field | Description |
| --- | --- |
| `0x9523F7C6` | Server public key |
| `0xC548C780` | Server nonce |
| `0xE67A9176` | Signature |

The public keys are randomly generated during the request. They are ECC keys on the NIST P-256 curve. The keys are used to derive a shared secret using Diffie-Hellman.

The nonces contain random bytes that are generated during the request. They add entropy to the signature. The server nonce is also used as a salt for the HKDF algorithm during Diffie-Hellman key agreement. The client nonce must contain 64 random bytes. The server nonce contains 32 random bytes.

The shared secret consists of 96 bytes and is generated using Diffie-Hellman with HKDF-SHA512. The HKDF algorithm is salted with the server nonce. The first 32 bytes of the shared secret contain an encryption key and the remaining 64 bytes contain an authentication key. More details on the encryption scheme can be found in the section on [encrypted requests](#encrypted-request).

The signature field contains an ECDSA signature that is calculated over the following payload: `client_public_key + server_public_key + client_nonce + server_nonce`.

The client can verify the ECDSA signature with the following public key:

```
Curve = NIST P-256
X = 79816927782759614966183695680585780019416944383131264144698128183481085292197
Y = 11328336846712708587769693750828162270808685653866558944416261431896196471925
```

### Encrypted Request
This method allows the client to transmit an encrypted request. This method can only be used after a successful [key agreement](#key-agreement) call, which generates an encryption and authentication key.

The request contains the following fields:

| Field | Description |
| --- | --- |
| `0x2169BA88` | Payload |

On success, the server transmits the following response:

| Field | Description |
| --- | --- |
| `0x3BA159BD` | Payload |

The payloads have the following structure:

| Offset | Size | Description |
| --- | --- | --- |
| 0x0 | N | Ciphertext, encrypted with AES-256-CBC |
| N | 16 | Random IV |
| N + 16 | 64 | HMAC-SHA512 over `ciphertext + iv` |

The plaintext contains either a [request](#request-message) or [response](#response-message) message, serialized with MessagePack.

### Reset Game
This method resets the game. No additional fields are present in the request. This method can only be called with an [encrypted request](#encrypted-request).

On success, the response contains the following fields:

| Field | Description |
| --- | --- |
| `0x03F597F3` | Initial [color](#color) |

### Solve Level
This method can be used to advance to the next level. The request contains the following fields:

| Field | Description |
| --- | --- |
| `0xE8CE6188` | Solution hash |

On success, the response contains the following fields:

| Field | Description |
| --- | --- |
| `0x36A961FA` | New [color](#color) |
| `0x2BACC4EA` | Flag |

The solution hash must be the SHA-512 hash of each [color value](#color) of the current level encoded in big-endian byte order. For example, if in level 3 the button sequence is red-blue-yellow, the solution hash is the SHA-512 hash of `43fba9bac83c9461a098f73c`.

The flag is only present when level 1000 has been solved.

### Color
| Value | Description |
| --- | --- |
| `0x43FBA9BA` | Red |
| `0x2675C224` | Green |
| `0xC83C9461` | Blue |
| `0xA098F73C` | Yellow |
