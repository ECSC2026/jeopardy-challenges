from __future__ import annotations

import re
from pathlib import Path

from playwright.sync_api import Locator, Page, expect

HERE = Path(__file__).resolve().parent
DOCUMENTS = HERE.parent / "public" / "documents"

CHF_PER_CREDIT = 1000
START_BALANCE = 200
WITHDRAW_FEE = 1


def chf(credits: int) -> re.Pattern[str]:
    """Match "CHF 200'000" without pinning the separator ICU picks for de-CH."""
    groups = f"{credits * CHF_PER_CREDIT:,}".split(",")
    return re.compile(r"CHF\s*" + r"\D?".join(re.escape(group) for group in groups))


def open_account(page: Page, serial: str) -> None:
    page.goto("/")
    page.fill("#serial", serial)
    page.locator("#enter").click()
    expect(page).to_have_url(re.compile(r"/assets$"))


def wait_for_balance(page: Page, credits: int) -> None:
    expect(page.locator(".chip")).to_contain_text(chf(credits))


def take_download(page: Page, control: Locator, into: Path) -> Path:
    with page.expect_download() as pending:
        control.click()
    download = pending.value
    into.mkdir(parents=True, exist_ok=True)
    target = into / download.suggested_filename
    download.save_as(target)
    return target


def issue_bond(page: Page, francs: int, into: Path) -> Path:
    page.goto("/issuance")
    page.fill("#amount", str(francs))
    expect(page.locator("#sign")).to_be_enabled()
    return take_download(page, page.locator("#sign"), into)


def buy_button(page: Page, item: str) -> Locator:
    """The flat grid gives each button the same index as its filename."""
    names = page.locator(".td:not(.td-about):not(.td-price)")
    expect(names.first).to_be_visible()
    return page.locator(".buy").nth(names.all_inner_texts().index(item))
