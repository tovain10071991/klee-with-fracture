#include "LLDBHelper.h"
#include "ptraceHelper.h"
#include "MapHelper.h"

#include <string>
#include <map>

#include <unistd.h>

using namespace std;

namespace saib {
namespace debug {

static pid_t pid;

set_debug(string binary)
{
  pid = saib::ptrace::create_child(binary);
  saib::lldb::create_target(binary);
  saib::ptrace::start_child_set_tls();
  map<addr_t, string> modules  = saib::ptrace::get_modules();
  saib::lldb::add_modules(modules);
}

} // namespace debug
} // namespace saib