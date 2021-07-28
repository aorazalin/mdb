#include <iostream>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/personality.h>
#include <unistd.h>

#include <linenoise.h>

#include "debugger.hpp"

//TODO --- add floating point & vector registers in the future

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

const std::array<RegDescriptor, n_registers> g_Register_descriptrs {{
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


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Program name not specified\n";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();

    if (pid == 0) {
        // child process --> debuggee
        //TODO check ptrace error codes
        personality(ADDR_NO_RANDOMIZE);
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr); // I allow
                                                     // parent process
                                                     // to trace me 
        execl(prog, prog, nullptr); // execute (???)
    } else if (pid >= 1) {
        // parent process --> debugger
        std::cout << "Started debugging process " << pid << '\n';
        Debugger dbg{prog, pid};
        dbg.Run();
    }
}