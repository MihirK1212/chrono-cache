#include "resp_parser.h"

std::optional<RespValue> RespParser::parse(const std::vector<uint8_t>& buf) {
    return parse(std::string_view(reinterpret_cast<const char*>(buf.data()), buf.size()));
}

std::optional<RespValue> RespParser::parse(std::string_view buf) {
    if (buf.empty()) {
        return std::nullopt;
    }

    switch (buf[0]) {
        case '+': return parse_simple_string(buf);
        case '-': return parse_error(buf);
        case ':': return parse_integer(buf);
        case '$': return parse_bulk_string(buf);
        case '*': return parse_array(buf);
        default:  return std::nullopt;
    }
}

std::optional<RespValue> RespParser::parse_simple_string(std::string_view buf) {
    return std::nullopt;
}

std::optional<RespValue> RespParser::parse_error(std::string_view buf) {
    return std::nullopt;
}

std::optional<RespValue> RespParser::parse_integer(std::string_view buf) {
    return std::nullopt;
}

std::optional<RespValue> RespParser::parse_bulk_string(std::string_view buf) {
    return std::nullopt;
}

std::optional<RespValue> RespParser::parse_array(std::string_view buf) {
    return std::nullopt;
}

size_t RespParser::find_crlf(std::string_view buf) {
    return std::string_view::npos;
}
