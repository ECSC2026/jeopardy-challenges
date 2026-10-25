import re

from playwright.sync_api import Page, expect

from pqsb import START_BALANCE, chf

ROLLED = re.compile(r"^pqsb1q[0-9a-f]{32}$")


def test_the_bank_offers_a_rolled_serial(page: Page) -> None:
    page.goto("/")
    field = page.locator("#serial")
    first = field.input_value()
    assert ROLLED.match(first), first

    page.locator("#roll").click()
    second = field.input_value()
    assert ROLLED.match(second), second
    assert second != first


def test_entering_opens_a_file_on_the_house_deposit(page: Page, serial: str) -> None:
    page.goto("/")
    page.fill("#serial", serial)
    page.locator("#enter").click()

    expect(page).to_have_url(re.compile(r"/assets$"))
    assert page.evaluate("localStorage.getItem('pqsb.serial')") == serial
    expect(page.locator(".chip")).to_contain_text(chf(START_BALANCE))


def test_entering_without_a_serial_complains(page: Page) -> None:
    page.goto("/")
    page.fill("#serial", "")
    page.locator("#enter").click()

    expect(page.locator("#keyNotice")).to_contain_text("Quote a serial number")
    expect(page.locator("#enter")).to_be_enabled()


def test_next_returns_to_the_page_that_asked(page: Page, serial: str) -> None:
    page.goto("/?next=%2Fissuance")
    page.fill("#serial", serial)
    page.locator("#enter").click()

    expect(page).to_have_url(re.compile(r"/issuance$"))
