#include "debugger.hpp"
#include "helper.hpp"

#include "linenoise.h"

#include <sys/wait.h>
#include <sys/ptrace.h>
#include <iomanip>
#include <iostream>

void Debugger::run() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid_, &wait_status, options); // wait for child process to finish

    char *line = nullptr;
    while ((line = linenoise("minidbg> ")) != nullptr) {
        handleCommand(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void Debugger::handleCommand(const std::string& line) {
    auto args = split(line, ' ');
    auto command = args[0];

    if (isPrefix(command, "continue")) {
        continueExecution();
    } 
    else if (isPrefix(command, "break")) {
        if (isHexNum(args[1])) {
        std::string addr {args[1], 2}; //TODO: check if the user has 
                                       // actually written 0xADDRESS in this exact for
        setBreakpoint(std::stol(addr, 0, 16));
        } else {
            std::cerr << "Invalid address format. Should be 0xADDRESS" << std::endl;
        }
    }
    else if (isPrefix(command, "register")) {
        if (isPrefix(args[1], "dump")) {
            dumpRegisters();
        }
        else if (isPrefix(args[1], "read")) {
            std::cout << /*dothis as before with setw*/getRegisterValue(m_pid_, getRegisterFromName(args[2])) << std::endl;
        }
        else if (isPrefix(args[1], "write")) {
            if (isHexNum(args[3])) {
                std::string val {args[3], 2};
                setRegisterValue(m_pid_, getRegisterFromName(val), std::stol(val, 0, 16));
            } else {
                std::cerr << "Invalid address format. Should be 0xADDRESS" << std::endl;
            }
        }
    }
}

void Debugger::dumpRegisters() {
    for (const auto &rd : g_register_descriptors) {
        std::cout << rd.name << " 0x"
                  << std::setfill('0') << std::setw(16) << std::hex << getRegisterValue(m_pid_, rd.r) 
                  << std::endl;
    }
}

void Debugger::continueExecution() {
    ptrace(PTRACE_CONT, m_pid_, nullptr, nullptr);
    int wait_status;
    auto options = 0;
    waitpid(m_pid_, &wait_status, options);
}

void Debugger::setBreakpoint(intptr_t at_addr) {
    std::cout << "Set breakpoint at address 0x" << std::hex << at_addr << std::endl;
    Breakpoint bp (m_pid_, at_addr);
    bp.enable();
    m_breakpoints_.insert({at_addr, bp});
}
