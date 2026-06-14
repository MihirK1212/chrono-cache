#!/usr/bin/env python3
import shlex
import socket
import struct
import sys

HOST = "127.0.0.1"
PORT = 2811

_HDR = struct.Struct("!I")


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise EOFError("Server closed the connection unexpectedly")
        buf += chunk
    return bytes(buf)


def to_resp(tokens: list[str]) -> bytes:
    parts = [f"*{len(tokens)}\r\n".encode()]
    for token in tokens:
        encoded = token.encode()
        parts.append(f"${len(encoded)}\r\n".encode())
        parts.append(encoded)
        parts.append(b"\r\n")
    return b"".join(parts)


def send_request(sock: socket.socket, resp: bytes) -> None:
    header = _HDR.pack(len(resp))
    sock.sendall(header + resp)


def recv_response(sock: socket.socket) -> bytes:
    (length,) = _HDR.unpack(_recv_exact(sock, _HDR.size))
    return _recv_exact(sock, length)


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
                    line = input("> ").strip()
                except EOFError:
                    print()
                    break

                if not line:
                    continue

                try:
                    tokens = shlex.split(line)
                except ValueError as e:
                    print(f"Parse error: {e}", file=sys.stderr)
                    continue

                resp_bytes = to_resp(tokens)
                print(f"  [sending RESP] {resp_bytes!r}")

                try:
                    send_request(s, resp_bytes)
                    raw = recv_response(s)
                    print(f"< {raw!r}")
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
