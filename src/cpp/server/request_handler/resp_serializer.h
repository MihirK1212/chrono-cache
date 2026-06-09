#ifndef CC_RESP_SERIALIZER_H
#define CC_RESP_SERIALIZER_H

#include <cstdint>
#include <string>
#include <vector>

#include "resp_types.h"

class RespSerializer {
public:
    // Serialize any RespValue to its wire representation
    static std::string serialize(const RespValue& value);

    // Convenience helpers for common responses
    static std::string ok();
    static std::string error(const std::string& msg);
    static std::string integer(int64_t n);
    static std::string bulk_string(const std::string& s);
    static std::string null_bulk();
    static std::string array(const std::vector<std::string>& items);
};

#endif
