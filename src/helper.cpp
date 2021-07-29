#include <algorithm>

#include <sys/ptrace.h>
#include <sys/user.h>

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

bool isPrefix(const std::string &s, const std::string& of) {
    if (s.size() > of.size()) return false;
    return std::equal(s.begin(), s.end(), of.begin());
}

bool isHexNum(const std::string &s) {
    if (s.size() <= 2 || s[0] != '0' || s[1] != 'x') return false;
    for (int i = 2; i < s.size(); ++i)
        if (!isxdigit(s[i])) return false;
    return true;
}

std::string getRegisterName(Reg r) {
    auto it = std::find_if(begin(g_register_descriptors), 
                          end(g_register_descriptors), 
                          [r](auto &&rd) { return r == rd.r; });
    return it->name;
}

Reg getRegisterFromName(const std::string &name) {
    auto it = std::find_if(begin(g_register_descriptors), 
                          end(g_register_descriptors), 
                          [name](auto &&rd) { return name == rd.name; });
    return it->r;
}


// gives address of a register 
uint64_t getRegisterValue(pid_t pid, Reg r) {
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs); // regs --- object that contains

    auto it = std::find_if(begin(g_register_descriptors), 
                           end(g_register_descriptors),
                           [r](auto &&rd) { return r == rd.r; });

    if (it == end(g_register_descriptors)) {
        throw std::out_of_range{"Unknown register"};
    }

    return *(reinterpret_cast<uint64_t*>(&regs) + (it - begin(g_register_descriptors)));
}

void setRegisterValue(pid_t pid, Reg r, uint64_t value) {
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs); // regs --- object that contains
                                                 // general purpose registers (see user.h) 
    auto it = std::find_if(begin(g_register_descriptors), 
                           end(g_register_descriptors),
                           [r](auto &&rd) { return r == rd.r; });

    if (it == end(g_register_descriptors)) {
        throw std::out_of_range{"Unknown register"};
    }

    *(reinterpret_cast<uint64_t*>(&regs) + (it - begin(g_register_descriptors))) = value;
    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

uint64_t getRegisterValueFromDwarfRegister (pid_t pid, unsigned regnum) {
    auto it = std::find_if(begin(g_register_descriptors), 
                           end(g_register_descriptors),
                           [regnum](auto &&rd) { return regnum == rd.dwarf_r; });
    
    if (it == end(g_register_descriptors)) {
        throw std::out_of_range{"Unknown dwarf register"};
    }

    return getRegisterValue(pid, it->r);
}