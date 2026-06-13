#include "resp_serializer.h"

static const std::string CRLF = "\r\n";

std::string RespSerializer::serialize(const RespValue& value) {
    if (value.is_null) {
        if (value.type == RespType::Array) {
            return "*-1" + CRLF;
        }
        return "$-1" + CRLF;
    }

    switch (value.type) {
        case RespType::SimpleString:
            return "+" + value.str_value + CRLF;
        case RespType::Error:
            return "-" + value.str_value + CRLF;
        case RespType::Integer:
            return ":" + std::to_string(value.int_value) + CRLF;
        case RespType::BulkString:
            return "$" + std::to_string(value.str_value.size()) + CRLF
                   + value.str_value + CRLF;
        case RespType::Array: {
            std::string out = "*" + std::to_string(value.array.size()) + CRLF;
            for (const auto& elem : value.array) {
                out += serialize(elem);
            }
            return out;
        }
        case RespType::Null:
            return "$-1" + CRLF;
    }
    return "-ERR internal serialization error" + CRLF;
}

std::string RespSerializer::ok() {
    return "+OK" + CRLF;
}

std::string RespSerializer::error(const std::string& msg) {
    return "-" + msg + CRLF;
}

std::string RespSerializer::integer(int64_t n) {
    return ":" + std::to_string(n) + CRLF;
}

std::string RespSerializer::bulk_string(const std::string& s) {
    return "$" + std::to_string(s.size()) + CRLF + s + CRLF;
}

std::string RespSerializer::null_bulk() {
    return "$-1" + CRLF;
}

std::string RespSerializer::array(const std::vector<std::string>& items) {
    std::string out = "*" + std::to_string(items.size()) + CRLF;
    for (const auto& item : items) {
        out += "$" + std::to_string(item.size()) + CRLF + item + CRLF;
    }
    return out;
}
