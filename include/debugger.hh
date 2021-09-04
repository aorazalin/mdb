#ifndef DEBUGGER_HH
#define DEBUGGER_HH

#include "dwarf++.hh"
#include "elf++.hh"

#include "breakpoint.hh"
#include "helper.hh"

#include <string>
#include <unordered_map>
#include <signal.h>



class Debugger {
public:
    // Constructor that takes program name & process ID
    Debugger (std::string prog_name, pid_t pid);

    // Start the debugger
    void run();

    // Handle command entered in cmd
    void handleCommand(const std::string& line);

    // Continue command
    void continueExecution();

    // Setting breakpoint at a given address
    void setBreakpointAtAddress(intptr_t at_addr);

    // Printout values of all registers
    void dumpRegisters();

    // Return value at that memory address
    uint64_t readMemory(uint64_t address);

    //TODO try process_vm_readv, process_vm_writev or /proc/<pid>/mem instead
    // write on memory
    void writeMemory(uint64_t address, uint64_t value); 

    // return Progam Counter (PC)
    uint64_t get_pc();

    // Set PC
    void set_pc(uint64_t);

    // Step over a line if on a breakpoint
    void stepOverBreakpoint();

    // Wait for signal of debugee 
    void waitForSignal();

    // Which function I am currently at?
    void whichFunction();

    // Which line I am currently at?
    void whichLine();

    // get #line number

    dwarf::line_table::iterator getLineEntryFromPC(uint64_t pc);

    void setBreakpointAtFunction(std::string f_name);

    void setBreakpointAtLine(const std::string &filename, unsigned line_number);

    void initLoadAddress();

    uint64_t offsetLoadAddress(uint64_t addr);

		uint64_t offsetDwarfAddress(uint64_t addr);

    void printSource(const std::string &file_name,
                               unsigned line,
                               unsigned n_lines_context = 2);

    siginfo_t getSignalInfo(); 

    void handleSigtrap(siginfo_t info); 

    void singleStepWithBreakpointCheck();

    void singleStep();

		uint64_t getOffsetPC();

		void stepOut();

		void stepIn();

		void stepOver();

		void removeBreakpoint(std::intptr_t remove_addr);

		dwarf::die getFunctionFromPC(uint64_t pc);

		std::vector<Symbol> lookupSymbol(const std::string &name);

		void loadSymbols();

		void printBacktrace();

		void readVariables();

		void readVariable(std::string name);
private:
    std::unordered_map<intptr_t, Breakpoint> breakpoints_;
    std::string prog_name_;
    pid_t pid_;
    uint64_t load_addr_{0};
    elf::elf elf_;
    dwarf::dwarf dwarf_;
		std::unordered_map<std::string, std::vector<Symbol>> symbols_;
};

#endif
