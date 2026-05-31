#!/usr/bin/env python3
"""Simple TCP client for the C++ server (port 2811).

Creates a new connection for each input line (server accepts one connection,
handles a single request, then closes the connection).
"""
import socket
import sys

HOST = "127.0.0.1"
PORT = 2811


def send_and_receive(message: str, timeout: float = 5.0) -> str:
    """Send a message to the server and return the response as a string."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(timeout)
        s.connect((HOST, PORT))
        s.sendall(message.encode())
        # server writes a small reply; read up to 4096 bytes
        data = s.recv(4096)
    return data.decode(errors="replace")


def main() -> None:
    print(f"Simple client -> {HOST}:{PORT} (creates a new connection per message)")
    try:
        while True:
            try:
                line = input("> ")
            except EOFError:
                print()
                break

            if not line:
                # skip empty lines
                continue

            try:
                resp = send_and_receive(line)
                print(resp)
            except ConnectionRefusedError:
                print("Connection refused. Is the C++ server running?", file=sys.stderr)
            except socket.timeout:
                print("Timed out waiting for server response.", file=sys.stderr)
            except Exception as e:
                print("Error:", e, file=sys.stderr)
    except KeyboardInterrupt:
        print("\nExiting.")


if __name__ == "__main__":
    main()

