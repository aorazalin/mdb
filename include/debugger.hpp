#include "breakpoint.hpp"
#include "helper.hpp"

#include <string>
#include <unordered_map>



class Debugger {
public:
    Debugger (std::string prog_name, pid_t pid);
// adding some shit
    // Start the debugger
    void run();

    // Handle command entered in cmd
    void handleCommand(const std::string& line);

    // Continue command
    void continueExecution();

    // Setting breakpoint at a given address
    void setBreakpoint(intptr_t at_addr);

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
private:
    std::unordered_map<intptr_t, Breakpoint> m_breakpoints_;
    std::string m_prog_name_;
    pid_t m_pid_;

    int fd;
    elf::elf ef;
    dwarf::dwarf dw;
};
