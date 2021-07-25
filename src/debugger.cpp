#include "debugger.hpp"
#include "helper.hpp"

#include "linenoise.h"

#include <sys/wait.h>
#include <sys/ptrace.h>
#include <iostream>

void Debugger::run() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options); // wait for child process to finish

    char *line = nullptr;
    while ((line = linenoise("minidbg> ")) != nullptr) {
        handle_command(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void Debugger::handle_command(const std::string& line) {
    auto args = split(line, ' ');
    auto command = args[0];

    if (is_prefix(command, "continue")) {
        continue_execution();
    } else if (is_prefix(command, "break")) {
        std::string addr {args[1], 2}; //TODO: check if the user has 
                                       // actually written 0xADDRESS in this exact form
        set_breakpoint(std::stol(addr, 0, 16));
    } else {
        std::cerr << "Unknown command\n";
    }
}

void Debugger::continue_execution() {
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);
}

void Debugger::set_breakpoint(intptr_t at_addr) {
    std::cout << "Set breakpoint at address 0x" << std::hex << at_addr << std::endl;
    Breakpoint bp (m_pid, at_addr);
    bp.enable();
    m_breakpoints.insert({at_addr, bp});
}
