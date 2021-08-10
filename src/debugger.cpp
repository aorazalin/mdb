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
            std::string addr {args[1], 2}; // 0xNUMSEQ->NUMSEQ
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
            std::cout << " 0x"
                      << std::setfill('0') << std::setw(16) << std::hex
                      << getRegisterValue(m_pid_, getRegisterFromName(args[2])) << std::endl;
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
    else if (isPrefix(command, "memory")) {
        if (isHexNum(args[2])) {
            std::string addr {args[2], 2};

            if (isPrefix(args[1], "read")) {
                std::cout << std::hex << readMemory(std::stol(addr, 0, 16)) << std::endl;
            }
        } else {
            std::cerr << "Invalid address format. Should be 0xADDRESS" << std::endl;
        }
    }
    else {
        std::cerr << "Invalid command" << std::endl;
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
    stepOverBreakpoint();
    ptrace(PTRACE_CONT, m_pid_, nullptr, nullptr);
    waitForSignal();
}


void Debugger::setBreakpoint(intptr_t at_addr) {
    std::cout << "Set breakpoint at address 0x" << std::hex << at_addr << std::endl;
    Breakpoint bp {m_pid_, at_addr};
    bp.enable();
    m_breakpoints_[at_addr] = bp;
}

//TODO try process_vm_readv, process_vm_writev or /proc/<pid>/mem instead --- to look at larger chunks of data
uint64_t Debugger::readMemory(uint64_t address) {
    return ptrace(PTRACE_PEEKDATA, m_pid_, address, nullptr);
}
//TODO try process_vm_readv, process_vm_writev or /proc/<pid>/mem instead --- to look at larger chunks of data
void Debugger::writeMemory(uint64_t address, uint64_t value) {
    ptrace(PTRACE_POKEDATA, m_pid_, address, value);
}

uint64_t Debugger::get_pc() {
    return getRegisterValue(m_pid_, Reg::rip);
}

void Debugger::set_pc(uint64_t pc) {
    setRegisterValue(m_pid_, Reg::rip, pc);
}

void Debugger::stepOverBreakpoint() {
    auto possible_breakpoint_location = get_pc() - 1;

    // if on a breakpoint
    if (m_breakpoints_.count(possible_breakpoint_location)) {
        auto& bp = m_breakpoints_[possible_breakpoint_location];
        
        if (bp.isEnabled()) {
            auto previous_instruction_address = possible_breakpoint_location;
            set_pc(previous_instruction_address);

            bp.disable();
            ptrace(PTRACE_SINGLESTEP, m_pid_, nullptr, nullptr);
            waitForSignal();
            bp.enable();
        }
    }
}

void Debugger::waitForSignal() {
    int wait_status, options = 0;
    waitpid(m_pid_, &wait_status, options);
}

