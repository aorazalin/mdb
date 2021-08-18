#include <sys/ptrace.h>

#include "breakpoint.hpp"
#include "helper.hpp"

void Breakpoint::enable() {
    auto data = ptrace(PTRACE_PEEKDATA, pid_, addr_, nullptr);
    saved_data_ = static_cast<uint8_t>(data & 0xff); //save bottom byte
    uint64_t int3 = 0xcc;
    uint64_t data_with_int3 = ((data & ~0xff) | int3); //set bottom byte to 0xcc
    ptrace(PTRACE_POKEDATA, pid_, addr_, data_with_int3);
    enabled_ = true;
}

void Breakpoint::disable() {
    auto data = ptrace(PTRACE_PEEKDATA, pid_, addr_, nullptr);
    auto restored_data = ((data & ~0xff) | saved_data_);
    ptrace(PTRACE_POKEDATA, pid_, addr_, restored_data);
    enabled_ = false;
}
