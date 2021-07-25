#include <iostream>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <unistd.h>

#include <linenoise.h>

#include "debugger.hpp"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Program name not specified\n";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();

    if (pid == 0) {
        // child process --> debuggee
        //TODO check ptrace error codes
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr); // I allow
                                                     // parent process
                                                     // to trace me 
        execl(prog, prog, nullptr); // execute (???)
    } else if (pid >= 1) {
        // parent process --> debugger
        std::cout << "Started debugging process " << pid << '\n';
        Debugger dbg{prog, pid};
        dbg.run();
    }
}