import os

from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from flask import Flask, make_response, redirect, render_template, request, url_for


app = Flask(__name__)

_AES_KEY = get_random_bytes(32)


def _encrypt_cookie(sponsortier: str) -> str:
    plaintext = f"sponsortier={sponsortier}".encode("ascii")
    cipher = AES.new(_AES_KEY, AES.MODE_GCM, _AES_KEY)
    ciphertext, tag = cipher.encrypt_and_digest(plaintext)
    payload = ciphertext + tag
    return payload.hex()

def _decrypt_cookie(encoded: str) -> bytes:
    try:
        payload = bytes.fromhex(encoded)
    except ValueError as exc:
        raise ValueError("invalid cookie") from exc

    ciphertext = payload[:-16]
    tag = payload[-16:]
    cipher = AES.new(_AES_KEY, AES.MODE_GCM, _AES_KEY)
    try:
        return cipher.decrypt_and_verify(ciphertext, tag)
    except ValueError as exc:
        raise ValueError("invalid cookie") from exc


@app.get("/")
def index():
    return render_template("index.html")


@app.post("/register")
def register():
    sponsortier = request.form.get("sponsortier", "")

    try:
        sponsortier.encode("ascii")
    except UnicodeEncodeError:
        return render_template("index.html", error="Tier names must contain only ASCII characters."), 400
    if sponsortier == "flag":
        return render_template("index.html", error="That sponsorship tier is already sold out."), 400

    response = make_response(redirect(url_for("hello")))
    response.set_cookie(
        "session",
        _encrypt_cookie(sponsortier),
        httponly=True,
        samesite="Lax",
    )
    return response


@app.get("/hello")
def hello():
    encoded = request.cookies.get("session")
    if not encoded:
        return "Access denied.", 403

    try:
        plaintext = _decrypt_cookie(encoded)
        if not plaintext.startswith(b"sponsortier="):
            raise ValueError("invalid cookie")
        sponsortier = plaintext.removeprefix(b"sponsortier=").decode("ascii")
    except (UnicodeDecodeError, ValueError):
        return "Access denied.", 403

    return render_template("hello.html", sponsortier=sponsortier)


@app.get("/flag")
def flag():
    encoded = request.cookies.get("session")
    if not encoded:
        return "Access denied.", 403

    try:
        plaintext = _decrypt_cookie(encoded)
    except ValueError:
        return "Access denied.", 403

    if plaintext != b"sponsortier=flag":
        return "Access denied.", 403
    return os.environ.get("FLAG", "ECSC{testflag}")


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000)
