"use strict";

const PQSB = (() => {
  const CHF_PER_CREDIT = 1000;
  const START_BALANCE = 200;
  const WITHDRAW_FEE = 1;
  const SERIAL_MAX = 64;
  const STORE = "pqsb.serial";

  const francs = (credits) => credits * CHF_PER_CREDIT;

  const amount = (credits) => francs(credits).toLocaleString("de-CH");

  const chf = (credits) => `CHF ${amount(credits)}`;

  const chfLines = (credits) => `CHF<br>${amount(credits)}`;

  const toCredits = (francsValue) => francsValue / CHF_PER_CREDIT;

  function newSerial() {
    const raw = new Uint8Array(16);
    crypto.getRandomValues(raw);
    const hex = [...raw].map((b) => b.toString(16).padStart(2, "0")).join("");
    return `pqsb1q${hex}`;
  }

  const validSerial = (s) => {
    const t = (s || "").trim();
    return t.length > 0 && t.length <= SERIAL_MAX;
  };

  const serial = () => {
    try {
      return localStorage.getItem(STORE);
    } catch {
      return null;
    }
  };

  function setSerial(value) {
    try {
      localStorage.setItem(STORE, value);
    } catch {}
  }

  function clearSerial() {
    try {
      localStorage.removeItem(STORE);
    } catch {}
  }

  const shortKey = (s) => {
    if (!s) return "—";
    return s.length <= 16 ? s : `${s.slice(0, 10)}…${s.slice(-2)}`;
  };

  async function api(path, payload) {
    const opts = { method: payload === undefined ? "GET" : "POST", headers: {} };
    const key = serial();
    if (key) opts.headers["Authorization"] = `Bearer ${key}`;
    if (payload !== undefined) {
      opts.headers["Content-Type"] = "application/json";
      opts.body = JSON.stringify(payload);
    }
    const res = await fetch(path, opts);
    const body = await res.json().catch(() => ({}));
    if (!res.ok) throw Object.assign(new Error(body.error || res.statusText), { status: res.status, body });
    return body;
  }

  async function buy(item) {
    const res = await fetch("/api/buy", {
      method: "POST",
      headers: { Authorization: `Bearer ${serial()}`, "Content-Type": "application/json" },
      body: JSON.stringify({ item }),
    });
    if (!res.ok) {
      const body = await res.json().catch(() => ({}));
      throw Object.assign(new Error(body.error || res.statusText), { status: res.status, body });
    }
    return {
      text: await res.text(),
      paid: Number(res.headers.get("X-Paid") || 0),
      balance: Number(res.headers.get("X-Balance") || 0),
    };
  }

  function download(filename, text, mime) {
    const url = URL.createObjectURL(new Blob([text], { type: mime || "text/plain" }));
    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 4000);
  }

  function crossCells() {
    const out = [];
    for (let row = 0; row < 7; row += 1) {
      for (let col = 0; col < 7; col += 1) {
        const arm = (col >= 2 && col <= 4 && row >= 1 && row <= 5) || (row >= 2 && row <= 4 && col >= 1 && col <= 5);
        if (arm) out.push(`<rect x="${col * 20}" y="${row * 20}" width="18" height="18" fill="currentColor"/>`);
      }
    }
    return out.join("");
  }

  const cross = (size) =>
    `<svg viewBox="0 0 138 138" width="${size}" height="${size}" aria-hidden="true">${crossCells()}</svg>`;

  const lockup = (tag, href) =>
    `<${tag}${href ? ` href="${href}"` : ""} class="lockup">${cross(44)}` +
    `<span class="lockup-text"><span class="lockup-pq">PQ</span>` +
    `<span class="lockup-sb">Swiss Bank</span></span></${tag}>`;

  const NAV = [
    ["/assets", "Assets for Sale"],
    ["/issuance", "Bond Issuance"],
    ["/redemption", "Bond Redemption"],
  ];

  function header(page, chipHtml) {
    const links = NAV.map(
      ([href, text]) => `<a href="${href}"${href === page ? ' class="here"' : ""}>${text}</a>`,
    ).join('<span class="pipe" aria-hidden="true">|</span>');
    const chip = chipHtml || `<a class="chip" href="/assets">Login</a>`;
    return (
      `<header class="site-header">${lockup(page === "/" ? "div" : "a", page === "/" ? "" : "/")}` +
      `<nav class="site-nav">${links}<span class="pipe" aria-hidden="true">|</span>${chip}</nav></header>`
    );
  }

  const LEGAL =
    `<div class="legal-bar"><span>&copy; 2026 PQ Swiss Bank AG, Zug. ` +
    `Not an offer to sell securities.</span>`;

  const legal = (back) =>
    LEGAL + (back ? `<a href="/">&larr; Back to site</a>` : `<span>Terms of Business</span>`) + `</div>`;

  function footer() {
    return (
      `<footer class="site-footer">` +
      `<div class="stack" style="max-width:26em">${lockup("div", "")}` +
      `<p>Digitally signed bearer bonds, issued from Zug.</p></div>` +
      `<div class="footer-col"><div class="footer-head">Client area</div>` +
      `<div class="footer-links">` +
      NAV.map(([href, text]) => `<a href="${href}">${text}</a>`).join("") +
      `</div></div>` +
      `<div class="footer-col"><div class="footer-head">The bank</div>` +
      `<div class="footer-links"><span>Vielgeldstrasse 12</span>` +
      `<span>FINMA licence 2030/1337</span><span>clients@pqsb.ch</span></div></div>` +
      `</footer>`
    );
  }

  let ticking = false;

  function statusBar() {
    const key = document.getElementById("keyId");
    if (key) key.textContent = shortKey(serial());

    const clock = document.getElementById("clock");
    if (!clock || ticking) return;
    ticking = true;
    const tick = () => {
      clock.textContent = new Date().toISOString().replace(/\.\d+Z$/, "Z");
    };
    tick();
    setInterval(tick, 1000);
  }

  function requireSerial() {
    const key = serial();
    if (!validSerial(key)) {
      window.location.href = `/?next=${encodeURIComponent(window.location.pathname)}`;
      return null;
    }
    return key;
  }

  const esc = (s) =>
    String(s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);

  return {
    CHF_PER_CREDIT,
    SERIAL_MAX,
    START_BALANCE,
    WITHDRAW_FEE,
    amount,
    api,
    buy,
    chf,
    chfLines,
    clearSerial,
    cross,
    download,
    esc,
    footer,
    francs,
    header,
    legal,
    newSerial,
    requireSerial,
    serial,
    setSerial,
    shortKey,
    statusBar,
    toCredits,
    validSerial,
  };
})();
