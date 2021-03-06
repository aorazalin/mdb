#include "debugger.hh"
#include "helper.hh"
#include "ptrace-expr-context.hh"

#include "linenoise.h"
#include "cwalk.h"

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
		loadSymbols();
}

void Debugger::run() {
    waitForSignal();
    initLoadAddress();
    
    char *line = nullptr;
    while ((line = linenoise("(mdb) ")) != nullptr) {
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

void Debugger::singleStep() {
    ptrace(PTRACE_SINGLESTEP, pid_, nullptr, nullptr);
    waitForSignal();
}

void Debugger::singleStepWithBreakpointCheck() {
    if (breakpoints_.count(get_pc())) stepOverBreakpoint();
    else singleStep();
}

uint64_t Debugger::offsetLoadAddress(uint64_t addr) {
    return addr - load_addr_;
}

uint64_t Debugger::offsetDwarfAddress(uint64_t addr) {
    return addr + load_addr_;
}

uint64_t Debugger::getOffsetPC() {
    return offsetLoadAddress(get_pc());
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
            setBreakpointAtAddress(std::stol(addr, 0, 16));
        }
				else if (args[1].find(':') != std::string::npos) {
						auto file_and_line = split(args[1], ':');
						setBreakpointAtLine(file_and_line[0], std::stoi(file_and_line[1]));
				}
				else {
						setBreakpointAtFunction(args[1]);
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
		else if (isPrefix(command, "step")) {
				stepIn();
		}
		else if (isPrefix(command, "next")) {
				stepOver();
		}
		else if (isPrefix(command, "finish")) {
				stepOut();
		}
		else if (isPrefix(command, "symbol")) {
				auto syms = lookupSymbol(args[1]);
				for (auto &&s : syms) {
						std::cout << s.name << " " << toString(s.type) << " 0x" 
										  << std::hex << s.addr << std::endl;
				}
		}
		else if (isPrefix(command, "backtrace")) {
				printBacktrace();
		}
		else if (isPrefix(command, "var")) {
				std::string var_name = args[1];
				readVariable(var_name);
		}
		else if (isPrefix(command, "allvars")) {
				readVariables();
		}
		else if (isPrefix(command, "clear")) {
				linenoiseClearScreen();
		}
		else if (isPrefix(command, "exit")) {
				kill(pid_, SIGTERM);
				exit(0);
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


void Debugger::setBreakpointAtAddress(intptr_t at_addr) {
    Breakpoint bp {pid_, at_addr};
    bp.enable();
    breakpoints_[at_addr] = bp;
		std::cout << "Set breakpoint at address 0x" 
							<< std::hex << at_addr << std::endl;
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
				case 0:
						//std::cout << "Program finished" << std::endl;
						exit(0);
				default:
						std::cout << "Got signal: " << strsignal(siginfo.si_signo)
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
            auto line_entry = getLineEntryFromPC(offset_pc);
            printSource(line_entry->file->path, line_entry->line);
            return;
        }
				// single stepping signal
        case TRAP_TRACE:
				// when debugger&debuggee start
				case SI_USER: {
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

void Debugger::stepOut() {
    auto frame_ptr = getRegisterValue(pid_, Reg::rbp);
    // return address is store 8 bytes after start of stack frame
    auto return_addr = readMemory(frame_ptr + 8);

    bool shouldRemoveBreakpoint = false;
    if (!breakpoints_.count(return_addr)) {
        setBreakpointAtAddress(return_addr);
        shouldRemoveBreakpoint = true;
    }

    continueExecution();

    if (shouldRemoveBreakpoint) {
        removeBreakpoint(return_addr);
    }
}

void Debugger::removeBreakpoint(std::intptr_t addr) {
    if (breakpoints_.at(addr).isEnabled()) 
        breakpoints_.at(addr).disable();
    breakpoints_.erase(addr); 
}

void Debugger::stepIn() {
    auto line = getLineEntryFromPC(getOffsetPC())->line;

    while (getLineEntryFromPC(getOffsetPC())->line == line)
        singleStepWithBreakpointCheck();

    auto line_entry = getLineEntryFromPC(getOffsetPC());
    printSource(line_entry->file->path, line_entry->line);
}

dwarf::die Debugger::getFunctionFromPC(uint64_t pc) {
		//TODO add recursion for an actual search
		for (auto &cu : dwarf_.compilation_units()) {
				if (die_pc_range(cu.root()).contains(pc)) {
						for (auto &d : cu.root()) 
								if (die_pc_range(d).contains(pc))
										return d;
				}
		}
   throw std::out_of_range("Cannot find function in getFunctionFromPC");
}

// simple solution to step over --- put breakpoint
// at every line inside that function + return address
void Debugger::stepOver() {
    auto func = getFunctionFromPC(getOffsetPC());
    auto funcStart = at_low_pc(func);
    auto funcEnd = at_high_pc(func);

    auto line = getLineEntryFromPC(funcStart);
    auto start_line = getLineEntryFromPC(getOffsetPC());
    
    // function to save up places to restore from
    // "polluted" breakpoints
    std::vector<std::intptr_t> to_delete;

    while (line->address < funcEnd) {
        auto load_address = offsetDwarfAddress(line->address);
        if (line->address != start_line->address 
                && !breakpoints_.count(load_address)) {
            setBreakpointAtAddress(load_address);
            to_delete.push_back(load_address);
        }
       ++line;
    }

    auto frame_ptr = getRegisterValue(pid_, Reg::rbp);
    auto return_addr = readMemory(frame_ptr + 8);
    if (!breakpoints_.count(return_addr)) {
				setBreakpointAtAddress(return_addr);
				to_delete.push_back(return_addr);
    }

		continueExecution();

		for (auto addr : to_delete) {
				removeBreakpoint(addr);
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

    

dwarf::line_table::iterator Debugger::getLineEntryFromPC(uint64_t pc) {
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
            auto entry = getLineEntryFromPC(low_pc);
            ++entry; 
            setBreakpointAtAddress(offsetDwarfAddress(entry->address));
            return;
        }
    }
    std::cerr << "Couldn't find function with name " 
              << f_name << std::endl;
}

void Debugger::setBreakpointAtLine(const std::string &filename,
																   unsigned b_line)  {
		bool noFile = true;
    for (const auto &cu : dwarf_.compilation_units()) {
				const char *p; size_t length;
				cwk_path_get_basename(at_name(cu.root()).c_str(), &p, &length);
				if (p != filename) continue;
				noFile = false;	

        const auto& lt = cu.get_line_table();

        for (const auto &entry : lt) {
            if (!entry.is_stmt || entry.line != b_line) continue;
            setBreakpointAtAddress(offsetDwarfAddress(entry.address));
            return;
        }
     }
		if (noFile)
				std::cerr << "File doesn't exist" << std::endl;
		else
				std::cerr << "Line out of range" << std::endl;
}

siginfo_t Debugger::getSignalInfo() {
    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, pid_, nullptr, &info);
    return info;
}

std::vector<Symbol> Debugger::lookupSymbol(const std::string &name) {
		if (symbols_.count(name)) return symbols_[name];
		return {};
}

void Debugger::loadSymbols() {

		auto isSymbolTable = [](const elf::section &sec) {
				return sec.get_hdr().type == elf::sht::symtab || 
						   sec.get_hdr().type == elf::sht::dynsym;
		};

		for (auto &sec : elf_.sections()) {
				if (!isSymbolTable(sec)) continue;

				for (auto sym : sec.as_symtab()) {
						auto &data = sym.get_data();
						Symbol my_sym{toSymbolType(data.type()),
												  sym.get_name(),
													data.value};
						symbols_[sym.get_name()].push_back(my_sym);
				}
		}
}

void Debugger::printBacktrace() {
		auto outputFrame = [frame_number = 0] (auto &&func) mutable {
				std::cout << "frame #" << frame_number++ << ": 0x" << dwarf::at_low_pc(func)
						      << ' ' << dwarf::at_name(func) << std::endl;
		};

		auto current_func = getFunctionFromPC(offsetLoadAddress(get_pc()));
		outputFrame(current_func);

		auto frame_ptr = getRegisterValue(pid_, Reg::rbp);
		auto return_addr = readMemory(frame_ptr + 8);
		
		while (dwarf::at_name(current_func) != "main") {
				current_func = getFunctionFromPC(offsetLoadAddress(return_addr));
				outputFrame(current_func);
				frame_ptr = readMemory(frame_ptr);
				return_addr = readMemory(frame_ptr + 8);
		}
}

void Debugger::readVariables() {
		using namespace dwarf;

		auto func = getFunctionFromPC(getOffsetPC());
		for (const auto &die : func) {
				if (die.tag != DW_TAG::variable) continue;

				auto loc_val = die[DW_AT::location];
				if (loc_val.get_type() != value::type::exprloc) continue;

				PtraceExprContext context {pid_};
				auto result = loc_val.as_exprloc().evaluate(&context);
				
				switch(result.location_type) {
				case expr_result::type::address: {
						auto value = readMemory(result.value);
						std::cout << at_name(die) << " (0x" << std::hex << result.value
										  << ") = " << value << std::endl;
						break;
				}
				case expr_result::type::reg: {
						auto value = getRegisterValueFromDwarfRegister(pid_, result.value);
						std::cout << at_name(die) << " (reg " << result.value << ") = "
										  << value << std::endl;
						break;
				}
				default:
						throw std::runtime_error("Unhandled variable location");
				}
		}
}

void Debugger::readVariable(std::string name) {
		using namespace dwarf;
		
		bool found_name = false;
		auto func = getFunctionFromPC(getOffsetPC());
		for (const auto &die : func) {
				if (die.tag != DW_TAG::variable || at_name(die) != name) continue;

				auto loc_val = die[DW_AT::location];
				//TODO read loclists
				if (loc_val.get_type() != value::type::exprloc) continue;

				PtraceExprContext context {pid_};
				auto result = loc_val.as_exprloc().evaluate(&context);
				
				found_name = true;
				switch(result.location_type) {
				case expr_result::type::address: {
						auto value = readMemory(result.value);
						std::cout << at_name(die) << " (0x" << std::hex << result.value
										  << ") = " << value << std::endl;
						break; 
				}
				case expr_result::type::reg: {
						auto value = getRegisterValueFromDwarfRegister(pid_, result.value);
						std::cout << at_name(die) << " (reg " << result.value << ") = "
										  << value << std::endl;
						break;
				}
				default:
						throw std::runtime_error("Unhandled variable location");
				}
		}

		if (!found_name) std::cerr << "Couldn't find variable with the given name"
														   << std::endl;
}
