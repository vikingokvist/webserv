#!/usr/bin/env python3
import os
import sys
import html
from urllib.parse import parse_qs

def read_stdin_body():
    cl = os.environ.get("CONTENT_LENGTH")
    if cl:
        try:
            n = int(cl)
        except ValueError:
            return ""
        return sys.stdin.read(n)
    return ""

def get_params():
    # 1) GET params via QUERY_STRING
    qs = os.environ.get("QUERY_STRING", "")
    params = parse_qs(qs, keep_blank_values=True)

    # 2) If POST, parse body (application/x-www-form-urlencoded)
    if os.environ.get("REQUEST_METHOD", "").upper() == "POST":
        body = read_stdin_body()
        post_params = parse_qs(body, keep_blank_values=True)
        # POST should override/augment QUERY_STRING keys
        for k, v in post_params.items():
            params.setdefault(k, []).extend(v)
    return params

def first_param(params, *keys, default=""):
    for k in keys:
        if k in params and params[k]:
            return params[k][0]
    return default

def fake_profile_html(name):
    safe = html.escape(name or "Anonymous")
    return """<html>
  <head><title>Profile: {safe}</title></head>
  <body>
    <h1>Profile for {safe}</h1>
    <p>Age: 27</p>
    <p>Location: Barcelona</p>
    <p>Bio: This is a demo profile for <strong>{safe}</strong>.</p>
  </body>
</html>""".format(safe=safe)

def main():
    params = get_params()
    name = first_param(params, "name", "user", "username")
    body = fake_profile_html(name)

    # CGI must print headers first, then blank line, then body
    sys.stdout.write("Content-Type: text/html\r\n")
    sys.stdout.write("Content-Length: {}\r\n".format(len(body.encode("utf-8"))))
    sys.stdout.write("\r\n")
    sys.stdout.write(body)

if __name__ == "__main__":
    main()
