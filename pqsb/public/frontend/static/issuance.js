"use strict";

(() => {
  if (!PQSB.requireSerial()) return;

  const state = { balance: 0, fee: PQSB.WITHDRAW_FEE, bond: null, filename: "" };
  const $ = (id) => document.getElementById(id);

  function chrome() {
    $("chrome").innerHTML = PQSB.header(
      "/issuance",
      `<span class="chip">Balance ${PQSB.chf(state.balance)}</span>`,
    );
    PQSB.statusBar();
  }

  function typed() {
    const raw = $("amount").value.replace(/['\s]/g, "");
    if (!raw) return null;
    if (!/^\d+$/.test(raw)) return NaN;
    return Number(raw);
  }

  function refresh() {
    const francs = typed();

    $("fee").innerHTML = PQSB.chfLines(state.fee);

    let reason = "";
    if (francs === null) {
      reason = "Enter a principal.";
    } else if (Number.isNaN(francs) || francs <= 0) {
      reason = "A principal is a whole number of francs.";
    } else if (francs % PQSB.CHF_PER_CREDIT !== 0) {
      reason = `Bonds are issued in multiples of a thousand.`;
    } else if (PQSB.toCredits(francs) + state.fee > state.balance) {
      reason = `Insufficient funds.`;
    }

    const ok = reason === "";
    if (ok) {
      const credits = PQSB.toCredits(francs);
      $("principal").innerHTML = PQSB.chfLines(credits);
      $("drawn").innerHTML = PQSB.chfLines(credits + state.fee);
      $("after").innerHTML = PQSB.chfLines(state.balance - credits - state.fee);
    } else {
      $("principal").textContent = "—";
      $("drawn").textContent = "—";
      $("after").textContent = "—";
    }

    const notice = $("notice");
    notice.textContent = ok
      ? `Issuance: ${PQSB.chf(PQSB.toCredits(francs) + state.fee)}.`
      : reason;
    notice.classList.remove("hidden");
    $("sign").disabled = !ok;
  }

  function bondFile(note, francs) {
    return {
      instrument: "PQ Swiss Bank bearer bond",
      principal: francs,
      currency: "CHF",
      issued_at: new Date().toISOString(),
      issuer_key: PQSB.serial(),
      note,
    };
  }

  async function sign() {
    const francs = typed();
    try {
      const body = await PQSB.api("/api/withdraw", { amount: PQSB.toCredits(francs) });
      state.balance = body.balance;

      const bond = bondFile(body.note, francs);
      state.bond = JSON.stringify(bond, null, 2);
      state.filename = `PQSB-CHF-${francs}.bond`;

      $("payload").textContent = state.bond;
      $("signed").classList.remove("hidden");
      $("unsigned").classList.add("hidden");
      $("objectStatus").textContent = PQSB.chf(PQSB.toCredits(francs));
      $("statusLine").textContent = `Signed ${PQSB.chf(PQSB.toCredits(francs))}`;
      PQSB.download(state.filename, state.bond, "application/json");

      chrome();
      refresh();
    } catch (err) {
      const notice = $("notice");
      notice.textContent = err.message;
      notice.classList.remove("hidden");
      $("sign").disabled = true;
    }
  }

  $("amount").addEventListener("input", refresh);
  $("sign").addEventListener("click", sign);
  $("again").addEventListener("click", () => {
    if (state.bond) PQSB.download(state.filename, state.bond, "application/json");
  });

  $("foot").innerHTML = PQSB.legal(true);
  chrome();
  refresh();

  PQSB.api("/api/balance")
    .then((body) => {
      state.balance = body.balance;
      chrome();
      refresh();
    })
    .catch((err) => {
      if (err.status === 401) {
        PQSB.clearSerial();
        window.location.href = "/";
      }
    });
})();
