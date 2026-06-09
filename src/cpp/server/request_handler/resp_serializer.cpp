#include "resp_serializer.h"

std::string RespSerializer::serialize(const RespValue& value) {
    return "";
}

std::string RespSerializer::ok() {
    return "";
}

std::string RespSerializer::error(const std::string& msg) {
    return "";
}

std::string RespSerializer::integer(int64_t n) {
    return "";
}

std::string RespSerializer::bulk_string(const std::string& s) {
    return "";
}

std::string RespSerializer::null_bulk() {
    return "";
}

std::string RespSerializer::array(const std::vector<std::string>& items) {
    return "";
}
