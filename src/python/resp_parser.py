from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Optional


class RespType(Enum):
    SimpleString = "SimpleString"
    Error = "Error"
    Integer = "Integer"
    BulkString = "BulkString"
    Array = "Array"
    Null = "Null"


@dataclass
class RespValue:
    type: RespType
    str_value: str = ""
    int_value: int = 0
    array: list["RespValue"] = field(default_factory=list)
    num_resp_bytes: Optional[int] = None
    is_null: bool = False

    @staticmethod
    def simple_string(s: str, num_resp_bytes: int) -> "RespValue":
        return RespValue(type=RespType.SimpleString, str_value=s, num_resp_bytes=num_resp_bytes)

    @staticmethod
    def error(msg: str, num_resp_bytes: Optional[int] = None) -> "RespValue":
        return RespValue(type=RespType.Error, str_value=msg, num_resp_bytes=num_resp_bytes)

    @staticmethod
    def integer(n: int, num_resp_bytes: int) -> "RespValue":
        return RespValue(type=RespType.Integer, int_value=n, num_resp_bytes=num_resp_bytes)

    @staticmethod
    def bulk_string(s: str, num_resp_bytes: int) -> "RespValue":
        return RespValue(type=RespType.BulkString, str_value=s, num_resp_bytes=num_resp_bytes)

    @staticmethod
    def null_bulk(num_resp_bytes: int) -> "RespValue":
        return RespValue(type=RespType.Null, is_null=True, num_resp_bytes=num_resp_bytes)

    @staticmethod
    def null_array(num_resp_bytes: int) -> "RespValue":
        return RespValue(type=RespType.Null, is_null=True, num_resp_bytes=num_resp_bytes)

    @staticmethod
    def array_from_parts(elements: list["RespValue"], num_resp_bytes: int) -> "RespValue":
        return RespValue(type=RespType.Array, array=elements, num_resp_bytes=num_resp_bytes)



def _find_crlf(buf: str) -> int:
    """Returns the index of the first '\\r\\n', or -1 if not found."""
    pos = buf.find("\r\n")
    return pos


def parse(buf: str | bytes) -> Optional[RespValue]:
    if isinstance(buf, (bytes, bytearray)):
        buf = buf.decode()

    if not buf:
        return None

    match buf[0]:
        case '+': return _parse_simple_string(buf)
        case '-': return _parse_error(buf)
        case ':': return _parse_integer(buf)
        case '$': return _parse_bulk_string(buf)
        case '*': return _parse_array(buf)
        case _:   return RespValue.error("ERR unknown RESP type byte")


def _parse_simple_string(buf: str) -> Optional[RespValue]:
    if buf[0] != '+':
        return RespValue.error("ERR invalid simple string format")

    pos = _find_crlf(buf)
    if pos == -1:
        return None

    value = buf[1:pos]
    return RespValue.simple_string(value, pos + 2)


def _parse_error(buf: str) -> Optional[RespValue]:
    if buf[0] != '-':
        return RespValue.error("ERR invalid error format")

    pos = _find_crlf(buf)
    if pos == -1:
        return None

    value = buf[1:pos]
    return RespValue.error(value, pos + 2)


def _parse_integer(buf: str) -> Optional[RespValue]:
    if buf[0] != ':':
        return RespValue.error("ERR invalid integer format")

    if len(buf) < 3:
        return None

    pos = _find_crlf(buf)
    if pos == -1:
        return None

    try:
        value = int(buf[1:pos])
    except ValueError:
        return RespValue.error("ERR invalid integer format")

    return RespValue.integer(value, pos + 2)


def _parse_bulk_string(buf: str) -> Optional[RespValue]:
    if buf[0] != '$':
        return RespValue.error("ERR invalid bulk string format")

    pos_1 = _find_crlf(buf)
    if pos_1 == -1:
        return None

    try:
        length = int(buf[1:pos_1])
    except ValueError:
        return RespValue.error("ERR invalid bulk string format")

    if length == -1:
        return RespValue.null_bulk(pos_1 + 2)

    if length < 0:
        return RespValue.error("ERR invalid bulk string format")

    data_start = pos_1 + 2
    total_bytes = data_start + length + 2

    if len(buf) < total_bytes:
        return None

    if buf[data_start + length] != '\r' or buf[data_start + length + 1] != '\n':
        return RespValue.error("ERR invalid bulk string format")

    value = buf[data_start:data_start + length]
    return RespValue.bulk_string(value, total_bytes)


def _parse_array(buf: str) -> Optional[RespValue]:
    if buf[0] != '*':
        return RespValue.error("ERR invalid array format")

    pos_1 = _find_crlf(buf)
    if pos_1 == -1:
        return None

    try:
        num_elements = int(buf[1:pos_1])
    except ValueError:
        return RespValue.error("ERR invalid array format")

    if num_elements == -1:
        return RespValue.null_array(pos_1 + 2)

    if num_elements < 0:
        return RespValue.error("ERR invalid array format")

    curr_pos = pos_1 + 2
    elements = []
    for _ in range(num_elements):
        element = parse(buf[curr_pos:])
        if element is None:
            return None
        if element.num_resp_bytes is None:
            return RespValue.error("ERR invalid array element")
        elements.append(element)
        curr_pos += element.num_resp_bytes

    return RespValue.array_from_parts(elements, curr_pos)
