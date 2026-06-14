#ifndef CC_RESP_PARSER_H
#define CC_RESP_PARSER_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "resp_value.h"

class RespParser {
public:
    // returns the parsed RESP value, or nullopt when more input is needed.
    static std::optional<RespValue> parse(std::string_view buf);
    static std::optional<RespValue> parse(const std::vector<uint8_t>& buf);  

private:
    static std::optional<RespValue> parse_simple_string(std::string_view buf);
    static std::optional<RespValue> parse_error(std::string_view buf);
    static std::optional<RespValue> parse_integer(std::string_view buf);
    static std::optional<RespValue> parse_bulk_string(std::string_view buf);
    static std::optional<RespValue> parse_array(std::string_view buf);

    // returns the offset of the first \r\n pair, or std::string_view::npos
    static size_t find_crlf(std::string_view buf);
};

#endif
