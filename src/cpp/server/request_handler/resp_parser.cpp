#include <cstdint>

#include "resp_parser.h"
#include "utils.h"


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
        default:  return RespValue::error("ERR unknown RESP type byte");
    }
}

std::optional<RespValue> RespParser::parse_simple_string(std::string_view buf) {
    if (buf[0] != '+') {
        return RespValue::error("ERR invalid simple string format");
    }

    size_t pos = find_crlf(buf);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    std::string_view value = buf.substr(1, pos - 1);
    return RespValue::simple_string(std::string(value), pos + 2);
}

std::optional<RespValue> RespParser::parse_error(std::string_view buf) {
    if (buf[0] != '-') {
        return RespValue::error("ERR invalid error format");
    }

    size_t pos = find_crlf(buf);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    std::string_view value = buf.substr(1, pos - 1);
    return RespValue::error(std::string(value), pos + 2);
}

std::optional<RespValue> RespParser::parse_integer(std::string_view buf) {
    if (buf[0] != ':') {
        return RespValue::error("ERR invalid integer format");
    }

    if (buf.size() < 3) {
        return std::nullopt;
    }

    size_t pos = find_crlf(buf);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    int value;
    if (!parse_string_as_int(buf.substr(1, pos - 1), value)) {
        return RespValue::error("ERR invalid integer format");
    }

    return RespValue::integer(value, pos + 2);
}

std::optional<RespValue> RespParser::parse_bulk_string(std::string_view buf) {
    if (buf[0] != '$') {
        return RespValue::error("ERR invalid bulk string format");
    }

    size_t pos_1 = find_crlf(buf);
    if (pos_1 == std::string_view::npos) {
        return std::nullopt;
    }

    int length;
    if (!parse_string_as_int(buf.substr(1, pos_1 - 1), length)) {
        return RespValue::error("ERR invalid bulk string format");
    }

    if (length == -1) {
        return RespValue::null_bulk(pos_1 + 2);
    }

    if (length < 0) {
        return RespValue::error("ERR invalid bulk string format");
    }

    size_t data_start = pos_1 + 2;
    size_t total_bytes = data_start + static_cast<size_t>(length) + 2;

    if (buf.size() < total_bytes) {
        return std::nullopt;
    }

    if (buf[data_start + length] != '\r' || buf[data_start + length + 1] != '\n') {
        return RespValue::error("ERR invalid bulk string format");
    }

    std::string_view value = buf.substr(data_start, static_cast<size_t>(length));
    return RespValue::bulk_string(std::string(value), total_bytes);
}

std::optional<RespValue> RespParser::parse_array(std::string_view buf) {
    if (buf[0] != '*') {
        return RespValue::error("ERR invalid array format");
    }

    size_t pos_1 = find_crlf(buf);
    if (pos_1 == std::string_view::npos) {
        return std::nullopt;
    }

    int num_elements;
    if (!parse_string_as_int(buf.substr(1, pos_1 - 1), num_elements)) {
        return RespValue::error("ERR invalid array format");
    }

    if (num_elements == -1) {
        return RespValue::null_array(pos_1 + 2);
    }

    if (num_elements < 0) {
        return RespValue::error("ERR invalid array format");
    }

    size_t curr_pos = pos_1 + 2;
    std::vector<RespValue> elements;
    for (int i = 0; i < num_elements; i++) {
        std::optional<RespValue> element = parse(buf.substr(curr_pos));
        if (!element.has_value()) {
            return std::nullopt;
        }
        RespValue v = element.value();
        if (v.num_resp_bytes == RespValue::kUnsetRespBytes) {
            return RespValue::error("ERR invalid array element");
        }
        elements.push_back(v);
        curr_pos += v.num_resp_bytes;
    }

    return RespValue::array_from_parts(elements, curr_pos);
}

size_t RespParser::find_crlf(std::string_view buf) {
    // returns 0-indexed position of the first "\r\n", or npos if not found.
    size_t pos = buf.find("\r\n");
    if (pos == std::string_view::npos) {
        return std::string_view::npos;
    }
    return pos;
}
