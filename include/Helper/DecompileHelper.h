#ifndef _DECOMPILER_HELPER_H_
#define _DECOMPILER_HELPER_H_

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

#include <string>

llvm::Module* get_module(std::string binary);
llvm::Function* getFunction(unsigned long addr);

#endif