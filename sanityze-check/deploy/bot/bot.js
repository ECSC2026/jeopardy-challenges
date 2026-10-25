process.env.HOME = "/tmp";

const net = require("net");
const puppeteer = require("puppeteer");

const FLAG = process.env.FLAG || "ECSC{fakeflag}";
const CHALLENGE_URL = process.env.CHALLENGE_URL || "http://127.0.0.1:8080";
const PORT = 55555;

const delay = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

function log(socket, message) {
	console.log(message);
	if (!socket.destroyed && socket.writable) socket.write(`${message}\n`);
}

async function goto(url, socket) {
	const browser = await puppeteer.launch({
		browser: "chrome",
		headless: true,
		acceptInsecureCerts: true,
		executablePath: process.env.CHROMIUM_BIN || "/usr/bin/chromium-browser",
		args: [
			"--no-sandbox",
			"--disable-dev-shm-usage",
			"--disable-gpu",
			"--js-flags=--jitless"
		]
	});

	try {
		const session = await browser.target().createCDPSession();
		session.on("Target.targetCrashed", () => log(socket, "Error: Page crashed!"));
		await session.send("Target.setDiscoverTargets", { discover: true });

		const [page] = await browser.pages();
		page.setDefaultNavigationTimeout(5000);

		try { await page.goto(`${CHALLENGE_URL}/?action=add&content=${encodeURIComponent(FLAG)}`); } catch {}
		await delay(1000);

		try { await page.goto(url); } catch {}
		await delay(5000);
	} finally {
		await browser.close();
	}
}

async function handleUrl(socket, url) {
	if (!url || !(url.startsWith("http://") || url.startsWith("https://"))) {
		log(socket, "ERROR: URL must start with http:// or https://");
		return;
	}
	try { await goto(url, socket); } catch {}
}

function handleSocket(socket) {
	let buffer = "";
	let started = false;

	const start = () => {
		if (started) return;
		started = true;

		const url = buffer.split(/\r?\n/)[0].trim();
		handleUrl(socket, url).finally(() => {
			if (!socket.destroyed && socket.writable) {
				socket.end();
			}
		});
	};

	socket.on("data", (data) => {
		buffer += data.toString();
		if (buffer.includes("\n")) start();
	});

	socket.on("end", start);
	socket.on("error", () => {});
}

net.createServer({ allowHalfOpen: true }, handleSocket).listen(PORT, "0.0.0.0");
