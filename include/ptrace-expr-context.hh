#ifndef PTRACE_EXPR_CONTEXT_HH
#define PTRACE_EXPR_CONTEXT_HH

#include "dwarf++.hh"
#include "helper.hh"

#include <sys/user.h>
#include <sys/ptrace.h>


class PtraceExprContext : public dwarf::expr_context {
public:
		PtraceExprContext(pid_t pid) : pid_{pid} { }
		
		dwarf::taddr reg(unsigned regnum) override {
				return getRegisterValueFromDwarfRegister(pid_, regnum);
		}

		dwarf::taddr pc() override {
				struct user_regs_struct regs;
				ptrace(PTRACE_GETREGS, pid_, nullptr, &regs);
				return regs.rip;
		}

		dwarf::taddr deref_size(dwarf::taddr address, unsigned size) override {
				//TODO take size into account
				return ptrace(PTRACE_PEEKDATA, pid_, address, nullptr);
		}
private:
		pid_t pid_;
};

#endif
