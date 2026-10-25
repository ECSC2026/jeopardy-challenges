import json
from pathlib import Path

import pytest
from playwright.sync_api import Page, expect

from pqsb import START_BALANCE, WITHDRAW_FEE, chf, issue_bond, wait_for_balance

CHECKS = ("#cStructure", "#cSignature", "#cKey", "#cNonce")


def present(page: Page, bond_file: Path) -> None:
    page.locator("#pick").set_input_files(bond_file)


def tamper(bond_file: Path, into: Path) -> Path:
    bond = json.loads(bond_file.read_text())
    header, claims, sig = bond["note"].split(".")
    flipped = "B" if sig[64] != "B" else "C"
    bond["note"] = f"{header}.{claims}.{sig[:64]}{flipped}{sig[65:]}"

    target = into / "tampered.bond"
    target.write_text(json.dumps(bond, indent=2))
    return target


def test_a_bond_round_trips_from_issuance_to_redemption(
    page: Page, client: str, tmp_path: Path
) -> None:
    bond_file = issue_bond(page, 25_000, tmp_path)

    page.goto("/redemption")
    present(page, bond_file)

    expect(page.locator("#statusLine")).to_have_text("Verified, awaiting redemption")
    for check in CHECKS:
        expect(page.locator(check)).to_have_text("pass")
    expect(page.locator("#vPrincipal")).to_contain_text(chf(25))

    page.locator("#redeem").click()
    expect(page.locator("#paidLine")).to_contain_text(chf(25))
    expect(page.locator("#statusLine")).to_have_text("Paid")
    wait_for_balance(page, START_BALANCE - WITHDRAW_FEE)


def test_a_redeemed_bond_cannot_be_presented_twice(
    page: Page, client: str, tmp_path: Path
) -> None:
    bond_file = issue_bond(page, 25_000, tmp_path)

    page.goto("/redemption")
    present(page, bond_file)
    expect(page.locator("#statusLine")).to_have_text("Verified, awaiting redemption")
    page.locator("#redeem").click()
    expect(page.locator("#statusLine")).to_have_text("Paid")

    page.locator("#another").click()
    present(page, bond_file)

    expect(page.locator("#statusLine")).to_have_text("Refused")
    expect(page.locator("#cSignature")).to_have_text("pass")
    expect(page.locator("#cNonce")).to_have_text("fail")
    expect(page.locator("#redeem")).to_be_disabled()
    wait_for_balance(page, START_BALANCE - WITHDRAW_FEE)


def test_a_tampered_signature_is_refused(
    page: Page, client: str, tmp_path: Path
) -> None:
    forged = tamper(issue_bond(page, 25_000, tmp_path), tmp_path)

    page.goto("/redemption")
    present(page, forged)

    expect(page.locator("#statusLine")).to_have_text("Refused")
    expect(page.locator("#cStructure")).to_have_text("pass")
    expect(page.locator("#cSignature")).to_have_text("fail")
    expect(page.locator("#redeem")).to_be_disabled()


@pytest.mark.parametrize(
    ("name", "body", "why"),
    [
        ("shopping-list.txt", "milk, bread, a lattice\n", "expected a .bond file"),
        (
            "empty.bond",
            '{"instrument": "PQ Swiss Bank bearer bond"}',
            "Missing field: note",
        ),
    ],
)
def test_a_file_that_is_not_a_bond_never_reaches_the_bank(
    page: Page, client: str, tmp_path: Path, name: str, body: str, why: str
) -> None:
    junk = tmp_path / name
    junk.write_text(body)

    page.goto("/redemption")
    present(page, junk)

    expect(page.locator("#reject")).to_be_visible()
    expect(page.locator("#rejectWhy")).to_contain_text(why)
    expect(page.locator("#presented")).to_be_hidden()
