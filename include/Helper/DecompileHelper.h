#ifndef _DECOMPILER_HELPER_H_
#define _DECOMPILER_HELPER_H_

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

#include <string>

llvm::Module* get_module(std::string binary);
llvm::Function* get_first_func(unsigned long addr);
llvm::Function* get_first_func(std::string func_name);
llvm::Module* get_module_with_function(std::string func_name);

#endif