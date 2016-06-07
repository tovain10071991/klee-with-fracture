#include <string>
#include <algorithm>

#include <unistd.h>

using namespace std;

string get_absolute(string name)
{
  if(name[0] == '/')
    return name;
  return string(get_current_dir_name()) + "/" + name;
}

string omit_case(string name)
{
  transform(name.begin(), name.end(), name.begin(), ::tolower);
  return name;
}