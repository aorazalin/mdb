#include <vector>
#include <string>
#include <sstream>
#include <array>

const uint64_t INTERRUPT_COMMAND = 0xcc;

enum class Reg {
    rax, rbx, rcx, rdx,
    rdi, rsi, rbp, rsp,
    r8,  r9,  r10, r11,
    r12, r13, r14, r15,
    rip, rflags,    cs,
    orig_rax, fs_base,
    gs_base,
    fs, gs, ss, ds, es
};

constexpr std::size_t n_registers = 27;

struct RegDescriptor {
    Reg r;
    int dwarf_r;
    std::string name;
};

const std::array<RegDescriptor, n_registers> g_register_descriptors {{
    { Reg::r15, 15, "r15" },
    { Reg::r14, 14, "r14" },
    { Reg::r13, 13, "r13" },
    { Reg::r12, 12, "r12" },
    { Reg::rbp, 6, "rbp" },
    { Reg::rbx, 3, "rbx" },
    { Reg::r11, 11, "r11" },
    { Reg::r10, 10, "r10" },
    { Reg::r9, 9, "r9" },
    { Reg::r8, 8, "r8" },
    { Reg::rax, 0, "rax" },
    { Reg::rcx, 2, "rcx" },
    { Reg::rdx, 1, "rdx" },
    { Reg::rsi, 4, "rsi" },
    { Reg::rdi, 5, "rdi" },
    { Reg::orig_rax, -1, "orig_rax" },
    { Reg::rip, -1, "rip" },
    { Reg::cs, 51, "cs" },
    { Reg::rflags, 49, "eflags" },
    { Reg::rsp, 7, "rsp" },
    { Reg::ss, 52, "ss" },
    { Reg::fs_base, 58, "fs_base" },
    { Reg::gs_base, 59, "gs_base" },
    { Reg::ds, 53, "ds" },
    { Reg::es, 50, "es" },
    { Reg::fs, 54, "fs" },
    { Reg::gs, 55, "gs" },
}};

std::string getRegisterName(Reg);

Reg getRegisterFromName(const std::string &);

uint64_t getRegisterValue(pid_t, Reg);

uint64_t getRegisterValueFromDwarfRegister(pid_t, unsigned);

void setRegisterValue(pid_t pid, Reg r, uint64_t value);

// Split a string into a list given a delimiter
std::vector<std::string> split(const std::string &, char);

bool isPrefix(const std::string &, const std::string &);

// Is a string in format 0xNUMSEQ
bool isHexNum(const std::string &);
