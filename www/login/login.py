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

    # Get the "username" and "password" parameters
    username = params.get("username", ["Anonymous"])[0]
    password = params.get("password", ["Unknown"])[0]
    safe_username = html.escape(username)
    safe_password = html.escape(password)

    # Generate HTML page
    html_body = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>{safe_username}'s Profile</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background-color: #ffffff;
            color: #000000;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }}
        .top-bar {{
            position: absolute;
            top: 0;
            width: 100%;
            background-color: #000000;
            color: #ffffff;
            display: flex;
            align-items: center;
            padding: 10px 20px;
            box-sizing: border-box;
        }}
        .back-button {{
            color: #ffffff;
            text-decoration: none;
            font-weight: 600;
            margin-right: 20px;
        }}
        .title {{
            font-size: 18px;
        }}
        .container {{
            text-align: center;
            padding: 40px;
            border: 1px solid #e0e0e0;
            border-radius: 20px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.05);
            width: 350px;
        }}
        .avatar {{
            width: 100px;
            height: 100px;
            border-radius: 50%;
            object-fit: cover;
            display: block;
            margin: 0 auto 20px;
            border: 2px solid #e0e0e0;
        }}
        .info {{
            margin: 10px 0;
            font-size: 16px;
        }}
    </style>
</head>
<body>
    <header class="top-bar">
        <a href="/index.html" class="back-button">← Back</a>
        <h1 class="title">{safe_username}'s Profile</h1>
    </header>

    <main class="container">
        <img src="images/avatar.png" alt="Avatar" class="avatar">
        <p class="info"><strong>Username:</strong> {safe_username}</p>
        <p class="info"><strong>Password:</strong> {safe_password}</p>
    </main>
</body>
</html>"""

    # CGI headers + body
    print("Content-Type: text/html")
    print()
    print(html_body)

if __name__ == "__main__":
    main()
