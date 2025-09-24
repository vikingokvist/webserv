#!/usr/bin/env python3
import os
import sys
import html
from urllib.parse import parse_qs

def main():
    # Get method
    method = os.environ.get("REQUEST_METHOD", "GET").upper()
    
    # Read POST data from stdin if POST
    post_data = ""
    if method == "POST":
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        if content_length > 0:
            post_data = sys.stdin.read(content_length)

    # Parse the data into a dictionary
    params = parse_qs(post_data)

    # Get the "user" parameter
    name = params.get("user", ["Anonymous"])[0]
    password = params.get("password", ["Unknown"])[0]
    safe_name = html.escape(name)
    safe_password = html.escape(password)

    # Generate HTML page
    html_body = f"""<html>
<head>
<title>{safe_name}'s Profile</title>
<style>
body {{
    font-family: Arial, sans-serif;
    background: #f0f2f5;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
    margin: 0;
}}
.profile-card {{
    background: #fff;
    padding: 30px;
    border-radius: 10px;
    box-shadow: 0 4px 12px rgba(0,0,0,0.1);
    width: 350px;
    text-align: center;
}}
.profile-card h1 {{
    margin-bottom: 10px;
    color: #333;
}}
.profile-card p {{
    color: #555;
    font-size: 14px;
}}
.profile-avatar {{
    width: 80px;
    height: 80px;
    border-radius: 50%;
    background: #ccc;
    display: block;
    margin: 0 auto 15px;
}}
</style>
</head>
<body>
<div class="profile-card">
    <div class="profile-avatar"></div>
    <h1>{safe_name}</h1>
    <p>Your password is: {safe_password}</p>
</div>
</body>
</html>"""

    # CGI headers + body
    print("Content-Type: text/html")
    print(f"Content-Length: {len(html_body.encode('utf-8'))}")
    print()
    print(html_body)

if __name__ == "__main__":
    main()
