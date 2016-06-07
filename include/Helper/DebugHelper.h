# pragma once

namespace saib {
namespace debug {

set_debug(string binary)
{
  pid = saib::ptrace::create_child(binary);
  saib::lldb::create_target(binary);
  saib::ptrace::start_child_set_tls();
  module_link_set = saib::ptrace::get_modules();
  map<addr_t, string> modules;
  for(auto iter = module_link_set.begin(); iter != module_link_set.end(); ++iter)
  {
    if(!iter->second.l_name)
      continue;
    modules[iter->first] = iter->second.l_name;
  }
  saib::lldb::add_modules(modules);
}

} // namespace debug
} // namespace saib