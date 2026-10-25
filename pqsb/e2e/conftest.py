from __future__ import annotations

import secrets

import pytest
from playwright.sync_api import Page

from pqsb import open_account


@pytest.fixture(scope="session", autouse=True)
def bank(base_url: str | None) -> str:
    if not base_url:
        pytest.exit("no bank to test: `just test`, or `just e2e URL` for a running one")
    return base_url


@pytest.fixture
def serial() -> str:
    return f"pqsb1q{secrets.token_hex(8)}"


@pytest.fixture
def client(page: Page, serial: str) -> str:
    open_account(page, serial)
    return serial
