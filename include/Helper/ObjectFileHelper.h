# pragma once

#include "Helper/common.h"

#include "llvm/Object/ObjectFile.h"

#include <string>

addr_t get_entry(std::string binary);
addr_t get_entry(llvm::object::ObjectFile* obj);