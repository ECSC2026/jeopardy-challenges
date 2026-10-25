import json
import re
from pathlib import Path

from playwright.sync_api import Page, expect

from pqsb import START_BALANCE, WITHDRAW_FEE, chf, issue_bond, wait_for_balance


def test_the_client_area_needs_a_serial(page: Page) -> None:
    page.goto("/issuance")
    expect(page).to_have_url(re.compile(r"/\?next=%2Fissuance$"))


def test_a_principal_is_whole_thousands(page: Page, client: str) -> None:
    page.goto("/issuance")
    page.fill("#amount", "1500")

    expect(page.locator("#notice")).to_contain_text("whole thousands")
    expect(page.locator("#sign")).to_be_disabled()


def test_the_principal_and_its_fee_fit_the_deposit(page: Page, client: str) -> None:
    page.goto("/issuance")
    wait_for_balance(page, START_BALANCE)

    page.fill("#amount", "200000")
    expect(page.locator("#notice")).to_contain_text("Principal exceeds")
    expect(page.locator("#sign")).to_be_disabled()

    page.fill("#amount", "199000")
    expect(page.locator("#sign")).to_be_enabled()


def test_signing_downloads_a_bond_and_draws_the_fee(
    page: Page, client: str, tmp_path: Path
) -> None:
    bond_file = issue_bond(page, 25_000, tmp_path)
    assert bond_file.name == "PQSB-CHF-25000.bond"

    bond = json.loads(bond_file.read_text())
    assert bond["principal"] == 25_000
    assert bond["currency"] == "CHF"
    assert bond["issuer_key"] == client
    assert len(bond["note"].split(".")) == 3

    expect(page.locator("#payload")).to_contain_text('"instrument"')
    expect(page.locator("#objectStatus")).to_contain_text(chf(25))
    wait_for_balance(page, START_BALANCE - 25 - WITHDRAW_FEE)
