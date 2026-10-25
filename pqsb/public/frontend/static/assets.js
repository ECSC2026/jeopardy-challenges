"use strict";

(() => {
  const state = { items: [], balance: 0, pick: null };
  const guest = () => !PQSB.serial();
  const LOGIN = `/?next=${encodeURIComponent("/assets")}#key`;
  const $ = (id) => document.getElementById(id);

  function chrome() {
    const chip = guest()
      ? `<a class="chip" href="${LOGIN}">Login</a>`
      : `<span class="chip">Balance ${PQSB.chf(state.balance)}</span>`;
    $("chrome").innerHTML = PQSB.header("/assets", chip);
    PQSB.statusBar();
  }

  function renderTable() {
    const table = $("table");
    while (table.children.length > 4) table.removeChild(table.lastChild);

    $("statusLine").textContent = `${state.items.length} assets listed`;

    for (const row of state.items) {
      const picked = state.pick === row.item;
      const tint = picked ? " row-picked" : "";
      const cell = (cls, html) => {
        const d = document.createElement("div");
        d.className = cls + tint;
        d.innerHTML = html;
        return d;
      };
      table.appendChild(cell("td", PQSB.esc(row.item)));
      table.appendChild(cell("td td-about", PQSB.esc(row.desc)));
      table.appendChild(cell("td td-price", PQSB.chf(row.price)));

      const act = document.createElement("div");
      act.className = "td-act" + tint;
      const btn = document.createElement("button");
      btn.type = "button";
      btn.className = "buy" + (picked || row.owned ? " on" : "");
      btn.textContent = row.owned ? "Download" : picked ? "Selected" : "Buy";
      btn.addEventListener("click", () => {
        if (guest()) {
          window.location.href = LOGIN;
        } else if (row.owned) {
          settle(row);
        } else {
          select(row.item);
        }
      });
      act.appendChild(btn);
      table.appendChild(act);
    }
  }

  function select(item) {
    state.pick = item;
    renderTable();
    renderTicket();
    $("ticket").scrollIntoView({ behavior: "smooth", block: "nearest" });
  }

  function renderTicket() {
    const row = state.items.find((i) => i.item === state.pick);
    if (!row) {
      $("ticket").classList.add("hidden");
      return;
    }
    const cost = row.price;
    const short = state.balance < cost;

    $("ticket").classList.remove("hidden");
    $("tFilename").textContent = row.item;
    $("tAbout").textContent = `${row.desc} Listed at ${PQSB.chf(row.price)}.`;
    $("tCons").innerHTML = PQSB.chfLines(cost);
    $("tBal").innerHTML = PQSB.chfLines(state.balance);
    $("tAfter").innerHTML = short ? "—" : PQSB.chfLines(state.balance - cost);
    $("tHeld").textContent = String(state.items.filter((i) => i.owned).length);

    const notice = $("tNotice");
    notice.classList.toggle("hidden", !short);
    if (short) {
      notice.textContent =
        `The consideration exceeds the deposit on this key. ` +
        `${PQSB.chf(cost)} against ${PQSB.chf(state.balance)} available.`;
    }
    $("settle").disabled = short;
  }

  async function settle(item) {
    item ||= state.items.find((row) => row.item === state.pick);
    if (!item) return;
    try {
      const got = await PQSB.buy(item.item);
      state.balance = got.balance;
      item.owned = true;

      PQSB.download(item.item, got.text);
      chrome();

      state.pick = null;
      $("ticket").classList.add("hidden");
      renderTable();
    } catch (err) {
      const notice = $("tNotice");
      notice.textContent = err.message;
      notice.classList.remove("hidden");
    }
  }

  $("settle").addEventListener("click", () => settle());
  $("cancel").addEventListener("click", () => {
    state.pick = null;
    $("ticket").classList.add("hidden");
    renderTable();
  });
  $("foot").innerHTML = PQSB.legal(true);
  chrome();

  PQSB.api("/api/shop")
    .then((body) => {
      state.items = body.catalogue;
      state.balance = body.balance ?? 0;
      chrome();
      renderTable();
    })
    .catch((err) => {
      if (err.status === 401) {
        PQSB.clearSerial();
        chrome();
        PQSB.api("/api/shop").then((body) => {
          state.items = body.catalogue;
          renderTable();
        });
      }
    });
})();
