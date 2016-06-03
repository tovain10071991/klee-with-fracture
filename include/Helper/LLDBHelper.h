#ifndef _LLDB_HELPER_H_
#define _LLDB_HELPER_H_

#include "llvm/Object/ObjectFile.h"

#include <string>

void create_debugger(llvm::object::ObjectFile* obj);
void create_debugger(std::string binary);
llvm::object::ObjectFile* get_object(unsigned long addr);
unsigned long get_base(std::string module_name);
unsigned long get_base(unsigned long addr);
bool get_reg(std::string reg_name, uint64_t& value);
bool get_reg(std::string reg_name, void* buf, unsigned buf_size, unsigned& val_size);
bool get_mem(uint64_t address, size_t size, void* buf);
int get_pid();
std::string get_func_name_in_plt(uint64_t addr);
unsigned long get_addr(std::string name);
std::string get_func_name(unsigned long addr);
unsigned long get_unload_addr(unsigned long addr);
std::string get_mangled_name(std::string name);
unsigned long get_section_load_addr(std::string obj_name, std::string sec_name);
unsigned get_sym_unload_endaddr(unsigned unload_addr, std::string obj_name, std::string sec_name);

#endif