#!/usr/bin/env python3
import argparse
import base64
import hashlib
import mimetypes
import os
import secrets
import time
from collections.abc import Callable
from dataclasses import dataclass
from enum import Enum
from typing import Literal

from bottle import Bottle, HTTPResponse, request, response, run, static_file
from pydantic import BaseModel, ConfigDict, Field, ValidationError

import mldsa
from lru import LRU

ALG = mldsa.Algorithm.ML_DSA_87

START_BALANCE = 200
WITHDRAW_FEE = 1

SERIAL_MAX = 64
MAX_ACCOUNTS = 1_000

HERE = os.path.dirname(os.path.abspath(__file__))
DOCUMENTS_DIR = os.path.join(HERE, "documents")
FRONTEND_DIR = os.path.join(HERE, "frontend")
STATIC_DIR = os.path.join(FRONTEND_DIR, "static")

mimetypes.add_type("image/svg+xml", ".svg")

PAGES = {
    "/": "index.html",
    "/assets": "assets.html",
    "/issuance": "issuance.html",
    "/redemption": "redemption.html",
}


def from_disk(name: str) -> Callable[[], bytes]:
    def produce() -> bytes:
        with open(os.path.join(DOCUMENTS_DIR, name), "rb") as fh:
            return fh.read()

    return produce


@dataclass(frozen=True)
class Item:
    price: int
    desc: str
    produce: Callable[[], bytes]


ITEMS: dict[str, Item] = {
    "mail.eml": Item(
        25,
        "Private Zurich-Moscow correspondence.",
        from_disk("mail.eml"),
    ),
    "accounts.txt": Item(
        175,
        "Russian private-client accounts.",
        from_disk("accounts.txt"),
    ),
    "dossier.txt": Item(
        500,
        "Due-diligence dossier: client ZIMA.",
        from_disk("dossier.txt"),
    ),
    "flag.txt": Item(
        1000,
        "European Cybersecurity Challenge Flag.",
        from_disk("flag.txt"),
    ),
}

app = Bottle()


class Account:
    def __init__(self, serial: str, signer: mldsa.Signer, balance: int) -> None:
        self.serial = serial
        self.signer = signer
        self.kid = hashlib.sha256(signer.pk().pack()).hexdigest()[:16]
        self.balance = balance
        self.spent: set[str] = set()
        self.owned: set[str] = set()

    @classmethod
    def fresh(cls, serial: str) -> "Account":
        return cls(serial, mldsa.Signer.generate(), START_BALANCE)

    def statement(self) -> dict:
        return {"serial": self.serial, "balance": self.balance, "kid": self.kid}

    def issue_note(self, amount: int) -> str:
        cost = amount + WITHDRAW_FEE
        if self.balance < cost:
            raise InsufficientFunds(amount, cost, self.balance)
        self.balance -= cost
        assert self.balance >= 0

        header = Header(alg=ALG, typ="JWT", kid=self.kid)
        claims = Note(
            acct=self.serial,
            amt=amount,
            nonce=secrets.token_hex(8),
            iat=int(time.time()),
        )
        msg = f"{b64url(compact(header))}.{b64url(compact(claims))}"
        sig = self.signer.sign(msg.encode("ascii"))
        return f"{msg}.{b64url(sig.pack())}"

    def check_note(self, raw: str) -> "Note":
        try:
            s_hdr, s_clm, s_sig = raw.split(".")
            header, claims, signature = (
                base64.urlsafe_b64decode(p + "=" * (-len(p) % 4))
                for p in (s_hdr, s_clm, s_sig)
            )
            parse(Header, header, "header")
            msg = f"{s_hdr}.{s_clm}".encode("ascii")
        except Exception:
            raise Rejected.Malformed from None

        try:
            self.signer.pk().verify(mldsa.Signature.unpack(signature), msg)
        except Exception:
            raise Rejected.BadSignature from None

        try:
            note = parse(Note, claims, "note")
        except ValueError:
            raise Rejected.BadClaims from None

        if note.acct != self.serial:
            raise Rejected.WrongAccount

        if note.nonce in self.spent:
            raise Rejected.Spent

        return note

    def deposit_note(self, raw: str) -> "Note":
        note = self.check_note(raw)
        self.spent.add(note.nonce)
        self.balance += note.amt
        return note


ACCOUNTS: LRU[str, Account] = LRU(MAX_ACCOUNTS)


class Check(str, Enum):
    STRUCTURE = "structure"
    SIGNATURE = "signature"
    KEY = "key"
    NONCE = "nonce"


class Withdrawal(BaseModel):
    model_config = ConfigDict(frozen=True, strict=True)

    amount: int = Field(ge=1)


class Deposit(BaseModel):
    model_config = ConfigDict(frozen=True, strict=True)

    note: str


class Purchase(BaseModel):
    model_config = ConfigDict(frozen=True, strict=True)

    item: str


class Header(BaseModel):
    model_config = ConfigDict(frozen=True, strict=True)

    alg: mldsa.Algorithm
    typ: Literal["JWT"]
    kid: str = ""


class Jwk(BaseModel):
    model_config = ConfigDict(frozen=True)

    kty: Literal["AKP"] = "AKP"
    alg: mldsa.Algorithm = ALG
    use: Literal["sig"] = "sig"
    kid: str
    pub: str


class Jwks(BaseModel):
    model_config = ConfigDict(frozen=True)

    keys: list[Jwk]


class Note(BaseModel):
    model_config = ConfigDict(frozen=True, strict=True)

    acct: str
    amt: int = Field(ge=0)
    nonce: str
    iat: int | None = None


def _explain(exc: ValidationError) -> str:
    first = exc.errors()[0]
    where = ".".join(str(part) for part in first["loc"])
    return f"{where}: {first['msg']}" if where else first["msg"]


def parse(model, raw: bytes, what: str):
    try:
        return model.model_validate_json(raw)
    except ValidationError as exc:
        raise ValueError(f"{what} {_explain(exc)}") from exc


def json_body(model):
    try:
        raw = request.json
    except (ValueError, TypeError):
        raw = None
    if not isinstance(raw, dict):
        raw = dict(request.forms) if request.forms else None
    if raw is None:
        fields = ", ".join(model.model_fields)
        raise HTTPResponse({"error": f"send a JSON object: {fields}"}, 400)
    try:
        return model.model_validate(raw)
    except ValidationError as exc:
        raise HTTPResponse({"error": _explain(exc)}, 400) from exc


def b64url(raw: bytes) -> str:
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode("ascii")


def compact(model: BaseModel) -> bytes:
    return model.model_dump_json().encode()


class InsufficientFunds(Exception):
    def __init__(self, amount: int, cost: int, balance: int) -> None:
        super().__init__(
            f"insufficient funds: {amount} plus a {WITHDRAW_FEE} "
            f"credit fee is {cost}, and you have {balance}"
        )


class Rejected(Exception, Enum):
    Malformed = (Check.STRUCTURE, "malformed note")
    BadSignature = (Check.SIGNATURE, "the signature does not verify")
    BadClaims = (Check.SIGNATURE, "the note says nothing this bank can read")
    WrongAccount = (Check.KEY, "this note was issued to another account")
    Spent = (Check.NONCE, "this note has already been deposited")

    @property
    def check(self) -> Check:
        return self.args[0]

    def __str__(self) -> str:
        return self.args[1]


def require_account() -> Account:
    header = request.get_header("Authorization", "")
    scheme, _, token = header.partition(" ")
    serial = token.strip()
    if scheme.lower() != "bearer" or not serial:
        raise HTTPResponse({"error": "quote a serial number"}, status=401)
    if len(serial) > SERIAL_MAX:
        raise HTTPResponse({"error": "that serial is too long"}, status=401)
    return ACCOUNTS.get(serial, lambda: Account.fresh(serial))


def download(name: str, body: bytes, paid: int, balance: int) -> HTTPResponse:
    return HTTPResponse(
        body=body,
        status=200,
        headers={
            "Content-Type": "text/plain; charset=utf-8",
            "Content-Disposition": f'attachment; filename="{name}"',
            "Content-Length": str(len(body)),
            "X-Paid": str(paid),
            "X-Balance": str(balance),
        },
    )


@app.get("/")
@app.get("/assets")
@app.get("/issuance")
@app.get("/redemption")
def page():
    return static_file(PAGES[request.path], root=FRONTEND_DIR)


@app.get("/static/<path:path>")
def asset(path):
    return static_file(path, root=STATIC_DIR)


def catalogue() -> list[dict]:
    return [
        {"item": name, "price": item.price, "desc": item.desc}
        for name, item in ITEMS.items()
    ]


@app.get("/api/balance")
def balance():
    return require_account().statement()


@app.post("/api/withdraw")
def withdraw():
    acct = require_account()
    slip = json_body(Withdrawal)
    try:
        note = acct.issue_note(slip.amount)
    except InsufficientFunds as exc:
        response.status = 402
        return {"error": str(exc)}

    return {
        "note": note,
        "amount": slip.amount,
        "fee": WITHDRAW_FEE,
        **acct.statement(),
    }


@app.post("/api/verify")
def verify():
    acct = require_account()
    slip = json_body(Deposit)
    try:
        note = acct.check_note(slip.note)
    except Rejected as exc:
        return {
            "ok": False,
            "reason": str(exc),
            "failed": exc.check.value,
            "amount": None,
        }
    return {"ok": True, "reason": "", "failed": None, "amount": note.amt}


@app.post("/api/deposit")
def deposit():
    acct = require_account()
    slip = json_body(Deposit)
    try:
        note = acct.deposit_note(slip.note)
    except Rejected as exc:
        response.status = 400
        return {"error": str(exc)}
    return {
        "deposited": note.amt,
        **acct.statement(),
    }


@app.get("/api/shop")
def shop():
    entries = catalogue()
    try:
        acct = require_account()
    except HTTPResponse:
        return {"catalogue": entries}
    for entry in entries:
        entry["owned"] = entry["item"] in acct.owned
    return {"catalogue": entries, "balance": acct.balance}


@app.post("/api/buy")
def buy():
    acct = require_account()
    order = json_body(Purchase)

    item = ITEMS.get(order.item)
    if item is None:
        response.status = 404
        return {"error": "we do not stock that"}
    paid = 0
    if order.item not in acct.owned:
        if acct.balance < item.price:
            response.status = 402
            return {
                "error": f"{order.item} costs {item.price} credits "
                f"and you have {acct.balance}",
                "price": item.price,
                "balance": acct.balance,
            }
        acct.balance -= item.price
        acct.owned.add(order.item)
        paid = item.price

    return download(order.item, item.produce(), paid, acct.balance)


@app.get("/api/key")
def own_key():
    return jwks(require_account())


@app.get("/api/accounts/<name>/key")
def account_key(name):
    if len(name) > SERIAL_MAX:
        response.status = 404
        return {"error": "that serial is too long"}
    return jwks(ACCOUNTS.get(name, lambda: Account.fresh(name)))


def jwks(acct: Account) -> dict:
    keys = Jwks(keys=[Jwk(kid=acct.kid, pub=b64url(acct.signer.pk().pack()))])
    return keys.model_dump(mode="json")


def main():
    ap = argparse.ArgumentParser(description="PQ Swiss Bank")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8080)
    args = ap.parse_args()
    run(app=app, host=args.host, port=args.port, quiet=True)


if __name__ == "__main__":
    main()
