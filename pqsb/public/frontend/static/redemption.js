"use strict";

(() => {
  if (!PQSB.requireSerial()) return;

  const state = { note: null, principal: null, balance: 0 };
  const $ = (id) => document.getElementById(id);

  function chrome() {
    $("chrome").innerHTML = PQSB.header(
      "/redemption",
      `<span class="chip">Balance ${PQSB.chf(state.balance)}</span>`,
    );
    PQSB.statusBar();
  }

  function reject(why) {
    $("rejectWhy").textContent = why;
    $("reject").classList.remove("hidden");
    $("presented").classList.add("hidden");
    $("empty").classList.remove("hidden");
    $("fileName").textContent = "No file";
    state.note = null;
  }

  function refuse() {
    $("reject").classList.add("hidden");
    $("redeem").classList.add("muted");
    $("redeem").disabled = true;
  }

  function clear() {
    state.note = null;
    state.principal = null;
    $("reject").classList.add("hidden");
    $("presented").classList.add("hidden");
    $("empty").classList.remove("hidden");
    $("fileName").textContent = "No file";
    $("statusLine").textContent = "Nothing presented";
    $("pick").value = "";
  }

  const ROWS = [
    ["cStructure", "structure"],
    ["cSignature", "signature"],
    ["cKey", "key"],
    ["cNonce", "nonce"],
  ];

  function pending() {
    for (const [id] of ROWS) {
      const cell = $(id);
      cell.textContent = "pending";
      cell.classList.remove("fail");
    }
  }

  function marks(failed) {
    const stop = failed === null ? ROWS.length : ROWS.findIndex(([, k]) => k === failed);
    ROWS.forEach(([id], i) => {
      const cell = $(id);
      const verdict = i < stop ? "pass" : "fail";
      cell.textContent = verdict;
      cell.classList.toggle("fail", verdict === "fail");
    });
  }

  function claims(note) {
    try {
      const raw = note.split(".")[1].replace(/-/g, "+").replace(/_/g, "/");
      return JSON.parse(atob(raw + "=".repeat((4 - (raw.length % 4)) % 4)));
    } catch {
      return {};
    }
  }

  function readFile(text, name) {
    let note = null;
    let terms = {};
    const trimmed = text.trim();
    if (trimmed.startsWith("{")) {
      let parsed;
      try {
        parsed = JSON.parse(trimmed);
      } catch {
        return reject("Not readable as a signed object — the file is not valid JSON.");
      }
      if (typeof parsed.note !== "string") {
        return reject("Missing field: note. Not a PQSB bond.");
      }
      note = parsed.note;
      terms = parsed;
    } else if (trimmed.split(".").length === 3) {
      note = trimmed;
    } else {
      return reject("Not readable as a signed object — expected a .bond file or a bare note.");
    }

    state.note = note;
    state.principal =
      typeof terms.principal === "number" ? PQSB.toCredits(terms.principal) : null;
    $("reject").classList.add("hidden");
    $("fileName").textContent = name;
    $("presented").classList.remove("hidden");
    $("empty").classList.add("hidden");
    $("paid").classList.add("hidden");
    $("actions").classList.remove("hidden");
    $("redeem").classList.remove("muted");

    $("vIssued").textContent = terms.issued_at ? terms.issued_at.slice(0, 10) : "—";
    $("vKey").textContent = terms.issuer_key ? PQSB.shortKey(terms.issuer_key) : "—";
    $("vNonce").textContent = claims(note).nonce || "—";
    const sig = note.split(".")[2] || "";
    $("sigShort").textContent = `${sig.slice(0, 48)}…${sig.slice(-8)}`;

    pending();
    verify();
  }

  async function verify() {
    try {
      const body = await PQSB.api("/api/verify", { note: state.note });
      marks(body.failed ?? null);
      const credits = body.amount ?? state.principal;
      $("vPrincipal").innerHTML = credits === null ? "—" : PQSB.chfLines(credits);
      $("redeem").textContent =
        credits === null ? "Redeem →" : `Redeem ${PQSB.chf(credits)} →`;
      $("statusLine").textContent = body.ok
        ? "Verified, awaiting redemption"
        : "Refused";
      if (body.ok) {
        $("redeem").classList.remove("muted");
        $("redeem").disabled = false;
      } else {
        refuse();
      }
    } catch (err) {
      if (err.status === 401) {
        PQSB.clearSerial();
        window.location.href = "/";
        return;
      }
      reject(err.message);
    }
  }

  async function redeem() {
    try {
      const body = await PQSB.api("/api/deposit", { note: state.note });
      state.balance = body.balance;
      chrome();
      $("actions").classList.add("hidden");
      $("paid").classList.remove("hidden");
      $("paidLine").textContent = `${PQSB.chf(body.deposited)} paid to bearer.`;
      $("statusLine").textContent = "Paid";
      marks(null);
    } catch {
      refuse();
      $("statusLine").textContent = "Refused";
    }
  }

  const plate = $("plate");
  const stop = (e) => {
    e.preventDefault();
    e.stopPropagation();
  };
  plate.addEventListener("dragover", (e) => {
    stop(e);
    plate.classList.add("over");
  });
  plate.addEventListener("dragenter", (e) => {
    stop(e);
    plate.classList.add("over");
  });
  plate.addEventListener("dragleave", () => plate.classList.remove("over"));
  plate.addEventListener("drop", async (e) => {
    stop(e);
    plate.classList.remove("over");
    const file = e.dataTransfer.files[0];
    if (file) readFile(await file.text(), file.name);
  });

  $("pick").addEventListener("change", async (e) => {
    const file = e.target.files[0];
    if (file) readFile(await file.text(), file.name);
  });

  $("redeem").addEventListener("click", redeem);
  $("discard").addEventListener("click", clear);
  $("another").addEventListener("click", clear);

  $("foot").innerHTML = PQSB.legal(true);
  chrome();

  PQSB.api("/api/balance")
    .then((body) => {
      state.balance = body.balance;
      chrome();
    })
    .catch((err) => {
      if (err.status === 401) {
        PQSB.clearSerial();
        window.location.href = "/";
      }
    });
})();
