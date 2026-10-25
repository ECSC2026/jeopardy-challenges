import re
from pathlib import Path

import pytest
from playwright.sync_api import Page, expect

from pqsb import (
    DOCUMENTS,
    START_BALANCE,
    buy_button,
    chf,
    take_download,
    wait_for_balance,
)

CATALOGUE = ("mail.eml", "accounts.txt", "dossier.txt", "flag.txt")
MAIL_PRICE = 25


def test_the_archive_is_public_but_buying_asks_for_a_serial(page: Page) -> None:
    page.goto("/assets")

    expect(page.locator("#statusLine")).to_have_text(f"{len(CATALOGUE)} assets listed")
    expect(page.locator(".chip")).to_have_text("Login")

    buy_button(page, "mail.eml").click()
    expect(page).to_have_url(re.compile(r"/\?next=%2Fassets"))


def test_settling_pays_the_price_and_downloads_the_document(
    page: Page, client: str, tmp_path: Path
) -> None:
    page.goto("/assets")
    wait_for_balance(page, START_BALANCE)

    buy_button(page, "mail.eml").click()
    expect(page.locator("#tFilename")).to_have_text("mail.eml")
    expect(page.locator("#tCons")).to_contain_text(chf(MAIL_PRICE))

    got = take_download(page, page.locator("#settle"), tmp_path)
    assert got.name == "mail.eml"
    assert got.read_bytes() == (DOCUMENTS / "mail.eml").read_bytes()

    wait_for_balance(page, START_BALANCE - MAIL_PRICE)
    expect(buy_button(page, "mail.eml")).to_have_text("Download")


def test_a_document_already_held_is_not_paid_for_twice(
    page: Page, client: str, tmp_path: Path
) -> None:
    page.goto("/assets")
    wait_for_balance(page, START_BALANCE)

    buy_button(page, "mail.eml").click()
    take_download(page, page.locator("#settle"), tmp_path)
    wait_for_balance(page, START_BALANCE - MAIL_PRICE)

    again = take_download(page, buy_button(page, "mail.eml"), tmp_path / "held")
    assert again.read_bytes() == (DOCUMENTS / "mail.eml").read_bytes()
    wait_for_balance(page, START_BALANCE - MAIL_PRICE)


def test_buying_both_affordable_records_empties_the_account(
    page: Page, client: str, tmp_path: Path
) -> None:
    page.goto("/assets")
    wait_for_balance(page, START_BALANCE)

    for name in ("accounts.txt", "mail.eml"):
        buy_button(page, name).click()
        got = take_download(page, page.locator("#settle"), tmp_path)
        assert got.read_bytes() == (DOCUMENTS / name).read_bytes()

    wait_for_balance(page, 0)


@pytest.mark.parametrize("name", ["dossier.txt", "flag.txt"])
def test_expensive_records_are_out_of_reach_of_the_house_deposit(
    page: Page, client: str, name: str
) -> None:
    page.goto("/assets")
    wait_for_balance(page, START_BALANCE)

    buy_button(page, name).click()
    expect(page.locator("#tNotice")).to_contain_text("exceeds the deposit")
    expect(page.locator("#settle")).to_be_disabled()
