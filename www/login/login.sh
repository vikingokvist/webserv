#!/usr/bin/bash

# Escape HTML special characters
html_escape() {
    echo "$1" | sed \
        -e 's/&/\&amp;/g' \
        -e 's/</\&lt;/g' \
        -e 's/>/\&gt;/g' \
        -e 's/"/\&quot;/g' \
        -e "s/'/\&#39;/g"
}

# URL decode (Bash-specific)
url_decode() {
    local data="$1"
    printf '%b' "${data//%/\\x}"
}

# Parse query string into variables
parse_qs() {
    local qs="$1"
    local key value
    IFS='&' 
    for pair in $qs; do
        key=$(echo "$pair" | cut -d= -f1)
        value=$(echo "$pair" | cut -d= -f2-)
        value=$(url_decode "$value")
        eval "$key=\"\$value\""
    done
    unset IFS
}

# Read POST data if necessary
method=$(echo "$REQUEST_METHOD" | tr '[:lower:]' '[:upper:]')
post_data=""
if [ "$method" = "POST" ]; then
    if [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
        post_data=$(dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null)
    fi
else
    post_data="$QUERY_STRING"
fi

# Parse POST/GET data
parse_qs "$post_data"

# Set defaults
[ -z "$username" ] && username="Anonymous"
[ -z "$password" ] && password="Unknown"

safe_username=$(html_escape "$username")
safe_password=$(html_escape "$password")

# Generate HTML body
html_body="<!DOCTYPE html>
<html lang=\"en\">
<head>
<meta charset=\"UTF-8\">
<title>$safe_username's Profile</title>
<style>
body { font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, Helvetica, Arial, sans-serif; background-color: #ffffff; color: #000000; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
.top-bar { position: absolute; top: 0; width: 100%; background-color: #000000; color: #ffffff; display: flex; align-items: center; padding: 10px 20px; box-sizing: border-box; }
.back-button { color: #ffffff; text-decoration: none; font-weight: 600; margin-right: 20px; }
.title { font-size: 18px; }
.container { text-align: center; padding: 40px; border: 1px solid #e0e0e0; border-radius: 20px; box-shadow: 0 4px 20px rgba(0,0,0,0.05); width: 350px; }
.avatar { width: 100px; height: 100px; border-radius: 50%; object-fit: cover; display: block; margin: 0 auto 20px; border: 2px solid #e0e0e0; }
.info { margin: 10px 0; font-size: 16px; }
</style>
</head>
<body>
<header class=\"top-bar\">
<a href=\"/index.html\" class=\"back-button\">← Back</a>
<h1 class=\"title\">$safe_username's Profile</h1>
</header>
<main class=\"container\">
<img src=\"avatar.png\" alt=\"Avatar\" class=\"avatar\">
<p class=\"info\"><strong>Username:</strong> $safe_username</p>
<p class=\"info\"><strong>Password:</strong> $safe_password</p>
</main>
</body>
</html>"

# Output CGI headers
printf "Content-Type: text/html\r\n"
printf "Content-Length: %s\r\n\r\n" "$(echo -n "$html_body" | wc -c)"

# Output body
printf "%s" "$html_body"
