#ifndef BREAKPOINT_HPP
#define BREAKPOINT_HPP

#include <sys/types.h>
#include <stdint.h>

class Breakpoint {
public:
    Breakpoint() = default;
    Breakpoint(pid_t pid, intptr_t addr)
        : pid_{pid}, addr_{addr}, enabled_{false}, saved_data_{} {}
    
    // Enable breakpoint
    void enable();

    // Disable breakpoint
    void disable();

    // Is breakpoint on
    bool isEnabled() const { return enabled_; }

    // Get address
    intptr_t getAddress() const { return addr_; }
private:
    pid_t pid_; // process id
    intptr_t addr_; // id of the breakpoint
    bool enabled_; // is breakpoint "on"
    uint8_t saved_data_; // saved data byte
};

#endif
