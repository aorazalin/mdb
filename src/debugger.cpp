#include "debugger.hpp"
#include "helper.hpp"

#include "linenoise/linenoise.h"

#include <sys/wait.h>
#include <sys/ptrace.h>
#include <iostream>

void Debugger::run() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options); // wait for child process to finish

    char *line = nullptr;
    while ((line = linenoise("minidbg> ")) != nullptr) {
        handle_command(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void Debugger::handle_command(const std::string& line) {
    auto args = split(line, ' ');
    auto command = args[0];

    if (is_prefix(command, "continue")) {
        continue_execution();
    } else {
        std::cerr << "Unknown command\n";
    }
}

void Debugger::continue_execution() {
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);
}
