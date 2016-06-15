#include "Helper/DecompileHelper.h"
#include "Helper/DebugHelper.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>
#include <vector>
#include <map>
#include <string>

#include <errno.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include "CodeInv/Disassembler.h"
#include "CodeInv/MCDirector.h"
#include "CodeInv/Decompiler.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"
#include "llvm/Object/ObjectFile.h"

using namespace fracture;

using namespace boost;
using namespace std;
using namespace llvm;

void handle_quit(const vector<string>& args);
void handle_launch(const vector<string>& args);
void handle_dec_by_name(const vector<string>& args);
void handle_dec_by_addr(const vector<string>& args);

map<string, void(*)(const vector<string>&)> cmd_func_map = {
  {"q", handle_quit},
  {"launch", handle_launch},
  {"dec_by_name", handle_dec_by_name},
  {"dec_by_addr", handle_dec_by_addr},
};

void print_prompt()
{
  cout << "(dec test) ";
}

vector<string> split_str(string str)
{
  vector<string> strs;
  split(strs, str, is_any_of(" "), token_compress_on);
  return strs;
}

void handle_cmd(vector<string> strs)
{
  assert(cmd_func_map.find(strs[0])!=cmd_func_map.end());
  cmd_func_map[strs[0]](vector<string>(++strs.begin(), strs.end()));
}

int main(int argc, char** argv)
{
  if(argc!=1)
  {
    // create_debugger(argv[1]);
    
    InitializeNativeTarget();
    InitializeNativeTargetDisassembler();
    InitializeNativeTargetAsmParser();
    
    string tripleName = sys::getDefaultTargetTriple();
  
    MCDirector* mcd = new MCDirector(tripleName, "generic", "", TargetOptions(), Reloc::DynamicNoPIC, CodeModel::Default, CodeGenOpt::Default, outs(), errs());
  
    object::ObjectFile* obj = object::ObjectFile::createObjectFile(argv[1]);
    assert(obj->isObject() && obj->isELF() && "it is not object");
  
    Disassembler* disassembler = new Disassembler(mcd, obj, NULL, outs(), outs());
    Decompiler* decompiler = new Decompiler(disassembler, NULL, nulls(), nulls());
    
    ::uint64_t addr = stoull(argv[2], 0, 0);
    assert(addr);
    decompiler->decompileFunction(addr);
    return 0;
  }
  while(1)
  {
    print_prompt();
    string input;
    getline(cin, input);
    vector<string> strs = split_str(input);
    handle_cmd(strs);
  }
  
  return 0;
}

void handle_quit(const vector<string>& args)
{
  assert(args.empty());
  cout << "quit now" << endl;
  exit(0);
}

void handle_launch(const vector<string>& args)
{
  assert(args.size()==1);
  Module* init_mdl = get_module(args[0]);
  cout << "luanch done: " << init_mdl->getModuleIdentifier() << endl;
}

void handle_dec_by_name(const vector<string>& args)
{
  pid_t pid = fork();
  if(pid<0)
    err(errno, "fork faild in handle_dec_by_name");
  else if(pid==0)
  {
    assert(args.size()==1);
    get_module_with_function(args[0]);
    cout << "dec done" << endl;
    exit(0);
  }
  else
  {
    int status;
    if(waitpid(pid, &status, 0)==-1)
      err(errno, "wait failed in handle_dec_by_name");
    return;
  }
}

void handle_dec_by_addr(const vector<string>& args)
{
    pid_t pid = fork();
  if(pid<0)
    err(errno, "fork faild in handle_dec_by_addr");
  else if(pid==0)
  {
    assert(args.size()==1);
    ::uint64_t addr = stoull(args[0], 0, 0);
    get_module_with_function(addr);
    cout << "dec done" << endl;
    exit(0);
  }
  else
  {
    int status;
    if(waitpid(pid, &status, 0)==-1)
      err(errno, "wait failed in handle_dec_by_addr");
    return;
  }
}