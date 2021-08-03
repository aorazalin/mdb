#include <iostream>
#include <algorithm>

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/personality.h>
#include <unistd.h>
#include <sys/user.h>

#include <linenoise.h>

#include "debugger.hpp"

//TODO --- add floating point & vector registers in the future


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Program name not specified\n";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();
    if (pid < 0) {
        std::cerr << "Forking failed!" << std::endl;
        exit(1);
    }
    else if (pid == 0) {
        // child process --> debuggee
        //TODO check ptrace error codes
        personality(ADDR_NO_RANDOMIZE); // turn off space randomization
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr); // I allow
                                                     // parent process
                                                     // to trace me 
        execl(prog, prog, nullptr); // execute (???)
    }
    else if (pid >= 1) {
        // parent process --> debugger
        std::cout << "Started debugging process " << pid << std::endl;
        Debugger dbg{prog, pid};
        dbg.run();
    }
}
