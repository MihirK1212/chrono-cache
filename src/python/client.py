import socket
import struct
import shlex

import resp_parser
from resp_serializer import RespSerializer
from typing import Any


class ChronoCacheClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.host, self.port))

    _HDR = struct.Struct("!I")

    def raw_request(self, request: str) -> Any:
        tokens = shlex.split(request)
        resp_request = RespSerializer.serialize_to_bulk_string(tokens)
        self._send_request(resp_request)
        
        response = self._recv_response()
        resp_value = resp_parser.parse(response)
        
        return self._get_value_from_resp_value(resp_value)

    def init(self, with_replay: bool):
        with_replay_str = "true" if with_replay else "false"
        return self.raw_request(f"INIT {with_replay_str}")

    def get(self, key: str):
        return self.raw_request(f"GET {key}")

    def set(self, key: str, value: str):
        return self.raw_request(f"SET {key} {value}")

    def delete(self, key: str):
        return self.raw_request(f"DEL {key}")

    def expire(self, key: str, seconds: int):
        return self.raw_request(f"EXPIRE {key} {seconds}")

    def persist(self, key: str):
        return self.raw_request(f"PERSIST {key}")

    def pttl(self, key: str):
        return self.raw_request(f"PTTL {key}")

    def zadd(self, key: str, score: float, member: str):
        return self.raw_request(f"ZADD {key} {score} {member}")

    def zscore(self, key: str, member: str):
        return self.raw_request(f"ZSCORE {key} {member}")

    def zrem(self, key: str, member: str):
        return self.raw_request(f"ZREM {key} {member}")

    def zrank(self, key: str, member: str):
        return self.raw_request(f"ZRANK {key} {member}")

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


    def _get_value_from_resp_value(self, resp_value: resp_parser.RespValue) -> Any:
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