#include "Helper/ptraceHelper.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace boost;
using namespace std;

void handle_quit(const vector<string>& args);
void handle_launch(const vector<string>& args);
void handle_get_mem(const vector<string>& args);

map<string, void(*)(const vector<string>&)> cmd_func_map = {
  {"q", handle_quit},
  {"launch", handle_launch},
  {"get_mem", handle_get_mem}
};

void print_prompt()
{
  cout << "(lldb test) ";
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
  while(1)
  {
    print_prompt();
    string input;
    getline(cin, input);
    vector<string> strs = split_str(input);
    handle_cmd(strs);
  }
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
  create_debugger_by_ptrace(args[0]);
}

void handle_get_mem(const vector<string>& args)
{
  assert(args.size()==1);
  ::uint64_t addr = stoull(args[0]);
  long data;
  get_mem(addr, sizeof(long), &data);
  cout << "0x" << hex << addr << ": 0x" << data << endl;
}