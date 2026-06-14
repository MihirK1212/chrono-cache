#ifndef CC_UTILS_H
#define CC_UTILS_H

#include <string_view>
#include <string>

bool parse_string_as_int(std::string_view sv, int& out);

std::string to_upper(std::string_view sv);

#endif
