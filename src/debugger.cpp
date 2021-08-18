#include "debugger.hpp"
#include "helper.hpp"

#include "linenoise.h"

#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <functional>

Debugger::Debugger (std::string prog_name, pid_t pid)
    : prog_name_(std::move(prog_name)), pid_(pid) {
    auto fd = open(prog_name_.c_str(), O_RDONLY);
    this->elf_ = elf::elf(elf::create_mmap_loader(fd));
    this->dwarf_  = dwarf::dwarf(dwarf::elf::create_loader(elf_));
}


void Debugger::run() {
    waitForSignal();
    initLoadAddress();
    
    char *line = nullptr;
    while ((line = linenoise("minidbg> ")) != nullptr) {
        handleCommand(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void Debugger::initLoadAddress() {
    //TODO look into it in the future
    // if it's dynamic library
    if (elf_.get_hdr().type == elf::et::dyn) {
        std::ifstream map_info("/proc/" + std::to_string(pid_) + "/maps");

        std::string addr;
        std::getline(map_info, addr, '-'); // read first line
        // <8bit> - <8bit>
        load_addr_ = std::stoi(addr, 0, 16);
    }
}

uint64_t Debugger::offsetLoadAddress(uint64_t addr) {
    return addr - load_addr_;
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
        if (isHexNum(args[1])) {
            std::string addr {args[1], 2}; // 0xNUMSEQ->NUMSEQ
            setBreakpoint(std::stol(addr, 0, 16));
            std::cout << "Set breakpoint at address 0x" 
                      << std::hex << addr << std::endl;
       
        }
    }
    else if (isPrefix(command, "register")) {
        if (isPrefix(args[1], "dump")) {
            dumpRegisters();
        }
        else if (isPrefix(args[1], "read")) {
            std::cout << args[1] << " 0x"
                      << std::setfill('0') << std::setw(16) << std::hex
                      << getRegisterValue(pid_, getRegisterFromName(args[2])) 
                      << std::endl;
        }
        else if (isPrefix(args[1], "write")) {
            if (isHexNum(args[3])) {
                std::string val {args[3], 2};
                //TODO CHECKIF args[2] is a valid name for a register?
                setRegisterValue(pid_, 
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
    else {
        std::cerr << "Invalid command" << std::endl;
    }
}

void Debugger::printSource(const std::string &file_name,
                          unsigned line,
                          unsigned n_lines_context) {

    std::ifstream file {file_name};

    auto start_line = line <= n_lines_context ? 1 : line - n_lines_context;
    auto end_line = line + n_lines_context + (line < n_lines_context ? n_lines_context - line : 0) + 1;

    char c{};
    auto current_line = 1u;
    //Skip lines up until start_line
    while (current_line != start_line && file.get(c)) {
        if (c == '\n') {
            ++current_line;
        }
    }

    //output cursor if we're at the current line
    std::cout << (current_line==line ? "> " : "  ");

    //write lines up until end_line
    while (current_line <= end_line && file.get(c)) {
        std::cout << c;
        if (c == '\n') {
            ++current_line;
            //output cursor if we're at the current line
            std::cout << (current_line==line ? "> " : "  ");
        }
    }

    //Write newline and make sure that the stream is flushed properly
    std::cout << std::endl;

} // IFSTREAM will close by itself <-> RAII

void Debugger::dumpRegisters() {
    for (const auto &rd : g_register_descriptors) {
        std::cout << rd.name << " 0x"
                  << std::setfill('0') << std::setw(16) << std::hex
                  << getRegisterValue(pid_, rd.r) 
                  << std::endl;
    }
}

void Debugger::continueExecution() {
    stepOverBreakpoint();
    ptrace(PTRACE_CONT, pid_, nullptr, nullptr);
    waitForSignal();
}


void Debugger::setBreakpoint(intptr_t at_addr) {
    Breakpoint bp {pid_, at_addr};
    bp.enable();
    breakpoints_[at_addr] = bp;
}

//TODO try process_vm_readv, process_vm_writev or /proc/<pid>/mem instead
// --- to look at larger chunks of data
uint64_t Debugger::readMemory(uint64_t address) {
    return ptrace(PTRACE_PEEKDATA, pid_, address, nullptr);
}
//TODO try process_vm_readv, process_vm_writev or /proc/<pid>/mem instead 
// --- to look at larger chunks of data
//because writeMemory writes only a word at a time
void Debugger::writeMemory(uint64_t address, uint64_t value) {
    ptrace(PTRACE_POKEDATA, pid_, address, value);
}

uint64_t Debugger::get_pc() {
    return getRegisterValue(pid_, Reg::rip);
}

void Debugger::set_pc(uint64_t pc) {
    setRegisterValue(pid_, Reg::rip, pc);
}

void Debugger::stepOverBreakpoint() {
    if (!breakpoints_.count(get_pc())) return;

    // if on a breakpoint
    auto& bp = breakpoints_[get_pc()];
    if (bp.isEnabled()) {
        bp.disable();
        ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr);
        waitForSignal();
        bp.enable();
    }
}

void Debugger::waitForSignal() {
    // waiting for signal
    int wait_status, options = 0;
    waitpid(pid_, &wait_status, options);

    // handling signal
    auto siginfo = getSignalInfo();

    switch (siginfo.si_signo) {
        case SIGTRAP:
            handleSigtrap(siginfo);
            break;
        case SIGSEGV:
            std::cout << "Good old segfault. Why: " << siginfo.si_code
                      << std::endl;
            break;
        default:
            std::cout << "Good signal " << strsignal(siginfo.si_code)
                      << std::endl;
    }
}

void Debugger::handleSigtrap(siginfo_t info) {
    switch (info.si_code) {
        // either of these signals will be sent when hitting breakpoint
        case SI_KERNEL:
        case TRAP_BRKPT: {
            set_pc(get_pc() - 1); // Since assynchronous auto increment
                                  // of PC
            std::cout << "Hit breakpoint at address 0x"
                      << std::hex << get_pc() << std::endl;
            // offset pc for querying DWARF
            auto offset_pc = offsetLoadAddress(get_pc());
            auto line_entry = getEntryFromPC(offset_pc);
            printSource(line_entry->file->path, line_entry->line);
            return;
        }
        case TRAP_TRACE: {
            return;
        }
        default: {
            std::cout << "Unknown signal with code " << info.si_code
                      << std::endl;
            return;
        }
    }
}

void Debugger::whichFunction() {
    dwarf::taddr pc = get_pc();

    // Find the CU containing pc
    // XXX Use .debug_aranges
    for (auto &cu : dwarf_.compilation_units()) {
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
    for (auto &cu : dwarf_.compilation_units()) {
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
    for (auto &cu : dwarf_.compilation_units()) {
        if (!die_pc_range(cu.root()).contains(pc)) continue;

        auto &lt = cu.get_line_table();
        auto it = lt.find_address(pc);
        if (it == lt.end()) throw std::out_of_range("Cannot find line entry");
        return it;
    }
    throw std::out_of_range("Cannot find line entry");
}

void Debugger::setBreakpointAtFunction(std::string f_name) {
    for (auto &cu : dwarf_.compilation_units()) {
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

    for (auto &cu : dwarf_.compilation_units()) {
        if (depthSearch(cu.root())) return;
    }
    std::cerr << "Couldn't find variable with name "
              << v_name << std::endl;
}

void Debugger::setBreakpointAtLine(unsigned b_line,
        const std::string &filename) {
    for (const auto &cu : dwarf_.compilation_units()) {
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

siginfo_t Debugger::getSignalInfo() {
    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, pid_, nullptr, &info);
    return info;
}
