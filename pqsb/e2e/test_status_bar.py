import re

import pytest
from playwright.sync_api import Page, expect

CLOCK = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")


@pytest.mark.parametrize(
    ("path", "state"),
    [
        ("/assets", "4 assets listed"),
        ("/issuance", "Nothing signed"),
        ("/redemption", "Nothing presented"),
    ],
)
def test_every_client_page_carries_the_same_status_bar(
    page: Page, client: str, path: str, state: str
) -> None:
    page.goto(path)

    expect(page.locator("#keyId")).to_have_text(f"{client[:10]}…{client[-2:]}")
    expect(page.locator("#clock")).to_have_text(CLOCK)
    expect(page.locator("#statusLine")).to_have_text(state)
