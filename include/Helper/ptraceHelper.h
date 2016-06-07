# pragma once

#include <string>

#include <unistd.h>

void create_debugger_by_ptrace(std::string binary);
bool get_mem(uint64_t address, size_t size, void* buf);
bool get_reg(std::string reg_name, void* buf, unsigned buf_size, unsigned& val_size);
pid_t get_pid();