#include "resp_types.h"

RespValue RespValue::simple_string(const std::string& s) {
    RespValue v;
    v.type = RespType::SimpleString;
    v.str_value = s;
    return v;
}

RespValue RespValue::error(const std::string& msg) {
    RespValue v;
    v.type = RespType::Error;
    v.str_value = msg;
    return v;
}

RespValue RespValue::integer(int64_t n) {
    RespValue v;
    v.type = RespType::Integer;
    v.int_value = n;
    return v;
}

RespValue RespValue::bulk_string(const std::string& s) {
    RespValue v;
    v.type = RespType::BulkString;
    v.str_value = s;
    return v;
}

RespValue RespValue::null_bulk() {
    RespValue v;
    v.type = RespType::BulkString;
    v.is_null = true;
    return v;
}

RespValue RespValue::null_array() {
    RespValue v;
    v.type = RespType::Array;
    v.is_null = true;
    return v;
}
