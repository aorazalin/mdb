#include <vector>
#include <string>
#include <sstream>

const uint64_t INTERRUPT_COMMAND = 0xcc;

std::vector<std::string> split(const std::string, char);

bool is_prefix(const std::string &, const std::string &);