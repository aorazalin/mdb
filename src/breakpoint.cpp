#include <sys/ptrace.h>

#include "breakpoint.hpp"
#include "helper.hpp"

void Breakpoint::enable() {
    // auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    // m_saved_data = static_cast<uint8_t>(data & 0xff); // bottom byte 
    // uint64_t int3 = INTERRUPT_COMMAND;
    // uint64_t data_with_int3 = ((data & ~0xff) | int3); // set bottom byte to 0xcc
    // ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);
    // m_enabled = true;
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    m_saved_data = static_cast<uint8_t>(data & 0xff); //save bottom byte
    uint64_t int3 = 0xcc;
    uint64_t data_with_int3 = ((data & ~0xff) | int3); //set bottom byte to 0xcc
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);

    m_enabled = true;
}

void Breakpoint::disable() {
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    auto restored_data = ((data & ~0xff) | m_saved_data);
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, restored_data);
    m_enabled = false;
}