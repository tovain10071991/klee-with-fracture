#include "Helper/ptraceHelper.h"
#include "Helper/LLDBHelper.h"
#include "Helper/ELFHelper.h"
#include "Helper/common.h"

#include <string>
#include <map>

#include <unistd.h>
#include <link.h>
#include <err.h>

using namespace std;

static string main_binary;

map<addr_t, string> get_link_modules(string binary)
{
  map<addr_t, string> link_modules;
  //从主模块获取链接信息
  struct link_map* lm_ptr;
  get_mem(get_got_plt_addr(binary)+8, 8, &lm_ptr);
  while(lm_ptr!=NULL)
  {
    struct link_map lm;
    get_mem((uint64_t)lm_ptr, sizeof(struct link_map), &lm);
    char name[200];
    get_mem((addr_t)lm.l_name, 200, name);
    assert(string(name).size()<200);
    link_modules[lm.l_addr] = name;
    lm_ptr=lm.l_next;
  }
  return link_modules;
}

void create_debugger(string binary)
{
  main_binary = binary;
  create_debugger_by_ptrace(binary);
  map<addr_t, string> link_modules = get_link_modules(binary);
  add_modules(link_modules);
}

unsigned long get_base(string module_name)
{
  if(!get_absolute(module_name).compare(get_absolute(main_binary)))
    return 0;
  //从主模块获取链接信息
  struct link_map* lm_ptr;
  get_mem(get_got_plt_addr(main_binary)+8, 8, &lm_ptr);  
  while(lm_ptr!=NULL)
  {
    struct link_map lm;
    get_mem((uint64_t)lm_ptr, sizeof(struct link_map), &lm);
    char name[200];
    get_mem((addr_t)lm.l_name, 200, name);
    assert(string(name).size()<200);
    if(!module_name.compare(name))
    {
      return lm.l_addr;
    }
    lm_ptr=lm.l_next;
  }
  errx(-1, "can't find module in get_base(name)");
}