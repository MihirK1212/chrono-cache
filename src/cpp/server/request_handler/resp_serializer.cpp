#include "resp_serializer.h"

static const std::string CRLF = "\r\n";

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
