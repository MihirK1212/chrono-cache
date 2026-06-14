#include "resp_value.h"

RespValue RespValue::simple_string(const std::string& s, size_t num_resp_bytes) {
    RespValue v;
    v.type = RespType::SimpleString;
    v.str_value = s;
    v.num_resp_bytes = num_resp_bytes;
    return v;
}

RespValue RespValue::error(const std::string& msg) {
    return error(msg, kUnsetRespBytes);
}

RespValue RespValue::error(const std::string& msg, size_t num_resp_bytes) {
    RespValue v;
    v.type = RespType::Error;
    v.str_value = msg;
    v.num_resp_bytes = num_resp_bytes;
    return v;
}

RespValue RespValue::integer(int64_t n, size_t num_resp_bytes) {
    RespValue v;
    v.type = RespType::Integer;
    v.int_value = n;
    v.num_resp_bytes = num_resp_bytes;
    return v;
}

RespValue RespValue::bulk_string(const std::string& s, size_t num_resp_bytes) {
    RespValue v;
    v.type = RespType::BulkString;
    v.str_value = s;
    v.num_resp_bytes = num_resp_bytes;
    return v;
}

RespValue RespValue::array_from_parts(const std::vector<RespValue>& elements, size_t num_resp_bytes) {
    RespValue v;
    v.type = RespType::Array;
    v.array = elements;
    v.num_resp_bytes = num_resp_bytes;
    return v;
}

RespValue RespValue::null_bulk(size_t num_resp_bytes) {
    RespValue v;
    v.type = RespType::BulkString;
    v.is_null = true;
    v.num_resp_bytes = num_resp_bytes;
    return v;
}

RespValue RespValue::null_array(size_t num_resp_bytes) {
    RespValue v;
    v.type = RespType::Array;
    v.is_null = true;
    v.num_resp_bytes = num_resp_bytes;
    return v;
}
