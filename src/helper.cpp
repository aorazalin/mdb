#include "helper.hpp"

std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> out{};
    std::stringstream ss{s};
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        out.push_back(item);
    }
    
    return out;
}

bool is_prefix(const std::string &s, const std::string& of) {
    if (s.size() > of.size()) return false;
    return std::equal(s.begin(), s.end(), of.begin());
}

bool is_hex_formatted(const std::string &s) {
    if (s.size() <= 2 || s[0] != '0' || s[1] != 'x') return false;
    for (int i = 2; i < s.size(); ++i)
        if (!isxdigit(s[i])) return false;
    return true;
}