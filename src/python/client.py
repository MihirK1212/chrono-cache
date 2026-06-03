#!/usr/bin/env python3
"""Interactive TCP client for the ChronoCache C++ server (port 2811).

Wire format (matches server expectation):
  Request:  [4-byte big-endian uint32 length][<length> bytes of UTF-8 message]
  Response: [4-byte big-endian uint32 length][<length> bytes of UTF-8 message]

A single persistent connection is kept open for the session; the server's
event loop keeps connections alive and handles multiple requests per connection.
"""
import socket
import struct
import sys

HOST = "127.0.0.1"
PORT = 2811

# struct format for the 4-byte big-endian length header
_HDR = struct.Struct("!I")


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    """Read exactly n bytes from sock, raising EOFError on premature close."""
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise EOFError("Server closed the connection unexpectedly")
        buf += chunk
    return bytes(buf)


def send_request(sock: socket.socket, message: str) -> None:
    """Frame and send a single message."""
    payload = message.encode()
    header = _HDR.pack(len(payload))
    sock.sendall(header + payload)


def recv_response(sock: socket.socket) -> str:
    """Read one framed response and return it as a string."""
    header = _recv_exact(sock, _HDR.size)
    (length,) = _HDR.unpack(header)
    payload = _recv_exact(sock, length)
    return payload.decode(errors="replace")


def main() -> None:
    print(f"ChronoCache client -> {HOST}:{PORT}  (Ctrl-D or Ctrl-C to quit)")
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5.0)
            try:
                s.connect((HOST, PORT))
            except ConnectionRefusedError:
                print("Connection refused. Is the server running?", file=sys.stderr)
                sys.exit(1)

            print("Connected.\n")

            while True:
                try:
                    line = input("> ")
                except EOFError:
                    print()
                    break

                if not line:
                    continue

                try:
                    send_request(s, line)
                    resp = recv_response(s)
                    print(f"< {resp}")
                except EOFError as e:
                    print(f"Server closed connection: {e}", file=sys.stderr)
                    break
                except socket.timeout:
                    print("Timed out waiting for server response.", file=sys.stderr)
                except Exception as e:
                    print(f"Error: {e}", file=sys.stderr)
                    break

    except KeyboardInterrupt:
        print("\nExiting.")


if __name__ == "__main__":
    main()

