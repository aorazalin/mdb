#include <sys/types.h>
#include <stdint.h>

class Breakpoint {
public:
    Breakpoint() = default;
    Breakpoint(pid_t pid, intptr_t addr)
        : m_pid_{pid}, m_addr_{addr}, m_enabled_{false}, m_saved_data_{} {}
    
    // Enable breakpoint
    void enable();

    // Disable breakpoint
    void disable();

    // Is breakpoint on
    bool isEnabled() const { return m_enabled_; }

    // Get address
    intptr_t getAddress() const { return m_addr_; }
private:
    pid_t m_pid_; // process id
    intptr_t m_addr_; // id of the breakpoint
    bool m_enabled_; // is breakpoint "on"
    uint8_t m_saved_data_; // saved data byte
};
