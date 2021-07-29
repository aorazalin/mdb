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

private:
    std::unordered_map<intptr_t, Breakpoint> m_breakpoints_;
    std::string m_prog_name_;
    pid_t m_pid_;
};