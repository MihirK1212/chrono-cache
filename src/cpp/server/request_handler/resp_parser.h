#ifndef CC_RESP_PARSER_H
#define CC_RESP_PARSER_H

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "resp_types.h"

class RespParser {
public:
    enum class Status {
        Complete,    // Successfully parsed one value
        Incomplete,  // Need more data before a full value is available
        Error        // Protocol violation
    };

    struct Result {
        Status status;
        RespValue value;
        size_t bytes_consumed;  // Caller must advance the buffer by this amount
    };

    // Attempt to parse one RESP value from the front of buf.
    // Does NOT modify buf; caller removes bytes_consumed on Complete.
    static Result parse(std::string_view buf);
    static Result parse(const std::vector<uint8_t>& buf);  // convenience overload for raw byte buffers

private:
    static Result parse_simple_string(std::string_view buf);
    static Result parse_error(std::string_view buf);
    static Result parse_integer(std::string_view buf);
    static Result parse_bulk_string(std::string_view buf);
    static Result parse_array(std::string_view buf);

    // Returns the offset of the first \r\n pair, or std::string_view::npos
    static size_t find_crlf(std::string_view buf);
};

#endif
