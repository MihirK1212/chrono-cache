#ifndef CC_RESP_TYPES_H
#define CC_RESP_TYPES_H

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
    RespType type;
    std::string str_value;         // SimpleString, Error, BulkString
    int64_t int_value = 0;         // Integer
    std::vector<RespValue> array;  // Array
    bool is_null = false;

    static RespValue simple_string(const std::string& s);
    static RespValue error(const std::string& msg);
    static RespValue integer(int64_t n);
    static RespValue bulk_string(const std::string& s);
    static RespValue null_bulk();
    static RespValue null_array();
};

#endif
