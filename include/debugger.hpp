#include "breakpoint.hpp"

#include <string>
#include <unordered_map>



class Debugger {
public:
    Debugger (std::string prog_name, pid_t pid)
        : m_prog_name_(std::move(prog_name)), m_pid_(pid) {}

    void run();

    void handleCommand(const std::string& line);

    void continueExecution();

    void setBreakpoint(intptr_t at_addr);

    void dumpRegisters();

    uint64_t readMemory(uint64_t address);

    //TODO try process_vm_readv, process_vm_writev or /proc/<pid>/mem instead
    void writeMemory(uint64_t address, uint64_t value); 

    uint64_t get_pc();

    void set_pc(uint64_t);

    void stepOverBreakpoint();

    void waitForSignal();
private:
    std::unordered_map<intptr_t, Breakpoint> m_breakpoints_;
    std::string m_prog_name_;
    pid_t m_pid_;
};