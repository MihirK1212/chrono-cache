class RespSerializer:
    @staticmethod
    def serialize_to_bulk_string(tokens: list[str]) -> bytes:
        parts = [f"*{len(tokens)}\r\n".encode()]
        for token in tokens:
            encoded = token.encode()
            parts.append(f"${len(encoded)}\r\n".encode())
            parts.append(encoded)
            parts.append(b"\r\n")
        return b"".join(parts)