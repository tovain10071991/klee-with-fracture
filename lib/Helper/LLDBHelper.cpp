
#include "Helper/LLDBHelper.h"
#include "Helper/ELFHelper.h"

#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/ELFObjectFile.h"

#include "lldb/API/SBDebugger.h"
#include "lldb/API/SBTarget.h"
#include "lldb/API/SBProcess.h"
#include "lldb/API/SBModule.h"
#include "lldb/API/SBBreakpoint.h"
#include "lldb/API/SBThread.h"
#include "lldb/API/SBFrame.h"
#include "lldb/API/SBValueList.h"
#include "lldb/API/SBValue.h"
#include "lldb/API/SBStream.h"

#include <link.h>
#include <err.h>
#include <unistd.h>
#include <locale>
#include <cctype>

// #include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <stdio.h>

using namespace std;
using namespace lldb;
using namespace llvm;

#define EXECUTABLE_BASE 0x400000
#define CONVERT(base) (base?base:EXECUTABLE_BASE) 

static SBDebugger debugger;
static SBTarget target;
static SBProcess process;
static object::ObjectFile* main_obj;

class LLDBInited {
public:
  LLDBInited()
  {
    SBDebugger::Initialize();
    debugger = SBDebugger::Create();
    debugger.SetAsync(false);
  }
};

static LLDBInited inited;

uint64_t get_entry(object::ObjectFile* obj)
{
  auto elf_file = dyn_cast<object::ELF64LEObjectFile>(obj)->getELFFile();
  auto ehdr = elf_file->getHeader();
  return ehdr->e_entry;
}

void create_debugger(object::ObjectFile* obj)
{
  main_obj = obj;
  target = debugger.CreateTarget(obj->getFileName().data());
  SBBreakpoint breakpoint_for_launch = target.BreakpointCreateByAddress(get_entry(obj));  
  assert(breakpoint_for_launch.IsValid());
  SBLaunchInfo launch_info(NULL);
  SBError error;
  process = target.Launch(launch_info, error);
  assert(error.Success());
  
  SBBreakpoint breakpoint_for_main = target.BreakpointCreateByAddress(get_addr("main"));
  assert(breakpoint_for_main.IsValid());
  error = process.Continue();
  assert(error.Success());
}

void create_debugger(string binary)
{
  main_obj = object::ObjectFile::createObjectFile(binary);
  assert(main_obj->isObject() && main_obj->isELF() && "it is not object");
  create_debugger(main_obj);
}

string get_absolute(string name)
{
  if(name[0] == '/')
    return name;
  return string(get_current_dir_name()) + "/" + name;
}

string get_absolute(SBFileSpec file_spec)
{
  return string(file_spec.GetDirectory())+"/"+file_spec.GetFilename();
}

unsigned long get_base(string module_name)
{
  if(module_name[0]!='/')
    module_name = string(get_current_dir_name())+"/"+module_name;
  if(!module_name.compare(main_obj->getFileName().front()!='/'?string(get_current_dir_name())+"/"+(main_obj->getFileName().str()):main_obj->getFileName().str()))
    return 0;

  //从主模块获取链接信息
  struct link_map* lm = new link_map;
  if(lm==NULL)
    errx(-1, "malloc: fail to allocate link_map\n");
  struct link_map* lm_ptr;
  get_mem(get_got_plt_addr(main_obj->getFileName().data())+8, 8, &lm_ptr);  
  while(lm_ptr!=NULL)
  {
    get_mem((uint64_t)lm_ptr, sizeof(struct link_map), lm);
    char name[200];
    SBError error;
    assert(process.ReadCStringFromMemory((addr_t)lm->l_name, name, 200, error)<200);
    if(!error.Success())
      errx(-1, "read mem failed in get_base(name): %s", error.GetCString());
    if(!module_name.compare(name))
    {
      return lm->l_addr;
    }
    lm_ptr=lm->l_next;
  }
  errx(-1, "can't find module in get_base(name)");
}

unsigned long get_base(unsigned long addr)
{
  for(unsigned i = 0, num = target.GetNumModules(); i < num; ++i)
  {
    SBModule module = target.GetModuleAtIndex(i);
    SBFileSpec file_spec = module.GetFileSpec();
    string module_name = string(file_spec.GetDirectory())+"/"+file_spec.GetFilename();
    unsigned long base = get_base(module_name);
    if(addr<CONVERT(base))
      continue;
    object::ObjectFile* obj = object::ObjectFile::createObjectFile(module_name);
    if(addr<CONVERT(base)+obj->getData().size())
    {
      delete obj;
      return base;
    }
    delete obj;
  }
  errx(-1, "can't find module in get_base(addr): %lx", addr);
}

object::ObjectFile* get_object(unsigned long addr)
{
  for(unsigned i = 0, num = target.GetNumModules(); i < num; ++i)
  {
    SBModule module = target.GetModuleAtIndex(i);
    SBFileSpec file_spec = module.GetFileSpec();
    string module_name = string(file_spec.GetDirectory())+"/"+file_spec.GetFilename();
    unsigned long base = get_base(module_name);
    if(addr<CONVERT(base))
      continue;
    object::ObjectFile* obj = object::ObjectFile::createObjectFile(module_name);
    if(addr<CONVERT(base)+obj->getData().size())
      return obj;
    delete obj;
  }
  errx(-1, "can't find module in get_object");
}

string omit_case(string name)
{
  transform(name.begin(), name.end(), name.begin(), ::tolower);
  return name;
}

SBValue get_child(SBValue val, string reg_name)
{
  if(!omit_case(val.GetName()).compare(omit_case(reg_name)))
    return val;
  SBValue child;
  assert(!child.IsValid());
  for(uint32_t i = 0; i < val.GetNumChildren(); ++i)
  {
    child = get_child(val.GetChildAtIndex(i), reg_name);
    if(child.IsValid())
      return child;
  }
  return child;
}

bool get_reg(string reg_name, SBData& data)
{
  SBThread thread = process.GetSelectedThread();
  SBFrame frame = thread.GetSelectedFrame();
  SBValueList vals = frame.GetRegisters();

  for(uint32_t i = 0; i < vals.GetSize(); ++i)
  {
    SBValue regs = vals.GetValueAtIndex(i);
    SBValue child = get_child(regs, reg_name);
    if(!child.IsValid())
      continue;
    data = child.GetData();
    return true;
  }
  warnx("can.t find reg: %s", reg_name.c_str());
  return false;
}

bool get_reg(string reg_name, uint64_t& value)
{
  if(!reg_name.compare("EFLAGS"))
    return false;
  SBThread thread = process.GetSelectedThread();
  SBFrame frame = thread.GetSelectedFrame();
  SBValueList vals = frame.GetRegisters();
  
  if(!reg_name.compare("OF") || !reg_name.compare("SF") || !reg_name.compare("ZF") || !reg_name.compare("AF") || !reg_name.compare("PF") || !reg_name.compare("CF") || !reg_name.compare("TF") || !reg_name.compare("IF") || !reg_name.compare("DF") || !reg_name.compare("RF") || !reg_name.compare("NT"))
  {
    uint64_t flags;
    assert(get_reg("rflags", flags));
    map<string, unsigned> flag_off_map = {
      {"CF", 0},
      {"PF", 2},
      {"AF", 4},
      {"ZF", 6},
      {"SF", 7},
      {"TF", 8},
      {"IF", 9},
      {"DF", 10},
      {"OF", 11},
      {"NT", 14},
      {"RF", 16}};
    value = (flags>>flag_off_map[reg_name])&1;
    return true;
  }
  
  for(uint32_t i = 0; i < vals.GetSize(); ++i)
  {
    SBValue regs = vals.GetValueAtIndex(i);
    SBValue child = get_child(regs, reg_name);
    if(!child.IsValid())
      continue;
    SBData data = child.GetData();
    if(data.GetByteSize() > 8)
      warnx("size > 8: %s", reg_name.c_str());
    SBError error;
    assert(data.ReadRawData(error, 0, &value, data.GetByteSize())==data.GetByteSize());
    cout << child.GetName() << " - size: " << data.GetByteSize() << " value: " << hex << value << endl;
    return true;
  }
  warnx("can.t find reg: %s", reg_name.c_str());
  return false;
}

bool get_reg(string reg_name, void* buf, unsigned buf_size, unsigned& val_size)
{
  SBData data;
  if(get_reg(reg_name, data))
  {
    val_size = data.GetByteSize();
    assert(val_size<=buf_size);
    SBError error;
    assert(data.ReadRawData(error, 0, buf, data.GetByteSize())==data.GetByteSize());
    return true;
  }
  else if(!reg_name.compare("OF") || !reg_name.compare("SF") || !reg_name.compare("ZF") || !reg_name.compare("AF") || !reg_name.compare("PF") || !reg_name.compare("CF") || !reg_name.compare("TF") || !reg_name.compare("IF") || !reg_name.compare("DF") || !reg_name.compare("RF") || !reg_name.compare("NT"))
  {
    uint64_t flags;
    assert(get_reg("rflags", flags));
    map<string, unsigned> flag_off_map = {
      {"CF", 0},
      {"PF", 2},
      {"AF", 4},
      {"ZF", 6},
      {"SF", 7},
      {"TF", 8},
      {"IF", 9},
      {"DF", 10},
      {"OF", 11},
      {"NT", 14},
      {"RF", 16}};
    *(uint64_t*)buf = (flags>>flag_off_map[reg_name])&1;
    val_size = 1;
    return true;
  }
  else if(!reg_name.compare("ZMM0") || !reg_name.compare("ZMM1") || !reg_name.compare("ZMM2") || !reg_name.compare("ZMM3") || !reg_name.compare("ZMM4") || !reg_name.compare("ZMM5") || !reg_name.compare("ZMM6") || !reg_name.compare("ZMM7") || !reg_name.compare("ZMM8") || !reg_name.compare("ZMM9") || !reg_name.compare("ZMM10") || !reg_name.compare("ZMM11") || !reg_name.compare("ZMM12") || !reg_name.compare("ZMM13") || !reg_name.compare("ZMM14") || !reg_name.compare("ZMM15"))
  {
    map<string, string> zmm_ymm_map = {
      {"ZMM0", "ymm0"},
      {"ZMM1", "ymm1"},
      {"ZMM2", "ymm2"},
      {"ZMM3", "ymm3"},
      {"ZMM4", "ymm4"},
      {"ZMM5", "ymm5"},
      {"ZMM6", "ymm6"},
      {"ZMM7", "ymm7"},
      {"ZMM8", "ymm8"},
      {"ZMM9", "ymm9"},
      {"ZMM10", "ymm10"},
      {"ZMM11", "ymm11"},
      {"ZMM12", "ymm12"},
      {"ZMM13", "ymm13"},
      {"ZMM14", "ymm14"},
      {"ZMM15", "ymm15"}};
    unsigned ymm_size = 0;
    assert(get_reg(zmm_ymm_map[reg_name], buf, 32, ymm_size));
    assert(ymm_size = 32);
    val_size = 64;
    return true;
  }
  else
  {
    warnx("can.t find reg: %s", reg_name.c_str());
    return false;
  }
}

bool get_mem(uint64_t address, size_t size, void* buf)
{
  SBError error;
  process.ReadMemory(address, buf, size, error);
  if(!error.Success())
    errx(-1, "read mem failed in get_mem(name): %s", error.GetCString());
  return true;
}

int get_pid()
{
  return process.GetProcessID();
}

unsigned long get_addr(string name)
{
  SBSymbolContextList symbolContextList = target.FindFunctions(name.c_str());
  assert(symbolContextList.IsValid());
  for(uint32_t i = 0; i < symbolContextList.GetSize(); ++i)
  {
    SBSymbolContext symbolContext = symbolContextList.GetContextAtIndex(i);
    SBFunction function = symbolContext.GetFunction();
    SBSymbol func_sym = symbolContext.GetSymbol();
    if(function.IsValid())
    {
      cerr << "judge func: " << (function.GetName()==NULL?"noname":function.GetName()) << " / " << (function.GetMangledName()==NULL?"noname":function.GetMangledName()) << endl;
      if(!name.compare(function.GetName()) || !name.compare(function.GetMangledName()))
      {
        SBAddress addr = function.GetStartAddress();
        assert(addr.IsValid());
        return addr.GetLoadAddress(target);
      }
    }
    else if(func_sym.IsValid())
    {
      cerr << "judge sym: " << (func_sym.GetName()==NULL?"noname":func_sym.GetName()) << " / " << (func_sym.GetMangledName()==NULL?"noname":func_sym.GetMangledName()) << endl;
      SBStream description;
      func_sym.GetDescription(description);
      cout << description.GetData() << endl;
      if(!name.compare(func_sym.GetName()) || !name.compare(func_sym.GetMangledName()))
      {
        SBAddress addr = func_sym.GetStartAddress();
        assert(addr.IsValid());
        if(!string(".text").compare(addr.GetSection().GetName()))
          return addr.GetLoadAddress(target);
      }
    }
  }
  warnx("can't find func: %s", name.c_str());
  return 0;
}

unsigned long get_unload_addr(unsigned long addr)
{
  SBAddress load_addr = target.ResolveLoadAddress(addr);
  assert(load_addr.IsValid());
  string file_from_addr(load_addr.GetModule().GetFileSpec().GetDirectory());
  file_from_addr = file_from_addr + "/" + load_addr.GetModule().GetFileSpec().GetFilename();
  string file_from_target(target.GetExecutable().GetDirectory());
  file_from_target = file_from_target + "/" + target.GetExecutable().GetFilename();
  if(!file_from_addr.compare(file_from_target))
    return addr;
  else
    return load_addr.GetSection().GetFileOffset() + load_addr.GetOffset();
}

string get_mangled_name(string name)
{
  SBSymbolContextList symbolContextList = target.FindFunctions(name.c_str());
  assert(symbolContextList.IsValid());
  for(uint32_t i = 0; i < symbolContextList.GetSize(); ++i)
  {
    SBSymbolContext symbolContext = symbolContextList.GetContextAtIndex(i);
    SBFunction function = symbolContext.GetFunction();
    SBSymbol func_sym = symbolContext.GetSymbol();
    if(function.IsValid())
    {
      cerr << "judge func: " << (function.GetName()==NULL?"noname":function.GetName()) << " / " << (function.GetMangledName()==NULL?"noname":function.GetMangledName()) << endl;
      if(!name.compare(function.GetName()) || !name.compare(function.GetMangledName()))
        return function.GetMangledName();
    }
    else if(func_sym.IsValid())
    {
      cerr << "judge sym: " << (func_sym.GetName()==NULL?"noname":func_sym.GetName()) << " / " << (func_sym.GetMangledName()==NULL?"noname":func_sym.GetMangledName()) << endl;
      if(!name.compare(func_sym.GetName()) || !name.compare(func_sym.GetMangledName()))
      {
        SBAddress addr = func_sym.GetStartAddress();
        assert(addr.IsValid());
        if(!string(".text").compare(addr.GetSection().GetName()))
          return func_sym.GetMangledName()?func_sym.GetMangledName():func_sym.GetName();
      }
    }
  }
  errx(-1, "can't find func: %s", name.c_str());
}

SBSection get_section(string obj_name, string sec_name)
{
  SBFileSpec obj_file(obj_name.c_str(), false);
  SBFileSpec obj_file_with_resolved(obj_name.c_str(), true);
  // SBFileSpec exec_file = target.GetExecutable();
  // if(!string(exec_file.GetDirectory()).compare(obj_file.GetDirectory()) && !string(exec_file.GetFilename()).compare(obj_file.GetFilename()))
    // return 0;
  SBModule obj_mdl = target.FindModule(obj_file);
  if(!obj_mdl.IsValid())
  {
    obj_mdl = target.FindModule(obj_file_with_resolved);
    assert(obj_mdl.IsValid());
  }
  SBSection section = obj_mdl.FindSection(sec_name.c_str());
  assert(section.IsValid());
  return section;
}

unsigned long get_section_load_addr(string obj_name, string sec_name)
{
  SBSection section = get_section(obj_name, sec_name);
  return section.GetLoadAddress(target);
}

unsigned long get_load_addr(unsigned long addr, string obj_name, string sec_name)
{
  SBSection section = get_section(obj_name, sec_name);
  unsigned long sec_load_base = section.GetLoadAddress(target);
  unsigned long sec_unload_base = section.GetFileAddress();
  return sec_load_base - sec_unload_base + addr;
}

SBSymbol get_func_sym_in_plt(uint64_t addr)
{
  for(unsigned i = 0, num = target.GetNumModules(); i < num; ++i)
  {
    SBModule module = target.GetModuleAtIndex(i);
    SBFileSpec file_spec = module.GetFileSpec();
    if(get_absolute(file_spec).compare(get_absolute(main_obj->getFileName().str())))
      continue;
    SBSection section = module.FindSection(".plt");
    assert(section.IsValid());
    addr_t plt_addr = section.GetFileAddress();
    addr_t plt_size = section.GetByteSize();
    assert(addr>=plt_addr && addr<plt_addr+plt_size);
    SBAddress func_addr = module.ResolveFileAddress(addr);
    SBSymbol func_sym = func_addr.GetSymbol();
    assert(func_sym.GetStartAddress().GetOffset() == func_addr.GetOffset());
    cerr << "found plt func: " << func_sym.GetName() << endl;
    return func_sym;
  }
  errx(-1, "can't find in get_func_name_in_plt");
}

SBSymbol get_func_sym(unsigned long addr)
{
  SBAddress load_addr = target.ResolveLoadAddress(addr);
  assert(load_addr.IsValid());
  SBStream description;
  load_addr.GetDescription(description);
  cout << description.GetData() << endl;
  if(!string(".plt").compare(load_addr.GetSection().GetName()))
    return get_func_sym_in_plt(addr);
  assert(!string(".text").compare(load_addr.GetSection().GetName()));
  SBSymbol func_sym = load_addr.GetSymbol();
  assert(func_sym.IsValid());
  return func_sym;
}

string get_func_name_in_plt(uint64_t addr)
{
  SBSymbol func_sym = get_func_sym_in_plt(addr);
  return func_sym.GetName();
}

string get_func_name(unsigned long addr)
{
  SBSymbol func_sym = get_func_sym(addr);
  return func_sym.GetName();
}

unsigned get_sym_unload_endaddr(unsigned unload_addr, string obj_name, string sec_name)
{
  SBSymbol func_sym = get_func_sym(get_load_addr(unload_addr, obj_name, sec_name));
  return get_unload_addr(func_sym.GetEndAddress().GetLoadAddress(target));
}