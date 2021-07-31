#include <sys/types.h>
#include <stdint.h>

class Breakpoint {
public:
    Breakpoint() = default;
    Breakpoint(pid_t pid, intptr_t addr)
        : m_pid_{pid}, m_addr_{addr}, m_enabled_{false}, m_saved_data_{} {}
    
    void enable();
    void disable();

    bool isEnabled() const { return m_enabled_; }
    intptr_t getAddress() const { return m_addr_; }

private:
    pid_t m_pid_; //TODO --- find meanings of these things
    intptr_t m_addr_; //
    bool m_enabled_; // is breakpoint functional
    uint8_t m_saved_data_; // data which used to be at the breakpoint address
};
