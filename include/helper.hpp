#include <vector>
#include <string>
#include <sstream>

const uint64_t INTERRUPT_COMMAND = 0xcc;

std::vector<std::string> Split(const std::string &, char);

bool IsPrefix(const std::string &, const std::string &);

bool IsHexFormatted(const std::string &);