#include "breakpoint.hpp"

#include <string>
#include <unordered_map>



class Debugger {
public:
    Debugger (std::string prog_name, pid_t pid)
        : m_prog_name(std::move(prog_name)), m_pid(pid) {}

    void run();

    void handle_command(const std::string& line);

    void continue_execution();

    void set_breakpoint(intptr_t at_addr);

private:
    std::unordered_map<intptr_t, Breakpoint> m_breakpoints;
    std::string m_prog_name;
    pid_t m_pid;
};