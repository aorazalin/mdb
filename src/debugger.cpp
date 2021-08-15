#include "debugger.hpp"
#include "helper.hpp"

#include "linenoise.h"

#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <functional>

Debugger::Debugger (std::string prog_name, pid_t pid)
    : m_prog_name_(std::move(prog_name)), m_pid_(pid) {
    m_fd_ = open(m_prog_name_.c_str(), O_RDONLY);
    this->m_elf_ = elf::elf(elf::create_mmap_loader(m_fd_));
    this->m_dwarf_ = dwarf::dwarf(dwarf::elf::create_loader(m_elf_));
}


void Debugger::run() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid_, &wait_status, options); // wait for 
                                            // child process to finish

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
        // break pc <memorylocation>
        // break line <linenumber>
        if (isPrefix(args[1], "pc")) {
            if (isHexNum(args[2])) {
                std::string addr {args[2], 2}; // 0xNUMSEQ->NUMSEQ
                setBreakpoint(std::stol(addr, 0, 16));
                std::cout << "Set breakpoint at address 0x" 
                          << std::hex << addr << std::endl;
            } else 
                std::cerr << "Invalid address format. Should be 0xADDRESS" 
                          << std::endl;
            
        }
        else if (isPrefix(args[1], "line")) {
            //TODO handling if args[2] is a number
            //TODO redo format to break line <#> <file>
            long line = stol(args[2]);
            setBreakpointAtLine(line, args[3]);
        } else if (isPrefix(args[1], "function")) {
            std::string f_name = args[2];
            setBreakpointAtFunction(f_name);
        } else 
            std::cerr << "Format: break {pc|line|function} [args]";
    }
    else if (isPrefix(command, "register")) {
        if (isPrefix(args[1], "dump")) {
            dumpRegisters();
        }
        else if (isPrefix(args[1], "read")) {
            std::cout << args[1] << " 0x"
                      << std::setfill('0') << std::setw(16) << std::hex
                      << getRegisterValue(m_pid_, getRegisterFromName(args[2])) 
                      << std::endl;
        }
        else if (isPrefix(args[1], "write")) {
            if (isHexNum(args[3])) {
                std::string val {args[3], 2};
                //TODO CHECKIF args[2] is a valid name for a register?
                setRegisterValue(m_pid_, 
                                 getRegisterFromName(args[2]),
                                 std::stol(val, 0, 16)); 
            } else {
                std::cerr << "Invalid number format. Should be 0xNUMSEQ"
                          << std::endl;
            }
        }
    }
    else if (isPrefix(command, "memory")) {
        if (isHexNum(args[2])) {
            std::string addr {args[2], 2};

            if (isPrefix(args[1], "read")) {
                std::cout << std::hex << readMemory(std::stol(addr, 0, 16))
                          << std::endl;
            }
        } else {
            std::cerr << "Invalid address format. Should be 0xADDRESS" 
                      << std::endl;
        }
    }
    else if (isPrefix(command, "which")) {
        if (isPrefix(args[1], "function")) {
            whichFunction();
        } else if (isPrefix(args[1], "line")) {
            whichLine();
        } else {
            std::cerr << "Invalid command" << std::endl;
        }
    } 
    else if (isPrefix(command, "read")) {
        if (isPrefix(args[1], "variable")) {
            std::string v_name {args[2]}; 
            readVariable(v_name);
        } else 
            std::cerr << "Invalid command" << std::endl;
    }
    else {
        std::cerr << "Invalid command" << std::endl;
    }
}

void Debugger::dumpRegisters() {
    for (const auto &rd : g_register_descriptors) {
        std::cout << rd.name << " 0x"
                  << std::setfill('0') << std::setw(16) << std::hex
                  << getRegisterValue(m_pid_, rd.r) 
                  << std::endl;
    }
}

void Debugger::continueExecution() {
    stepOverBreakpoint();
    ptrace(PTRACE_CONT, m_pid_, nullptr, nullptr);
    waitForSignal();
}


void Debugger::setBreakpoint(intptr_t at_addr) {
    Breakpoint bp {m_pid_, at_addr};
    bp.enable();
    m_breakpoints_[at_addr] = bp;
}

//TODO try process_vm_readv, process_vm_writev or /proc/<pid>/mem instead
// --- to look at larger chunks of data
uint64_t Debugger::readMemory(uint64_t address) {
    return ptrace(PTRACE_PEEKDATA, m_pid_, address, nullptr);
}
//TODO try process_vm_readv, process_vm_writev or /proc/<pid>/mem instead 
// --- to look at larger chunks of data
//because writeMemory writes only a word at a time
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
    auto possible_breakpoint_location = get_pc() - 1; // -1 because PC increments
                                                      // before execution

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

void Debugger::whichFunction() {
    dwarf::taddr pc = get_pc();

    // Find the CU containing pc
    // XXX Use .debug_aranges
    for (auto &cu : m_dwarf_.compilation_units()) {
        if (!die_pc_range(cu.root()).contains(pc)) continue;
        // Map PC to a line
        auto &lt = cu.get_line_table();
        auto it = lt.find_address(pc);
        // print info about Compilation Unit
        if (it == lt.end())  std::cerr << "Can't find line number location"
                                       << std::endl;
        else                 std::cout << it->get_description() << std::endl;

        // Map PC to an object
        // XXX Index/helper/something for looking up PCs
        // XXX DW_AT_specification and DW_AT_abstract_origin
        std::vector<dwarf::die> stack;
        bool found = false;
        if (find_pc(cu.root(), pc, &stack)) {
            for (auto &d : stack) {
                 if (!found) { 
                        std::cout << "Inlined (more to less specific) in:\n"; 
                        found = true;
                 }
                 dump_die(d);
            }
        }

        break;
    }
}

void Debugger::whichLine() {
    dwarf::taddr pc = get_pc();
    // Find the CU containing pc
    // XXX Use .debug_aranges
    for (auto &cu : m_dwarf_.compilation_units()) {
        if (!die_pc_range(cu.root()).contains(pc)) continue;
        // Map PC to a line
        auto &lt = cu.get_line_table();
        auto it = lt.find_address(pc);

        // print info about Compilation Unit
        if (it == lt.end())  
            std::cerr << "Can't find line number location" << std::endl;
        else                 
            std::cout << it->get_description() << std::endl;

        break;
     
    }
}

    

dwarf::line_table::iterator Debugger::getEntryFromPC(uint64_t pc) {
    for (auto &cu : m_dwarf_.compilation_units()) {
        if (!die_pc_range(cu.root()).contains(pc)) continue;

        auto &lt = cu.get_line_table();
        auto it = lt.find_address(pc);
        if (it == lt.end()) throw std::out_of_range("Cannot find line entry");
        return it;
    }
    throw std::out_of_range("Cannot find line entry");
}

void Debugger::setBreakpointAtFunction(std::string f_name) {
    for (auto &cu : m_dwarf_.compilation_units()) {
        for (auto &die : cu.root()) {
            if (!die.has(dwarf::DW_AT::name) || at_name(die) != f_name) continue;
            auto low_pc = at_low_pc(die);
            auto entry = getEntryFromPC(low_pc);
            ++entry; 
            setBreakpoint(entry->address);
            std::cout << "Breakpoint set on function " << f_name 
                      << std::endl;
            return;
        }
    }
    std::cerr << "Couldn't find function with name " 
              << f_name << std::endl;
}

void Debugger::readVariable(std::string v_name) {

    std::function<bool(const dwarf::die &)> depthSearch;
    depthSearch = [&depthSearch,v_name](const dwarf::die &die) {
        if (die.tag == dwarf::DW_TAG::variable
                && at_name(die) == v_name) {
            //TODO process DIE
            dump_die(die);
            return true;
        }
        
        for (auto &child : die)
            if (depthSearch(child)) return true;
        return false;
    };

    for (auto &cu : m_dwarf_.compilation_units()) {
        if (depthSearch(cu.root())) return;
    }
    std::cerr << "Couldn't find variable with name "
              << v_name << std::endl;
}

void Debugger::setBreakpointAtLine(unsigned b_line,
        const std::string &filename) {
    for (const auto &cu : m_dwarf_.compilation_units()) {
       if (!isSuffix(filename, at_name(cu.root()))) continue; 

       const auto& lt = cu.get_line_table();

       for (const auto &entry : lt) {
           if (!entry.is_stmt || entry.line != b_line) continue;
           setBreakpoint(entry.address);
           return;
       }
    }
    throw std::out_of_range("setBreakpointAtLine");
}
