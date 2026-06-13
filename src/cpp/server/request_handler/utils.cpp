#include "utils.h"

bool parse_string_as_int(std::string_view sv, int& out) {
    if (sv.empty()) return false;

    bool negative = false;
    size_t i = 0;
    if (sv[0] == '-') {
        negative = true;
        i = 1;
        if (i == sv.size()) return false;
    } else if (sv[0] == '+') {
        i = 1;
        if (i == sv.size()) return false;
    }

    int value = 0;
    for (; i < sv.size(); ++i) {
        char c = sv[i];
        if (c < '0' || c > '9') return false;
        int digit = c - '0';
        value = value * 10 + digit;
    }

    out = negative ? -value : value;
    return true;
}
