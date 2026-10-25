# sanityze-check solver

The intended solution involves using a tab-crashing input (e.g. SVG)
to build a crash oracle and do a cross-site-search based on that.

Searches show the 10 newest notes by default, but both order and limit
are configurable. The reference solver requests `limit=1&sort=oldest`,
so a correct prefix renders the older flag while an incorrect prefix
renders the newer, attacker-controlled crashing note.

For local testing, start `public/compose.yml` and run `./solve.py` from this
directory. The public deployment uses `ECSC{fakeflag}`.

A crashing input can easily be constructed from the Chromium issue
tracker, e.g. this one:

```html
<svg><text textLength=43>\nbase\n<tspan display=ruby-text>\nannotation
```

from https://issues.chromium.org/issues/438986656.
