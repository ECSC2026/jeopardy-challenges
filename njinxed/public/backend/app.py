import os
import secrets

from flask import Flask, Response, abort, request

app = Flask(__name__)

FLAG = os.environ.get("FLAG", "ecsc{fake_flag_for_local_testing}")

DOCUMENTS = {
    "handbook": "Employee handbook (public).",
    "faq": "Frequently asked questions (public).",
    "ops/console": "Internal ops console\n\n" + FLAG,
}

PUBLIC_DOCS = ("handbook", "faq")

VAULT_KEY = secrets.token_hex(8)

FOOTER = '<hr><small>Internal use only</small>\n'

PAGE = """<html>
<head><title>Welcome Portal</title></head>
<body>
<h1>Welcome Portal</h1>
%s
<!--#include virtual="/fragments/footer" -->
</body>
</html>
"""


@app.route("/")
def home():
    body = (
        "<h2>Documents</h2><ul>"
        '<li><a href="/portal/docs/handbook">Employee handbook</a></li>'
        '<li><a href="/portal/docs/faq">FAQ</a></li>'
        "</ul>"
        '<form action="/portal/search"><input name="q" placeholder="search documents">'
        "</form>"
    )
    return Response(PAGE % body, mimetype="text/html")


@app.route("/fragments/footer")
def fragment_footer():
    return Response(FOOTER, mimetype="text/html")


@app.route("/portal/search")
def portal_search():
    ## TODO: mock implementation; add search feature later...
    q = request.args.get("q", "")
    return Response(PAGE % ("<p>No documents matched %s</p>" % q), mimetype="text/html")


@app.route("/portal/docs/<path:name>")
def portal_docs(name):
    if name not in PUBLIC_DOCS:
        abort(404)
    return Response(DOCUMENTS[name] + "\n", mimetype="text/plain")


@app.route("/internal/docs/<path:name>")
def internal_docs(name):
    if request.headers.get("X-Edge") == "external":
        abort(403, "the internal API does not serve external requests")

    for bad in ("/", ".."):
        if bad in name:
            abort(403, "invalid document name")

    resp = Response()
    resp.headers["X-Accel-Redirect"] = "/vault/%s/%s" % (VAULT_KEY, name)
    return resp


@app.route("/vault/<key>/<path:name>")
def vault(key, name):
    if key != VAULT_KEY:
        abort(404)

    doc = DOCUMENTS.get(name)
    if doc is None:
        abort(404)

    return Response(doc + "\n", mimetype="text/plain")


@app.route("/healthz")
def healthz():
    return "ok"
