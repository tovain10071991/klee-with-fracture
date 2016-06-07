#include "Helper/common.h"
#include "Helper/LLDBHelper.h"
#include "Helper/ObjectFileHelper.h"

#include <string>
#include <map>

#include <link.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <sys/user.h>

using namespace std;

static pid_t pid;
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

pid_t create_child(string binary)
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
  wait_assert();
  return pid;
}

addr_t get_start_addr(string binary)
{
  addr_t start_addr = get_addr("main");
  if(!start_addr)
    start_addr = get_entry(binary);
  return start_addr;
}

long get_reg(string reg_name)
{
  struct user_regs_struct regs;
  ptrace_assert(PTRACE_GETREGS, pid, 0, &regs);
  map<string, long> name_reg_map = {
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
  if(name_reg_map.find(omit_case(reg_name))==name_reg_map.end())
    errx(-1, "can't find reg: %s", reg_name.c_str());
  return name_reg_map[reg_name];
}

bool get_reg(std::string reg_name, void* buf, unsigned buf_size, unsigned& val_size)
{
  assert(buf_size = sizeof(long));
  *(long*)buf = get_reg(reg_name);
  val_size = sizeof(long);
  return true;
}

bool get_mem(addr_t addr, size_t size, void* buf)
{
  size_t ts = (size+sizeof(long))/sizeof(long);
  long* tmp = (long*)malloc(ts*sizeof(long));
  if(tmp==NULL)
    errx(-1, "malloc: fail to allocate tmp in readata()\n");
  for(size_t i=0;i<ts;i++)
    *(tmp+i) = ptrace_assert(PTRACE_PEEKDATA, pid, (void*)(addr+sizeof(long)*i), 0, "read to tmp in get_mem");
  memcpy(buf, tmp, size);
  free(tmp);
  return true;
}

static long breakpoint_bytes;

void set_breakpoint(addr_t addr)
{
	//设置断点
	//将addr的头一个字节(第一个字的低字节)换成0xCC
  get_mem(addr, sizeof(long), &breakpoint_bytes);
  long temp = (breakpoint_bytes & 0xFFFFFFFFFFFFFF00) | 0xCC;
  ptrace_assert(PTRACE_POKETEXT, pid, (void*)addr, (void*)temp);
}

void remove_breakpoint(addr_t addr)
{
  //恢复断点
  struct user_regs_struct regs;
  ptrace_assert(PTRACE_GETREGS, pid, NULL, &regs);
  //软件断点会在断点的下一个字节停住,所以还要将RIP向前恢复一个字节
  regs.rip-=1;
  assert(addr == regs.rip);
  // printf("0x%llx\n", regs.rip);
  ptrace_assert(PTRACE_SETREGS, pid, NULL, &regs);
  ptrace_assert(PTRACE_POKETEXT, pid, (void*)regs.rip, (void*)breakpoint_bytes);
}

void set_syscall_intercept()
{
  ptrace_assert(PTRACE_SYSCALL, pid, NULL, NULL);
}

void continue_process()
{
  //执行子进程
  ptrace_assert(PTRACE_CONT, pid, 0, 0);
  wait_assert();
}

bool is_reach_syscall()
{
  unsigned long long pc = get_reg("rip");
  uint8_t inst_bytes[2];
  get_mem(pc-2, 2, inst_bytes);
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
  return false;
}

bool is_reach_start()
{
  unsigned long long pc = get_reg("rip");
  uint8_t inst_bytes[1];
  get_mem(pc-1, 1, inst_bytes);
  if(inst_bytes[0] != 0xcc)
    return false;
  else
    return true;
}

static bool tls_use_fs;
static addr_t tls_base;

void start_child_set_tls(string binary)
{
  // launch child to main or entry and intercept arch_prctl by the way
  // get main's addr, if can't, get entry
  addr_t start_addr = get_start_addr(binary);
  set_breakpoint(start_addr);
  unsigned meet_arch_prctl = 0;
  while(1)
  {
    set_syscall_intercept();
    wait_assert();
    if(is_arch_prctl())
      ++meet_arch_prctl;
    if(is_arch_prctl() && meet_arch_prctl==1)
    {
      unsigned long long prctl_code = get_reg("rdi");
      assert((unsigned long long)((int)prctl_code) == prctl_code);
      if(prctl_code == ARCH_SET_FS || prctl_code == ARCH_SET_GS)
      {
        if(prctl_code == ARCH_SET_FS)
          tls_use_fs = true;
        else if(prctl_code == ARCH_SET_GS)
          tls_use_fs = false;
        tls_base = get_reg("rsi");
      }
    }
    assert(!is_arch_prctl() || meet_arch_prctl == 1 || meet_arch_prctl == 2);
    if(is_reach_start())
    {
      unsigned long long pc = get_reg("rip");
      remove_breakpoint(pc-1);
      return;
    }
  }
}

void create_debugger_by_ptrace(string binary)
{
  pid = create_child(binary);
  create_debugger_by_lldb(binary);
  start_child_set_tls(binary);
}

pid_t get_pid()
{
  return pid;
}