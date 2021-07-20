#include <sys/types.h>
#include <stdint.h>

class Breakpoint {
public:
    Breakpoint(pid_t pid, intptr_t addr)
        : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_data{} {}
    
    void enable();
    void disable();

    bool is_enabled() const { return m_enabled; }
    intptr_t get_address() const { return m_addr; }

private:
    pid_t m_pid; //TODO --- find meanings of these things
    intptr_t m_addr; //
    bool m_enabled; // is breakpoint functional
    uint8_t m_saved_data; // data which used to be at the breakpoint address
};