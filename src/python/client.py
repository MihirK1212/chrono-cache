import socket
import struct
import shlex

import resp_parser
from resp_serializer import RespSerializer
from typing import Any, Optional, Union

RespResult = Union[str, int, list[Any], None]

class ChronoCacheClient:
    _HDR = struct.Struct("!I")

    def __init__(self, host: str, port: int, timeout: Optional[float] = None):
        self.host = host
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if timeout is not None:
            self.socket.settimeout(timeout)
        self.socket.connect((self.host, self.port))

    def close(self) -> None:
        self.socket.close()

    def __enter__(self) -> "ChronoCacheClient":
        return self

    def __exit__(self, *args: Any) -> None:
        self.close()

    def ping(self, message: Optional[str] = None) -> str:
        tokens = ["PING"] if message is None else ["PING", message]
        return self._request_str(tokens)

    def init(self, with_replay: bool) -> str:
        return self._request_str(["INIT", "true" if with_replay else "false"])

    def get(self, key: str) -> Optional[str]:
        return self._request_optional_str(["GET", key])

    def set(self, key: str, value: str, ttl_ms: Optional[int] = None) -> str:
        if ttl_ms is not None:
            return self._request_str(["SET", key, value, "EX", str(ttl_ms)])
        return self._request_str(["SET", key, value])

    def delete(self, *keys: str) -> int:
        if not keys:
            raise ValueError("delete() requires at least one key")
        return self._request_int(["DEL", *keys])

    def expire(self, key: str, ttl_ms: int) -> int:
        return self._request_int(["EXPIRE", key, str(ttl_ms)])

    def persist(self, key: str) -> int:
        return self._request_int(["PERSIST", key])

    def pttl(self, key: str) -> int:
        return self._request_int(["PTTL", key])

    def zadd(self, key: str, score: float, member: str) -> int:
        return self._request_int(["ZADD", key, str(score), member])

    def zscore(self, key: str, member: str) -> Optional[float]:
        result = self._request_optional_str(["ZSCORE", key, member])
        return float(result) if result is not None else None

    def zrem(self, key: str, member: str) -> int:
        return self._request_int(["ZREM", key, member])

    def zrank(self, key: str, member: str) -> Optional[int]:
        return self._request_optional_int(["ZRANK", key, member])

    def raw_request(self, request: str) -> RespResult:
        return self._request(shlex.split(request))

    def _request_str(self, tokens: list[str]) -> str:
        result = self._request(tokens)
        if not isinstance(result, str):
            raise ValueError(f"Expected str response, got {type(result).__name__}")
        return result

    def _request_int(self, tokens: list[str]) -> int:
        result = self._request(tokens)
        if not isinstance(result, int):
            raise ValueError(f"Expected int response, got {type(result).__name__}")
        return result

    def _request_optional_str(self, tokens: list[str]) -> Optional[str]:
        result = self._request(tokens)
        if result is not None and not isinstance(result, str):
            raise ValueError(f"Expected str or null response, got {type(result).__name__}")
        return result

    def _request_optional_int(self, tokens: list[str]) -> Optional[int]:
        result = self._request(tokens)
        if result is not None and not isinstance(result, int):
            raise ValueError(f"Expected int or null response, got {type(result).__name__}")
        return result

    def _request(self, tokens: list[str]) -> RespResult:
        payload = RespSerializer.serialize_to_bulk_string(tokens)
        self._send_request(payload)
        response = self._recv_response()
        resp_value = resp_parser.parse(response)
        return self._get_value_from_resp_value(resp_value)

    def _send_request(self, req: bytes) -> None:
        if not isinstance(req, bytes):
            raise ValueError("req must be bytes")
        header = self._HDR.pack(len(req))
        self.socket.sendall(header + req)

    def _recv_response(self) -> bytes:
        def _recv_exact(n: int) -> bytes:
            buf = bytearray()
            while len(buf) < n:
                chunk = self.socket.recv(n - len(buf))
                if not chunk:
                    raise EOFError("Server closed the connection unexpectedly")
                buf += chunk
            return bytes(buf)

        (length,) = self._HDR.unpack(_recv_exact(self._HDR.size))
        return _recv_exact(length)

    def _get_value_from_resp_value(self, resp_value: resp_parser.RespValue) -> RespResult:
        if resp_value is None:
            raise ValueError("Invalid response from server")
        if resp_value.type == resp_parser.RespType.SimpleString:
            return resp_value.str_value
        if resp_value.type == resp_parser.RespType.Error:
            raise ValueError(resp_value.str_value)
        if resp_value.type == resp_parser.RespType.BulkString:
            return resp_value.str_value
        if resp_value.type == resp_parser.RespType.Integer:
            return int(resp_value.int_value)
        if resp_value.type == resp_parser.RespType.Array:
            return [self._get_value_from_resp_value(token) for token in resp_value.array]
        if resp_value.type == resp_parser.RespType.Null:
            return None
        raise ValueError("Invalid response from server")
