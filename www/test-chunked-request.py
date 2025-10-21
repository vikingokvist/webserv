import socket
import time

HOST = '127.0.0.1'
PORT = 9999

# The chunks we want to send
chunks = [
    b"user=",
    b"jaime",
    b"&password=",
    b"1234567"
]

# Connect to the server
with socket.create_connection((HOST, PORT)) as sock:
    # Send the headers first
    headers = (
        "POST /login/login.py HTTP/1.1\r\n"
        "Host: 127.0.0.1:9999\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
    )
    sock.sendall(headers.encode())

    # Send each chunk
    for chunk in chunks:
        hex_len = f"{len(chunk):X}\r\n"
        sock.sendall(hex_len.encode())
        sock.sendall(chunk)
        sock.sendall(b"\r\n")
        print(f"Sent chunk: {chunk}")
        time.sleep(1)  # optional delay to simulate slow sending

    # Send the terminating chunk
    sock.sendall(b"0\r\n\r\n")
    print("Sent terminating chunk.")

    # Read the response
    response = sock.recv(4096)
    print("Response from server:")
    print(response.decode())

