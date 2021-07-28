#include "debugger.hpp"
#include "helper.hpp"

#include "linenoise.h"

#include <sys/wait.h>
#include <sys/ptrace.h>
#include <iostream>

void Debugger::Run() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid_, &wait_status, options); // wait for child process to finish

    char *line = nullptr;
    while ((line = linenoise("minidbg> ")) != nullptr) {
        HandleCommand(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void Debugger::HandleCommand(const std::string& line) {
    auto args = Split(line, ' ');
    auto command = args[0];

    if (IsPrefix(command, "continue")) {
        ContinueExecution();
    } else if (IsPrefix(command, "break")) {
        if (IsHexFormatted(args[1])) {
        std::string addr {args[1], 2}; //TODO: check if the user has 
                                       // actually written 0xADDRESS in this exact for
        SetBreakpoint(std::stol(addr, 0, 16));
        } else 
            std::cerr << "Invalid address\n";
    } else {
        std::cerr << "Unknown command\n";
    }
}

void Debugger::ContinueExecution() {
    ptrace(PTRACE_CONT, m_pid_, nullptr, nullptr);
    int wait_status;
    auto options = 0;
    waitpid(m_pid_, &wait_status, options);
}

void Debugger::SetBreakpoint(intptr_t at_addr) {
    std::cout << "Set breakpoint at address 0x" << std::hex << at_addr << std::endl;
    Breakpoint bp (m_pid_, at_addr);
    bp.Enable();
    m_breakpoints_.insert({at_addr, bp});
}
