# pragma once

#include <unistd.h>

#include <string>

namespace saib {
namespace ptrace {

void create_child(std::string binary);

} // namespace ptrace
} // namespace saib