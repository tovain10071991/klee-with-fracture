#include "Helper/common.h"
#include "Helper/ptraceHelper.h"

#include <link.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

#include <string>
#include <map>

using namespace std;
using namespace llvm;

static map<addr_t, struct link_map> module_link_set;
static addr_t tls_base;
static bool tls_use_fs;
static pid_t pid;

namespace saib {
namespace ptrace {

long ptrace_assert(enum __ptrace_request req, int pid, void* addr, void* data, string msg="")
{
	errno = 0;
	long ret;
	if((ret = ptrace(req, pid, addr, data))==-1&&errno!=0)
		err(errno, "%s", msg.c_str());
	return ret;
}

bool wait_assert(string msg="")
{
	int status;
	if(waitpid(pid, &status, 0)==-1)
		err(errno, "fail in wait(%s)", msg.c_str());
	if(WIFEXITED(status))
	{
		warnx("normally exit(%s): %d", msg.c_str(), WEXITSTATUS(status));
		return false;
	}
	if(WIFSTOPPED(status)&&WSTOPSIG(status)!=SIGTRAP)
		errx(-1, "don't handler this STOPSIG(%s): %d\n", msg.c_str(), WSTOPSIG(status));
	if(WIFSIGNALED(status))
		errx(-1, "don't handler this TERMSIG(%s): %d\n", msg.c_str(), WTERMSIG(status));
	return true;
}

pid_t create_child(string bianry)
{
  pid = fork();
  if(pid==0)
  {
    ptrace_assert(PTRACE_TRACEME, 0, 0, 0, "PTRACE_TRACEME in create_child");
    if(execl(binary.c_str(), binary.c_str(), NULL)==-1)
      err(errno, "execv in create_child");
  }
  else if(pid<0)
    err(errno, "fork in create_child");
  return pid;
}

addr_t get_func_load_addr(string func_name)
{
}

class Breakpoint {
  static addr_t setted_addr;
  static long hitted_long;
public:
  static void set_breakpoint(addr_t addr);
  static void remove_breakpoint(addr_t addr);
};
addr_t Breakpoint::setted_addr;
long Breakpoint::hitted_byte_set;

get_reg(string reg_name)
{
  struct user_regs_struct regs;
  ptrace_assert(PTRACE_GETREGS, pid, 0, &regs);
  map name_reg_map = {
   {"r15", regs.r15},
   {"r14", regs.r14},
   {"r13", regs.r13},
   {"r12", regs.r12},
   {"rbp", regs.rbp},
   {"rbx", regs.rbx},
   {"r11", regs.r11},
   {"r10", regs.r10},
   {"r9", regs.r9},
   {"r8", regs.r8},
   {"rax", regs.rax},
   {"rcx", regs.rcx},
   {"rdx", regs.rdx},
   {"rsi", regs.rsi},
   {"rdi", regs.rdi},
   {"orig_rax", regs.orig_rax},
   {"rip", regs.rip},
   {"cs", regs.cs},
   {"eflags", regs.eflags},
   {"rsp", regs.rsp},
   {"ss", regs.ss},
   {"fs_base", regs.fs_base},
   {"gs_base", regs.gs_base},
   {"ds", regs.ds},
   {"es", regs.es},
   {"fs", regs.fs},
   {"gs", regs.gs}};
  if(name_reg_map.find(get_lower(reg_name))==name_reg_map.end())
    errx(-1, "can't find reg: %s", name.c_str());
  return name_reg_map[reg_name];
}

void read_memory(addr_t addr, void* buf, size_t size)
{
	size_t ts = (size+sizeof(long))/sizeof(long);
	long* tmp = malloc(ts*sizeof(long));
	if(tmp==NULL)
		errx(-1, "malloc: fail to allocate tmp in readata()\n");
	for(size_t i=0;i<ts;i++)
		*(tmp+i) = ptrace_assert(PTRACE_PEEKDATA, pid, (void*)(addr+sizeof(long)*i), 0, "read to tmp in read_memory");
	memcpy(buf, tmp, size);
	free(tmp);
}

void Breakpoint::set_breakpoint(addr_t addr)
{
	//设置断点
	//将addr的头一个字节(第一个字的低字节)换成0xCC
  setted_addr = addr;
  read_memory(addr, &hitted_long, sizeof(long));
	hitted_long=ptrace_assert(PTRACE_PEEKTEXT, pid, addr, 0);
  hitted_byte_set[addr] = hitted_long & 0xff;
	long temp = hitted_long & 0xFFFFFFFFFFFFFF00 | 0xCC;
	ptrace_assert(PTRACE_POKETEXT, pid, addr, temp);
}

void Breakpoint::remove_breakpoint(addr_t addr)
{
  assert(setted_addr == addr);  
	//恢复断点
	ptrace_assert(PTRACE_GETREGS, pid, NULL, &regs);
	//软件断点会在断点的下一个字节停住,所以还要将EIP向前恢复一个字节
	regs.rip-=1;
  assert(addr == regs.rip);
	printf("0x%llx\n", regs.rip);
	ptrace_assert(PTRACE_SETREGS, pid, NULL, &regs);
	ptrace_assert(PTRACE_POKETEXT, pid, regs.rip, hitted_long);
}

void set_syscall_intercept()
{
  ptrace_assert(PTRACE_SYSCALL, pid, NULL, NULL);
}

void continue()
{
  //执行子进程
	ptrace_assert(PTRACE_CONT, pid, 0, 0);
	wait_assert();
}

bool is_reach_syscall()
{
  unsigned long long pc = get_reg("rip");
  uint8_t inst_bytes[2];
  read_memory(pc-2, inst_bytes, 2);
  if(inst_bytes[0] != 0xf || inst_bytes[1] != 5)
    return false;
  else
    return true;
}

bool is_arch_prctl()
{
  if(!is_reach_syscall())
    return false;
  unsigned long long sys_num = get_reg("orig_rax");
  if(sys_num == SYS_arch_prctl)
    return true;
}

bool is_reach_start()
{
  unsigned long long pc = get_reg("rip");
  uint8_t inst_bytes[1];
  read_memory(pc-1, inst_bytes, 1);
  if(inst_bytes[0] != 0xcc)
    return false;
  else
    return true;
}

addr_t start_child_set_tls()
{
  // launch child to main or entry and intercept arch_prctl by the way
  // get main's addr, if can't, get entry
  addr_t start_addr = get_load_start_addr();
  set_breakpoint(start_addr);
  while(1)
  {
    set_syscall_intercept();
    continue();
    if(is_arch_prctl())
    {
      unsigned long long prctl_code = get_reg("rdi");
      assert((unsigned long long)((int)prctl_code) == prctl_code);
      if(prctl_code == ARCH_SET_FS || prctl_code == ARCH_SET_GS)
      {
        if(prctl_code == ARCH_SET_FS)
          tls_use_fs = true;
        else if(prctl_code == ARCH_SET_GS)
          tls_use_fs = false;
        tls_base = get_reg("rdi");
      }
    }
    if(is_reach_start())
    {
      unsigned long long pc = get_reg("rip");
      Breakpoint::remove_breakpoint(pc-1);
      return start_addr;
    }
  }
}

map<addr_t, string> get_modules()
{
  //从主模块获取链接信息
  struct link_map* lm_ptr;
  read_memory(get_got_plt_addr()+8, &lm_ptr, sizeof(addr_t));
  while(lm_ptr!=NULL)
  {
    struct link_map lm;
    read_memory(lm_ptr, &lm, sizeof(struct link_map));
    module_link_set[lm.l_addr] = lm;
    lm_ptr=lm.l_next;
  }

  map<addr_t, string> modules;
  for(auto iter = module_link_set.begin(); iter != module_link_set.end(); ++iter)
  {
    if(!iter->second.l_name)
      continue;
    modules[iter->first] = iter->second.l_name;
  }
  return modules;
}

create_child(string binary)
{
  pid = create_child(binary);
  create_target(binary);
  start_child_set_tls();
  module_link_set = get_modules();
  map<addr_t, string> modules;
  for(auto iter = module_link_set.begin(); iter != module_link_set.end(); ++iter)
  {
    if(!iter->second.l_name)
      continue;
    modules[iter->first] = iter->second.l_name;
  }
}

} // namespace ptrace
} // namespace saib