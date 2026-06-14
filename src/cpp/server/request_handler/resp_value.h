#ifndef CC_RESP_VALUE_H
#define CC_RESP_VALUE_H

#include <cstdint>
#include <string>
#include <vector>

enum class RespType {
    SimpleString,  // +OK\r\n
    Error,         // -ERR message\r\n
    Integer,       // :1000\r\n
    BulkString,    // $5\r\nhello\r\n
    Array,         // *2\r\n...\r\n
    Null           // $-1\r\n or *-1\r\n
};

struct RespValue {
    static constexpr size_t kUnsetRespBytes = static_cast<size_t>(-1);

    RespType type;
    std::string str_value;         // SimpleString, Error, BulkString
    int64_t int_value = 0;         // Integer
    std::vector<RespValue> array;  // Array
    size_t num_resp_bytes = kUnsetRespBytes;
    bool is_null = false;

    static RespValue simple_string(const std::string& s, size_t num_resp_bytes);
    static RespValue error(const std::string& msg);
    static RespValue error(const std::string& msg, size_t num_resp_bytes);
    static RespValue integer(int64_t n, size_t num_resp_bytes);
    static RespValue bulk_string(const std::string& s, size_t num_resp_bytes);
    static RespValue null_bulk(size_t num_resp_bytes);
    static RespValue null_array(size_t num_resp_bytes);
    static RespValue array_from_parts(const std::vector<RespValue>& elements, size_t num_resp_bytes);
};

#endif
