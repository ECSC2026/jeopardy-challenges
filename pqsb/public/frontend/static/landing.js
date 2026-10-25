"use strict";

(() => {
  const held = PQSB.serial();

  function chrome(balance) {
    const chip =
      balance === undefined
        ? `<a class="chip" href="#key">Login</a>`
        : `<a class="chip" href="/assets">Balance ${PQSB.chf(balance)}</a>`;
    document.getElementById("chrome").innerHTML = PQSB.header("/", chip);
  }

  chrome();
  document.getElementById("foot").innerHTML = PQSB.footer() + PQSB.legal(false);

  if (held) {
    PQSB.api("/api/balance")
      .then((body) => chrome(body.balance))
      .catch((err) => {
        if (err.status === 401) PQSB.clearSerial();
      });
  }
  document.getElementById("startDeposit").textContent = PQSB.chf(PQSB.START_BALANCE);

  const field = document.getElementById("serial");
  const notice = document.getElementById("keyNotice");
  const enterBtn = document.getElementById("enter");
  field.value = held || PQSB.newSerial();

  document.getElementById("roll").addEventListener("click", () => {
    field.value = PQSB.newSerial();
    notice.classList.add("hidden");
    field.focus();
  });

  function complain(text) {
    notice.textContent = text;
    notice.classList.remove("hidden");
  }

  async function enter() {
    const value = field.value.trim();
    if (!PQSB.validSerial(value)) {
      complain(
        value
          ? `A serial is at most ${PQSB.SERIAL_MAX} characters.`
          : "Quote a serial number, or roll one.",
      );
      return;
    }
    enterBtn.disabled = true;
    PQSB.setSerial(value);
    try {
      await PQSB.api("/api/balance");
      const next = new URLSearchParams(window.location.search).get("next");
      window.location.href = next && next.startsWith("/") ? next : "/assets";
    } catch (err) {
      PQSB.clearSerial();
      complain(err.message);
      enterBtn.disabled = false;
    }
  }

  enterBtn.addEventListener("click", enter);
  field.addEventListener("keydown", (e) => {
    if (e.key === "Enter") enter();
  });
})();
