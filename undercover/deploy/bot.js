const fs = require('fs');
const https = require('https');
const path = require('path');
const express = require('express');
const rateLimit = require('express-rate-limit');
const { chromium } = require('playwright');

const PORT = parseInt(process.env.PORT || '1337', 10);
const CERTS = process.env.CERTS || '/certs';
const SAFEHOUSE_HOST = 'safehouse.internal';
const TIMEOUT_MS = parseInt(process.env.TIMEOUT_MS || '5000', 10);
const FLAG = process.env.FLAG || 'ECSC{fake_flag}';

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function startSafehouse() {
  const opts = {
    key: fs.readFileSync(path.join(CERTS, 'signer.key')),
    cert: fs.readFileSync(path.join(CERTS, 'signer.crt')),
  };
  const srv = https.createServer(opts, (req, res) => {
    if (req.url.startsWith('/flag')) {
      res.writeHead(200, { 'content-type': 'text/plain' });
      return res.end(FLAG);
    }
    res.writeHead(200, { 'content-type': 'text/html' });
    res.end('<!doctype html><meta charset=utf-8><title>Safehouse</title>' +
      '<h1>Safehouse (internal)</h1><p>The flag lives at <code>/flag</code></p>');
  });
  srv.listen(443, '127.0.0.1', () => console.log(`[safehouse] https://${SAFEHOUSE_HOST}/ -> 127.0.0.1:443`));
}

async function visit(url) {
  const browser = await chromium.launch({
    headless: true,
    pipe: true,
    args: [
      '--disable-extensions',
      '--disable-gpu',
      '--disable-software-rasterizer',
      '--js-flags=--noexpose_wasm,--jitless',
      '--no-sandbox',
      `--host-resolver-rules=MAP ${SAFEHOUSE_HOST} 127.0.0.1`,
    ],
  });
  const context = await browser.newContext();
  try {
    const page = await context.newPage();
    page.on('console', (m) => console.log('[browser]', m.text()));
    await page.goto(url, { timeout: 5000, waitUntil: 'load' }).catch(() => { });
    await sleep(TIMEOUT_MS);
  } finally {
    await context.close();
    await browser.close();
  }
}

const app = express();
app.use(express.json({ limit: '8kb' }));


const reportLimiter = rateLimit({
  windowMs: 1 * 60 * 1000, // 1 minute
  max: 2,
  message: { error: 'Too many requests, please try again later.' }
});

app.post('/report', reportLimiter, async (req, res) => {
  const { url } = req.body || {};
  if (typeof url !== 'string' || !/^https?:\/\/./.test(url)) {
    return res.status(400).json({ error: 'URL must be a string starting with http(s)://' });
  }
  console.log('[bot] visiting:', url);
  try {
    await visit(url);
    res.json({ success: true });
  } catch (err) {
    console.error('[bot] error:', err.message);
    res.status(500).json({ error: 'Failed to visit url!' });
  }
});


app.get('/ca.crt', (_req, res) => {
  res.type('application/x-pem-file');
  res.setHeader('Content-Disposition', 'attachment; filename="ca.crt"');
  res.sendFile('ca.crt', { root: CERTS });
});

app.get('/', (_req, res) => res.sendFile('bot.html', { root: __dirname }));

startSafehouse();
app.listen(PORT, () => console.log(`[bot] listening on http://0.0.0.0:${PORT}`));
