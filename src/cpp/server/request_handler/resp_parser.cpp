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
    if(buf[0] != '+') {
        return RespValue::error("ERR invalid simple string format");
    }

    size_t pos = find_crlf(buf);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    std::string_view value = buf.substr(1, pos - 1);
    
    RespValue v = RespValue::simple_string(std::string(value), pos + 2);
    return v;
}

std::optional<RespValue> RespParser::parse_error(std::string_view buf) {
    if(buf[0] != '-') {
        return RespValue::error("ERR invalid error format");
    }

    size_t pos = find_crlf(buf);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    std::string_view value = buf.substr(1, pos - 1);
    return RespValue::error(std::string(value));
}

std::optional<RespValue> RespParser::parse_integer(std::string_view buf) {
    if(buf[0] != ':') {
        return RespValue::error("ERR invalid integer format");
    }

    char has_sign_byte = !(buf[1] == '+' || buf[1] == '-');
    char sign = has_sign_byte ? buf[1] : '+';

    size_t pos = find_crlf(buf); 
    if (pos == std::string_view::npos) { 
        return std::nullopt;
    }
    int offset = has_sign_byte ? 2 : 1;
    std::string_view abs_value_str = buf.substr(offset, pos - offset);

    int64_t value = 0;
    for(char c : abs_value_str) {
        if(c < '0' || c > '9') {
            return RespValue::error("ERR invalid integer format");

        }
        value = value * 10 + (c - '0');
    }
    value = sign == '-' ? -value : value;

    return RespValue::integer(value, pos + 2);
}

std::optional<RespValue> RespParser::parse_bulk_string(std::string_view buf) {
    if(buf[0] != '$') {
        return RespValue::error("ERR invalid bulk string format");
    }

    size_t pos_1 = find_crlf(buf);
    if (pos_1 == std::string_view::npos) {
        return std::nullopt;
    }
    int length = std::stoi(std::string(buf.substr(1, pos_1 - 1)));

    if(length < 0) {
        return RespValue::error("ERR invalid bulk string format");
    }

    size_t pos_2 = find_crlf(buf.substr(pos_1 + 2));
    if (pos_2 == std::string_view::npos) {
        return std::nullopt;
    }
    std::string_view value = buf.substr(pos_1 + 2, pos_2 - pos_1 - 1);
    return RespValue::bulk_string(std::string(value), pos_2 + 2);
}

std::optional<RespValue> RespParser::parse_array(std::string_view buf) {
    if(buf[0] != '*') {
        return RespValue::error("ERR invalid array format");
    }

    size_t pos_1 = find_crlf(buf);
    if (pos_1 == std::string_view::npos) {
        return std::nullopt;
    }
    int num_elements = std::stoi(std::string(buf.substr(1, pos_1 - 1)));

    if(num_elements < 0) {
        return RespValue::error("ERR invalid array format");
    }

    int curr_parse_start_pos = pos_1 + 2;

    std::vector<RespValue> elements;
    for(int i = 0; i < num_elements; i++) {
        std::optional<RespValue> element = parse(buf.substr(curr_parse_start_pos));
        if(!element.has_value()) {
            return RespValue::error("ERR invalid array format");
        }
        RespValue v = element.value(); 

        elements.push_back(v);

        if(int(v.num_resp_bytes) == -1) {
            return RespValue::error("ERR invalid array format");
        }
        curr_parse_start_pos += v.num_resp_bytes;
    }
    RespValue v = RespValue::array_from_parts(elements, curr_parse_start_pos + 2);
    return v;
}

size_t RespParser::find_crlf(std::string_view buf) {
    // returns 0-indexed position of the first occurrence of "\r\n"
    // or std::string_view::npos if not found
    size_t pos = buf.find("\r\n");
    if (pos == std::string_view::npos) {
        return std::string_view::npos;
    }
    return pos;
}
